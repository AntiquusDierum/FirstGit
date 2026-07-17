/*
 * adw_uart_menu.h
 *
 *  Created on: 4 Dec 2020
 *      Author: Alan
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef INC_ADW_UART_MENU_H_
#define INC_ADW_UART_MENU_H_

#define ASCII_ESC 27

void ClrTerm(UART_HandleTypeDef * huart);
void CursorHome(UART_HandleTypeDef * huart);
void ClrLine(UART_HandleTypeDef * huart);
void CursorNL(UART_HandleTypeDef * huart);
void CursorPrompt(UART_HandleTypeDef * huart);
void DrwTblTop(UART_HandleTypeDef * huart);
void DrwTtl(UART_HandleTypeDef * huart);
void DrwTblBarDbl(UART_HandleTypeDef * huart);
void DrwTblBase(UART_HandleTypeDef * huart);
void DrwTblBarSngl(UART_HandleTypeDef * huart);
void DrwBlnkRow(UART_HandleTypeDef * huart);
void DrwCellAt(uint8_t x,uint8_t y,char name_str[], char value_str[],UART_HandleTypeDef * huart);
void DrwTextAt(uint8_t x,uint8_t y,char name_str[],UART_HandleTypeDef * huart);
void DisplayPrompt(UART_HandleTypeDef * huart);
uint8_t ReadTerm(UART_HandleTypeDef * huart);
void SendTerm(uint8_t sendChar, UART_HandleTypeDef * huart);
void EchoTerm(UART_HandleTypeDef * huart);

#endif /* INC_ADW_UART_MENU_H_ */
