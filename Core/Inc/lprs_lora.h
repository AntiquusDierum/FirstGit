/*
 * lprs_lora.h
 *
 *  Created on: 9 Jul 2026
 *      Author: alan
 */

#ifndef LPRS_LORA_H_
#define LPRS_LORA_H_

void LoRa_Baud_2400(UART_HandleTypeDef * huart);
void LoRa_Baud_4800(UART_HandleTypeDef * huart);
void LoRa_Baud_9600(UART_HandleTypeDef * huart);
void LoRa_Baud_19200(UART_HandleTypeDef * huart);
void LoRa_Baud_38400(UART_HandleTypeDef * huart);
void LoRa_Baud_31250(UART_HandleTypeDef * huart);
void LoRa_Baud_76800(UART_HandleTypeDef * huart);
void LoRa_Baud_115200(UART_HandleTypeDef * huart);
void LoRa_Baud_GetUART(UART_HandleTypeDef * huart);

#endif /* LPRS_LORA_H_ */
