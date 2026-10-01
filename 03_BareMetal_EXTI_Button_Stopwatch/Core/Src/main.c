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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RCC_BASE_M        0x40023800UL  // Reset and Clock Control
#define GPIOA_BASE_M      0x40020000UL  // Puerto A: Base para LEDs y Testigo
#define GPIOC_BASE_M      0x40020800UL  // Puerto C: Botón de usuario PC13
#define SYSCFG_BASE_M     0x40013800UL  // System Configuration (Multiplexor EXTI)
#define EXTI_BASE_M       0x40013C00UL  // External Interrupt Controller
#define TIM4_BASE_M       0x40000800UL  // Timer 4: Telemetría de microsegundos
#define NVIC_BASE_M       0xE000E100UL  // Controlador de Interrupciones de la CPU

#define RCC_AHB1ENR_M     (*(volatile uint32_t *)(RCC_BASE_M + 0x30)) // Reloj GPIO
#define RCC_APB1ENR_M     (*(volatile uint32_t *)(RCC_BASE_M + 0x40)) // Reloj TIM4
#define RCC_APB2ENR_M     (*(volatile uint32_t *)(RCC_BASE_M + 0x44)) // Reloj SYSCFG

// --- REGISTROS DE GPIOA (LEDs y Testigo) ---
#define GPIOA_MODER_M     (*(volatile uint32_t *)(GPIOA_BASE_M + 0x00)) // Modo de pines
#define GPIOA_ODR_M       (*(volatile uint32_t *)(GPIOA_BASE_M + 0x14)) // Datos de Salida

// --- REGISTROS DE GPIOC (Botón) ---
#define GPIOC_MODER_M     (*(volatile uint32_t *)(GPIOC_BASE_M + 0x00)) // Modo de Puerto C
#define GPIOC_IDR_M       (*(volatile uint32_t *)(GPIOC_BASE_M + 0x10)) // Datos de Entrada

// --- REGISTROS DE INTERRUPCIÓN (SYSCFG + EXTI) ---
#define SYSCFG_EXTICR4_M  (*(volatile uint32_t *)(SYSCFG_BASE_M + 0x14)) // Enrutador EXTI13
#define EXTI_IMR_M        (*(volatile uint32_t *)(EXTI_BASE_M + 0x00)) // Máscara
#define EXTI_FTSR_M       (*(volatile uint32_t *)(EXTI_BASE_M + 0x0C)) // Flanco de Bajada
#define EXTI_PR_M         (*(volatile uint32_t *)(EXTI_BASE_M + 0x14)) // Bandera Pendiente

// --- REGISTROS DEL TIMER 4 ---
#define TIM4_CR1_M        (*(volatile uint32_t *)(TIM4_BASE_M + 0x00)) // Control 1
#define TIM4_PSC_M        (*(volatile uint32_t *)(TIM4_BASE_M + 0x28)) // Prescaler
#define TIM4_CNT_M        (*(volatile uint32_t *)(TIM4_BASE_M + 0x24)) // Contador Vivo

// --- REGISTRO MAESTRO CPU (NVIC) ---
#define NVIC_ISER1_M      (*(volatile uint32_t *)(NVIC_BASE_M + 0x04)) // Habilitador maestro
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTask02 */
osThreadId_t myTask02Handle;
const osThreadAttr_t myTask02_attributes = {
  .name = "myTask02",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myTask03 */
osThreadId_t myTask03Handle;
const osThreadAttr_t myTask03_attributes = {
  .name = "myTask03",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myMutex01 */
osMutexId_t myMutex01Handle;
const osMutexAttr_t myMutex01_attributes = {
  .name = "myMutex01"
};
/* USER CODE BEGIN PV */
volatile uint8_t sistema_pausa = 0;
volatile uint32_t tiempo_ejecucionLD1 = 0;
volatile uint32_t tiempo_ejecucionLD2 = 0;
volatile uint32_t tiempo_ejecucionLD3 = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void StartDefaultTask(void *argument);
void StartTask02(void *argument);
void StartTask03(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  RCC_AHB1ENR_M |= (1U << 0) | (1U << 2); // HABILITAR PUERTOS A Y C
  RCC_APB1ENR_M |= (1U << 2);   // HABILITACION DE RELOJ TIM4
  RCC_APB2ENR_M |= (1U << 14);  // HABILITAR RELOJ SYSCFG

  // Configuración de modos de pines (PA4, PA5, PA6 y PA7 como salidas lógicas)
  // Pin 4: bits 8-9 | Pin 5: bits 10-11 | Pin 6: bits 12-13 | Pin 7: bits 14-15
  GPIOA_MODER_M &= ~((3U << 8) | (3U << 10) | (3U << 12) | (3U << 14));  // Creamos la mascara
  GPIOA_MODER_M |=  ((1U << 8) | (1U << 10) | (1U << 12) | (1U << 14));  // Asignamos "01" (Salida)

  GPIOC_MODER_M &= ~(3U << 26); // Configuración del boton user en el puerto C como entrada (00)

  // Configuración del puente EXTI13 mapeado al Puerto C
  SYSCFG_EXTICR4_M &= ~(0XFU << 4);     // Limpiar bits de la línea 13 (bits 4 al 7)
  SYSCFG_EXTICR4_M |= (2U << 4);        // 0x2 selecciona Puerto C para la línea EXTI13

  EXTI_IMR_M |= (1U << 13);             // Desenmascarar línea 13
  EXTI_FTSR_M |= (1U << 13);            // Configurar flanco de bajada

  TIM4_PSC_M = 15;                      // Configurar Prescaler (Conteo cada 1 microsegundo)
  TIM4_CR1_M |= (1U << 0);              // Arrancar Timer 4 (CEN = 1)

  NVIC_ISER1_M |= (1U << 8);            // Habilitar la interrupción EXTI15_10 en el NVIC
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of myMutex01 */
  myMutex01Handle = osMutexNew(&myMutex01_attributes);

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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of myTask02 */
  myTask02Handle = osThreadNew(StartTask02, NULL, &myTask02_attributes);

  /* creation of myTask03 */
  myTask03Handle = osThreadNew(StartTask03, NULL, &myTask03_attributes);

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
// Rutina de Servicio de Interrupción (ISR)
void EXTI15_10_IRQHandler(void){

	GPIOA_ODR_M |= (1U << 5); // Encender pin testigo PA5 inmediatamente al entrar

	if ((EXTI_IMR_M & (1U << 13)) && (EXTI_PR_M & (1U << 13)))
	{
		// Alternar el estado de pausa de las tareas
		if (sistema_pausa == 0) {
			sistema_pausa = 1;
		} else {
			sistema_pausa = 0;
		}

		// OBLIGATORIO BARE-METAL: Limpiar bandera escribiendo un 1 lógico
		EXTI_PR_M |= (1U << 13);
	}

	GPIOA_ODR_M &= ~(1U << 5); // Apagar pin testigo PA5 justo antes de salir
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
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
    while (sistema_pausa == 1){
    	osDelay(10);
    }
    if (osMutexAcquire(myMutex01Handle, osWaitForever) == osOK ){
    	TIM4_CNT_M = 0;
    	GPIOA_ODR_M |= (1U << 6); // ENCENDER LED 1 EN PIN PA6

    	osDelay(500);

    	GPIOA_ODR_M &= ~(1U << 6); // APAGAR LED 1 EN PIN PA6
    	tiempo_ejecucionLD1 = TIM4_CNT_M;

    	osMutexRelease(myMutex01Handle);
    }
    osDelay(10);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the myTask02 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  for(;;)
  {
    while (sistema_pausa == 1){
    	osDelay(10);
    }
    if (osMutexAcquire(myMutex01Handle, osWaitForever) == osOK)
    {
		// --- INICIO DE SECCIÓN CRÍTICA ---
		TIM4_CNT_M = 0;
		GPIOA_ODR_M |= (1U << 7);   // ENCENDER LED 2 EN PIN PA7

		osDelay(500);

		GPIOA_ODR_M &= ~(1U << 7);  // APAGAR LED 2 EN PIN PA7
		tiempo_ejecucionLD2 = TIM4_CNT_M;
		// --- FIN DE SECCIÓN CRÍTICA ---

		osMutexRelease(myMutex01Handle);
	}

	osDelay(10);
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the myTask03 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void *argument)
{
  /* USER CODE BEGIN StartTask03 */
  /* Infinite loop */
  for(;;)
  {
    while (sistema_pausa == 1){
    	osDelay(10);
    }
    if (osMutexAcquire(myMutex01Handle, osWaitForever) == osOK)
    {
		// --- INICIO DE SECCIÓN CRÍTICA ---
		TIM4_CNT_M = 0;
		GPIOA_ODR_M |= (1U << 4);   // Enceder LED 3 en PA4 (Se queda intacto)

		osDelay(600);

		GPIOA_ODR_M &= ~(1U << 4);  // Apagar LED 3 en PA4
		tiempo_ejecucionLD3 = TIM4_CNT_M;
		// --- FIN DE SECCIÓN CRÍTICA ---

		osMutexRelease(myMutex01Handle);
	}

	osDelay(10);
  }
  /* USER CODE END StartTask03 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
