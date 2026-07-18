/*
 * terminal_console.h
 *
 *  Created on: 18 Jul 2026
 *      Author: alan
 */
#ifndef TERMINAL_CONSOLE_H
#define TERMINAL_CONSOLE_H

#include "main.h"

/*
 * Display the command-console heading and initial prompt.
 */
void DisplayCommandScreen(UART_HandleTypeDef *huart);

/*
 * Display the list of supported commands.
 */
void DisplayOptions(UART_HandleTypeDef *huart);

/*
 * Execute a completed command from cmd_buffer.
 *
 * The command buffer and command-ready flag are still owned by main.c
 * during this first stage of the refactor.
 */
void TerminalCommands(UART_HandleTypeDef *huart);

#endif /* TERMINAL_CONSOLE_H */
