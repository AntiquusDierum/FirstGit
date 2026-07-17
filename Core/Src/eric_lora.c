/*
 * eric_lora.c
 *
 *  Created on: 9 Jul 2026
 *      Author: alan
 */
#include "eric_lora.h"
#include "ringbuffer.h"

#define ERIC_RX_BUFFER_SIZE    256U

static UART_HandleTypeDef *eric_handle;

static RingBuffer_t rx_fifo;

static uint8_t rx_storage[ERIC_RX_BUFFER_SIZE];

ERIC_Status_t ERIC_Init(UART_HandleTypeDef *huart)
{
    eric_handle = huart;

    RingBuffer_Init(&rx_fifo,
                    rx_storage,
                    ERIC_RX_BUFFER_SIZE);

    return ERIC_OK;
}

void ERIC_UART_RxByte(uint8_t byte)
{
    RingBuffer_Put(&rx_fifo, byte);
}

bool ERIC_ReadByte(uint8_t *byte)
{
    return RingBuffer_Get(&rx_fifo, byte);
}

uint16_t ERIC_Available(void)
{
    return RingBuffer_Count(&rx_fifo);
}

ERIC_Status_t ERIC_Send(const uint8_t *data,
                        uint16_t length)
{
    if (HAL_UART_Transmit(eric_handle,
                          (uint8_t *)data,
                          length,
                          1000) != HAL_OK)
    {
        return ERIC_UART_ERROR;
    }

    return ERIC_OK;
}

ERIC_Status_t ERIC_SendString(const char *text)
{
    return ERIC_Send((const uint8_t *)text,
                     strlen(text));
}

void ERIC_Task(void)
{
    static uint32_t lastTick = 0;

    if ((HAL_GetTick() - lastTick) > 2000)
    {
        lastTick = HAL_GetTick();

        HAL_GPIO_TogglePin(GPIOA, LED_Status_Pin);

        ERIC_SendString("<<< STM32 TEST >>>\r\n");
    }
}
/*
static void ERIC_Debug(const char *msg)
{
    HAL_UART_Transmit(eric_handle,
                      (uint8_t *)msg,
                      strlen(msg),
                      1000);
}
*/
