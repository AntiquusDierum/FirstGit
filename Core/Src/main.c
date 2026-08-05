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
#include "water_sensor.h"
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
uint8_t debug_tx[80];
uint32_t adcBuffer[ADC_CHANS];

uint8_t debug_uart_rx;
uint8_t eric_uart_rx;

uint32_t waterSensorLastUpdate = 0;
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
        TerminalConsole_RxByte(debug_uart_rx);

        HAL_UART_Receive_IT(debug_uart,&debug_uart_rx,1);
    }
    else if (huart == eric_uart)
    {
        ERIC_UART_RxByte(eric_uart_rx);

        HAL_UART_Receive_IT(eric_uart,&eric_uart_rx,1);
    }
}

extern uint32_t adcBuffer[ADC_CHANS];

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
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
  static uint32_t lastRefresh = 0;
  uint32_t lastSecond = 0xFFFFFFFF;
  uint32_t lastMinute = 0xFFFFFFFF;
  ERIC_Status_t status;
  WaterSensor_Measurement_t water_measurement;
  static uint32_t heartbeat_cycle_start = 0U;
  static GPIO_PinState heartbeat_state = GPIO_PIN_SET;
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
  MX_UART5_Init();
  MX_RTC_Init();
  MX_TIM4_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  if (WaterSensor_Init(&htim2, &htim4) != HAL_OK)
  {
      Error_Handler();
  }
  HAL_UART_Receive_IT(debug_uart, &debug_uart_rx, 1);
  HAL_Delay(50);
  HAL_GPIO_WritePin(GPIOB, en_LoRa_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(LED_Heartbeat_GPIO_Port, LED_Heartbeat_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_Status_GPIO_Port, LED_Status_Pin, LED_STATUS_OFF_STATE);

  /* Allow the eRIC4 module to power up. */
  HAL_Delay(500);

  status = ERIC_Init(eric_uart,&eric_uart_rx);

  if (status != ERIC_OK)
  {
      Error_Handler();
  }

  status = ERIC_DetectUartBaudRate();

  if (status != ERIC_OK)
  {
      Error_Handler();
  }

  /* If you want the radio always standardised at 115200, follow detection with:
  if (eric_uart->Init.BaudRate != 115200U)
  {
      status = ERIC_SetUartBaudRate(8U);

      if (status != ERIC_OK)
      {
          Error_Handler();
      }
  }
  */

  HAL_Delay(500);

  if (status == ERIC_OK)
  {
      HAL_Delay(100);
  }

  HAL_Delay(500);

  ERIC_RefreshSettings();

  if (SHT25_Init(&hi2c1) != HAL_OK)
  {
      const char message[] = "SHT25 sensor not detected\r\n";

      HAL_UART_Transmit(debug_uart, (uint8_t *)message, sizeof(message) - 1U, HAL_MAX_DELAY);
  }

  if (HAL_ADC_Start_DMA(&hadc, adcBuffer, ADC_CHANS) != HAL_OK)
  {
      Error_Handler();
  }

  /*
   * Give the ADC time to complete at least one scan before
   * displaying ADC-derived values.
   */
  HAL_Delay(10U);

  Dashboard_Show(debug_uart);
  Dashboard_Refresh(debug_uart);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  static GPIO_PinState status_led_state = LED_STATUS_OFF_STATE;

	  GPIO_PinState required_status_state =
	      TerminalConsole_IsActive()
	          ? LED_STATUS_ON_STATE
	          : LED_STATUS_OFF_STATE;

	  if (required_status_state != status_led_state)
	  {
	      status_led_state = required_status_state;

	      HAL_GPIO_WritePin(LED_Status_GPIO_Port, LED_Status_Pin, status_led_state);
	  }

	  if ((HAL_GetTick() - waterSensorLastUpdate) >= WATER_SENSOR_UPDATE_PERIOD_MS)
	  {
	      waterSensorLastUpdate = HAL_GetTick();

	      if (WaterSensor_Measure(&water_measurement) == HAL_OK)
	      {
	          Dashboard_SetWaterValues(
	              water_measurement.count,
	              water_measurement.gate_us,
	              water_measurement.frequency_hz);
	      }
	  }
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

	  if ((HAL_GetTick() - lastRefresh) >= 60000)
	  {
	      lastRefresh = HAL_GetTick();

	      if (!TerminalConsole_IsActive())
	      {
	    	  Dashboard_Refresh(debug_uart);
	      }
	  }

	  uint32_t heartbeat_elapsed;
	  GPIO_PinState required_state;

	  heartbeat_elapsed = HAL_GetTick() - heartbeat_cycle_start;

	  /*
	   * Start a new one-second heartbeat cycle.
	   */
	  if (heartbeat_elapsed >= 1000U)
	  {
	      heartbeat_cycle_start += 1000U;
	      heartbeat_elapsed = HAL_GetTick() - heartbeat_cycle_start;
	  }

	  /*
	   * Active-low LED:
	   *
	   * First 50 ms  -> RESET -> LED on
	   * Remaining    -> SET   -> LED off
	   */
	  required_state = (heartbeat_elapsed < 50U)
	          ? GPIO_PIN_RESET
	          : GPIO_PIN_SET;

	  /*
	   * Only write to the GPIO when the required LED state changes.
	   */
	  if (required_state != heartbeat_state)
	  {
	      heartbeat_state = required_state;

	      HAL_GPIO_WritePin(LED_Heartbeat_GPIO_Port, LED_Heartbeat_Pin, heartbeat_state);
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
