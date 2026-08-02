/*
 * water_sensor.h
 *
 *  Created on: 29 Jul 2026
 *      Author: alan
 */

#ifndef INC_WATER_SENSOR_H_
#define INC_WATER_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l1xx_hal.h"
#include <stdint.h>

#define WATER_SENSOR_UPDATE_PERIOD_MS    250U

typedef struct
{
    uint16_t count;
    uint16_t gate_us;
    uint32_t frequency_hz;
} WaterSensor_Measurement_t;


/**
 * @brief Initialise the water-level frequency measurement.
 *
 * Starts the reference timer and the external pulse counter.
 *
 * @param htim_reference Timer running at 1 MHz.
 * @param htim_counter   Timer counting sensor pulses.
 *
 * @return HAL_OK if both timers started successfully.
 */
HAL_StatusTypeDef WaterSensor_Init(TIM_HandleTypeDef *htim_reference,
                                   TIM_HandleTypeDef *htim_counter);


/**
 * @brief Perform one blocking frequency measurement.
 *
 * @param measurement Destination for the completed measurement.
 *
 * @return HAL_OK if successful.
 */
HAL_StatusTypeDef WaterSensor_Measure(
    WaterSensor_Measurement_t *measurement);

/**
 * @brief Copy the most recent successful measurement.
 *
 * @param measurement Destination for the cached measurement.
 *
 * @return HAL_OK when a valid measurement is available.
 */
HAL_StatusTypeDef WaterSensor_GetLatestMeasurement(
    WaterSensor_Measurement_t *measurement);

#ifdef __cplusplus
}
#endif

#endif /* INC_WATER_SENSOR_H_ */
