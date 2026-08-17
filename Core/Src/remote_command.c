/*
 * remote_command.c
 *
 *  Created on: 10 Aug 2026
 *      Author: alan
 */

#include "remote_command.h"

#include "main.h"
#include "eric_lora.h"
#include "project_info.h"
#include "status_led.h"
#include "rtc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define REMOTE_CMD_BUFFER_SIZE    80U
#define REMOTE_CMD_TIMEOUT_MS     15000U

typedef enum
{
    REMOTE_MODE_STREAM = 0,
    REMOTE_MODE_COMMAND

} RemoteCommand_Mode_t;

static RemoteCommand_Mode_t remote_mode = REMOTE_MODE_STREAM;

static char remote_cmd_buffer[REMOTE_CMD_BUFFER_SIZE];
static uint8_t remote_cmd_index = 0U;

static bool stream_just_resumed = false;
static uint32_t remote_cmd_last_activity = 0U;

static void RemoteCommand_ClearBuffer(void);
static void RemoteCommand_ProcessLine(void);
static void RemoteCommand_CommandVersion(void);
static void RemoteCommand_CommandUptime(void);
static void RemoteCommand_CommandHelp(void);
static void RemoteCommand_CommandUnknown(void);
static void RemoteCommand_CommandSetDateTime(void);

void RemoteCommand_Init(void)
{
    remote_mode = REMOTE_MODE_STREAM;

    remote_cmd_index = 0U;
    remote_cmd_buffer[0] = '\0';

    stream_just_resumed = false;
}


bool RemoteCommand_IsStreamMode(void)
{
    return (remote_mode == REMOTE_MODE_STREAM);
}


bool RemoteCommand_StreamJustResumed(void)
{
    bool result;

    result = stream_just_resumed;
    stream_just_resumed = false;

    return result;
}


void RemoteCommand_Task(void)
{
    uint8_t eric_byte;

    while (ERIC_ReadByte(&eric_byte))
    {
        if (remote_mode == REMOTE_MODE_STREAM)
        {
        	if (eric_byte == '\r')
        	{
        	    remote_mode = REMOTE_MODE_COMMAND;
        	    remote_cmd_last_activity = HAL_GetTick();

        	    RemoteCommand_ClearBuffer();

        	    /*
        	     * Solid blue indicates remote command mode.
        	     */
        	    StatusLed_SetState(STATUS_LED_COMMAND);

        	    ERIC_SendString(
        	        "REMOTE,MODE=COMMAND\r\n");
        	}
        }
        else
        {
        	remote_cmd_last_activity = HAL_GetTick();
            if (eric_byte == '\r')
            {
                remote_cmd_buffer[remote_cmd_index] = '\0';

                RemoteCommand_ProcessLine();

                RemoteCommand_ClearBuffer();
            }
            else if (eric_byte == '\n')
            {
                /*
                 * Ignore LF.
                 */
            }
            else
            {
                if (remote_cmd_index <
                    (REMOTE_CMD_BUFFER_SIZE - 1U))
                {
                    remote_cmd_buffer[remote_cmd_index] =
                        (char)eric_byte;

                    remote_cmd_index++;

                    remote_cmd_buffer[remote_cmd_index] = '\0';
                }
            }
        }
    }
    if (remote_mode == REMOTE_MODE_COMMAND)
    {
        if ((HAL_GetTick() - remote_cmd_last_activity) >=
            REMOTE_CMD_TIMEOUT_MS)
        {
            remote_mode = REMOTE_MODE_STREAM;
            stream_just_resumed = true;

            RemoteCommand_ClearBuffer();

            StatusLed_SetState(STATUS_LED_NORMAL);

            ERIC_SendString(
                "REMOTE,MODE=STREAM\r\n");
        }
    }
}


static void RemoteCommand_ClearBuffer(void)
{
    remote_cmd_index = 0U;
    remote_cmd_buffer[0] = '\0';
}


static void RemoteCommand_ProcessLine(void)
{
	if ((strcmp(remote_cmd_buffer, "quit") == 0) ||
	    (strcmp(remote_cmd_buffer, "exit") == 0))
	{
	    remote_mode = REMOTE_MODE_STREAM;
	    stream_just_resumed = true;

	    /*
	     * Return the blue LED to normal stream behaviour.
	     */
	    StatusLed_SetState(STATUS_LED_NORMAL);

	    ERIC_SendString(
	        "REMOTE,MODE=STREAM\r\n");
	}
    else if (strcmp(remote_cmd_buffer, "version") == 0)
    {
        RemoteCommand_CommandVersion();
    }
    else if (strcmp(remote_cmd_buffer, "uptime") == 0)
    {
        RemoteCommand_CommandUptime();
    }
    else if (strcmp(remote_cmd_buffer, "help") == 0)
    {
        RemoteCommand_CommandHelp();
    }
    else if (strncmp(remote_cmd_buffer, "setdt ", 6U) == 0)
    {
        RemoteCommand_CommandSetDateTime();
    }
    else
    {
        RemoteCommand_CommandUnknown();
    }
}


static void RemoteCommand_CommandVersion(void)
{
    char response[128];

    snprintf(
        response,
        sizeof(response),
        "REMOTE,VERSION=%s,"
        "BOARD=%s,"
        "BUILD=%s %s\r\n",
        FW_VERSION,
        FW_BOARD,
        __DATE__,
        __TIME__);

    ERIC_SendString(response);
}


static void RemoteCommand_CommandUptime(void)
{
    char response[96];

    uint32_t total_seconds;
    uint32_t days;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;

    total_seconds = HAL_GetTick() / 1000U;

    days = total_seconds / 86400U;
    total_seconds %= 86400U;

    hours = total_seconds / 3600U;
    total_seconds %= 3600U;

    minutes = total_seconds / 60U;
    seconds = total_seconds % 60U;

    snprintf(
        response,
        sizeof(response),
        "REMOTE,UPTIME=%lud,%02lu:%02lu:%02lu\r\n",
        (unsigned long)days,
        (unsigned long)hours,
        (unsigned long)minutes,
        (unsigned long)seconds);

    ERIC_SendString(response);
}


static void RemoteCommand_CommandHelp(void)
{
	ERIC_SendString(
	    "REMOTE,COMMANDS=version,uptime,setdt,help,quit,exit\r\n");
}


static void RemoteCommand_CommandUnknown(void)
{
    char response[128];

    snprintf(
        response,
        sizeof(response),
        "REMOTE,ERROR=UNKNOWN_COMMAND,CMD=%s\r\n",
        remote_cmd_buffer);

    ERIC_SendString(response);
}
static void RemoteCommand_CommandSetDateTime(void)
{
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;

    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    char response[80];

    /*
     * Expected format:
     *
     * setdt yyyy-mm-dd hh:mm:ss
     */
    if (sscanf(
            remote_cmd_buffer,
            "setdt %u-%u-%u %u:%u:%u",
            &year,
            &month,
            &day,
            &hour,
            &minute,
            &second) != 6)
    {
        ERIC_SendString(
            "REMOTE,ERROR=INVALID_DATETIME\r\n");

        return;
    }

    /*
     * Basic range checking.
     */
    if ((year < 2000U) ||
        (year > 2099U) ||
        (month < 1U) ||
        (month > 12U) ||
        (day < 1U) ||
        (day > 31U) ||
        (hour > 23U) ||
        (minute > 59U) ||
        (second > 59U))
    {
        ERIC_SendString(
            "REMOTE,ERROR=INVALID_DATETIME\r\n");

        return;
    }

    time.Hours = (uint8_t)hour;
    time.Minutes = (uint8_t)minute;
    time.Seconds = (uint8_t)second;

    date.Year = (uint8_t)(year - 2000U);
    date.Month = (uint8_t)month;
    date.Date = (uint8_t)day;

    /*
     * The weekday is not currently important to our application.
     */
    date.WeekDay = RTC_WEEKDAY_MONDAY;

    if ((HAL_RTC_SetTime(
            &hrtc,
            &time,
            RTC_FORMAT_BIN) != HAL_OK) ||
        (HAL_RTC_SetDate(
            &hrtc,
            &date,
            RTC_FORMAT_BIN) != HAL_OK))
    {
        ERIC_SendString(
            "REMOTE,ERROR=RTC_SET_FAILED\r\n");

        return;
    }

    snprintf(
        response,
        sizeof(response),
        "REMOTE,DATETIME=SET,"
        "DATE=%04u-%02u-%02u,"
        "TIME=%02u:%02u:%02u\r\n",
        year,
        month,
        day,
        hour,
        minute,
        second);

    ERIC_SendString(response);
}
