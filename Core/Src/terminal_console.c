/*
 * terminal_console.c
 *
 *  Created on: 18 Jul 2026
 *      Author: Alan & Dione
 */

#include <project_info.h>
#include "terminal_console.h"
#include "dashboard.h"
#include "rtc_service.h"
#include "eric_lora.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

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

typedef void (*TerminalCommandHandler_t)(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);

typedef struct
{
    const char *name;
    TerminalCommandHandler_t handler;
    const char *usage;
} TerminalCommand_t;

static void TerminalConsole_CommandHelp(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandRefresh(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandTemperature(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandHumidity(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandDateTime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandSetDateTime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandSetDateTime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandEric(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandVersion(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static void TerminalConsole_CommandUptime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);
static bool TerminalConsole_ParseUint8(const char *text,uint8_t *value);

static const TerminalCommand_t terminal_commands[] =
{
    {
        "datetime", TerminalConsole_CommandDateTime,
        "Show RTC date and time"
    },
	{
	    "eric",     TerminalConsole_CommandEric,
	    "eric on|off|status|test|baud|rate|channel"
	},
    {
        "help",     TerminalConsole_CommandHelp,
        "Display available commands"
    },
    {
        "humid",    TerminalConsole_CommandHumidity,
        "Stream relative humidity"
    },
    {
        "refresh",  TerminalConsole_CommandRefresh,
        "Refresh the dashboard"
    },
    {
        "setdt",    TerminalConsole_CommandSetDateTime,
        "setdt yyyy-mm-dd hh:mm:ss"
    },
    {
        "temp",     TerminalConsole_CommandTemperature,
        "Stream temperature"
    },
	{
	    "uptime",   TerminalConsole_CommandUptime,
	    "Display system uptime"
	},
    {
        "version",  TerminalConsole_CommandVersion,
        "Display firmware information"
    }
};

#define TERMINAL_COMMAND_COUNT \
    (sizeof(terminal_commands) / sizeof(terminal_commands[0]))

static void TerminalConsole_WriteString(UART_HandleTypeDef *huart, const char *text)
{
    if ((huart == NULL) ||
        (text == NULL))
    {
        return;
    }

    HAL_UART_Transmit(huart, (uint8_t *)text, (uint16_t)strlen(text), HAL_MAX_DELAY);
}

static bool TerminalConsole_DispatchCommand( UART_HandleTypeDef *huart, uint8_t argc, char *argv[]);

static void TerminalConsole_Echo(const uint8_t *data, uint16_t length)
{
    if ((terminal_uart != NULL) &&
        (data != NULL) &&
        (length > 0U))
    {
        HAL_UART_Transmit(terminal_uart, (uint8_t *)data, length, HAL_MAX_DELAY);
    }
}

static void TerminalConsole_PrintEricResult(UART_HandleTypeDef *huart, ERIC_Status_t status, const char *response)
{
    char text[80];

    if ((status == ERIC_OK) && (response != NULL))
    {
        snprintf(text, sizeof(text), "eRIC response: %s\r\n", response);
    }
    else
    {
        snprintf(text, sizeof(text), "eRIC error: %d\r\n", (int)status);
    }

    TerminalConsole_WriteString(huart, text);
}

void TerminalConsole_ShowScreen(UART_HandleTypeDef *huart)
{
    terminal_uart = huart;

    ClrTerm(huart);
    CursorHome(huart);

    TerminalConsole_WriteString(
        huart,
        "========================================\r\n"
        "         COMMAND CONSOLE\r\n"
        "========================================\r\n"
        "\r\n"
        "Type 'help' for commands.\r\n"
        "Press ENTER on a blank line to return.\r\n"
        "\r\n"
        "Command> ");
}

void TerminalConsole_ShowHelp(UART_HandleTypeDef *huart)
{
    char text[96];

    TerminalConsole_WriteString(huart,"Available commands:\r\n");

    for (size_t index = 0U; index < TERMINAL_COMMAND_COUNT; index++)
    {
        snprintf(text,sizeof(text),"%-10s %s\r\n",
            terminal_commands[index].name,
            terminal_commands[index].usage);

        TerminalConsole_WriteString(huart,text);
    }
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

void TerminalConsole_Task(UART_HandleTypeDef *huart)
{
    char *argv[TERMINAL_MAX_ARGUMENTS];
    uint8_t argc;

    if (!cmd_ready)
    {
        return;
    }

    cmd_ready = false;

    argc = TerminalConsole_Tokenise(cmd_buffer, argv, TERMINAL_MAX_ARGUMENTS);

    if (!TerminalConsole_DispatchCommand(huart, argc, argv))
    {
        TerminalConsole_WriteString(huart, "Unknown command\r\n");
    }

    if (command_mode)
    {
        TerminalConsole_WriteString(huart, "\r\nCommand> ");
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
    (void)argv;

    if (argc != 1U)
    {
        TerminalConsole_WriteString(huart, "Usage: help\r\n");
        return;
    }

    TerminalConsole_ShowHelp(huart);
}

static void TerminalConsole_CommandRefresh(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argv;

    if (argc != 1U)
    {
        TerminalConsole_WriteString(huart, "Usage: refresh\r\n");
        return;
    }

    Dashboard_Show(huart);
    Dashboard_Refresh(huart);
}

static void TerminalConsole_CommandTemperature(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argv;

    if (argc != 1U)
    {
        TerminalConsole_WriteString(huart, "Usage: temp\r\n");
        return;
    }

    Dashboard_StreamTemperature(huart);
}

static void TerminalConsole_CommandHumidity(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argv;

    if (argc != 1U)
    {
        TerminalConsole_WriteString(huart, "Usage: humid\r\n");
        return;
    }

    Dashboard_StreamHumidity(huart);
}

static void TerminalConsole_CommandDateTime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    (void)argv;

    if (argc != 1U)
    {
        TerminalConsole_WriteString(huart, "Usage: datetime\r\n");
        return;
    }

    RTCService_PrintDateTime(huart);
}

static void TerminalConsole_CommandSetDateTime(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    bool format_ok = true;

    uint16_t year = 0U;
    uint8_t month = 0U;
    uint8_t day = 0U;
    uint8_t hour = 0U;
    uint8_t minute = 0U;
    uint8_t second = 0U;

    if (argc != 3U)
    {
        TerminalConsole_WriteString(huart, "Usage: setdt yyyy-mm-dd hh:mm:ss\r\n");
        return;
    }

    size_t date_length = strlen(argv[1]);
    size_t time_length = strlen(argv[2]);

    if ((date_length != 10U) ||
        (time_length != 8U))
    {
        format_ok = false;
    }

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

    if (format_ok)
    {
        for (uint8_t index = 0U; index < 10U; index++)
        {
            if ((index != 4U) &&
                (index != 7U) &&
                ((argv[1][index] < '0') ||
                 (argv[1][index] > '9')))
            {
                format_ok = false;
                break;
            }
        }
    }

    if (format_ok)
    {
        for (uint8_t index = 0U; index < 8U; index++)
        {
            if ((index != 2U) &&
                (index != 5U) &&
                ((argv[2][index] < '0') ||
                 (argv[2][index] > '9')))
            {
                format_ok = false;
                break;
            }
        }
    }

    if (!format_ok)
    {
        TerminalConsole_WriteString(huart, "Bad format.\r\n");
        return;
    }

    year =
        ((uint16_t)(argv[1][0] - '0') * 1000U) +
        ((uint16_t)(argv[1][1] - '0') * 100U) +
        ((uint16_t)(argv[1][2] - '0') * 10U) +
        ((uint16_t)(argv[1][3] - '0'));

    month =
        ((uint8_t)(argv[1][5] - '0') * 10U) +
        ((uint8_t)(argv[1][6] - '0'));

    day =
        ((uint8_t)(argv[1][8] - '0') * 10U) +
        ((uint8_t)(argv[1][9] - '0'));

    hour =
        ((uint8_t)(argv[2][0] - '0') * 10U) +
        ((uint8_t)(argv[2][1] - '0'));

    minute =
        ((uint8_t)(argv[2][3] - '0') * 10U) +
        ((uint8_t)(argv[2][4] - '0'));

    second =
        ((uint8_t)(argv[2][6] - '0') * 10U) +
        ((uint8_t)(argv[2][7] - '0'));

    if (RTCService_SetDateTime(
            (uint8_t)(year - 2000U),
            month,
            day,
            hour,
            minute,
            second) == HAL_OK)
    {
        TerminalConsole_WriteString(huart, "Date and time updated\r\n");
    }
    else
    {
        TerminalConsole_WriteString(huart, "Invalid date or time\r\n");
    }
}

static void TerminalConsole_CommandEric(UART_HandleTypeDef *huart,
                                        uint8_t argc,
                                        char *argv[])
{
    char response[32];
    uint8_t value;
    ERIC_Status_t status;

    if ((argc < 2U) || (argc > 3U))
    {
        TerminalConsole_WriteString(
            huart,
            "Usage:\r\n"
            "  eric on|off|status|test\r\n"
            "  eric baud\r\n"
            "  eric rate [code]\r\n"
            "  eric channel [code]\r\n");

        return;
    }

    if (strcmp(argv[1], "on") == 0)
    {
        if (argc != 2U)
        {
            TerminalConsole_WriteString(
                huart,
                "Usage: eric on\r\n");

            return;
        }

        HAL_GPIO_WritePin(
            en_LoRa_GPIO_Port,
            en_LoRa_Pin,
            GPIO_PIN_SET);

        /*
         * Allow the module's regulator and eRIC4 firmware
         * time to start before querying it.
         */
        HAL_Delay(500U);

        status = ERIC_RefreshSettings();

        if (status == ERIC_OK)
        {
            TerminalConsole_WriteString(huart,"eRIC radio switched on; settings refreshed\r\n");
        }
        else
        {
            TerminalConsole_WriteString(huart,"eRIC radio switched on; settings refresh failed\r\n");
        }
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        if (argc != 2U)
        {
            TerminalConsole_WriteString(
                huart,
                "Usage: eric off\r\n");

            return;
        }

        HAL_GPIO_WritePin(
            en_LoRa_GPIO_Port,
            en_LoRa_Pin,
            GPIO_PIN_RESET);

        TerminalConsole_WriteString(
            huart,
            "eRIC radio switched off\r\n");
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        GPIO_PinState pin_state;

        if (argc != 2U)
        {
            TerminalConsole_WriteString(
                huart,
                "Usage: eric status\r\n");

            return;
        }

        pin_state = HAL_GPIO_ReadPin(
            en_LoRa_GPIO_Port,
            en_LoRa_Pin);

        if (pin_state == GPIO_PIN_SET)
        {
            TerminalConsole_WriteString(
                huart,
                "eRIC radio is on\r\n");
        }
        else
        {
            TerminalConsole_WriteString(
                huart,
                "eRIC radio is off\r\n");
        }
    }
    else if (strcmp(argv[1], "test") == 0)
    {
        static const char test_message[] =
            "Hello from Watson Cybernetics\r\n";

        if (argc != 2U)
        {
            TerminalConsole_WriteString(
                huart,
                "Usage: eric test\r\n");

            return;
        }

        if (HAL_GPIO_ReadPin(
                en_LoRa_GPIO_Port,
                en_LoRa_Pin) != GPIO_PIN_SET)
        {
            TerminalConsole_WriteString(
                huart,
                "eRIC radio is switched off\r\n");

            return;
        }

        status = ERIC_SendString(test_message);

        if (status == ERIC_OK)
        {
            TerminalConsole_WriteString(
                huart,
                "Radio test message sent\r\n");
        }
        else
        {
            char text[64];

            snprintf(text,
                     sizeof(text),
                     "Radio test failed: %d\r\n",
                     (int)status);

            TerminalConsole_WriteString(
                huart,
                text);
        }
    }
    else if (strcmp(argv[1], "baud") == 0)
    {
        if (argc == 2U)
        {
            status = ERIC_QueryUartBaudRate(
                response,
                sizeof(response));

            TerminalConsole_PrintEricResult(
                huart,
                status,
                response);

            return;
        }

        if (!TerminalConsole_ParseUint8(argv[2], &value))
        {
            TerminalConsole_WriteString(
                huart,
                "Usage: eric baud <0-8>\r\n");

            return;
        }

        status = ERIC_SetUartBaudRate(value);

        if (status == ERIC_OK)
        {
            const ERIC_Settings_t *settings =
                ERIC_GetSettings();

            TerminalConsole_WriteString(
                huart,
                "eRIC UART baud confirmed as ");

            TerminalConsole_WriteString(
                huart,
                settings->uart_baud);

            TerminalConsole_WriteString(
                huart,
                "\r\n");
        }
        else
        {
            char text[64];

            snprintf(text,
                     sizeof(text),
                     "eRIC UART baud change failed: %d\r\n",
                     (int)status);

            TerminalConsole_WriteString(
                huart,
                text);
        }
    }
    else if (strcmp(argv[1], "rate") == 0)
    {
        if (argc == 2U)
        {
            status = ERIC_QueryAirDataRate(
                response,
                sizeof(response));

            TerminalConsole_PrintEricResult(
                huart,
                status,
                response);

            return;
        }

        if (!TerminalConsole_ParseUint8(argv[2], &value))
        {
            TerminalConsole_WriteString(
                huart,
                "Usage: eric rate <numeric code>\r\n");

            return;
        }

        status = ERIC_SetParameter(
            ERIC_PARAMETER_AIR_DATA_RATE,
            value);

        if (status != ERIC_OK)
        {
            TerminalConsole_PrintEricResult(
                huart,
                status,
                "");

            return;
        }

        /*
         * Read the setting back and update the dashboard cache.
         */
        status = ERIC_RefreshSettings();

        if (status == ERIC_OK)
        {
            const ERIC_Settings_t *settings;

            settings = ERIC_GetSettings();

            TerminalConsole_WriteString(huart,"eRIC air rate confirmed as ");

            TerminalConsole_WriteString(huart,settings->air_data_rate);

            TerminalConsole_WriteString(huart,"\r\n");

        }
        else
        {
            TerminalConsole_WriteString(huart,"eRIC air rate command sent, but verification failed\r\n");
        }
    }
    else if (strcmp(argv[1], "channel") == 0)
    {
        if (argc == 2U)
        {
            status = ERIC_QueryChannel(response,sizeof(response));

            TerminalConsole_PrintEricResult(huart,status,response);

            return;
        }

        if (!TerminalConsole_ParseUint8(argv[2], &value))
        {
            TerminalConsole_WriteString(huart,"Usage: eric channel <numeric code>\r\n");

            return;
        }

        status = ERIC_SetParameter(ERIC_PARAMETER_CHANNEL,value);

        if (status != ERIC_OK)
        {
            TerminalConsole_PrintEricResult(huart,status,"");

            return;
        }

        /*
         * Read the setting back and update the dashboard cache.
         */
        status = ERIC_RefreshSettings();

        if (status == ERIC_OK)
        {
            const ERIC_Settings_t *settings;

            settings = ERIC_GetSettings();

            TerminalConsole_WriteString(huart,"eRIC channel confirmed as ");

            TerminalConsole_WriteString(huart,settings->channel);

            TerminalConsole_WriteString(huart,"\r\n");
        }
        else
        {
            TerminalConsole_WriteString(huart,"eRIC channel command sent, but verification failed\r\n");
        }
    }
    else
    {
        TerminalConsole_WriteString(
            huart,
            "Unknown eRIC command\r\n");
    }
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

static bool TerminalConsole_DispatchCommand(UART_HandleTypeDef *huart, uint8_t argc, char *argv[])
{
    if (argc == 0U)
    {
        return true;
    }

    for (size_t index = 0U; index < TERMINAL_COMMAND_COUNT; index++)
    {
        if (strcmp(argv[0], terminal_commands[index].name) == 0)
        {
            terminal_commands[index].handler(huart, argc, argv);

            return true;
        }
    }

    return false;
}

static void TerminalConsole_CommandVersion(
    UART_HandleTypeDef *huart,
    uint8_t argc,
    char *argv[])
{
    char text[96];

    (void)argv;

    if (argc != 1U)
    {
        TerminalConsole_WriteString(huart, "Usage: version\r\n");
        return;
    }

    snprintf(text, sizeof(text), "%s\r\n", FW_NAME);

    TerminalConsole_WriteString(huart, text);

    snprintf(text, sizeof(text), "Version : %s\r\n", FW_VERSION);

    TerminalConsole_WriteString(huart, text);

    snprintf(text, sizeof(text), "Board   : %s\r\n", FW_BOARD);

    TerminalConsole_WriteString(huart, text);

    snprintf(text, sizeof(text), "Built   : %s %s\r\n", __DATE__, __TIME__);

    TerminalConsole_WriteString(huart, text);
}

static void TerminalConsole_CommandUptime(
    UART_HandleTypeDef *huart,
    uint8_t argc,
    char *argv[])
{
    uint32_t total_seconds;
    uint32_t days;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    char text[96];

    (void)argv;

    if (argc != 1U)
    {
        TerminalConsole_WriteString(huart, "Usage: uptime\r\n");
        return;
    }

    total_seconds = HAL_GetTick() / 1000U;

    days = total_seconds / 86400U;
    total_seconds %= 86400U;

    hours = total_seconds / 3600U;
    total_seconds %= 3600U;

    minutes = total_seconds / 60U;
    seconds = total_seconds % 60U;

    snprintf(text, sizeof(text), "Uptime  : %lu days, %02lu:%02lu:%02lu\r\n",
        (unsigned long)days,
        (unsigned long)hours,
        (unsigned long)minutes,
        (unsigned long)seconds);

    TerminalConsole_WriteString(huart, text);
}

static bool TerminalConsole_ParseUint8(const char *text,
                                       uint8_t *value)
{
    char *end_ptr;
    unsigned long parsed_value;

    if ((text == NULL) || (value == NULL) || (*text == '\0'))
    {
        return false;
    }

    parsed_value = strtoul(text, &end_ptr, 10);

    /*
     * Reject non-numeric characters and values too large
     * for uint8_t.
     */
    if ((*end_ptr != '\0') || (parsed_value > 255UL))
    {
        return false;
    }

    *value = (uint8_t)parsed_value;

    return true;
}
