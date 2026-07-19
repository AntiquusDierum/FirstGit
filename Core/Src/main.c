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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"
#include "eric_lora.h"
#include "terminal_console.h"
#include "dashboard.h"
#include "sht25.h"
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

/* USER CODE BEGIN PV */
uint8_t debug_rx[5];
uint8_t eric_uart_rx;
uint8_t debug_tx[80];
uint32_t adcBuffer[ADC_CHANS];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void InitializeTimer(void);
void DisplayString(UART_HandleTypeDef * huart);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == debug_uart)
	{
	    TerminalConsole_RxByte(debug_rx[0]);

	    HAL_UART_Receive_IT(debug_uart,
	                        debug_rx,
	                        1);
	}
    if (huart == eric_uart)
    {
        ERIC_UART_RxByte(eric_uart_rx);
        HAL_UART_Receive_IT(eric_uart, &eric_uart_rx, 1);
    }
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadcHandle)
{
    if (hadcHandle == &hadc)
    {
        Dashboard_UpdateAdcValues(adcBuffer, ADC_CHANS);
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  uint32_t timerValue = 0;
  static uint32_t lastTimerValue = 0xFFFFFFFF;
  static uint32_t lastRefresh = 0;
//  static UART_HandleTypeDef *lora_uart;
  uint32_t lastSecond = 0xFFFFFFFF;
  uint32_t lastMinute = 0xFFFFFFFF;
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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC_Init();
  MX_I2C1_Init();
  MX_TIM6_Init();
  MX_UART5_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim6);
  HAL_UART_Receive_IT(debug_uart,debug_rx,1);
  Dashboard_Show(debug_uart);
  HAL_Delay(50);
  HAL_GPIO_WritePin(GPIOB, en_LoRa_Pin, GPIO_PIN_SET);
  if (SHT25_Init(&hi2c1) != HAL_OK)
  {
      const char message[] = "SHT25 sensor not detected\r\n";

      HAL_UART_Transmit(debug_uart,
                        (uint8_t *)message,
                        sizeof(message) - 1U,
                        HAL_MAX_DELAY);
  }
  ERIC_Init(eric_uart);
  HAL_UART_Receive_IT(eric_uart, &eric_uart_rx, 1);
  HAL_ADC_Start_DMA(&hadc, adcBuffer, 4);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  ERIC_Task();
	  uint8_t c;
//	  static uint8_t old_redraw = 0xFF;

	  if (TerminalConsole_RedrawCommandScreen())
	  {
	      TerminalConsole_ShowScreen(debug_uart);
	  }

	  if (TerminalConsole_RedrawDashboard())
	  {
	      Dashboard_Show(debug_uart);
	      Dashboard_Refresh(debug_uart);

	      /*
	       * Prevent the periodic code from immediately treating the
	       * current second/minute as a new update.
	       */
	      RTC_TimeTypeDef currentTime;
	      RTC_DateTypeDef currentDate;

	      HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN);
	      HAL_RTC_GetDate(&hrtc, &currentDate, RTC_FORMAT_BIN);

	      lastSecond = currentTime.Seconds;
	      lastMinute = currentTime.Minutes;
	  }
	  while (ERIC_ReadByte(&c))
	  {
	      HAL_UART_Transmit(debug_uart, &c, 1, HAL_MAX_DELAY);

	      if (c == '\r')
	      {
	          HAL_UART_Transmit(debug_uart,
	                            (uint8_t *)"\n\r",
	                            2,
	                            HAL_MAX_DELAY);
	      }
	  }

	  TerminalConsole_Task(debug_uart);

	  RTC_TimeTypeDef sTime;
	  RTC_DateTypeDef sDate;

	  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	  if (sTime.Seconds != lastSecond)
	  {
	      lastSecond = sTime.Seconds;
	      if (!TerminalConsole_IsActive())
	      {
	    	  Dashboard_DisplayTime(debug_uart);
	      }
	  }

	  if (sTime.Minutes != lastMinute)
	  {
	      lastMinute = sTime.Minutes;

	      if (!TerminalConsole_IsActive())
	      {
	    	  Dashboard_DisplayDate(debug_uart);
	    	  Dashboard_DisplayTemperature(debug_uart);
	    	  Dashboard_DisplayHumidity(debug_uart);
	      }
	  }

	  timerValue = __HAL_TIM_GET_COUNTER(&htim6);

	  if ((HAL_GetTick() - lastRefresh) >= 60000)
	  {
	      lastRefresh = HAL_GetTick();

	      if (!TerminalConsole_IsActive())
	      {
	    	  Dashboard_Refresh(debug_uart);
	      }
	  }

	  if (timerValue != lastTimerValue)
	  {
		  lastTimerValue = timerValue;

		  switch (timerValue)
		  {
		  	  case 0:
		  		  HAL_GPIO_WritePin(GPIOA, LED_Heartbeat_Pin, GPIO_PIN_SET);
		  		  break;

		  	  case 100:
//		  		  Dashboard_DisplayLoRaEnable(debug_uart);
		  		  break;

		  	  case 150:
//	  			  Dashboard_StreamTemperature(debug_uart);
		  		  break;

		  	  case 250:
		  		  HAL_GPIO_WritePin(GPIOA, LED_Heartbeat_Pin, GPIO_PIN_RESET);
		  		  break;

		  	  case 400:
		  		  break;

		  }
	  }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
