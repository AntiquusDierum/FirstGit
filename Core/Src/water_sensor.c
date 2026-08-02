/*
 * water_sensor.c
 *
 *  Created on: 29 Jul 2026
 *      Author: alan
 */

#include "water_sensor.h"

#define WATER_SENSOR_GATE_US       20000U
#define WATER_SENSOR_TIMER_HZ      1000000ULL

static TIM_HandleTypeDef *water_reference_timer = NULL;
static TIM_HandleTypeDef *water_counter_timer = NULL;

static WaterSensor_Measurement_t water_latest_measurement;
static uint8_t water_latest_measurement_valid = 0U;

HAL_StatusTypeDef WaterSensor_Init(TIM_HandleTypeDef *htim_reference,
                                   TIM_HandleTypeDef *htim_counter)
{
    if ((htim_reference == NULL) || (htim_counter == NULL))
    {
        return HAL_ERROR;
    }

    water_reference_timer = htim_reference;
    water_counter_timer = htim_counter;

    water_latest_measurement.count = 0U;
    water_latest_measurement.gate_us = 0U;
    water_latest_measurement.frequency_hz = 0U;
    water_latest_measurement_valid = 0U;

    if (HAL_TIM_Base_Start(water_reference_timer) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_TIM_Base_Start(water_counter_timer) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}


HAL_StatusTypeDef WaterSensor_Measure(
    WaterSensor_Measurement_t *measurement)
{
    uint16_t time_start;
    uint16_t time_end;
    uint16_t count_start;
    uint16_t count_end;

    if ((measurement == NULL) ||
        (water_reference_timer == NULL) ||
        (water_counter_timer == NULL))
    {
        return HAL_ERROR;
    }

    time_start =
        (uint16_t)__HAL_TIM_GET_COUNTER(water_reference_timer);

    count_start =
        (uint16_t)__HAL_TIM_GET_COUNTER(water_counter_timer);

    /*
     * Unsigned subtraction allows the 16-bit reference timer
     * to roll over once during the measurement gate.
     */
    do
    {
        time_end =
            (uint16_t)__HAL_TIM_GET_COUNTER(water_reference_timer);
    }
    while ((uint16_t)(time_end - time_start) <
           WATER_SENSOR_GATE_US);

    count_end =
        (uint16_t)__HAL_TIM_GET_COUNTER(water_counter_timer);

    measurement->gate_us =
        (uint16_t)(time_end - time_start);

    measurement->count =
        (uint16_t)(count_end - count_start);

    if (measurement->gate_us == 0U)
    {
        measurement->frequency_hz = 0U;
        return HAL_ERROR;
    }

    measurement->frequency_hz =
        (uint32_t)
        (((uint64_t)measurement->count * WATER_SENSOR_TIMER_HZ) /
         measurement->gate_us);

    water_latest_measurement = *measurement;
    water_latest_measurement_valid = 1U;

    return HAL_OK;
}

HAL_StatusTypeDef WaterSensor_GetLatestMeasurement(
    WaterSensor_Measurement_t *measurement)
{
    if (measurement == NULL)
    {
        return HAL_ERROR;
    }

    if (water_latest_measurement_valid == 0U)
    {
        return HAL_ERROR;
    }

    *measurement = water_latest_measurement;

    return HAL_OK;
}
