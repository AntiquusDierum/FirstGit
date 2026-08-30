/*
 * pump_control.h
 *
 *  Created on: 30 Aug 2026
 *      Author: alan
 */

#ifndef INC_PUMP_CONTROL_H_
#define INC_PUMP_CONTROL_H_

#include <stdbool.h>

#define PUMP_CONTROL_MIN_SAFE_PERCENT   10.0f
#define PUMP_CONTROL_ON_PERCENT         80.0f
#define PUMP_CONTROL_OFF_PERCENT        30.0f

void PumpControl_Init(void);

void PumpControl_Update(float water_percent);

bool PumpControl_IsRequested(void);

void PumpControl_SetAutomatic(bool enabled);

bool PumpControl_IsAutomatic(void);

#endif /* INC_PUMP_CONTROL_H_ */
