/*
 * telemetry.c
 *
 *  Created on: 2 Aug 2026
 *      Author: alan
 */

#include "telemetry.h"

#include "rtc_service.h"

#include <stdio.h>

void Telemetry_BuildString(char *buffer,uint16_t buffer_size)
{
    RTCService_DateTime_t date_time;

    if ((buffer == NULL) ||
        (buffer_size == 0U))
    {
        return;
    }

    if (RTCService_GetDateTime(&date_time) != HAL_OK)
    {
        snprintf(buffer,buffer_size,"WB1,DATE=INVALID,TIME=INVALID");

        return;
    }

    snprintf(buffer,
             buffer_size,
             "WB1,"
             "DATE=%04u-%02u-%02u,"
             "TIME=%02u:%02u:%02u",
             date_time.year,
             date_time.month,
             date_time.day,
             date_time.hour,
             date_time.minute,
             date_time.second);
}
