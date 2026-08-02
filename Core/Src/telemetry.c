/*
 * telemetry.c
 *
 *  Created on: 2 Aug 2026
 *      Author: alan
 */

#include "telemetry.h"

#include <stdio.h>

void Telemetry_BuildString(char *buffer,
                           uint16_t buffer_size)
{
    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    snprintf(buffer,
             buffer_size,
             "WB1,Hello");
}
