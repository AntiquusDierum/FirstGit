/*
 * terminal_console.c
 *
 *  Created on: 18 Jul 2026
 *      Author: alan
 */

#include "terminal_console.h"
#include "dashboard.h"
#include "rtc_service.h"
#include "eric_lora.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * These variables are still defined in main.c.
 *
 * We will move them into this module during the next stage of the
 * refactor, once this first extraction has been built and tested.
 */
static char cmd_buffer[CMD_BUFFER_SIZE];
static uint8_t cmd_index = 0;

static volatile uint8_t cmd_ready = 0;
static volatile uint8_t command_mode = 0;
static volatile uint8_t redraw_command_screen = 0;
static volatile uint8_t redraw_dashboard = 0;
static UART_HandleTypeDef *terminal_uart = NULL;

static void TerminalConsole_Echo(const uint8_t *data,
                                 uint16_t length)
{
    if (terminal_uart != NULL)
    {
        HAL_UART_Transmit(terminal_uart,
                          (uint8_t *)data,
                          length,
                          HAL_MAX_DELAY);
    }
}
static void TerminalConsole_PrintEricResult(
    UART_HandleTypeDef *huart,
    ERIC_Status_t status,
    const char *response)
{
    char text[80];

    if (status == ERIC_OK)
    {
        snprintf(text,
                 sizeof(text),
                 "eRIC response: %s\r\n",
                 response);
    }
    else
    {
        snprintf(text,
                 sizeof(text),
                 "eRIC error: %d\r\n",
                 (int)status);
    }

    HAL_UART_Transmit(huart,
                      (uint8_t *)text,
                      strlen(text),
                      HAL_MAX_DELAY);
}

void TerminalConsole_ShowScreen(UART_HandleTypeDef *huart)
{
    const char text[] =
        "========================================\r\n"
        "         COMMAND CONSOLE\r\n"
        "========================================\r\n"
        "\r\n"
        "Type 'help' for commands.\r\n"
        "Press ENTER on a blank line to return.\r\n"
        "\r\n"
        "Command> ";

    ClrTerm(huart);
    CursorHome(huart);

    HAL_UART_Transmit(huart,
                      (uint8_t *)text,
                      strlen(text),
                      HAL_MAX_DELAY);
}

void TerminalConsole_ShowHelp(UART_HandleTypeDef *huart)
{
    const char options[] =
        "Available commands:\r\n"
        "help\r\n"
        "refresh\r\n"
        "temp\r\n"
        "humid\r\n"
        "datetime\r\n"
    	"eric baud\r\n"
    	"eric rate\r\n"
    	"eric channel\r\n"
    	"setdt yyyy-mm-dd hh:mm:ss\r\n";

    HAL_UART_Transmit(huart,
                      (uint8_t *)options,
                      strlen(options),
                      HAL_MAX_DELAY);
}

void TerminalConsole_Task(UART_HandleTypeDef *huart)
{
    char buffer[80];

    if (!cmd_ready)
    {
        return;
    }

    cmd_ready = 0;

    snprintf(buffer,
             sizeof(buffer),
             "Command=[%s]\r\n",
             cmd_buffer);

    HAL_UART_Transmit(huart,
                      (uint8_t *)buffer,
                      strlen(buffer),
                      HAL_MAX_DELAY);

    {
        const char message[] = "Executing command...\r\n";

        HAL_UART_Transmit(huart,
                          (uint8_t *)message,
                          strlen(message),
                          HAL_MAX_DELAY);
    }

    if (strcmp(cmd_buffer, "help") == 0)
    {
        TerminalConsole_ShowHelp(huart);
    }
    else if (strcmp(cmd_buffer, "refresh") == 0)
    {
    	Dashboard_Show(huart);
    	Dashboard_Refresh(huart);
    }
    else if (strcmp(cmd_buffer, "temp") == 0)
    {
    	Dashboard_StreamTemperature(huart);
    }
    else if (strcmp(cmd_buffer, "humid") == 0)
    {
    	Dashboard_StreamHumidity(huart);
    }
    else if (strcmp(cmd_buffer, "datetime") == 0)
    {
        RTCService_PrintDateTime(huart);
    }
    else if (strncmp(cmd_buffer, "setdt ", 6U) == 0)
    {
        uint8_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;

        uint16_t full_year;
        HAL_StatusTypeDef status;

        bool format_ok = true;
        size_t length = strlen(cmd_buffer);

        /*
         * Expected format:
         *
         * setdt 2026-07-17 18:33:00
         *
         * Character positions:
         *
         *  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
         *  s e t d t   2 0 2 6  -  0  7  -  1  7     1  8  :  3  3  :  0  0
         */
        if (length != 25U)
        {
            format_ok = false;
        }

        if (format_ok)
        {
            if ((cmd_buffer[10] != '-') ||
                (cmd_buffer[13] != '-') ||
                (cmd_buffer[16] != ' ') ||
                (cmd_buffer[19] != ':') ||
                (cmd_buffer[22] != ':'))
            {
                format_ok = false;
            }
        }

        /*
         * Check that each date/time field contains decimal digits.
         */
        if (format_ok)
        {
            for (uint8_t i = 6U; i < 25U; i++)
            {
                if ((i == 10U) ||
                    (i == 13U) ||
                    (i == 16U) ||
                    (i == 19U) ||
                    (i == 22U))
                {
                    continue;
                }

                if ((cmd_buffer[i] < '0') ||
                    (cmd_buffer[i] > '9'))
                {
                    format_ok = false;
                    break;
                }
            }
        }

        if (!format_ok)
        {
            const char message[] =
                "Bad format. Use:\r\n"
                "setdt yyyy-mm-dd hh:mm:ss\r\n";

            HAL_UART_Transmit(huart,
                              (uint8_t *)message,
                              strlen(message),
                              HAL_MAX_DELAY);
        }
        else
        {
            full_year =
                (uint16_t)((cmd_buffer[6] - '0') * 1000U) +
                (uint16_t)((cmd_buffer[7] - '0') * 100U) +
                (uint16_t)((cmd_buffer[8] - '0') * 10U) +
                (uint16_t)(cmd_buffer[9] - '0');

            year = (uint8_t)(full_year - 2000U);

            month =
                (uint8_t)(((cmd_buffer[11] - '0') * 10U) +
                          (cmd_buffer[12] - '0'));

            day =
                (uint8_t)(((cmd_buffer[14] - '0') * 10U) +
                          (cmd_buffer[15] - '0'));

            hour =
                (uint8_t)(((cmd_buffer[17] - '0') * 10U) +
                          (cmd_buffer[18] - '0'));

            minute =
                (uint8_t)(((cmd_buffer[20] - '0') * 10U) +
                          (cmd_buffer[21] - '0'));

            second =
                (uint8_t)(((cmd_buffer[23] - '0') * 10U) +
                          (cmd_buffer[24] - '0'));

            if ((full_year < 2000U) ||
                (full_year > 2099U) ||
                (month < 1U) ||
                (month > 12U) ||
                (day < 1U) ||
                (day > 31U) ||
                (hour > 23U) ||
                (minute > 59U) ||
                (second > 59U))
            {
                const char message[] =
                    "Date or time value out of range\r\n";

                HAL_UART_Transmit(huart,
                                  (uint8_t *)message,
                                  strlen(message),
                                  HAL_MAX_DELAY);
            }
            else
            {
            	status = RTCService_SetDateTime(year,
            	                                month,
            	                                day,
            	                                hour,
            	                                minute,
            	                                second);

            	if (status == HAL_OK)
            	{
            	    const char message[] =
            	        "Date and time updated\r\n";

            	    HAL_UART_Transmit(huart,
            	                      (uint8_t *)message,
            	                      strlen(message),
            	                      HAL_MAX_DELAY);
            	}
            	else
            	{
            	    const char message[] =
            	        "Invalid calendar date or time\r\n";

            	    HAL_UART_Transmit(huart,
            	                      (uint8_t *)message,
            	                      strlen(message),
            	                      HAL_MAX_DELAY);
            	}
            }
        }
    }
    else if (strcmp(cmd_buffer, "eric baud") == 0)
    {
        char response[32];

        ERIC_Status_t status =
            ERIC_QueryUartBaudRate(response,
                                   sizeof(response));

        TerminalConsole_PrintEricResult(huart,
                                        status,
                                        response);
    }
    else if (strcmp(cmd_buffer, "eric rate") == 0)
    {
        char response[32];

        ERIC_Status_t status =
            ERIC_QueryAirDataRate(response,
                                  sizeof(response));

        TerminalConsole_PrintEricResult(huart,
                                        status,
                                        response);
    }
    else if (strcmp(cmd_buffer, "eric channel") == 0)
    {
        char response[32];

        ERIC_Status_t status =
            ERIC_QueryChannel(response,
                              sizeof(response));

        TerminalConsole_PrintEricResult(huart,
                                        status,
                                        response);
    }
    else
    {
        const char message[] = "Unknown command\r\n";

        HAL_UART_Transmit(huart,
                          (uint8_t *)message,
                          strlen(message),
                          HAL_MAX_DELAY);
    }

    if (command_mode)
    {
        const char prompt[] = "\r\nCommand> ";

        HAL_UART_Transmit(huart,
                          (uint8_t *)prompt,
                          strlen(prompt),
                          HAL_MAX_DELAY);
    }
}

uint8_t TerminalConsole_IsActive(void)
{
    return command_mode;
}

uint8_t TerminalConsole_RedrawDashboard(void)
{
    uint8_t value = redraw_dashboard;
    redraw_dashboard = 0;
    return value;
}

uint8_t TerminalConsole_RedrawCommandScreen(void)
{
    uint8_t value = redraw_command_screen;
    redraw_command_screen = 0;
    return value;
}

void TerminalConsole_ClearRedraws(void)
{
    redraw_dashboard = 0;
    redraw_command_screen = 0;
}

void TerminalConsole_RxByte(uint8_t ch)
{
    if (ch == '\r')
    {
        if (command_mode == 0U)
        {
            /*
             * Enter command mode.
             */
            command_mode = 1U;
            cmd_index = 0U;
            cmd_buffer[0] = '\0';

            redraw_command_screen = 1U;
        }
        else
        {
            /*
             * Finish the current command line.
             */
            cmd_buffer[cmd_index] = '\0';

            if (cmd_index == 0U)
            {
                /*
                 * Blank command: leave command mode and return
                 * to the dashboard.
                 */
                command_mode = 0U;
                redraw_dashboard = 1U;
            }
            else
            {
                /*
                 * A complete command is ready for the main loop.
                 */
                cmd_ready = 1U;
            }

            cmd_index = 0U;
        }
    }
    else if ((ch == '\b') || (ch == 0x7FU))
    {
        if ((command_mode != 0U) && (cmd_index > 0U))
        {
            cmd_index--;
            cmd_buffer[cmd_index] = '\0';

            TerminalConsole_Echo((uint8_t *)"\b \b", 3U);
        }
    }
    else if (command_mode != 0U)
    {
        if (cmd_index < (CMD_BUFFER_SIZE - 1U))
        {
            cmd_buffer[cmd_index] = (char)ch;
            cmd_index++;
            cmd_buffer[cmd_index] = '\0';

            TerminalConsole_Echo(&ch, 1U);
        }
    }
}
