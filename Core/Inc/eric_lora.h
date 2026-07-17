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
#include <string.h>

typedef enum
{
    ERIC_OK = 0,

    ERIC_BUSY,

    ERIC_TIMEOUT,

    ERIC_UART_ERROR,

    ERIC_BUFFER_OVERFLOW

} ERIC_Status_t;

ERIC_Status_t ERIC_Init(UART_HandleTypeDef *huart);

void ERIC_Task(void);

void ERIC_UART_RxByte(uint8_t byte);

ERIC_Status_t ERIC_Send(const uint8_t *data,
                        uint16_t length);

ERIC_Status_t ERIC_SendString(const char *text);

bool ERIC_ReadByte(uint8_t *byte);

uint16_t ERIC_Available(void);

#endif /* INC_ERIC_LORA_H_ */
