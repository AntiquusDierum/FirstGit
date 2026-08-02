/*
 * rtc_service.h
 *
 *  Created on: 18 Jul 2026
 *      Author: alan
 */

#ifndef INC_RTC_SERVICE_H_
#define INC_RTC_SERVICE_H_

#include "main.h"

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} RTCService_DateTime_t;

HAL_StatusTypeDef RTCService_GetDateTime(RTCService_DateTime_t *date_time);

void RTCService_PrintDateTime(UART_HandleTypeDef *huart);

HAL_StatusTypeDef RTCService_SetDateTime(uint8_t year,
                                         uint8_t month,
                                         uint8_t day,
                                         uint8_t hour,
                                         uint8_t minute,
                                         uint8_t second);



#endif /* INC_RTC_SERVICE_H_ */
