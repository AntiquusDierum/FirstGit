/*
 * sht25.c
 *
 *  Created on: 18 Jul 2026
 *      Author: alan with the help of ai
 *
 * sht25.c
 *
 * Driver for the Sensirion SHT25 temperature and humidity sensor.
 */

#include "sht25.h"

#include <stddef.h>

#define SHT25_I2C_TIMEOUT_MS    	100U
#define SHT25_TEMP_TIMEOUT_MS       100U
#define SHT25_HUMIDITY_TIMEOUT_MS    50U

/*
 * Pointer to the I2C peripheral used by the sensor.
 *
 * It is private to this source file so the rest of the program does not
 * need to know which physical I2C peripheral the SHT25 uses.
 */
static I2C_HandleTypeDef *sht25_i2c = NULL;

static HAL_StatusTypeDef SHT25_WriteCommand(uint8_t command);

static uint8_t SHT25_CalculateCrc(const uint8_t *data,
                                  uint16_t length);

static HAL_StatusTypeDef SHT25_ReadMeasurement(uint8_t *data,
                                               uint16_t length,
                                               uint32_t timeoutMs);

HAL_StatusTypeDef SHT25_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef status;

    if (hi2c == NULL)
    {
        return HAL_ERROR;
    }

    sht25_i2c = hi2c;

    status = SHT25_IsConnected();

    if (status != HAL_OK)
    {
        return status;
    }

    status = SHT25_Reset();

    if (status != HAL_OK)
    {
        return status;
    }

    return SHT25_IsConnected();
}

HAL_StatusTypeDef SHT25_IsConnected(void)
{
    if (sht25_i2c == NULL)
    {
        return HAL_ERROR;
    }

    return HAL_I2C_IsDeviceReady(sht25_i2c,
                                 SHT25_I2C_ADDRESS,
                                 3U,
								 SHT25_I2C_TIMEOUT_MS);
}
HAL_StatusTypeDef SHT25_Reset(void)
{
    uint8_t command = SHT25_CMD_SOFT_RESET;
    HAL_StatusTypeDef status;

    if (sht25_i2c == NULL)
    {
        return HAL_ERROR;
    }

    status = HAL_I2C_Master_Transmit(sht25_i2c,
                                     SHT25_I2C_ADDRESS,
                                     &command,
                                     1U,
									 SHT25_I2C_TIMEOUT_MS);

    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * The sensor needs time to reload its calibration data
     * following a soft reset.
     */
    HAL_Delay(20U);

    return HAL_OK;
}

static HAL_StatusTypeDef SHT25_WriteCommand(uint8_t command)
{
    if (sht25_i2c == NULL)
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Master_Transmit(sht25_i2c,
                                   SHT25_I2C_ADDRESS,
                                   &command,
                                   1U,
                                   SHT25_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef SHT25_ReadMeasurement(uint8_t *data,
                                               uint16_t length,
                                               uint32_t timeoutMs)
{
    uint32_t startTick;
    HAL_StatusTypeDef status;

    if ((sht25_i2c == NULL) ||
        (data == NULL) ||
        (length == 0U))
    {
        return HAL_ERROR;
    }

    startTick = HAL_GetTick();

    do
    {
        status = HAL_I2C_Master_Receive(sht25_i2c,
                                        SHT25_I2C_ADDRESS,
                                        data,
                                        length,
                                        SHT25_I2C_TIMEOUT_MS);

        if (status == HAL_OK)
        {
            return HAL_OK;
        }

        HAL_Delay(1U);

    } while ((HAL_GetTick() - startTick) < timeoutMs);

    return HAL_TIMEOUT;
}

static uint8_t SHT25_CalculateCrc(const uint8_t *data,
                                  uint16_t length)
{
    uint8_t crc = 0x00U;
    uint16_t byteIndex;
    uint8_t bitIndex;

    if (data == NULL)
    {
        return 0xFFU;
    }

    for (byteIndex = 0U; byteIndex < length; byteIndex++)
    {
        crc ^= data[byteIndex];

        for (bitIndex = 0U; bitIndex < 8U; bitIndex++)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)((crc << 1U) ^ 0x31U);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return crc;
}

HAL_StatusTypeDef SHT25_ReadTemperatureRaw(uint16_t *rawTemperature)
{
    uint8_t data[3];

    if (rawTemperature == NULL)
    {
        return HAL_ERROR;
    }

    if (SHT25_WriteCommand(SHT25_CMD_TRIGGER_TEMP_NO_HOLD) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /*
     * Typical conversion is around 85 ms for 14-bit temperature.
     */
    if (SHT25_ReadMeasurement(data,
                              sizeof(data),
                              SHT25_TEMP_TIMEOUT_MS) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (SHT25_CalculateCrc(data, 2U) != data[2])
    {
        return HAL_ERROR;
    }

    *rawTemperature =
        (((uint16_t)data[0] << 8) |
         (uint16_t)data[1]) & 0xFFFCU;

    return HAL_OK;
}

HAL_StatusTypeDef SHT25_ReadTemperature(float *temperature)
{
    uint16_t rawTemperature;
    HAL_StatusTypeDef status;

    if (temperature == NULL)
    {
        return HAL_ERROR;
    }

    status = SHT25_ReadTemperatureRaw(&rawTemperature);

    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * SHT25 temperature conversion:
     *
     * Temperature = -46.85 + 175.72 × raw / 65536
     */
    *temperature =
        -46.85f +
        (175.72f * (float)rawTemperature / 65536.0f);

    return HAL_OK;
}

HAL_StatusTypeDef SHT25_ReadHumidityRaw(uint16_t *rawHumidity)
{
    uint8_t data[3];

    if (rawHumidity == NULL)
    {
        return HAL_ERROR;
    }

    if (SHT25_WriteCommand(SHT25_CMD_TRIGGER_HUMIDITY_NO_HOLD) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (SHT25_ReadMeasurement(data,
                              sizeof(data),
                              SHT25_HUMIDITY_TIMEOUT_MS) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (SHT25_CalculateCrc(data, 2U) != data[2])
    {
        return HAL_ERROR;
    }

    *rawHumidity =
        ((((uint16_t)data[0]) << 8) |
          (uint16_t)data[1]) & 0xFFFCU;

    return HAL_OK;
}

HAL_StatusTypeDef SHT25_ReadHumidity(float *humidity)
{
    uint16_t rawHumidity;
    HAL_StatusTypeDef status;

    if (humidity == NULL)
    {
        return HAL_ERROR;
    }

    status = SHT25_ReadHumidityRaw(&rawHumidity);

    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * SHT25 humidity conversion:
     *
     * RH = -6 + 125 × raw / 65536
     */
    *humidity =
        -6.0f +
        (125.0f * (float)rawHumidity / 65536.0f);

    return HAL_OK;
}
