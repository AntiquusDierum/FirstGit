/*
 * status_led.h
 *
 *  Created on: 5 Aug 2026
 *      Author: alan
 */

#ifndef INC_STATUS_LED_H_
#define INC_STATUS_LED_H_

#include "main.h"

typedef enum
{
    STATUS_LED_NORMAL = 0,
    STATUS_LED_COMMAND,
    STATUS_LED_WARNING,
    STATUS_LED_FAULT,
    STATUS_LED_BOOTING

} StatusLed_State_t;

void StatusLed_Init(void);

void StatusLed_SetState(StatusLed_State_t state);

StatusLed_State_t StatusLed_GetState(void);

void StatusLed_PulseTx(void);

void StatusLed_Task(void);

#endif /* INC_STATUS_LED_H_ */
