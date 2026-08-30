/*
 * water_level.h
 *
 *  Created on: 30 Aug 2026
 *      Author: alan
 */

#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <stdint.h>

float WaterLevel_FrequencyToCm(uint32_t frequency_hz);
float WaterLevel_DepthToPercent(float depth_cm);

#endif
