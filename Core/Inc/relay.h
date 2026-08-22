/*
 * relay.h
 *
 *  Created on: 21 Aug 2026
 *      Author: alan
 */

#ifndef RELAY_H
#define RELAY_H

#include "main.h"

typedef enum
{
    RELAY_1 = 1,
    RELAY_2 = 2
} Relay_t;

typedef enum
{
    RELAY_OFF = 0,
    RELAY_ON = 1
} RelayState_t;

void Relay_Init(void);

void Relay_Set(
    Relay_t relay,
    RelayState_t state);

RelayState_t Relay_Get(
    Relay_t relay);

#endif
