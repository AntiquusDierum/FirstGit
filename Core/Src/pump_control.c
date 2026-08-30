/*
 * pump_control.c
 *
 *  Created on: 30 Aug 2026
 *      Author: alan
 */

#include "pump_control.h"


static bool pump_requested = false;
static bool automatic_enabled = true;

void PumpControl_Init(void)
{
    pump_requested = false;
    automatic_enabled = true;
}

void PumpControl_SetAutomatic(
    bool enabled)
{
    automatic_enabled = enabled;
}

bool PumpControl_IsAutomatic(void)
{
    return automatic_enabled;
}

void PumpControl_Update(
    float water_percent)
{
    /*
     * Absolute low-level protection.
     */
    if (water_percent <=
        PUMP_CONTROL_MIN_SAFE_PERCENT)
    {
        pump_requested = false;
        return;
    }

    /*
     * Hysteresis:
     *
     * OFF -> ON only at the upper threshold.
     * ON  -> OFF only at the lower threshold.
     *
     * Between the thresholds, retain the
     * previous requested state.
     */
    if (!pump_requested)
    {
        if (water_percent >=
            PUMP_CONTROL_ON_PERCENT)
        {
            pump_requested = true;
        }
    }
    else
    {
        if (water_percent <=
            PUMP_CONTROL_OFF_PERCENT)
        {
            pump_requested = false;
        }
    }
}


bool PumpControl_IsRequested(void)
{
    return pump_requested;
}
