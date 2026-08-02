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
#define ERIC_UART_BAUD_CODE_COUNT  		9U
#define ERIC_UART_SETTLE_MS        		100U

static UART_HandleTypeDef *eric_handle;

static RingBuffer_t rx_fifo;

static uint8_t rx_storage[ERIC_RX_BUFFER_SIZE];

static ERIC_Settings_t eric_settings;

static uint8_t *eric_rx_byte = NULL;

static const uint32_t eric_uart_baud_rates
    [ERIC_UART_BAUD_CODE_COUNT] =
{
    1200U,      /* U0 */
    2400U,      /* U1 */
    4800U,      /* U2 */
    9600U,      /* U3 */
    19200U,     /* U4 */
    38400U,     /* U5 */
    57600U,     /* U6 */
    76800U,     /* U7 */
    115200U     /* U8 */
};

static ERIC_Status_t ERIC_SendCommandOnce(const char *command, char *response, uint16_t response_size);
static ERIC_Status_t ERIC_QueryCommand(const char *command, char *response, uint16_t response_size);
static ERIC_Status_t ERIC_ReadResponse(char *response, uint16_t response_size, uint32_t timeout_ms);
static void ERIC_ClearReceiveBuffer(void);
static ERIC_Status_t ERIC_ApplyCommand(const char *command);
static const char *ERIC_GetQueryCommand(ERIC_Parameter_t parameter);
static char ERIC_GetParameterCommandLetter(ERIC_Parameter_t parameter);
static ERIC_Status_t ERIC_BuildSetCommand(ERIC_Parameter_t parameter, uint8_t value, char *command, uint16_t command_size);
static bool ERIC_IsValidParameterResponse(const char *response, char parameter_letter);
static ERIC_Status_t ERIC_ReinitialiseUart(uint32_t baud_rate);
static bool ERIC_BaudResponseMatches(const char *response,uint8_t code);

ERIC_Status_t ERIC_Init(UART_HandleTypeDef *huart,uint8_t *rx_byte)
{
    if ((huart == NULL) || (rx_byte == NULL))
    {
        return ERIC_INVALID_ARGUMENT;
    }

    eric_handle = huart;
    eric_rx_byte = rx_byte;

    RingBuffer_Init(&rx_fifo,rx_storage,ERIC_RX_BUFFER_SIZE);

    memset(&eric_settings, 0, sizeof(eric_settings));

    strcpy(eric_settings.uart_baud, "Unknown");
    strcpy(eric_settings.air_data_rate, "Unknown");
    strcpy(eric_settings.channel, "Unknown");

    if (HAL_UART_Receive_IT(eric_handle,eric_rx_byte,1U) != HAL_OK)
    {
        eric_handle = NULL;
        eric_rx_byte = NULL;

        return ERIC_UART_ERROR;
    }

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

            /*
             * Ignore non-printable startup or framing bytes.
             */
            if ((byte < 0x20U) || (byte > 0x7EU))
            {
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

const ERIC_Settings_t *ERIC_GetSettings(void)
{
    return &eric_settings;
}

ERIC_Status_t ERIC_RefreshSettings(void)
{
    ERIC_Status_t status;
    ERIC_Status_t overall_status = ERIC_OK;

    status = ERIC_QueryUartBaudRate(eric_settings.uart_baud, sizeof(eric_settings.uart_baud));

    if ((status == ERIC_OK) && ERIC_IsValidParameterResponse(eric_settings.uart_baud,'U'))
    {
        eric_settings.uart_baud_valid = true;
    }
    else
    {
        eric_settings.uart_baud_valid = false;
        strcpy(eric_settings.uart_baud, "Unavailable");

        if (status == ERIC_OK)
        {
            status = ERIC_BAD_RESPONSE;
        }

        overall_status = status;
    }
    status = ERIC_QueryAirDataRate(eric_settings.air_data_rate, sizeof(eric_settings.air_data_rate));

    if (status == ERIC_OK)
    {
        eric_settings.air_data_rate_valid = true;
    }
    else
    {
        eric_settings.air_data_rate_valid = false;
        strcpy(eric_settings.air_data_rate, "Unavailable");

        if (overall_status == ERIC_OK)
        {
            overall_status = status;
        }
    }

    status = ERIC_QueryChannel(eric_settings.channel, sizeof(eric_settings.channel));

    if (status == ERIC_OK)
    {
        eric_settings.channel_valid = true;
    }
    else
    {
        eric_settings.channel_valid = false;
        strcpy(eric_settings.channel, "Unavailable");

        if (overall_status == ERIC_OK)
        {
            overall_status = status;
        }
    }

    return overall_status;
}
static bool ERIC_IsValidParameterResponse(const char *response, char parameter_letter)
{
    if (response == NULL)
    {
        return false;
    }

    if (strlen(response) < 9U)
    {
        return false;
    }

    if (strncmp(response, "ER_CMD#", 7U) != 0)
    {
        return false;
    }

    if (response[7] != parameter_letter)
    {
        return false;
    }

    return true;
}

static ERIC_Status_t ERIC_ReinitialiseUart(
    uint32_t baud_rate)
{
    if ((eric_handle == NULL) ||
        (eric_rx_byte == NULL) ||
        (baud_rate == 0U))
    {
        return ERIC_NOT_INITIALISED;
    }

    /*
     * Stop any outstanding interrupt reception before
     * disabling and reconfiguring the UART.
     */
    if (HAL_UART_AbortReceive(eric_handle) != HAL_OK)
    {
        return ERIC_UART_ERROR;
    }

    if (HAL_UART_DeInit(eric_handle) != HAL_OK)
    {
        return ERIC_UART_ERROR;
    }

    eric_handle->Init.BaudRate = baud_rate;

    if (HAL_UART_Init(eric_handle) != HAL_OK)
    {
        return ERIC_UART_ERROR;
    }

    /*
     * Discard bytes received using the previous baud rate.
     */
    RingBuffer_Init(&rx_fifo,
                    rx_storage,
                    ERIC_RX_BUFFER_SIZE);

    if (HAL_UART_Receive_IT(eric_handle,
                            eric_rx_byte,
                            1U) != HAL_OK)
    {
        return ERIC_UART_ERROR;
    }

    return ERIC_OK;
}

static bool ERIC_BaudResponseMatches(
    const char *response,
    uint8_t code)
{
    char expected[16];
    int written;

    if (response == NULL)
    {
        return false;
    }

    written = snprintf(expected,
                       sizeof(expected),
                       "ER_CMD#U%u",
                       (unsigned int)code);

    if ((written < 0) ||
        ((size_t)written >= sizeof(expected)))
    {
        return false;
    }

    return strcmp(response, expected) == 0;
}

ERIC_Status_t ERIC_SetUartBaudRate(uint8_t code)
{
    uint32_t old_baud_rate;
    uint32_t new_baud_rate;
    char response[16];
    ERIC_Status_t status;

    if ((eric_handle == NULL) ||
        (eric_rx_byte == NULL))
    {
        return ERIC_NOT_INITIALISED;
    }

    if (code >= ERIC_UART_BAUD_CODE_COUNT)
    {
        return ERIC_INVALID_ARGUMENT;
    }

    old_baud_rate = eric_handle->Init.BaudRate;
    new_baud_rate = eric_uart_baud_rates[code];

    /*
     * If the UART is already at this baud, do not send a
     * setting command unnecessarily.  Verify communication.
     */
    if (old_baud_rate == new_baud_rate)
    {
        status = ERIC_QueryUartBaudRate(
            response,
            sizeof(response));

        if ((status == ERIC_OK) &&
            ERIC_BaudResponseMatches(response, code))
        {
            strncpy(eric_settings.uart_baud,
                    response,
                    sizeof(eric_settings.uart_baud) - 1U);

            eric_settings.uart_baud[
                sizeof(eric_settings.uart_baud) - 1U] = '\0';

            eric_settings.uart_baud_valid = true;

            return ERIC_OK;
        }

        return (status == ERIC_OK)
             ? ERIC_BAD_RESPONSE
             : status;
    }

    /*
     * Send ER_CMD#Ux, wait for its complete echo, and send ACK
     * while both ends are still using the old baud rate.
     */
    status = ERIC_SetParameter(
        ERIC_PARAMETER_UART_BAUD,
        code);

    if (status != ERIC_OK)
    {
        return status;
    }

    /*
     * The module may change baud immediately after receiving ACK.
     */
    HAL_Delay(ERIC_UART_SETTLE_MS);

    status = ERIC_ReinitialiseUart(new_baud_rate);

    if (status != ERIC_OK)
    {
        /*
         * Make one attempt to restore the STM32 UART to its
         * previous configuration.  The module may nevertheless
         * already be using the new rate.
         */
        (void)ERIC_ReinitialiseUart(old_baud_rate);

        return status;
    }

    HAL_Delay(ERIC_UART_SETTLE_MS);

    /*
     * Verify at the newly selected baud rate.
     * ERIC_QueryCommand() already retries a timed-out query once.
     */
    status = ERIC_QueryUartBaudRate(
        response,
        sizeof(response));

    if ((status == ERIC_OK) &&
        ERIC_BaudResponseMatches(response, code))
    {
        strncpy(eric_settings.uart_baud,
                response,
                sizeof(eric_settings.uart_baud) - 1U);

        eric_settings.uart_baud[
            sizeof(eric_settings.uart_baud) - 1U] = '\0';

        eric_settings.uart_baud_valid = true;

        return ERIC_OK;
    }

    /*
     * Verification at the new rate failed.  Check whether the
     * module remained at the old rate.
     */
    if (ERIC_ReinitialiseUart(old_baud_rate) == ERIC_OK)
    {
        HAL_Delay(ERIC_UART_SETTLE_MS);

        if (ERIC_QueryUartBaudRate(
                response,
                sizeof(response)) == ERIC_OK)
        {
            /*
             * The module is still reachable at the original
             * rate, so leave the STM32 safely synchronised there.
             */
            strncpy(eric_settings.uart_baud,
                    response,
                    sizeof(eric_settings.uart_baud) - 1U);

            eric_settings.uart_baud[
                sizeof(eric_settings.uart_baud) - 1U] = '\0';

            eric_settings.uart_baud_valid = true;

            return ERIC_BAD_RESPONSE;
        }
    }

    /*
     * Neither rate verified.  Return the STM32 to the requested
     * baud because the module most likely accepted the command,
     * even though verification failed.
     */
    (void)ERIC_ReinitialiseUart(new_baud_rate);

    eric_settings.uart_baud_valid = false;

    strncpy(eric_settings.uart_baud,
            "Unverified",
            sizeof(eric_settings.uart_baud) - 1U);

    eric_settings.uart_baud[
        sizeof(eric_settings.uart_baud) - 1U] = '\0';

    return (status == ERIC_OK)
         ? ERIC_BAD_RESPONSE
         : status;
}

ERIC_Status_t ERIC_DetectUartBaudRate(void)
{
    static const uint32_t baud_rates[] =
    {
        115200U,
        19200U
    };

    char response[16];
    ERIC_Status_t status;

    if ((eric_handle == NULL) ||
        (eric_rx_byte == NULL))
    {
        return ERIC_NOT_INITIALISED;
    }

    for (uint8_t index = 0U;
         index < (sizeof(baud_rates) / sizeof(baud_rates[0]));
         index++)
    {
        status = ERIC_ReinitialiseUart(baud_rates[index]);

        if (status != ERIC_OK)
        {
            continue;
        }

        HAL_Delay(100U);

        status = ERIC_QueryUartBaudRate(
            response,
            sizeof(response));

        if ((status == ERIC_OK) &&
            ERIC_IsValidParameterResponse(response, 'U'))
        {
            strncpy(eric_settings.uart_baud,
                    response,
                    sizeof(eric_settings.uart_baud) - 1U);

            eric_settings.uart_baud[
                sizeof(eric_settings.uart_baud) - 1U] = '\0';

            eric_settings.uart_baud_valid = true;

            return ERIC_OK;
        }
    }

    eric_settings.uart_baud_valid = false;

    strncpy(eric_settings.uart_baud,
            "Unavailable",
            sizeof(eric_settings.uart_baud) - 1U);

    eric_settings.uart_baud[
        sizeof(eric_settings.uart_baud) - 1U] = '\0';

    return ERIC_TIMEOUT;
}
