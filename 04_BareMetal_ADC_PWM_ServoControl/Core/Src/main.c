/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// ====================================================================================================
// 1. DIRECCIONES BASE DE LOS PERIFÉRICOS (Mapeo de Memoria Absoluto del Sistema - Reference Manual)
// ====================================================================================================
#define RCC_BASE      0x40023800  // Base del Bloque Reset and Clock Control (Bus AHB1)
#define GPIOA_BASE    0x40020000  // Base del Puerto GPIOA (Bus AHB1)
#define TIM3_BASE     0x40000400  // Base del Temporizador TIM3 (Bus APB1)
#define ADC1_BASE     0x40012000  // Base del Convertidor ADC1 (Bus APB2)

// ====================================================================================================
// 2. REGISTROS ESPECÍFICOS USANDO LAS BASES + OFFSETS
// ====================================================================================================

// --- REGISTROS DEL RCC ---
#define REG_RCC_AHB1ENR   (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define REG_RCC_APB1ENR   (*(volatile uint32_t*)(RCC_BASE + 0x40))
#define REG_RCC_APB2ENR   (*(volatile uint32_t*)(RCC_BASE + 0x44))

// --- REGISTROS DEL PUERTO GPIOA ---
#define REG_GPIOA_MODER   (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define REG_GPIOA_AFRL    (*(volatile uint32_t*)(GPIOA_BASE + 0x20))

// --- REGISTROS DEL TIMER 3 (PWM del Servo) ---
#define REG_TIM3_CR1      (*(volatile uint32_t*)(TIM3_BASE + 0x00))
#define REG_TIM3_EGR      (*(volatile uint32_t*)(TIM3_BASE + 0x14)) // Registro EGR para forzar actualización
#define REG_TIM3_CCMR1    (*(volatile uint32_t*)(TIM3_BASE + 0x18))
#define REG_TIM3_CCER     (*(volatile uint32_t*)(TIM3_BASE + 0x20))
#define REG_TIM3_PSC      (*(volatile uint32_t*)(TIM3_BASE + 0x28))
#define REG_TIM3_ARR      (*(volatile uint32_t*)(TIM3_BASE + 0x2C))
#define REG_TIM3_CCR1     (*(volatile uint32_t*)(TIM3_BASE + 0x34))

// --- REGISTROS DEL ADC1 (Muestreo del Potenciómetro) ---
#define REG_ADC1_SR       (*(volatile uint32_t*)(ADC1_BASE + 0x00))
#define REG_ADC1_CR1      (*(volatile uint32_t*)(ADC1_BASE + 0x04))
#define REG_ADC1_CR2      (*(volatile uint32_t*)(ADC1_BASE + 0x08))
#define REG_ADC1_SQR3     (*(volatile uint32_t*)(ADC1_BASE + 0x34))
#define REG_ADC1_DR       (*(volatile uint32_t*)(ADC1_BASE + 0x4C))

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Definitions for Lectura_Task */
osThreadId_t Lectura_TaskHandle;
const osThreadAttr_t Lectura_Task_attributes = {
  .name = "Lectura_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Servo_Task */
osThreadId_t Servo_TaskHandle;
const osThreadAttr_t Servo_Task_attributes = {
  .name = "Servo_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void StartDefaultTask(void *argument);
void StartTask02(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Variable global volátil compartida entre la tarea de lectura y la tarea del servo
volatile uint16_t adc_valor_pot = 0;

// Función de inicialización manual de hardware a nivel de registros puros
void MCU_Hardware_Init(void) {
    // ------------------------------------------------------------------------------------------------
    // PASO 1: ENERGIZAR PERIFÉRICOS Y ESTABILIZAR RELOJ
    // ------------------------------------------------------------------------------------------------
    REG_RCC_AHB1ENR |= (1 << 0);    // Despierta GPIOA
    REG_RCC_APB1ENR |= (1 << 1);    // Despierta Timer 3
    REG_RCC_APB2ENR |= (1 << 8);    // Despierta ADC1

    // Bus synchronization delay (dummy read)
    volatile uint32_t delay_calculo;
    delay_calculo = REG_RCC_APB1ENR;
    delay_calculo = REG_RCC_APB2ENR;
    (void)delay_calculo; // Evita warning del compilador por variable no usada

    // ------------------------------------------------------------------------------------------------
    // PASO 2: CONFIGURACIÓN ELÉCTRICA DE PINES (Módulo GPIOA)
    // ------------------------------------------------------------------------------------------------
    REG_GPIOA_MODER &= ~((3 << (0 * 2)) | (3 << (6 * 2))); // Limpiar bits de PA0 y PA6

    REG_GPIOA_MODER |= (3 << (0 * 2));                    // PA0 Analógico (11)
    REG_GPIOA_MODER |= (2 << (6 * 2));                    // PA6 Función Alterna (10)

    REG_GPIOA_AFRL  &= ~(0xF << (6 * 4));                 // Limpiar multiplexor del pin 6
    REG_GPIOA_AFRL  |=  (2   << (6 * 4));                 // AF2 (TIM3_CH1)

    // ------------------------------------------------------------------------------------------------
    // PASO 3: CONFIGURACIÓN DEL TIMER 3 (PWM a 50Hz) - ¡CORREGIDO PARA RELOJ DE 84 MHz!
    // ------------------------------------------------------------------------------------------------
    // Como SystemClock_Config aceleró el reloj a 84 MHz, dividimos entre 84 (escribiendo 83)
    REG_TIM3_PSC   = 83;                  // Prescaler: 84 MHz / 84 = 1 MHz. (1 cuenta = 1 us)
    REG_TIM3_ARR   = 19999;               // ARR: 20,000 us = 20 ms de periodo (50 Hz)

    REG_TIM3_CCMR1 &= ~(0x3 << 0);        // Forzar Canal 1 como Salida (CC1S = 00)
    REG_TIM3_CCMR1 &= ~(0x7 << 4);        // Limpiar modo OC1M
    REG_TIM3_CCMR1 |= (6 << 4) | (1 << 3); // Modo PWM 1 (110) + Preload activo

    REG_TIM3_CCER  &= ~(1 << 1);          // Polaridad activa en alto
    REG_TIM3_CCER  |= (1 << 0);           // Habilitar salida en PA6

    REG_TIM3_CCR1  = 1500;                // Posición inicial: Centro (1.5 ms)

    REG_TIM3_EGR   |= (1 << 0);           // Generar evento Update de inmediato
    REG_TIM3_CR1   |= (1 << 0);           // Arrancar el temporizador

    // ------------------------------------------------------------------------------------------------
    // PASO 4: CONFIGURACIÓN DEL CONVERTIDOR ANALÓGICO (ADC1)
    // ------------------------------------------------------------------------------------------------
    REG_ADC1_CR1   &= ~(3 << 24);         // Forzar resolución de 12 bits
    REG_ADC1_SQR3  &= ~(0x1F);            // Leer primero el Canal 0 (PA0)

    REG_ADC1_CR2   |= (1 << 0);           // Energizar el ADC
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  // Disparo manual y seguro de nuestro hardware de bajo nivel
  MCU_Hardware_Init();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Lectura_Task */
  Lectura_TaskHandle = osThreadNew(StartDefaultTask, NULL, &Lectura_Task_attributes);

  /* creation of Servo_Task */
  Servo_TaskHandle = osThreadNew(StartTask02, NULL, &Servo_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the Lectura_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    REG_ADC1_CR2 |= (1 << 30); // Lanzar conversión

    // Esperar a que la bandera EOC se levante
    while (!(REG_ADC1_SR & (1 << 1)));

    // Guardar el valor puro de 12 bits
    adc_valor_pot = REG_ADC1_DR;

    // Ceder el control al RTOS
    osDelay(20);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the Servo_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
  uint32_t ccr_calculado = 1500;

  /* Infinite loop */
  for(;;)
  {
      // Matemática para mapear de forma lineal los 12 bits del ADC a un Duty Cycle de 1000 a 2000
      ccr_calculado = 1000 + ((uint32_t)adc_valor_pot * 1000) / 4095;

      // Inyectar al registro del Timer
      REG_TIM3_CCR1 = ccr_calculado;

      // Ceder el procesador
      osDelay(30);
  }
  /* USER CODE END StartTask02 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
