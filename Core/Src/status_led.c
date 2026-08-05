/*
 * status_led.c
 *
 *  Created on: 5 Aug 2026
 *      Author: alan
 */

#include "status_led.h"

#include <stdbool.h>

/*
 * Set these two definitions to the polarity you proved on the board.
 *
 * For an active-high LED:
 *
 *     ON  = GPIO_PIN_SET
 *     OFF = GPIO_PIN_RESET
 *
 * For an active-low LED:
 *
 *     ON  = GPIO_PIN_RESET
 *     OFF = GPIO_PIN_SET
 */
#define STATUS_LED_ON_STATE     GPIO_PIN_SET
#define STATUS_LED_OFF_STATE    GPIO_PIN_RESET

#define STATUS_LED_TX_PULSE_MS          100U
#define STATUS_LED_WARNING_HALF_MS      500U
#define STATUS_LED_BOOT_HALF_MS         100U
#define STATUS_LED_FAULT_PERIOD_MS     2000U

static StatusLed_State_t status_led_state = STATUS_LED_NORMAL;

static GPIO_PinState status_led_pin_state = STATUS_LED_OFF_STATE;

static uint32_t status_led_state_start = 0U;

static bool status_led_tx_pulse_active = false;
static uint32_t status_led_tx_pulse_start = 0U;

static void StatusLed_Write(GPIO_PinState required_state)
{
    if (required_state != status_led_pin_state)
    {
        status_led_pin_state = required_state;

        HAL_GPIO_WritePin(
            LED_Status_GPIO_Port,
            LED_Status_Pin,
            status_led_pin_state);
    }
}

void StatusLed_Init(void)
{
    status_led_state = STATUS_LED_BOOTING;
    status_led_state_start = HAL_GetTick();

    status_led_tx_pulse_active = false;
    status_led_tx_pulse_start = 0U;

    status_led_pin_state = STATUS_LED_OFF_STATE;

    HAL_GPIO_WritePin(
        LED_Status_GPIO_Port,
        LED_Status_Pin,
        STATUS_LED_OFF_STATE);
}

void StatusLed_SetState(StatusLed_State_t state)
{
    if (state != status_led_state)
    {
        status_led_state = state;
        status_led_state_start = HAL_GetTick();
    }
}

StatusLed_State_t StatusLed_GetState(void)
{
    return status_led_state;
}

void StatusLed_PulseTx(void)
{
    status_led_tx_pulse_active = true;
    status_led_tx_pulse_start = HAL_GetTick();
}

void StatusLed_Task(void)
{
    uint32_t now;
    uint32_t elapsed;
    GPIO_PinState required_state;

    now = HAL_GetTick();

    /*
     * A radio-transmission pulse temporarily overrides the normal
     * indication, except when a warning or fault is being shown.
     */
    if (status_led_tx_pulse_active &&
        (status_led_state != STATUS_LED_WARNING) &&
        (status_led_state != STATUS_LED_FAULT))
    {
        if ((now - status_led_tx_pulse_start) <
            STATUS_LED_TX_PULSE_MS)
        {
            StatusLed_Write(STATUS_LED_ON_STATE);
            return;
        }

        status_led_tx_pulse_active = false;
    }

    elapsed = now - status_led_state_start;

    switch (status_led_state)
    {
        case STATUS_LED_NORMAL:
            required_state = STATUS_LED_OFF_STATE;
            break;

        case STATUS_LED_COMMAND:
            required_state = STATUS_LED_ON_STATE;
            break;

        case STATUS_LED_WARNING:
            required_state =
                ((elapsed % 1000U) <
                 STATUS_LED_WARNING_HALF_MS)
                    ? STATUS_LED_ON_STATE
                    : STATUS_LED_OFF_STATE;
            break;

        case STATUS_LED_FAULT:
        {
            uint32_t position;

            position = elapsed % STATUS_LED_FAULT_PERIOD_MS;

            /*
             * Two 100 ms flashes:
             *
             * 0–99 ms       on
             * 100–199 ms    off
             * 200–299 ms    on
             * remainder     off
             */
            required_state =
                ((position < 100U) ||
                 ((position >= 200U) &&
                  (position < 300U)))
                    ? STATUS_LED_ON_STATE
                    : STATUS_LED_OFF_STATE;

            break;
        }

        case STATUS_LED_BOOTING:
            required_state =
                ((elapsed %
                  (2U * STATUS_LED_BOOT_HALF_MS)) <
                 STATUS_LED_BOOT_HALF_MS)
                    ? STATUS_LED_ON_STATE
                    : STATUS_LED_OFF_STATE;
            break;

        default:
            required_state = STATUS_LED_OFF_STATE;
            break;
    }

    StatusLed_Write(required_state);
}
