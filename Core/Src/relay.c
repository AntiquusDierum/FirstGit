/*
 * relay.c
 *
 *  Created on: 21 Aug 2026
 *      Author: alan
 */

#include "relay.h"
#include <stdbool.h>

static RelayState_t relay1_state = RELAY_OFF;
static RelayState_t relay2_state = RELAY_OFF;

#define RELAY1_DEFAULT_TIMEOUT_MS   10000U
#define RELAY1_DEFAULT_LOCKOUT_MS   20000U

static uint32_t relay1_timeout_ms = RELAY1_DEFAULT_TIMEOUT_MS;
static uint32_t relay1_lockout_ms = RELAY1_DEFAULT_LOCKOUT_MS;
static uint32_t relay1_on_started = 0U;
static uint32_t relay1_lockout_started = 0U;
static bool relay1_lockout_active = false;

void Relay_Init(void)
{
    HAL_GPIO_WritePin(
        En_Relay1_GPIO_Port,
        En_Relay1_Pin,
        GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        En_Relay2_GPIO_Port,
        En_Relay2_Pin,
        GPIO_PIN_RESET);

    relay1_state = RELAY_OFF;
    relay2_state = RELAY_OFF;

    relay1_on_started = 0U;
    relay1_lockout_started = 0U;
    relay1_lockout_active = false;
}

RelayResult_t Relay_Set(Relay_t relay, RelayState_t state)
{
    GPIO_PinState pin_state;

    pin_state =
        (state == RELAY_ON)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;

    switch (relay)
    {
    	case RELAY_1:
    		if ((state == RELAY_ON) && relay1_lockout_active)
    		{
    			return RELAY_RESULT_LOCKED_OUT;
    		}

    		if ((state == RELAY_ON) && (relay1_state == RELAY_OFF))
    		{
    			relay1_on_started = HAL_GetTick();
    		}
    		else if (state == RELAY_OFF)
    		{
    			relay1_on_started = 0U;
    		}

    		HAL_GPIO_WritePin(En_Relay1_GPIO_Port, En_Relay1_Pin, pin_state);

    		relay1_state = state;

    		return RELAY_RESULT_OK;

    	case RELAY_2:
    	    HAL_GPIO_WritePin(En_Relay2_GPIO_Port, En_Relay2_Pin, pin_state);

    	    relay2_state = state;

    	    return RELAY_RESULT_OK;

    	default:
    	    return RELAY_RESULT_INVALID;
    }
}

void Relay_Task(void)
{
    uint32_t now;

    now = HAL_GetTick();

    /*
     * Relay 1 automatic timeout.
     */
    if (relay1_state == RELAY_ON)
    {
        if ((now - relay1_on_started) >=
            relay1_timeout_ms)
        {
            HAL_GPIO_WritePin(
                En_Relay1_GPIO_Port,
                En_Relay1_Pin,
                GPIO_PIN_RESET);

            relay1_state = RELAY_OFF;
            relay1_on_started = 0U;

            relay1_lockout_active = true;
            relay1_lockout_started = now;
        }
    }

    /*
     * Relay 1 lock-out expiry.
     */
    if (relay1_lockout_active)
    {
        if ((now - relay1_lockout_started) >=
            relay1_lockout_ms)
        {
            relay1_lockout_active = false;
            relay1_lockout_started = 0U;
        }
    }
}

RelayState_t Relay_Get(
    Relay_t relay)
{
    switch (relay)
    {
        case RELAY_1:
            return relay1_state;

        case RELAY_2:
            return relay2_state;

        default:
            return RELAY_OFF;
    }
}
