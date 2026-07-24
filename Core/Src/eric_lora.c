/*
 * eric_lora.c
 *
 *  Created on: 9 Jul 2026
 *      Author: alan
 */
#include "eric_lora.h"
#include "ringbuffer.h"
#include <string.h>
#include <stdio.h>

#define ERIC_RX_BUFFER_SIZE    			256U
#define ERIC_UART_TIMEOUT_MS  			1000U
#define ERIC_RESPONSE_IDLE_TIMEOUT_MS   20U

static UART_HandleTypeDef *eric_handle;

static RingBuffer_t rx_fifo;

static uint8_t rx_storage[ERIC_RX_BUFFER_SIZE];

static ERIC_Status_t ERIC_SendCommandOnce(const char *command,
                                          char *response,
                                          uint16_t response_size);

static ERIC_Status_t ERIC_QueryCommand(const char *command,
                                       char *response,
                                       uint16_t response_size);

static ERIC_Status_t ERIC_ReadResponse(char *response,
                                       uint16_t response_size,
                                       uint32_t timeout_ms);

static void ERIC_ClearReceiveBuffer(void);

static ERIC_Status_t ERIC_ApplyCommand(const char *command);

static const char *ERIC_GetQueryCommand(ERIC_Parameter_t parameter);

static char ERIC_GetParameterCommandLetter(ERIC_Parameter_t parameter);

static ERIC_Status_t ERIC_BuildSetCommand(ERIC_Parameter_t parameter,
                                          uint8_t value,
                                          char *command,
                                          uint16_t command_size);

ERIC_Status_t ERIC_Init(UART_HandleTypeDef *huart)
{
	if (huart == NULL)
	{
	    return ERIC_INVALID_ARGUMENT;
	}

    eric_handle = huart;

    RingBuffer_Init(&rx_fifo,
                    rx_storage,
                    ERIC_RX_BUFFER_SIZE);

    return ERIC_OK;
}

void ERIC_UART_RxByte(uint8_t byte)
{
    if (eric_handle == NULL)
    {
        return;
    }

    RingBuffer_Put(&rx_fifo, byte);
}

bool ERIC_ReadByte(uint8_t *byte)
{
    if ((eric_handle == NULL) || (byte == NULL))
    {
        return false;
    }

    return RingBuffer_Get(&rx_fifo, byte);
}

uint16_t ERIC_Available(void)
{
    if (eric_handle == NULL)
    {
        return 0U;
    }

    return RingBuffer_Count(&rx_fifo);
}

ERIC_Status_t ERIC_Send(const uint8_t *data,
                        uint16_t length)
{
	if (eric_handle == NULL)
	{
	    return ERIC_NOT_INITIALISED;
	}

    if ((data == NULL) || (length == 0U))
    {
        return ERIC_INVALID_ARGUMENT;
    }

    if (HAL_UART_Transmit(eric_handle,
                          (uint8_t *)data,
                          length,
                          ERIC_UART_TIMEOUT_MS) != HAL_OK)
    {
        return ERIC_UART_ERROR;
    }

    return ERIC_OK;
}

ERIC_Status_t ERIC_SendString(const char *text)
{
	if (text == NULL)
	{
	    return ERIC_INVALID_ARGUMENT;
	}

    return ERIC_Send((const uint8_t *)text,
                     (uint16_t)strlen(text));
}

static ERIC_Status_t ERIC_SendCommandOnce(const char *command,
                                          char *response,
                                          uint16_t response_size)
{
    ERIC_Status_t status;

    if (command == NULL)
    {
        return ERIC_INVALID_ARGUMENT;
    }

    ERIC_ClearReceiveBuffer();

    status = ERIC_SendString(command);

    if (status != ERIC_OK)
    {
        return status;
    }

    if (response == NULL)
    {
        return ERIC_OK;
    }

    return ERIC_ReadResponse(response,
                             response_size,
                             ERIC_UART_TIMEOUT_MS);
}

static ERIC_Status_t ERIC_QueryCommand(const char *command,
                                       char *response,
                                       uint16_t response_size)
{
    ERIC_Status_t status;

    /*
     * Some eRIC4 modules appear to discard the first command or
     * response following a power cycle. Retry a read-only query
     * once if the first attempt times out.
     */
    for (uint8_t attempt = 0U; attempt < 2U; attempt++)
    {
        status = ERIC_SendCommandOnce(command,
                                      response,
                                      response_size);

        if (status == ERIC_OK)
        {
            return ERIC_OK;
        }

        if (status != ERIC_TIMEOUT)
        {
            return status;
        }

        /*
         * Allow the module and UART receiver to settle before
         * repeating the query.
         */
        HAL_Delay(50U);
    }

    return ERIC_TIMEOUT;
}

static char ERIC_GetParameterCommandLetter(ERIC_Parameter_t parameter)
{
    switch (parameter)
    {
        case ERIC_PARAMETER_UART_BAUD:
            return 'U';

        case ERIC_PARAMETER_AIR_DATA_RATE:
            return 'B';

        case ERIC_PARAMETER_CHANNEL:
            return 'C';

        default:
            return '\0';
    }
}

static ERIC_Status_t ERIC_ApplyCommand(const char *command)
{
    char echo[32];
    ERIC_Status_t status;

    if (command == NULL)
    {
        return ERIC_INVALID_ARGUMENT;
    }

    ERIC_ClearReceiveBuffer();

    status = ERIC_SendString(command);

    if (status != ERIC_OK)
    {
        return status;
    }

    /*
     * A setting command must first be echoed completely
     * by the module.
     */
    status = ERIC_ReadResponse(echo,
                               sizeof(echo),
                               ERIC_UART_TIMEOUT_MS);

    if (status != ERIC_OK)
    {
        return status;
    }

    if (strcmp(echo, command) != 0)
    {
        return ERIC_BAD_RESPONSE;
    }

    /*
     * Only after receiving the complete echo may ACK
     * be sent.
     */
    return ERIC_SendString("ACK");
}

static void ERIC_ClearReceiveBuffer(void)
{
    uint8_t byte;

    while (ERIC_ReadByte(&byte))
    {
        /* Discard any previously received bytes. */
    }
}

static ERIC_Status_t ERIC_ReadResponse(char *response,
                                       uint16_t response_size,
                                       uint32_t timeout_ms)
{
    uint32_t start_tick;
    uint32_t last_byte_tick;
    uint16_t index = 0U;
    uint8_t byte;
    bool received_byte = false;

    if ((response == NULL) || (response_size < 2U))
    {
        return ERIC_INVALID_ARGUMENT;
    }

    response[0] = '\0';

    start_tick = HAL_GetTick();
    last_byte_tick = start_tick;

    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {
        if (ERIC_ReadByte(&byte))
        {
            received_byte = true;
            last_byte_tick = HAL_GetTick();

            if ((byte == '\r') || (byte == '\n'))
            {
                if (index > 0U)
                {
                    response[index] = '\0';
                    return ERIC_OK;
                }

                continue;
            }

            if (index >= (response_size - 1U))
            {
                response[response_size - 1U] = '\0';
                return ERIC_BUFFER_OVERFLOW;
            }

            response[index] = (char)byte;
            index++;
        }
        else if (received_byte &&
                 ((HAL_GetTick() - last_byte_tick) >=
                  ERIC_RESPONSE_IDLE_TIMEOUT_MS))
        {
            response[index] = '\0';
            return ERIC_OK;
        }
    }

    response[index] = '\0';

    return ERIC_TIMEOUT;
}

ERIC_Status_t ERIC_QueryUartBaudRate(char *response,
                                     uint16_t response_size)
{
    return ERIC_QueryParameter(ERIC_PARAMETER_UART_BAUD,
                               response,
                               response_size);
}

ERIC_Status_t ERIC_QueryAirDataRate(char *response,
                                    uint16_t response_size)
{
    return ERIC_QueryParameter(ERIC_PARAMETER_AIR_DATA_RATE,
                               response,
                               response_size);
}

ERIC_Status_t ERIC_QueryChannel(char *response,
                                uint16_t response_size)
{
    return ERIC_QueryParameter(ERIC_PARAMETER_CHANNEL,
                               response,
                               response_size);
}

static const char *ERIC_GetQueryCommand(ERIC_Parameter_t parameter)
{
    switch (parameter)
    {
        case ERIC_PARAMETER_UART_BAUD:
            return "ER_CMD#U?";

        case ERIC_PARAMETER_AIR_DATA_RATE:
            return "ER_CMD#B?";

        case ERIC_PARAMETER_CHANNEL:
            return "ER_CMD#C?";

        default:
            return NULL;
    }
}

ERIC_Status_t ERIC_QueryParameter(ERIC_Parameter_t parameter,
                                  char *response,
                                  uint16_t response_size)
{
    const char *command;

    if ((response == NULL) || (response_size < 2U))
    {
        return ERIC_INVALID_ARGUMENT;
    }

    command = ERIC_GetQueryCommand(parameter);

    if (command == NULL)
    {
        return ERIC_INVALID_ARGUMENT;
    }

    return ERIC_QueryCommand(command,
                             response,
                             response_size);
}

static ERIC_Status_t ERIC_BuildSetCommand(ERIC_Parameter_t parameter,
                                          uint8_t value,
                                          char *command,
                                          uint16_t command_size)
{
    char command_letter;
    int written;

    if ((command == NULL) || (command_size == 0U))
    {
        return ERIC_INVALID_ARGUMENT;
    }

    /*
     * Current eRIC parameter commands use a single decimal digit.
     * Parameter-specific validation can be tightened later.
     */
    if (value > 9U)
    {
        return ERIC_INVALID_ARGUMENT;
    }

    command_letter = ERIC_GetParameterCommandLetter(parameter);

    if (command_letter == '\0')
    {
        return ERIC_INVALID_ARGUMENT;
    }

    written = snprintf(command,
                       command_size,
                       "ER_CMD#%c%u",
                       command_letter,
                       (unsigned int)value);

    if ((written < 0) ||
        ((uint16_t)written >= command_size))
    {
        return ERIC_BUFFER_OVERFLOW;
    }

    return ERIC_OK;
}

ERIC_Status_t ERIC_SetParameter(ERIC_Parameter_t parameter,
                                uint8_t value)
{
    char command[16];
    ERIC_Status_t status;

    status = ERIC_BuildSetCommand(parameter,
                                  value,
                                  command,
                                  sizeof(command));

    if (status != ERIC_OK)
    {
        return status;
    }

    return ERIC_ApplyCommand(command);
}
