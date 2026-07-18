/*
 * terminal_console.c
 *
 *  Created on: 18 Jul 2026
 *      Author: alan
 */

#include "terminal_console.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * These variables are still defined in main.c.
 *
 * We will move them into this module during the next stage of the
 * refactor, once this first extraction has been built and tested.
 */
extern char cmd_buffer[];
extern volatile uint8_t cmd_ready;
extern volatile uint8_t command_mode;

/*
 * These application functions currently remain in main.c.
 *
 * Later they will move into dashboard, sensor and RTC modules.
 */
extern void DisplayWelcome(UART_HandleTypeDef *huart);
extern void RefreshDashboard(UART_HandleTypeDef *huart);
extern void StreamTemperature(UART_HandleTypeDef *huart);
extern void StreamHumidity(UART_HandleTypeDef *huart);
extern void RTC_PrintDateTime(UART_HandleTypeDef *huart);

extern HAL_StatusTypeDef RTC_SetDateTime(uint8_t year,
                                         uint8_t month,
                                         uint8_t day,
                                         uint8_t hour,
                                         uint8_t minute,
                                         uint8_t second);

void DisplayCommandScreen(UART_HandleTypeDef *huart)
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

void DisplayOptions(UART_HandleTypeDef *huart)
{
    const char options[] =
        "Available commands:\r\n"
        "help\r\n"
        "refresh\r\n"
        "temp\r\n"
        "humid\r\n"
        "datetime\r\n"
        "setdt yyyy-mm-dd hh:mm:ss\r\n";

    HAL_UART_Transmit(huart,
                      (uint8_t *)options,
                      strlen(options),
                      HAL_MAX_DELAY);
}

void TerminalCommands(UART_HandleTypeDef *huart)
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
        DisplayOptions(huart);
    }
    else if (strcmp(cmd_buffer, "refresh") == 0)
    {
        DisplayWelcome(huart);
        RefreshDashboard(huart);
    }
    else if (strcmp(cmd_buffer, "temp") == 0)
    {
        StreamTemperature(huart);
    }
    else if (strcmp(cmd_buffer, "humid") == 0)
    {
        StreamHumidity(huart);
    }
    else if (strcmp(cmd_buffer, "datetime") == 0)
    {
        RTC_PrintDateTime(huart);
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
                status = RTC_SetDateTime(year,
                                         month,
                                         day,
                                         hour,
                                         minute,
                                         second);

                snprintf(buffer,
                         sizeof(buffer),
                         "RTC status=%d\r\n",
                         (int)status);

                HAL_UART_Transmit(huart,
                                  (uint8_t *)buffer,
                                  strlen(buffer),
                                  HAL_MAX_DELAY);
            }
        }
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
