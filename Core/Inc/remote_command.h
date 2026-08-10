/*
 * remote_command.h
 *
 *  Created on: 10 Aug 2026
 *      Author: alan
 */
#ifndef INC_REMOTE_COMMAND_H_
#define INC_REMOTE_COMMAND_H_

#include <stdbool.h>

/*
 * Initialise the remote command interface.
 *
 * Power-on/default state is telemetry stream mode.
 */
void RemoteCommand_Init(void);

/*
 * Process any received eRIC data.
 *
 * Call repeatedly from the main loop.
 */
void RemoteCommand_Task(void);

/*
 * Returns true while automatic telemetry should be transmitted.
 */
bool RemoteCommand_IsStreamMode(void);

/*
 * Returns true once when command mode has just returned to
 * stream mode.
 *
 * Used by main.c to resume telemetry immediately.
 */
bool RemoteCommand_StreamJustResumed(void);

#endif /* INC_REMOTE_COMMAND_H_ */
