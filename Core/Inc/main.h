/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adw_uart_menu.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define A5_Pin GPIO_PIN_0
#define A5_GPIO_Port GPIOC
#define A4_Pin GPIO_PIN_1
#define A4_GPIO_Port GPIOC
#define A0_Pin GPIO_PIN_0
#define A0_GPIO_Port GPIOA
#define A1_Pin GPIO_PIN_1
#define A1_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define LED_Heartbeat_Pin GPIO_PIN_8
#define LED_Heartbeat_GPIO_Port GPIOA
#define LED_Status_Pin GPIO_PIN_9
#define LED_Status_GPIO_Port GPIOA
#define En_Relay1_Pin GPIO_PIN_10
#define En_Relay1_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define En_Relay2_Pin GPIO_PIN_3
#define En_Relay2_GPIO_Port GPIOB
#define en_LoRa_Pin GPIO_PIN_5
#define en_LoRa_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define ASCII_ESC 27
#define SET 1
#define RESET 0
#define LEFT_COL 3
#define RIGHT_COL 41
#define ADC_SCALE3V 0.001037
#define ADC_CHANS 4
#define ADC_9V 0
#define ADC_5V 1
#define ADC_3V3V 2
#define ADC_ILORA 3
#define LOOPA_MAX 4
#define LOOP_TEMP 0
#define LOOP_HUMD 1
//#define lora_uart &huart2
#define debug_uart &huart5
#define eric_uart &huart2
#define CMD_BUFFER_SIZE 64
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
