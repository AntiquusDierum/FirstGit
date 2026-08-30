/*
 * water_level.c
 *
 *  Created on: 30 Aug 2026
 *      Author: alan
 */

#include "water_level.h"

#include <stddef.h>


typedef struct
{
    float depth_cm;
    uint32_t frequency_hz;

} WaterLevelCalibration_t;


static const WaterLevelCalibration_t calibration[] =
{
    {   0.0f, 1942934U },
    {  10.0f, 1923931U },
    {  20.0f, 1910090U },
    {  30.0f, 1896290U },
    {  40.0f, 1883620U },
    {  50.0f, 1870067U },
    {  60.0f, 1857326U },
    {  70.0f, 1845110U },
    {  80.0f, 1834063U },
    {  90.0f, 1822194U },
    { 100.0f, 1809855U }
};


#define CALIBRATION_COUNT \
    (sizeof(calibration) / sizeof(calibration[0]))


float WaterLevel_FrequencyToCm(
    uint32_t frequency_hz)
{
    size_t i;

    /*
     * Frequency falls as more of the sensor is submerged.
     *
     * Clamp readings outside the calibrated range.
     */
    if (frequency_hz >= calibration[0].frequency_hz)
    {
        return calibration[0].depth_cm;
    }

    if (frequency_hz <=
        calibration[CALIBRATION_COUNT - 1U].frequency_hz)
    {
        return calibration[CALIBRATION_COUNT - 1U].depth_cm;
    }

    /*
     * Find the two calibration points surrounding
     * the measured frequency and interpolate between them.
     */
    for (i = 0U; i < (CALIBRATION_COUNT - 1U); i++)
    {
        const WaterLevelCalibration_t *upper;
        const WaterLevelCalibration_t *lower;

        upper = &calibration[i];
        lower = &calibration[i + 1U];

        if ((frequency_hz <= upper->frequency_hz) &&
            (frequency_hz >= lower->frequency_hz))
        {
            float fraction;

            fraction =
                (float)(upper->frequency_hz - frequency_hz) /
                (float)(upper->frequency_hz -
                        lower->frequency_hz);

            return
                upper->depth_cm +
                fraction *
                (lower->depth_cm - upper->depth_cm);
        }
    }

    return 0.0f;
}

float WaterLevel_DepthToPercent(float depth_cm)
{
    const float empty_depth_cm = 10.0f;
    const float full_depth_cm = 80.0f;

    float percent;

    if (depth_cm <= empty_depth_cm)
    {
        return 0.0f;
    }

    if (depth_cm >= full_depth_cm)
    {
        return 100.0f;
    }

    percent =
        ((depth_cm - empty_depth_cm) /
         (full_depth_cm - empty_depth_cm)) *
        100.0f;

    return percent;
}

float WaterLevel_DepthToLitres(
    float depth_cm)
{
    const float empty_depth_cm = 10.0f;
    const float full_depth_cm = 80.0f;
    const float usable_capacity_litres = 83.0f;

    float litres;

    if (depth_cm <= empty_depth_cm)
    {
        return 0.0f;
    }

    if (depth_cm >= full_depth_cm)
    {
        return usable_capacity_litres;
    }

    litres =
        ((depth_cm - empty_depth_cm) /
         (full_depth_cm - empty_depth_cm)) *
        usable_capacity_litres;

    return litres;
}
