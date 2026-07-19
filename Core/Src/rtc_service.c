/*
 * rtc_service.c
 *
 *  Created on: 18 Jul 2026
 *      Author: alan
 */
#include "rtc_service.h"

#include "rtc.h"

#include <stdio.h>
#include <string.h>

static uint8_t RTCService_IsLeapYear(uint16_t year);
static uint8_t RTCService_DaysInMonth(uint16_t year,
                                      uint8_t month);
static uint8_t RTCService_CalculateWeekday(uint16_t year,
                                           uint8_t month,
                                           uint8_t day);

void RTCService_PrintDateTime(UART_HandleTypeDef *huart)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    char buffer[64];

    if (huart == NULL)
    {
        return;
    }

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);

    /*
     * The STM32 RTC date must be read immediately after the time.
     * This unlocks the RTC shadow registers.
     */
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    snprintf(buffer,
             sizeof(buffer),
             "%02u-%02u-20%02u %02u:%02u:%02u\r\n",
             date.Date,
             date.Month,
             date.Year,
             time.Hours,
             time.Minutes,
             time.Seconds);

    HAL_UART_Transmit(huart,
                      (uint8_t *)buffer,
                      strlen(buffer),
                      HAL_MAX_DELAY);
}

HAL_StatusTypeDef RTCService_SetDateTime(uint8_t year,
                                         uint8_t month,
                                         uint8_t day,
                                         uint8_t hour,
                                         uint8_t minute,
                                         uint8_t second)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};
    uint16_t full_year = (uint16_t)year + 2000U;

    /*
     * Validate the time.
     */
    if ((hour > 23U) ||
        (minute > 59U) ||
        (second > 59U))
    {
        return HAL_ERROR;
    }

    /*
     * Validate the month.
     */
    if ((month < 1U) ||
        (month > 12U))
    {
        return HAL_ERROR;
    }

    /*
     * Validate the day against the actual month length.
     */
    if ((day < 1U) ||
        (day > RTCService_DaysInMonth(full_year, month)))
    {
        return HAL_ERROR;
    }

    time.Hours = hour;
    time.Minutes = minute;
    time.Seconds = second;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    date.Year = year;
    date.Month = month;
    date.Date = day;
    date.WeekDay =
        RTCService_CalculateWeekday(full_year,
                                    month,
                                    day);

    if (HAL_RTC_SetTime(&hrtc,
                        &time,
                        RTC_FORMAT_BIN) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_RTC_SetDate(&hrtc,
                        &date,
                        RTC_FORMAT_BIN) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}
static uint8_t RTCService_IsLeapYear(uint16_t year)
{
    if ((year % 400U) == 0U)
    {
        return 1U;
    }

    if ((year % 100U) == 0U)
    {
        return 0U;
    }

    if ((year % 4U) == 0U)
    {
        return 1U;
    }

    return 0U;
}
static uint8_t RTCService_DaysInMonth(uint16_t year,
                                      uint8_t month)
{
    switch (month)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;

        case 4:
        case 6:
        case 9:
        case 11:
            return 30;

        case 2:

            if (RTCService_IsLeapYear(year))
            {
                return 29;
            }

            return 28;

        default:

            return 0;
    }
}
static uint8_t RTCService_CalculateWeekday(uint16_t year,
                                           uint8_t month,
                                           uint8_t day)
{
    static const uint8_t offset[] =
    {
        0, 3, 2, 5, 0, 3,
        5, 1, 4, 6, 2, 4
    };

    uint16_t y = year;

    if (month < 3U)
    {
        y--;
    }

    switch ((y + y/4 - y/100 + y/400 +
             offset[month - 1U] + day) % 7U)
    {
        case 0: return RTC_WEEKDAY_SUNDAY;
        case 1: return RTC_WEEKDAY_MONDAY;
        case 2: return RTC_WEEKDAY_TUESDAY;
        case 3: return RTC_WEEKDAY_WEDNESDAY;
        case 4: return RTC_WEEKDAY_THURSDAY;
        case 5: return RTC_WEEKDAY_FRIDAY;
        default: return RTC_WEEKDAY_SATURDAY;
    }
}
