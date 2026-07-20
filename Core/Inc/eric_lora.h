/*
 * eric_lora.h
 *
 *  Created on: 9 Jul 2026
 *      Author: alan
 */

#ifndef INC_ERIC_LORA_H_
#define INC_ERIC_LORA_H_

#include "main.h"
#include <stdbool.h>

typedef enum
{
    ERIC_OK = 0,

    ERIC_BUSY,

    ERIC_TIMEOUT,

    ERIC_UART_ERROR,

    ERIC_BUFFER_OVERFLOW,

    ERIC_INVALID_ARGUMENT,

	ERIC_NOT_INITIALISED,

	ERIC_BAD_RESPONSE

} ERIC_Status_t;

ERIC_Status_t ERIC_Init(UART_HandleTypeDef *huart);

void ERIC_Task(void);

void ERIC_UART_RxByte(uint8_t byte);

ERIC_Status_t ERIC_Send(const uint8_t *data,
                        uint16_t length);

ERIC_Status_t ERIC_SendString(const char *text);

ERIC_Status_t ERIC_QueryUartBaudRate(char *response,
                                     uint16_t response_size);

ERIC_Status_t ERIC_QueryChannel(char *response,
                                uint16_t response_size);

ERIC_Status_t ERIC_QueryAirDataRate(char *response,
                                    uint16_t response_size);

ERIC_Status_t ERIC_QueryOperatingMode(char *response,
                                      uint16_t response_size);

ERIC_Status_t ERIC_SetAirDataRateB4(char *echo,
                                    uint16_t echo_size);

bool ERIC_ReadByte(uint8_t *byte);

uint16_t ERIC_Available(void);

#endif /* INC_ERIC_LORA_H_ */
