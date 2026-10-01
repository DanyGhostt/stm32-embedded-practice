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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Definitions for vTaskTemp */
osThreadId_t vTaskTempHandle;
const osThreadAttr_t vTaskTemp_attributes = {
  .name = "vTaskTemp",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for vTaskConsole */
osThreadId_t vTaskConsoleHandle;
const osThreadAttr_t vTaskConsole_attributes = {
  .name = "vTaskConsole",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for QueueTemp */
osMessageQueueId_t QueueTempHandle;
const osMessageQueueAttr_t QueueTemp_attributes = {
  .name = "QueueTemp"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void StartTaskTemp(void *argument);
void StartTaskConsole(void *argument);

/* USER CODE BEGIN PFP */
void MCU_Hardware_Init(void);
static void Delay_us(uint32_t us);
static uint8_t DS18B20_Reset(void); // el pulso reset de el sensor exige un tiempo de 480 microsegundos (especificados en el data sheet)
static void DS18B20_WriteBit(uint8_t bit);
static uint8_t DS18B20_ReadBit(void);
static void DS18B20_WriteByte(uint8_t data);
static uint8_t DS18B20_ReadByte(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void MCU_Hardware_Init(void)
{
  // PASO 1: Habilitar energía de periféricos en el RCC
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;    // Reloj para GPIOA (PA0 = Datos Sonda, PA2 = TX ST-LINK)
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;     // Reloj para TIM2 (Cronómetro de us)
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;   // Reloj para USART2 (Consola virtual PC)

  // Sincronización: 2 ciclos de retardo para estabilizar los buses APB/AHB
  __asm volatile("nop");
  __asm volatile("nop");

  // PASO 2: Configurar PA0 para bus 1-Wire (Open-Drain + Pull-Up Interno)
  GPIOA->MODER   &= ~(3U << (0 * 2));     // Limpiar modo previo
  GPIOA->MODER   |=  (1U << (0 * 2));     // Salida de propósito general (01)
  GPIOA->OTYPER  |=  (1U << 0);           // Salida Open-Drain (Drenador Abierto obligatorio)
  GPIOA->OSPEEDR |=  (2U << (0 * 2));     // Alta velocidad
  GPIOA->PUPDR   &= ~(3U << (0 * 2));
  GPIOA->PUPDR   |=  (1U << (0 * 2));     // Pull-Up interno activado (01)
  GPIOA->BSRR     =  (1U << 0);           // Bus en reposo (nivel ALTO por defecto)

  // PASO 3: Configurar TIM2 como cronómetro de microsegundos (Bus APB1 Timers = 84 MHz)
  // Prescaler = (84 MHz / 1 MHz) - 1 = 83 -> 1 tick = 1 microsegundo exacto
  TIM2->PSC = 83;
  TIM2->ARR = 0xFFFFFFFF;                 // Rango máximo de 32 bits nativos
  TIM2->CR1 |= TIM_CR1_CEN;               // Encender el contador

  // PASO 4: Conectar PA2 como USART2_TX (Función Alterna AF7)
  GPIOA->MODER   &= ~(3U << (2 * 2));
  GPIOA->MODER   |=  (2U << (2 * 2));     // Modo Función Alterna (10)
  GPIOA->AFR[0]  &= ~(0xFU << (2 * 4));   // Limpiar multiplexor AFRL para pin 2
  GPIOA->AFR[0]  |=  (7U << (2 * 4));     // Inyectar AF7 (USART2_TX según Reference Manual)

  // PASO 5: Configurar baudios de USART2 a 115200 (Bus APB1 = 42 MHz)
  // 42 MHz / (16 * 115200) = 22.786 -> Mantisa = 22 (0x16), Fracción = 0.786 * 16 = 12 (0xC)
  USART2->BRR = (22 << 4) | 12;           // Registro de velocidad = 0x16C
  USART2->CR1 |= USART_CR1_TE | USART_CR1_UE; // Habilitar Transmisor y Periférico
}

static void Delay_us(uint32_t us)
{
  uint32_t start = TIM2->CNT;
  while ((TIM2->CNT - start) < us);
}

// ====================================================================================================
// 3. REDIRECCIÓN DE PRINTF HACIA EL REGISTRO DE DATOS DE USART2
// ====================================================================================================
int __io_putchar(int ch)
{
  while (!(USART2->SR & USART_SR_TXE));   // Esperar a que el buffer de hardware esté libre
  USART2->DR = (uint8_t)ch;               // Inyectar el byte al bus físico
  return ch;
}

// ====================================================================================================
// 4. PROTOCOLO 1-WIRE POR REGISTROS (DS18B20)
// ====================================================================================================
static uint8_t DS18B20_Reset(void)
{
  uint8_t presence = 0;

  // Pulso de Reset: Drenar la línea a 0V durante 480 us
  GPIOA->BSRR = (1U << (0 + 16));
  Delay_us(480);

  // Liberar línea (el pull-up la eleva) y capturar presencia
  taskENTER_CRITICAL();
  GPIOA->BSRR = (1U << 0);
  Delay_us(70);                           // Ventana de respuesta del DS18B20

  // Si la sonda responde, jala la línea a 0V
  if (!(GPIOA->IDR & (1U << 0)))
  {
    presence = 1;
  }
  taskEXIT_CRITICAL();

  Delay_us(410);                          // Completar la ranura de 480 us
  return presence;
}

static void DS18B20_WriteBit(uint8_t bit)
{
  taskENTER_CRITICAL();
  GPIOA->BSRR = (1U << (0 + 16));         // Iniciar ranura en BAJO
  if (bit)
  {
    Delay_us(6);                          // Bit 1: pulso corto
    GPIOA->BSRR = (1U << 0);              // Liberar línea
    Delay_us(64);
  }
  else
  {
    Delay_us(60);                         // Bit 0: pulso sostenido
    GPIOA->BSRR = (1U << 0);              // Liberar línea
    Delay_us(10);
  }
  taskEXIT_CRITICAL();
}

static uint8_t DS18B20_ReadBit(void)
{
  uint8_t bit = 0;
  taskENTER_CRITICAL();
  GPIOA->BSRR = (1U << (0 + 16));         // Disparo en BAJO
  Delay_us(2);
  GPIOA->BSRR = (1U << 0);                // Liberar línea
  Delay_us(10);                           // Muestreo del estado

  if (GPIOA->IDR & (1U << 0))
  {
    bit = 1;
  }
  taskEXIT_CRITICAL();
  Delay_us(50);                           // Completar ciclo de 60 us
  return bit;
}

static void DS18B20_WriteByte(uint8_t data)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    DS18B20_WriteBit(data & 0x01);
    data >>= 1;
  }
}

static uint8_t DS18B20_ReadByte(void)
{
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++)
  {
    if (DS18B20_ReadBit())
    {
      data |= (1U << i);
    }
  }
  return data;
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

  /* Create the queue(s) */
  /* creation of QueueTemp */
  QueueTempHandle = osMessageQueueNew (4, sizeof(uint32_t), &QueueTemp_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of vTaskTemp */
  vTaskTempHandle = osThreadNew(StartTaskTemp, NULL, &vTaskTemp_attributes);

  /* creation of vTaskConsole */
  vTaskConsoleHandle = osThreadNew(StartTaskConsole, NULL, &vTaskConsole_attributes);

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

/* USER CODE BEGIN Header_StartTaskTemp */
/**
  * @brief  Function implementing the vTaskTemp thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskTemp */
void StartTaskTemp(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	  if (DS18B20_Reset())
	  {
	        DS18B20_WriteByte(0xCC);          // Comando Dallas Skip ROM (sensor único)
	        DS18B20_WriteByte(0x44);          // Comando Convert T

	        // Suspensión reactiva: 0% consumo de CPU durante los 750 ms requeridos
	        osDelay(750);

	        // 2. Recuperar el resultado desde el Scratchpad
	        if (DS18B20_Reset())
	        {
	          DS18B20_WriteByte(0xCC);        // Skip ROM
	          DS18B20_WriteByte(0xBE);        // Leer memoria de temperatura (Scratchpad)

	          uint8_t temp_lsb = DS18B20_ReadByte();
	          uint8_t temp_msb = DS18B20_ReadByte();

	          // Empaquetar los 16 bits crudos
	          uint32_t raw_temp = (uint32_t)((temp_msb << 8) | temp_lsb);

	          // Inyectar el paquete crudo a la cola de FreeRTOS
	          osMessageQueuePut(QueueTempHandle, &raw_temp, 0U, 0U);
	        }
	      }

	      // Intervalo de descanso entre lecturas completas
	      osDelay(1000);
	    }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTaskConsole */
/**
* @brief Function implementing the vTaskConsole thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskConsole */
void StartTaskConsole(void *argument)
{
  /* USER CODE BEGIN StartTaskConsole */
	uint32_t raw_data = 0;
  /* Infinite loop */
  for(;;)
  {

	  if (osMessageQueueGet(QueueTempHandle, &raw_data, NULL, osWaitForever) == osOK)
	      {
	        int16_t raw_signed = (int16_t)raw_data;

	        // Resolución de 12 bits: 1 tick = 0.0625 °C (1/16 de grado)
	        float temp_celsius = (float)raw_signed * 0.0625f;

	        // Transmisión directa por USART2 -> Consola ST-LINK de la IDE
	        printf("[DS18B20 CMSIS+RTOS] Temp: %.2f *C\r\n", temp_celsius);
	      }
  }
  /* USER CODE END StartTaskConsole */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
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
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
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
