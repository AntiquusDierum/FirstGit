/*
 * sht25.h
 *
 *  Created on: 18 Jul 2026
 *      Author: alan with the help of ai
 *
 *
 *
 * Driver for the Sensirion SHT25 temperature and humidity sensor.
 */

#ifndef INC_SHT25_H_
#define INC_SHT25_H_

#include "main.h"

/*
 * The SHT25 has a fixed 7-bit I2C address of 0x40.
 *
 * STM32 HAL expects the address shifted left by one bit.
 */
#define SHT25_I2C_ADDRESS       (0x40U << 1)
#define SHT25_CMD_SOFT_RESET    0xFEU
/*
 * Measurement commands
 */
#define SHT25_CMD_TRIGGER_TEMP_NO_HOLD     0xF3U
#define SHT25_CMD_TRIGGER_HUMID_NO_HOLD    0xF5U
#define SHT25_CMD_TRIGGER_HUMIDITY_NO_HOLD    0xF5U
/*
 * Public driver functions.
 */
HAL_StatusTypeDef SHT25_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef SHT25_IsConnected(void);

HAL_StatusTypeDef SHT25_Reset(void);

HAL_StatusTypeDef SHT25_ReadTemperatureRaw(uint16_t *rawTemperature);

HAL_StatusTypeDef SHT25_ReadTemperature(float *temperature);

HAL_StatusTypeDef SHT25_ReadHumidityRaw(uint16_t *rawHumidity);

HAL_StatusTypeDef SHT25_ReadHumidity(float *humidity);

#endif /* INC_SHT25_H_ */
