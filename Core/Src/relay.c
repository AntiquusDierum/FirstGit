/*
 * relay.c
 *
 *  Created on: 21 Aug 2026
 *      Author: alan
 */

#include "relay.h"

static RelayState_t relay1_state = RELAY_OFF;
static RelayState_t relay2_state = RELAY_OFF;

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
}

void Relay_Set(
    Relay_t relay,
    RelayState_t state)
{
    GPIO_PinState pin_state;

    pin_state =
        (state == RELAY_ON)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;

    switch (relay)
    {
        case RELAY_1:
            HAL_GPIO_WritePin(
                En_Relay1_GPIO_Port,
                En_Relay1_Pin,
                pin_state);

            relay1_state = state;
            break;

        case RELAY_2:
            HAL_GPIO_WritePin(
                En_Relay2_GPIO_Port,
                En_Relay2_Pin,
                pin_state);

            relay2_state = state;
            break;

        default:
            break;
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
