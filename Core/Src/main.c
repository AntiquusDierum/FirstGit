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
uint32_t adcValues[ADC_CHANS];
uint8_t userLoopA = 0;
uint8_t userLoopC = 0;
char cmd_buffer[CMD_BUFFER_SIZE];
uint8_t cmd_index = 0;
volatile uint8_t cmd_ready = 0;
//volatile uint8_t cmd_mode = 0;
volatile uint8_t command_mode = 0;
volatile uint8_t redraw_command_screen = 0;
volatile uint8_t redraw_dashboard = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void InitializeTimer(void);
void DisplayWelcome(UART_HandleTypeDef * huart);
void DisplayOptions(UART_HandleTypeDef * huart);
void DisplayTable(UART_HandleTypeDef * huart);
void DisplayString(UART_HandleTypeDef * huart);
void TerminalCommands(UART_HandleTypeDef * huart);
void DisplayHeartbeat(UART_HandleTypeDef * huart);
void DisplayTemperature(UART_HandleTypeDef * huart);
void DisplayHumidity(UART_HandleTypeDef * huart);
void Display9VV(UART_HandleTypeDef * huart);
void Display5VV(UART_HandleTypeDef * huart);
void Display3V3V(UART_HandleTypeDef * huart);
void DisplayLoRaI(UART_HandleTypeDef * huart);
void DisplayLoRaEnable(UART_HandleTypeDef * huart);
void StreamTemperature(UART_HandleTypeDef * huart);
void StreamHumidity(UART_HandleTypeDef * huart);
void RTC_PrintDateTime(UART_HandleTypeDef *huart);
HAL_StatusTypeDef RTC_SetDateTime(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void RefreshDashboard(UART_HandleTypeDef *huart);
void DisplayDate(UART_HandleTypeDef *huart);
void DisplayTime(UART_HandleTypeDef *huart);
void DisplayCommandScreen(UART_HandleTypeDef *huart);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == debug_uart)
    {
        uint8_t ch = debug_rx[0];

        if (ch == '\r')
        {
            if (command_mode == 0)
            {
                /* Enter command mode. */
                command_mode = 1;
                cmd_index = 0;
                cmd_buffer[0] = '\0';

                redraw_command_screen = 1;
            }
            else
            {
                /* Finish the current command line. */
                cmd_buffer[cmd_index] = '\0';

                if (cmd_index == 0)
                {
                    /* Blank command: return to dashboard. */
                    command_mode = 0;
                    redraw_dashboard = 1;
                }
                else
                {
                    /* Let the main loop execute the command. */
                    cmd_ready = 1;
                }

                cmd_index = 0;
            }
        }
        else if (ch == '\n')
        {
            /* Ignore LF in case another terminal sends CR+LF. */
        }
        else if (command_mode)
        {
            if (cmd_index < (CMD_BUFFER_SIZE - 1U))
            {
                cmd_buffer[cmd_index++] = (char)ch;
            }
        }

        HAL_UART_Receive_IT(debug_uart, debug_rx, 1);
    }

    if (huart == eric_uart)
    {
        ERIC_UART_RxByte(eric_uart_rx);
        HAL_UART_Receive_IT(eric_uart, &eric_uart_rx, 1);
    }
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	for (int i = 0; i < ADC_CHANS; i++) {
		adcValues[i] = adcBuffer[i];	// store the values in adcValues from adcBuffer
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
//  HAL_I2C_DeInit(&hi2c1);
//  HAL_Delay(10);
//  MX_I2C1_Init();

  HAL_TIM_Base_Start(&htim6);
  HAL_UART_Receive_IT(debug_uart,debug_rx,1);
  DisplayWelcome(debug_uart);
  HAL_Delay(50);
  HAL_GPIO_WritePin(GPIOB, en_LoRa_Pin, GPIO_PIN_SET);
  SHT2x_Init(&hi2c1);

  SHT2x_SoftReset();
  HAL_Delay(20);

  ERIC_Init(eric_uart);
//  float t = SHT2x_GetTemperature(1);
//  float h = SHT2x_GetRelativeHumidity(1);
  HAL_UART_Receive_IT(eric_uart, &eric_uart_rx, 1);
//  SHT2x_SetResolution(RES_14_12);
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

	  if (redraw_command_screen)
	  {
	      redraw_command_screen = 0;
	      DisplayCommandScreen(debug_uart);
	  }

	  if (redraw_dashboard)
	  {
	      redraw_dashboard = 0;

	      DisplayWelcome(debug_uart);
	      RefreshDashboard(debug_uart);

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

	  TerminalCommands(debug_uart);

	  RTC_TimeTypeDef sTime;
	  RTC_DateTypeDef sDate;

	  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	  if (sTime.Seconds != lastSecond)
	  {
	      lastSecond = sTime.Seconds;
	      if (!command_mode)
	      {
	    	  DisplayTime(debug_uart);
	      }
	  }

	  if (sTime.Minutes != lastMinute)
	  {
	      lastMinute = sTime.Minutes;

	      if (!command_mode)
	      {
	    	  DisplayDate(debug_uart);

	    	  DisplayTemperature(debug_uart);

	    	  DisplayHumidity(debug_uart);
	      }
	  }

	  timerValue = __HAL_TIM_GET_COUNTER(&htim6);

	  if ((HAL_GetTick() - lastRefresh) >= 60000)
	  {
	      lastRefresh = HAL_GetTick();

	      if (!command_mode)
	      {
	    	  RefreshDashboard(debug_uart);
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
//		  		  DisplayLoRaEnable(debug_uart);
		  		  break;

		  	  case 150:
//	  			  StreamTemperature(debug_uart);
		  		  break;

		  	  case 250:
		  		  HAL_GPIO_WritePin(GPIOA, LED_Heartbeat_Pin, GPIO_PIN_RESET);
		  		  break;

		  	  case 400:
//	  			  StreamHumidity(debug_uart);
		  		  if (userLoopA == ADC_9V) {
//		  			  Display9VV(debug_uart);
		  		  } else if (userLoopA == ADC_5V) {
//		  			  Display5VV(debug_uart);
		  		  } else if (userLoopA == ADC_3V3V) {
//		  			  Display3V3V(debug_uart);
		  		  } else {
//		  			  DisplayLoRaI(debug_uart);
		  		  }
		  		  break;

		  }
	  }



//	    HAL_GPIO_TogglePin(GPIOA, LED_Heartbeat_Pin);
//	    HAL_GPIO_WritePin(GPIOA, LD2_Pin|LED_Heartbeat_Pin|LED_Status_Pin|En_Relay1_Pin, GPIO_PIN_RESET);
//	    HAL_Delay(500);      // 500 ms
//	    HAL_GPIO_TogglePin(GPIOA, LED_Status_Pin);
//	    HAL_Delay(500);      // 500 ms
//	    HAL_GPIO_TogglePin(GPIOA, En_Relay1_Pin);
//	    HAL_Delay(500);      // 500 ms
//	    HAL_GPIO_TogglePin(GPIOB, En_Relay2_Pin);
//	    HAL_Delay(500);      // 500 ms


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
void DisplayWelcome(UART_HandleTypeDef * huart) {
	DisplayTable(huart);
//	DisplayOptions(huart);
	DisplayPrompt(huart);
}
void DisplayCommandScreen(UART_HandleTypeDef *huart)
{
    ClrTerm(huart);
    CursorHome(huart);

    const char *text =
        "========================================\r\n"
        "         COMMAND CONSOLE\r\n"
        "========================================\r\n"
        "\r\n"
        "Type 'help' for commands.\r\n"
        "Press ENTER on a blank line to return.\r\n"
        "\r\n"
        "Command> ";

    HAL_UART_Transmit(huart,
                      (uint8_t *)text,
                      strlen(text),
                      HAL_MAX_DELAY);
}
void DisplayOptions(UART_HandleTypeDef *huart)
{
    char options_str[256];

    sprintf(options_str,
            "Available commands:\r\n"
            "help\r\n"
            "refresh\r\n"
            "temp\r\n"
            "humid\r\n"
            "datetime\r\n"
            "setdt yyyy-mm-dd hh:mm:ss\r\n");

    HAL_UART_Transmit(huart,
                      (uint8_t *)options_str,
                      strlen(options_str),
                      HAL_MAX_DELAY);
}
void DisplayTable(UART_HandleTypeDef * huart) {
	int i;

	ClrTerm(huart);
	CursorHome(huart);

	DrwTblTop(huart);			// Line 1
	DrwTtl(huart);				// Line 2
	DrwTblBarDbl(huart);		// Line 3

	for (i=0;i<15;i++) {
		DrwBlnkRow(huart);		// Lines 4-
	}

//	DrwTblBarSngl(huart);		// Line 8

//	DrwBlnkRow(huart);			// Line 9 (Left/Right Titles)

//	DrwTblBarSngl(huart);		// Line 10

//	for (i=0;i<8;i++) {
//		DrwBlnkRow(huart);		// Lines 11-18
//	}

	DrwTblBarSngl(huart);		// Line 19

	DrwBlnkRow(huart);			// Line 20

	DrwTblBase(huart);			// Line 21
}
void TerminalCommands(UART_HandleTypeDef *huart)
{
    char buffer[80];

    if (!cmd_ready)
    {
        return;
    }

    cmd_ready = 0;

    sprintf(buffer,
            "Command=[%s]\r\n",
            cmd_buffer);

    HAL_UART_Transmit(debug_uart,
                      (uint8_t *)buffer,
                      strlen(buffer),
                      HAL_MAX_DELAY);

    HAL_UART_Transmit(debug_uart,
                      (uint8_t *)"Executing command...\r\n",
                      22,
                      HAL_MAX_DELAY);


    if (strcmp(cmd_buffer, "help") == 0)
    {
        DisplayOptions(debug_uart);
    }

    else if (strcmp(cmd_buffer, "refresh") == 0)
    {
        DisplayWelcome(debug_uart);

        RefreshDashboard(debug_uart);
    }

    else if (strcmp(cmd_buffer, "temp") == 0)
    {
        StreamTemperature(debug_uart);
    }

    else if (strcmp(cmd_buffer, "humid") == 0)
    {
        StreamHumidity(debug_uart);
    }
    else if (strcmp(cmd_buffer, "datetime") == 0)
    {
        RTC_PrintDateTime(debug_uart);
    }
    else if (strncmp(cmd_buffer, "setdt ", 6) == 0)
    {
        uint8_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;

        uint16_t fullYear;
        HAL_StatusTypeDef status;

        bool format_ok = true;
        size_t length = strlen(cmd_buffer);

        /*
         * Expected format:
         *
         * setdt 2026-07-17 18:33:00
         *
         * Character positions:
         *
         *  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
         *  s e t d t   2 0 2 6  -  0  7  -  1  7     1  8  :  3  3  :  0  0
         */

        if (length != 25U)
        {
            format_ok = false;
        }

        if (format_ok)
        {
            if ((cmd_buffer[10] != '-') ||
                (cmd_buffer[13] != '-') ||
                (cmd_buffer[16] != ' ') ||
                (cmd_buffer[19] != ':') ||
                (cmd_buffer[22] != ':'))
            {
                format_ok = false;
            }
        }

        /*
         * Check that every non-separator character is a decimal digit.
         */
        if (format_ok)
        {
            for (uint8_t i = 6U; i < 25U; i++)
            {
                if ((i == 10U) ||
                    (i == 13U) ||
                    (i == 16U) ||
                    (i == 19U) ||
                    (i == 22U))
                {
                    continue;
                }

                if ((cmd_buffer[i] < '0') ||
                    (cmd_buffer[i] > '9'))
                {
                    format_ok = false;
                    break;
                }
            }
        }

        if (!format_ok)
        {
            const char message[] =
                "Bad format. Use:\r\n"
                "setdt yyyy-mm-dd hh:mm:ss\r\n";

            HAL_UART_Transmit(huart,
                              (uint8_t *)message,
                              strlen(message),
                              HAL_MAX_DELAY);
        }
        else
        {
            /*
             * Parse the fixed-position fields.
             */
            fullYear =
                (cmd_buffer[6] - '0') * 1000U +
                (cmd_buffer[7] - '0') * 100U +
                (cmd_buffer[8] - '0') * 10U +
                (cmd_buffer[9] - '0');

            year = (uint8_t)(fullYear - 2000U);

            month =
                (uint8_t)(((cmd_buffer[11] - '0') * 10U) +
                           (cmd_buffer[12] - '0'));

            day =
                (uint8_t)(((cmd_buffer[14] - '0') * 10U) +
                           (cmd_buffer[15] - '0'));

            hour =
                (uint8_t)(((cmd_buffer[17] - '0') * 10U) +
                           (cmd_buffer[18] - '0'));

            minute =
                (uint8_t)(((cmd_buffer[20] - '0') * 10U) +
                           (cmd_buffer[21] - '0'));

            second =
                (uint8_t)(((cmd_buffer[23] - '0') * 10U) +
                           (cmd_buffer[24] - '0'));

            /*
             * This is where the value-range validation goes:
             * after parsing, before RTC_SetDateTime().
             */
            if ((fullYear < 2000U) ||
                (fullYear > 2099U) ||
                (month < 1U) ||
                (month > 12U) ||
                (day < 1U) ||
                (day > 31U) ||
                (hour > 23U) ||
                (minute > 59U) ||
                (second > 59U))
            {
                const char message[] =
                    "Date or time value out of range\r\n";

                HAL_UART_Transmit(huart,
                                  (uint8_t *)message,
                                  strlen(message),
                                  HAL_MAX_DELAY);
            }
            else
            {
                /*
                 * Only call the RTC function after all validation passes.
                 */
                status = RTC_SetDateTime(year,
                                         month,
                                         day,
                                         hour,
                                         minute,
                                         second);

                snprintf(buffer,
                         sizeof(buffer),
                         "RTC status=%d\r\n",
                         status);

                HAL_UART_Transmit(huart,
                                  (uint8_t *)buffer,
                                  strlen(buffer),
                                  HAL_MAX_DELAY);
            }
        }
    }
    else
    {
//        printf("Unknown command\r\n");
    	HAL_UART_Transmit(debug_uart,
    	                  (uint8_t *)"Unknown command\r\n",
    	                  17,
    	                  HAL_MAX_DELAY);
    }
    if (command_mode)
    {
        const char prompt[] = "\r\nCommand> ";

        HAL_UART_Transmit(huart,
                          (uint8_t *)prompt,
                          strlen(prompt),
                          HAL_MAX_DELAY);
    }
}
void DisplayHeartbeat(UART_HandleTypeDef * huart) {
	char name_str[128] = "Heartbeat:\0";
	char value_str[128] = "\0";

	if (HAL_GPIO_ReadPin(GPIOA, LED_Heartbeat_Pin)) {
		sprintf(value_str,"Low(0)");
	}
	else {
		sprintf(value_str,"High(1)");
	}

	DrwCellAt(RIGHT_COL,11,name_str,value_str,huart);
}
void DisplayTemperature(UART_HandleTypeDef * huart) {
	char name_str[128] = "Temperature:\0";
	char value_str[128] = "\0";
	float sht_celsius = 0;

	sht_celsius = SHT2x_GetTemperature(1);

	sprintf(value_str,"%.1f°C",sht_celsius);
	userLoopC = LOOP_HUMD;

	DrwCellAt(LEFT_COL,16,name_str,value_str,huart);
}
void DisplayHumidity(UART_HandleTypeDef * huart) {
	char name_str[128] = "Relative Humidity:\0";
	char value_str[128] = "\0";
	float sht_humid = 0;

	sht_humid = SHT2x_GetRelativeHumidity(1);

	sprintf(value_str,"%.1f%%",sht_humid);
	userLoopC = LOOP_TEMP;

	DrwCellAt(RIGHT_COL,16,name_str,value_str,huart);
}
void Display9VV(UART_HandleTypeDef * huart) {
	char name_str[128] = "V_+9V_Vin:\0";
	char value_str[128] = "\0";
	float vin9v = 0;

	vin9v = ( (float)adcValues[ADC_9V] / 4095 ) * 3.3 * 3;
	userLoopA = ADC_5V;

	sprintf(value_str,"%.3fV",vin9v);

	DrwCellAt(LEFT_COL,4,name_str,value_str,huart);
}
void Display5VV(UART_HandleTypeDef * huart) {
	char name_str[128] = "V_+5V Rail:\0";
	char value_str[128] = "\0";
	float nucleo5v = 0;

	nucleo5v = ( ( (float)adcValues[ADC_5V] / 4095 ) * 3.3 * 1.667 );  // 1.667 = 10K / (10K + 15K)
	userLoopA = ADC_3V3V;

	sprintf(value_str,"%.3fV",nucleo5v);

	DrwCellAt(LEFT_COL,5,name_str,value_str,huart);
}
void Display3V3V(UART_HandleTypeDef * huart) {
	char name_str[128] = "V_+3V3 Rail:\0";
	char value_str[128] = "\0";
	float nucleo3v3v = 0;

	nucleo3v3v = (float)adcValues[ADC_3V3V] / 4095 * 3.3 * 1.067;
//	nucleo3v3v = (float)adcValues[ADC_MAIN3V] * ADC_SCALE3V;
	userLoopA = ADC_ILORA;

	sprintf(value_str,"%.2fV",nucleo3v3v);

	DrwCellAt(LEFT_COL,6,name_str,value_str,huart);
}
void DisplayLoRaI(UART_HandleTypeDef * huart) {
	char name_str[128] = "LoRa 5V Rail Current:\0";
	char value_str[128] = "\0";
	float lorai = 0;

	lorai = ( ( (float)adcValues[ADC_ILORA] / 4095 ) * 2660 );  // 2660 = 1000 * 3.3 / ( 0.1 * 0.0002 * 62000 ) ie Rs * 200µ * Rl
	userLoopA = ADC_9V;

	sprintf(value_str,"%.2fmA",lorai);

	DrwCellAt(LEFT_COL,7,name_str,value_str,huart);
}
void DisplayLoRaEnable(UART_HandleTypeDef * huart) {
	char name_str[128] = "LoRa Enable:\0";
	char value_str[128] = "\0";

	if (HAL_GPIO_ReadPin(GPIOB, en_LoRa_Pin)) {
		sprintf(value_str,"High(1)");
	}
	else {
		sprintf(value_str,"Low(0)");
	}

	DrwCellAt(RIGHT_COL,4,name_str,value_str,huart);
}
void DisplayDate(UART_HandleTypeDef *huart)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    char name_str[128] = "Date:\0";
    char value_str[128] = "\0";

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    sprintf(value_str,
            "20%02u-%02u-%02u",
            sDate.Year,
            sDate.Month,
            sDate.Date);

    DrwCellAt(RIGHT_COL, 5, name_str, value_str, huart);
}
void DisplayTime(UART_HandleTypeDef *huart)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    char name_str[128] = "Time:\0";
    char value_str[128] = "\0";

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    /*
     * Important:
     * You must always read the date immediately after the time.
     */

    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    sprintf(value_str,
            "%02u:%02u:%02u",
            sTime.Hours,
            sTime.Minutes,
            sTime.Seconds);

    DrwCellAt(RIGHT_COL, 6, name_str, value_str, huart);
}
void StreamTemperature(UART_HandleTypeDef * huart) {
	char name_str[128] = "Temperature:\0";
	char value_str[128] = "\0";
	float sht_celsius = 0;
	uint8_t cell_str[128];
	int i;

	sht_celsius = SHT2x_GetTemperature(1);

	sprintf(value_str,"%.1f°C\n\r",sht_celsius);

	/* construct cell string */
	strcpy((char*)cell_str,name_str);
	for (i=1;i<4;i=i+2) strcat((char*)cell_str,".");
	strcat((char*)cell_str,value_str);

	HAL_UART_Transmit(huart,cell_str,strlen((char*)cell_str),HAL_MAX_DELAY);
}
void StreamHumidity(UART_HandleTypeDef * huart) {
	char name_str[128] = "Relative Humidity:\0";
	char value_str[128] = "\0";
	float sht_humid = 0;
	uint8_t cell_str[128];
	int i;

	sht_humid = SHT2x_GetRelativeHumidity(1);

	sprintf(value_str,"%.1f%%\n\r",sht_humid);

	/* construct cell string */
	strcpy((char*)cell_str,name_str);
	for (i=1;i<4;i=i+2) strcat((char*)cell_str,".");
	strcat((char*)cell_str,value_str);

	HAL_UART_Transmit(huart,cell_str,strlen((char*)cell_str),HAL_MAX_DELAY);
}
void RTC_PrintDateTime(UART_HandleTypeDef *huart)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    char buffer[64];

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    /*
     * IMPORTANT:
     * Read the date immediately after the time.
     * The HAL requires this to unlock the shadow registers.
     */
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    sprintf(buffer,
            "%02u-%02u-20%02u %02u:%02u:%02u\r\n",
            sDate.Date,
            sDate.Month,
            sDate.Year,
            sTime.Hours,
            sTime.Minutes,
            sTime.Seconds);

    HAL_UART_Transmit(huart,
                      (uint8_t *)buffer,
                      strlen(buffer),
                      HAL_MAX_DELAY);
}
HAL_StatusTypeDef RTC_SetDateTime(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    sTime.Hours = hour;
    sTime.Minutes = minute;
    sTime.Seconds = second;

    sDate.Year = year;
    sDate.Month = month;
    sDate.Date = day;
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}
void RefreshDashboard(UART_HandleTypeDef *huart)
{
    Display9VV(huart);
    Display5VV(huart);
    Display3V3V(huart);
    DisplayLoRaI(huart);

    DisplayLoRaEnable(huart);

    DisplayDate(huart);
    DisplayTime(huart);

    DisplayTemperature(huart);
    DisplayHumidity(huart);
}
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
