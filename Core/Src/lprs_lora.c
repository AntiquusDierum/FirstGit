/*
 * lprs_lora.c
 *
 *  Created on: 9 Jul 2026
 *      Author: alan
 */
#include "stm32l1xx_hal.h"
#include "lprs_lora.h"
#include "string.h"				// Included for access to strcpy
#include "stdio.h"				// Included for access to sprintf

void LoRa_Baud_2400(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U1");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}
void LoRa_Baud_4800(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U2");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}
void LoRa_Baud_9600(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U3");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}
void LoRa_Baud_19200(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U4");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}
void LoRa_Baud_38400(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U5");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}
void LoRa_Baud_31250(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U6");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}
void LoRa_Baud_76800(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U7");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}
void LoRa_Baud_115200(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U8");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}
void LoRa_Baud_GetUART(UART_HandleTypeDef * huart) {
	uint8_t lora_cmd[12];

	snprintf((char*)lora_cmd, sizeof(lora_cmd),"ER_CMD#U?");
  	HAL_UART_Transmit(huart,lora_cmd,strlen((char*)lora_cmd),HAL_MAX_DELAY);
}


