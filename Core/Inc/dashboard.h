/*
 * dashboard.h
 *
 *  Created on: 18 Jul 2026
 *      Author: alan
 */

#ifndef INC_DASHBOARD_H_
#define INC_DASHBOARD_H_

#include "main.h"

void Dashboard_Show(UART_HandleTypeDef *huart);
void Dashboard_Refresh(UART_HandleTypeDef *huart);

void Dashboard_DisplayDate(UART_HandleTypeDef *huart);
void Dashboard_DisplayWeekday(UART_HandleTypeDef *huart);
void Dashboard_DisplayTime(UART_HandleTypeDef *huart);

void Dashboard_DisplayTemperature(UART_HandleTypeDef *huart);
void Dashboard_DisplayHumidity(UART_HandleTypeDef *huart);

void Dashboard_Display9V(UART_HandleTypeDef *huart);
void Dashboard_Display5V(UART_HandleTypeDef *huart);
void Dashboard_Display3V3(UART_HandleTypeDef *huart);

void Dashboard_DisplayLoRaCurrent(UART_HandleTypeDef *huart);
void Dashboard_DisplayLoRaEnable(UART_HandleTypeDef *huart);
void Dashboard_DisplayHeartbeat(UART_HandleTypeDef *huart);

void Dashboard_SetWaterValues(uint16_t count, uint16_t gate_us, uint32_t frequency_hz);
void Dashboard_DisplayWaterCount(UART_HandleTypeDef *huart);
void Dashboard_DisplayWaterGate(UART_HandleTypeDef *huart);
void Dashboard_DisplayWaterFrequency(UART_HandleTypeDef *huart);

void Dashboard_StreamTemperature(UART_HandleTypeDef *huart);
void Dashboard_StreamHumidity(UART_HandleTypeDef *huart);

void Dashboard_UpdateAdcValues(const uint32_t *values, uint8_t count);

#endif /* INC_DASHBOARD_H_ */
