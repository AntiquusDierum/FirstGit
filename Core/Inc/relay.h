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

typedef enum
{
    RELAY_RESULT_OK = 0,
    RELAY_RESULT_INVALID,
    RELAY_RESULT_LOCKED_OUT

} RelayResult_t;

RelayResult_t Relay_Set(Relay_t relay, RelayState_t state);
RelayState_t Relay_Get(Relay_t relay);
void Relay_Task(void);
void Relay1_SetTimeoutMs(uint32_t timeout_ms);
void Relay1_SetLockoutMs(uint32_t lockout_ms);
uint32_t Relay1_GetTimeoutMs(void);
uint32_t Relay1_GetLockoutMs(void);
uint32_t Relay1_GetLockoutRemainingMs(void);

#endif
