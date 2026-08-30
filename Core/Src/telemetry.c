/*
 * telemetry.c
 *
 *  Created on: 2 Aug 2026
 *      Author: alan
 */

/*
 * telemetry.c
 *
 *  Created on: 2 Aug 2026
 *      Author: alan
 */

#include "telemetry.h"

#include "rtc_service.h"
#include "sht25.h"
#include "water_sensor.h"
#include "relay.h"

#include <stdio.h>

void Telemetry_BuildString(char *buffer,
                           uint16_t buffer_size)
{
    RTCService_DateTime_t date_time;
    WaterSensor_Measurement_t water;
    float temperature;
    float humidity;
    HAL_StatusTypeDef rtc_status;
    HAL_StatusTypeDef temperature_status;
    HAL_StatusTypeDef humidity_status;
    HAL_StatusTypeDef water_status;
    RelayState_t pump_state;
    uint32_t lockout_remaining_ms;

    if ((buffer == NULL) ||
        (buffer_size == 0U))
    {
        return;
    }

    rtc_status = RTCService_GetDateTime(&date_time);

    temperature_status = SHT25_ReadTemperature(&temperature);

    humidity_status = SHT25_ReadHumidity(&humidity);

    water_status = WaterSensor_GetLatestMeasurement(&water);

    pump_state = Relay_Get(RELAY_1);

    lockout_remaining_ms = Relay1_GetLockoutRemainingMs();

    if (rtc_status != HAL_OK)
    {
        snprintf(buffer, buffer_size,
                 "WB1,"
                 "DATE=INVALID,"
                 "TIME=INVALID,"
                 "TEMP=INVALID,"
                 "HUM=INVALID,"
                 "WATER=INVALID");

        return;
    }

    if ((temperature_status == HAL_OK) &&
        (humidity_status == HAL_OK) &&
        (water_status == HAL_OK))
    {
        snprintf(buffer,
                 buffer_size,
                 "WB1,"
                 "DATE=%04u-%02u-%02u,"
                 "TIME=%02u:%02u:%02u,"
                 "TEMP=%.2fC,"
                 "HUM=%.2f%%,"
				 "WATER=%luHz,"
				 "WATER_FILT=%luHz,"
				 "PUMP=%s,"
				 "LOCKOUT=%u",
                 date_time.year,
                 date_time.month,
                 date_time.day,
                 date_time.hour,
                 date_time.minute,
                 date_time.second,
                 temperature,
                 humidity,
                 (unsigned long)water.frequency_hz,
				 (unsigned long)water.filtered_frequency_hz,
				 (pump_state == RELAY_ON) ? "ON" : "OFF",
				 (lockout_remaining_ms > 0U) ? 1U : 0U);

        return;
    }

    /*
     * Preserve valid readings while clearly marking any
     * unavailable measurement.
     */
    if ((temperature_status == HAL_OK) &&
        (humidity_status == HAL_OK))
    {
        snprintf(buffer,
                 buffer_size,
                 "WB1,"
                 "DATE=%04u-%02u-%02u,"
                 "TIME=%02u:%02u:%02u,"
                 "TEMP=%.2fC,"
                 "HUM=%.2f%%,"
                 "WATER=INVALID",
                 date_time.year,
                 date_time.month,
                 date_time.day,
                 date_time.hour,
                 date_time.minute,
                 date_time.second,
                 temperature,
                 humidity);

        return;
    }

    if (water_status == HAL_OK)
    {
        snprintf(buffer,
                 buffer_size,
                 "WB1,"
                 "DATE=%04u-%02u-%02u,"
                 "TIME=%02u:%02u:%02u,"
                 "TEMP=INVALID,"
                 "HUM=INVALID,"
                 "WATER=%luHz,"
				 "WATER_FILT=%luHz",
                 date_time.year,
                 date_time.month,
                 date_time.day,
                 date_time.hour,
                 date_time.minute,
                 date_time.second,
				 (unsigned long)water.frequency_hz,
				 (unsigned long)water.filtered_frequency_hz);
    }
    else
    {
        snprintf(buffer,
                 buffer_size,
                 "WB1,"
                 "DATE=%04u-%02u-%02u,"
                 "TIME=%02u:%02u:%02u,"
                 "TEMP=INVALID,"
                 "HUM=INVALID,"
                 "WATER=INVALID",
                 date_time.year,
                 date_time.month,
                 date_time.day,
                 date_time.hour,
                 date_time.minute,
                 date_time.second);
    }
}
