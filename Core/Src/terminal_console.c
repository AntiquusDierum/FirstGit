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

#define TERMINAL_MAX_ARGUMENTS  8U

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
    if ((terminal_uart != NULL) &&
        (data != NULL) &&
        (length > 0U))
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
	terminal_uart = huart;

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

static uint8_t TerminalConsole_Tokenise(char *text,
                                       char *argv[],
                                       uint8_t argv_size)
{
    uint8_t argc = 0U;
    char *current = text;

    if ((text == NULL) ||
        (argv == NULL) ||
        (argv_size == 0U))
    {
        return 0U;
    }

    while (*current != '\0')
    {
        /*
         * Skip spaces between arguments.
         */
        while (*current == ' ')
        {
            current++;
        }

        if (*current == '\0')
        {
            break;
        }

        /*
         * Stop if the argument array is full.
         */
        if (argc >= argv_size)
        {
            break;
        }

        /*
         * Record the beginning of this argument.
         */
        argv[argc] = current;
        argc++;

        /*
         * Find the end of this argument.
         */
        while ((*current != '\0') &&
               (*current != ' '))
        {
            current++;
        }

        /*
         * Replace the separator with a string terminator.
         */
        if (*current == ' ')
        {
            *current = '\0';
            current++;
        }
    }

    return argc;
}

static void TerminalConsole_CommandHelp(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandRefresh(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandTemperature(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandHumidity(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandDateTime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandSetDateTime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandEric(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);

void TerminalConsole_Task(UART_HandleTypeDef *huart)
{
    if (!cmd_ready)
    {
        return;
    }

    cmd_ready = 0;

    char *argv[TERMINAL_MAX_ARGUMENTS];
    uint8_t argc;

    argc = TerminalConsole_Tokenise(cmd_buffer, argv, TERMINAL_MAX_ARGUMENTS);

    if ((argc == 1U) && (strcmp(argv[0], "help") == 0))
    {
        TerminalConsole_ShowHelp(huart);
    }
    else if ((argc == 1U) && (strcmp(argv[0], "refresh") == 0))
    {
    	Dashboard_Show(huart);
    	Dashboard_Refresh(huart);
    }
    else if ((argc == 1U) && (strcmp(argv[0], "temp") == 0))
    {
    	Dashboard_StreamTemperature(huart);
    }
    else if ((argc == 1U) && (strcmp(argv[0], "humid") == 0))
    {
    	Dashboard_StreamHumidity(huart);
    }
    else if ((argc == 1U) && (strcmp(argv[0], "datetime") == 0))
    {
        RTCService_PrintDateTime(huart);
    }
    else if ((argc == 3U) && (strcmp(argv[0], "setdt") == 0))
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

        size_t date_length = strlen(argv[1]);
        size_t time_length = strlen(argv[2]);

        if ((date_length != 10U) ||
            (time_length != 8U))
        {
            format_ok = false;
        }

        /*
         * Expected arguments:
         *
         * argv[1] = yyyy-mm-dd
         * argv[2] = hh:mm:ss
         */
        if (format_ok)
        {
            if ((argv[1][4] != '-') ||
                (argv[1][7] != '-') ||
                (argv[2][2] != ':') ||
                (argv[2][5] != ':'))
            {
                format_ok = false;
            }
        }

        /*
         * Check that each date/time field contains decimal digits.
         */
        if (format_ok)
        {
            for (uint8_t i = 0U; i < 10U; i++)
            {
                if ((i == 4U) || (i == 7U))
                {
                    continue;
                }

                if ((argv[1][i] < '0') ||
                    (argv[1][i] > '9'))
                {
                    format_ok = false;
                    break;
                }
            }
        }

        if (format_ok)
        {
            for (uint8_t i = 0U; i < 8U; i++)
            {
                if ((i == 2U) || (i == 5U))
                {
                    continue;
                }

                if ((argv[2][i] < '0') ||
                    (argv[2][i] > '9'))
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
        	    (uint16_t)((argv[1][0] - '0') * 1000U) +
        	    (uint16_t)((argv[1][1] - '0') * 100U) +
        	    (uint16_t)((argv[1][2] - '0') * 10U) +
        	    (uint16_t)(argv[1][3] - '0');

        	year = (uint8_t)(full_year - 2000U);

        	month =
        	    (uint8_t)(((argv[1][5] - '0') * 10U) +
        	              (argv[1][6] - '0'));

        	day =
        	    (uint8_t)(((argv[1][8] - '0') * 10U) +
        	              (argv[1][9] - '0'));

        	hour =
        	    (uint8_t)(((argv[2][0] - '0') * 10U) +
        	              (argv[2][1] - '0'));

        	minute =
        	    (uint8_t)(((argv[2][3] - '0') * 10U) +
        	              (argv[2][4] - '0'));

        	second =
        	    (uint8_t)(((argv[2][6] - '0') * 10U) +
        	              (argv[2][7] - '0'));

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
    else if ((argc == 2U) && (strcmp(argv[0], "eric") == 0) && (strcmp(argv[1], "baud") == 0))
    {
        char response[32];

        ERIC_Status_t status = ERIC_QueryUartBaudRate(response, sizeof(response));

        TerminalConsole_PrintEricResult(huart, status, response);
    }
    else if ((argc == 2U) && (strcmp(argv[0], "eric") == 0) && (strcmp(argv[1], "rate") == 0))
    {
        char response[32];

        ERIC_Status_t status = ERIC_QueryAirDataRate(response, sizeof(response));

        TerminalConsole_PrintEricResult(huart, status, response);
    }
    else if ((argc == 2U) && (strcmp(argv[0], "eric") == 0) && (strcmp(argv[1], "channel") == 0))
    {
        char response[32];

        ERIC_Status_t status = ERIC_QueryChannel(response, sizeof(response));

        TerminalConsole_PrintEricResult(huart, status, response);
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

static void TerminalConsole_CommandHelp(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    TerminalConsole_ShowHelp(huart);
}

static void TerminalConsole_CommandRefresh(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Dashboard_Show(huart);
    Dashboard_Refresh(huart);
}

static void TerminalConsole_CommandTemperature(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Dashboard_StreamTemperature(huart);
}

static void TerminalConsole_CommandHumidity(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Dashboard_StreamHumidity(huart);
}

static void TerminalConsole_CommandDateTime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    RTCService_PrintDateTime(huart);
}

void TerminalConsole_RxByte(uint8_t ch)
{
    if (ch == '\r')
    {
        if (command_mode == 0U)
        {
            /* Enter command mode. */
            command_mode = 1U;
            cmd_index = 0U;
            cmd_buffer[0] = '\0';

            redraw_command_screen = 1U;
        }
        else
        {
            /* Finish the current command line. */
            cmd_buffer[cmd_index] = '\0';

            if (cmd_index == 0U)
            {
                /* Blank command: leave command mode and return to the dashboard. */
                command_mode = 0U;
                redraw_dashboard = 1U;
            }
            else
            {
                /* A complete command is ready for the main loop. */
            	TerminalConsole_Echo((const uint8_t *)"\r\n", 2U);
            	cmd_ready = 1U;
            }

            cmd_index = 0U;
        }
    }
    else if (ch == '\n')
    {
        /* Ignore LF. This allows terminals configured for CR+LF to work without entering an extra blank command. */
    }
    else if (ch == 0x1BU)
    {
        /* Escape abandons the current command line. */
        if (command_mode != 0U)
        {
            cmd_index = 0U;
            cmd_buffer[0] = '\0';

            TerminalConsole_Echo((const uint8_t *)"\r\nCommand> ", 11U);
        }
    }
    else if ((ch == '\b') || (ch == 0x7FU))
    {
        /* Remove one character, but never erase the prompt. */
        if ((command_mode != 0U) && (cmd_index > 0U))
        {
            cmd_index--;
            cmd_buffer[cmd_index] = '\0';

            TerminalConsole_Echo((const uint8_t *)"\b \b", 3U);
        }
    }
    else if (command_mode != 0U)
    {
        /* Store and echo a normal character. */
        if (cmd_index < (CMD_BUFFER_SIZE - 1U))
        {
            cmd_buffer[cmd_index] = (char)ch;
            cmd_index++;
            cmd_buffer[cmd_index] = '\0';

            TerminalConsole_Echo(&ch, 1U);
        }
    }
}
