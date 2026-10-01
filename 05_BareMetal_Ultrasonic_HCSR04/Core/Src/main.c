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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIMEOUT_TICKS  60000U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t echo_duracion_us = 0;
volatile float distancia_cm = 0.0f;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Ultrasonic_Init_Registers(void);
static void Ultrasonic_Trigger_Pulse(void);
static void Delay_us_Timer(uint32_t us);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Delay_us_Timer(uint32_t us)
{
  uint16_t start = (uint16_t)TIM1->CNT;
  while ((uint16_t)(TIM1->CNT - start) < us);
}

static void Ultrasonic_Init_Registers(void)
{

	/* 1. Habilitar el bus AHB1 (GPIOA) y el bus APB2 (TIM1) */
	  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

	  __asm volatile("nop"); // __asm Exprecion de C para ejecucion, volatile evita que el compilador borre el contenido.
	  __asm volatile("nop");  // nop =No Operation evita que la cpu haga calgulos y ordena que solo se ejecute 1 ciclo

	  /* 2. PA9 (Trigger) -> Salida Digital (01) en Push-Pull a Alta Velocidad */
	  GPIOA->MODER   &= ~(3U << (9 * 2)); // se prepara la mascara en 11
	  GPIOA->MODER   |=  (1U << (9 * 2)); // se establece el bit 18 en modo salida o output
	  GPIOA->OTYPER  &= ~(1U << 9);       // se prepara la mascara
	  GPIOA->OSPEEDR |=  (2U << (9 * 2));


	  //estados del moder del comportamiento de los bits
	  // 00 0x0 , 0 se establece como salida
	  // 01  0x1 , 1 se establece como Entrada analogica
	  // 10  0x2 , 2 Funcion Alternativa
	  // 11 0x3 , 3 modo analogo



	  /* 3. PA8 (Echo) -> Entrada en Función Alterna (10), No-Pull (FT) */
	  GPIOA->MODER   &= ~(3U << (8 * 2));
	  GPIOA->MODER   |=  (2U << (8 * 2));
	  GPIOA->PUPDR   &= ~(3U << (8 * 2));

	  /* Mapear PA8 al periférico TIM1_CH1 usando AF1 (AFRH posición 0) */
	  GPIOA->AFR[1]  &= ~(0xFU << ((8 - 8) * 4));
	  GPIOA->AFR[1]  |=  (1U   << ((8 - 8) * 4));

	  /* 4. Base de tiempo del TIM1 a 1 MHz (1 tick = 1 us) */
	  /* SYSCLK = 84 MHz, APB2 Prescaler = 1 -> Bus APB2 = 84 MHz */
	  TIM1->PSC = (84 - 1);
	  TIM1->ARR = 0xFFFF;

	  /* 5. Configurar Canal 1 en Input Capture mapeado a TI1 */
	  TIM1->CCMR1 &= ~TIM_CCMR1_CC1S;
	  TIM1->CCMR1 |=  (1U << TIM_CCMR1_CC1S_Pos); // CC1S = 01

	  /* 6. Flanco de subida inicial y habilitación de captura */
	  TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
	  TIM1->CCER |=  TIM_CCER_CC1E;

	  /* 7. Encender contador */
	  TIM1->CR1 |= TIM_CR1_CEN;
	}

	static void Ultrasonic_Trigger_Pulse(void)
	{
	  GPIOA->BSRR = (1U << 9);       // PA9 = 3.3V (Inicio del pulso)
	  Delay_us_Timer(10);            // Pulso sostenido de 10 us
	  GPIOA->BSRR = (1U << (9 + 16)); // PA9 = 0V (Fin del pulso)
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
  /* USER CODE BEGIN 2 */
  Ultrasonic_Init_Registers();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  	  	  uint32_t t_subida = 0;
	 	      uint32_t t_bajada = 0;
	 	      uint32_t timeout  = TIMEOUT_TICKS;

	 	      /* A. Limpiar banderas residuales antes de emitir */
	 	      TIM1->SR &= ~TIM_SR_CC1IF;

	 	      /* B. Disparar el pulso de 10 us */
	 	      Ultrasonic_Trigger_Pulse();

	 	      /* C. Esperar flanco de subida (inicio del eco) */
	 	      while (!(TIM1->SR & TIM_SR_CC1IF))
	 	      {
	 	        if (--timeout == 0) break;
	 	      }

	 	      if (timeout > 0)
	 	      {
	 	        t_subida = TIM1->CCR1; // Captura de t1

	 	        /* Invertir polaridad para capturar flanco de bajada */
	 	        TIM1->CCER |= TIM_CCER_CC1P;
	 	        TIM1->SR &= ~TIM_SR_CC1IF;

	 	        timeout = TIMEOUT_TICKS;
	 	        /* D. Esperar flanco de bajada (fin del eco) */
	 	        while (!(TIM1->SR & TIM_SR_CC1IF))
	 	        {
	 	          if (--timeout == 0) break;
	 	        }

	 	        if (timeout > 0)
	 	        {
	 	          t_bajada = TIM1->CCR1; // Captura de t2

	 	          /* Calcular duración con manejo de desbordamiento a 16 bits */
	 	          if (t_bajada >= t_subida)
	 	          {
	 	            echo_duracion_us = t_bajada - t_subida;
	 	          }
	 	          else
	 	          {
	 	            echo_duracion_us = (0xFFFF - t_subida) + t_bajada + 1;
	 	          }

	 	          distancia_cm = (float)echo_duracion_us * 0.01715f;
	 	        }

	 	        /* Restaurar polaridad a flanco de subida para el siguiente ciclo */
	 	        TIM1->CCER &= ~TIM_CCER_CC1P;
	 	        TIM1->SR &= ~TIM_SR_CC1IF;
	 	      }

	 	      /* Retardo de 60 ms entre mediciones para evitar interferencia por ecos */
	 	      for (volatile uint32_t i = 0; i < 300000; i++);
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

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
