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
void TerminalConsole_ShowScreen(UART_HandleTypeDef *huart);

/*
 * Display the list of supported commands.
 */
void TerminalConsole_ShowHelp(UART_HandleTypeDef *huart);

/*
 * Execute a completed command from cmd_buffer.
 *
 * The command buffer and command-ready flag are still owned by main.c
 * during this first stage of the refactor.
 */
void TerminalConsole_Task(UART_HandleTypeDef *huart);

uint8_t TerminalConsole_IsActive(void);

uint8_t TerminalConsole_RedrawDashboard(void);

uint8_t TerminalConsole_RedrawCommandScreen(void);

void TerminalConsole_ClearRedraws(void);

void TerminalConsole_RxByte(uint8_t ch);

#endif /* TERMINAL_CONSOLE_H */
