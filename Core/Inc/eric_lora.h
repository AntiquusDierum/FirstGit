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

typedef enum
{
    ERIC_PARAMETER_UART_BAUD = 0,
    ERIC_PARAMETER_AIR_DATA_RATE,
    ERIC_PARAMETER_CHANNEL

} ERIC_Parameter_t;

typedef struct
{
    char uart_baud[16];
    char air_data_rate[16];
    char channel[16];

    bool uart_baud_valid;
    bool air_data_rate_valid;
    bool channel_valid;
} ERIC_Settings_t;

ERIC_Status_t ERIC_Init(UART_HandleTypeDef *huart);

void ERIC_UART_RxByte(uint8_t byte);

ERIC_Status_t ERIC_Send(const uint8_t *data,
                        uint16_t length);

ERIC_Status_t ERIC_SendString(const char *text);

bool ERIC_ReadByte(uint8_t *byte);

uint16_t ERIC_Available(void);

ERIC_Status_t ERIC_RefreshSettings(void);

const ERIC_Settings_t *ERIC_GetSettings(void);

/*
 * Generic parameter query.
 */
ERIC_Status_t ERIC_QueryParameter(ERIC_Parameter_t parameter,
                                  char *response,
                                  uint16_t response_size);
/*
 * Set a parameter using its eRIC command value.
 *
 * The value is the numeric code used by the eRIC protocol,
 * for example 4 for ER_CMD#B4.
 */
ERIC_Status_t ERIC_SetParameter(ERIC_Parameter_t parameter,
                                uint8_t value);
/*
 * Convenience query functions.
 */
ERIC_Status_t ERIC_QueryUartBaudRate(char *response,
                                     uint16_t response_size);

ERIC_Status_t ERIC_QueryAirDataRate(char *response,
                                    uint16_t response_size);

ERIC_Status_t ERIC_QueryChannel(char *response,
                                uint16_t response_size);

#endif /* INC_ERIC_LORA_H_ */
