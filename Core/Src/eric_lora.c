/*
 * eric_lora.c
 *
 *  Created on: 9 Jul 2026
 *      Author: alan
 */
#include "eric_lora.h"
#include "ringbuffer.h"
#include <string.h>

#define ERIC_RX_BUFFER_SIZE    256U
#define ERIC_UART_TIMEOUT_MS  1000U

static UART_HandleTypeDef *eric_handle;

static RingBuffer_t rx_fifo;

static uint8_t rx_storage[ERIC_RX_BUFFER_SIZE];

static ERIC_Status_t ERIC_SendCommand(const char *command,
                                      char *response,
                                      uint16_t response_size);

static ERIC_Status_t ERIC_ReadResponse(char *response,
                                       uint16_t response_size,
                                       uint32_t timeout_ms);

static void ERIC_ClearReceiveBuffer(void);

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

static ERIC_Status_t ERIC_SendCommand(const char *command,
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
    uint16_t index = 0U;
    uint8_t byte;

    if ((response == NULL) || (response_size < 2U))
    {
        return ERIC_INVALID_ARGUMENT;
    }

    start_tick = HAL_GetTick();

    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {
        if (ERIC_ReadByte(&byte))
        {
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
    }

    response[index] = '\0';

    return ERIC_TIMEOUT;
}

ERIC_Status_t ERIC_QueryUartBaudRate(char *response,
                                     uint16_t response_size)
{
	if ((response == NULL) || (response_size < 2U))
	{
	    return ERIC_INVALID_ARGUMENT;
	}

    return ERIC_SendCommand("ER_CMD#U?",
                            response,
                            response_size);
}

void ERIC_Task(void)
{
    static uint32_t lastTick = 0;

    if ((HAL_GetTick() - lastTick) > 2000)
    {
        lastTick = HAL_GetTick();
    }
}
