/*
 * adw_uart_menu.c
 *
 *  Created on: 4 Dec 2020
 *      Author: Alan
 */
#include "stm32l1xx_hal.h"
#include "adw_uart_menu.h"
#include "string.h"				// Included for access to strcpy
#include "stdio.h"				// Included for access to sprintf

void ClrTerm(UART_HandleTypeDef * huart) {
	uint8_t ec_clrterm[5];

	sprintf((char*)ec_clrterm,"%c[2J",ASCII_ESC);
  	HAL_UART_Transmit(huart,ec_clrterm,strlen((char*)ec_clrterm),HAL_MAX_DELAY);
}

void CursorHome(UART_HandleTypeDef * huart) {
	uint8_t ec_curhome[5];

	sprintf((char*)ec_curhome,"%c[H",ASCII_ESC);
	HAL_UART_Transmit(huart,ec_curhome,strlen((char*)ec_curhome),HAL_MAX_DELAY);
}

void ClrLine(UART_HandleTypeDef * huart) {
	uint8_t ec_clrline[5];

	sprintf((char*)ec_clrline,"%c[2K",ASCII_ESC);
	HAL_UART_Transmit(huart,ec_clrline,strlen((char*)ec_clrline),HAL_MAX_DELAY);
}

void CursorNL(UART_HandleTypeDef * huart){
	uint8_t ec_curnl[5];

	sprintf((char*)ec_curnl,"%cE",ASCII_ESC);
	HAL_UART_Transmit(huart,ec_curnl,strlen((char*)ec_curnl),HAL_MAX_DELAY);
}

void CursorPrompt(UART_HandleTypeDef * huart) {
	uint8_t ec_curprmpt[10];

	sprintf((char*)ec_curprmpt,"%c[%d;%dH",ASCII_ESC,23,3);
	HAL_UART_Transmit(huart,ec_curprmpt,strlen((char*)ec_curprmpt),HAL_MAX_DELAY);
}

void DrwTblTop(UART_HandleTypeDef * huart) {
	uint8_t tbl_str[256];													// declare buffer string for printing to terminal.
	uint8_t u2550[5] = "\u2550\0";											// declare string containing Unicode character "�?"
	uint8_t u2554[5] = "\u2554\0";											// declare string containing Unicode character "╔"
	uint8_t u2557[5] = "\u2557\0";											// declare string containing Unicode character "╗"
	int i;																	// declare integer for loop counter.

  	strcpy((char*)tbl_str,"");												// Clear buffer.
  	strcat((char*)tbl_str,(char*)u2554);									// Copy top-left corner character to buffer string.
  	for (i=1;i<150;i=i+2) {	strcat((char*)tbl_str,(char*)u2550); }			// Repeatedly copy top border character to buffer string.
  	strcat((char*)tbl_str,(char*)u2557);									// Copy top-right corner character to buffer string.
	HAL_UART_Transmit(huart,tbl_str,strlen((char*)tbl_str),HAL_MAX_DELAY);// Send the text buffer over UART2 to the terminal.
	CursorNL(huart);
}

void DrwTtl(UART_HandleTypeDef * huart) {
	uint8_t tbl_str[256];													// declare buffer string for printing to terminal.
	uint8_t title[256] = "          Welcome to the Watson Cybernetics Development Terminal           \0";
	uint8_t u2551[5] = "\u2551\0";											// declare string containing Unicode character "║"

  	strcpy((char*)tbl_str,"");												// Clear buffer.
  	strcat((char*)tbl_str,(char*)u2551);									// Copy left/right border character to buffer string.
  	strcat((char*)tbl_str,(char*)title);									// Copy title text to buffer string.
  	strcat((char*)tbl_str,(char*)u2551);									// Copy left/right border character to buffer string.
	HAL_UART_Transmit(huart,tbl_str,strlen((char*)tbl_str),HAL_MAX_DELAY);// Send the text buffer over UART2 to the terminal.
	CursorNL(huart);
}

void DrwTblBarDbl(UART_HandleTypeDef * huart) {
	uint8_t tbl_str[256];													// declare buffer string for printing to terminal.
	uint8_t u2550[5] = "\u2550\0";											// declare string containing Unicode character "�?"
	uint8_t u2560[5] = "\u2560\0";											// declare string containing Unicode character "╠"
	uint8_t u2563[5] = "\u2563\0";											// declare string containing Unicode character "╣"
	uint8_t u2564[5] = "\u2564\0";											// declare string containing Unicode character "╤"
	int i;																	// declare integer for loop counter.

  	strcpy((char*)tbl_str,"");												// Clear buffer.
  	strcat((char*)tbl_str,(char*)u2560);									// Copy left/right border character to buffer string.
  	for (i=1;i<74;i=i+2) strcat((char*)tbl_str,(char*)u2550);				// Repeatedly copy double-bar border character to buffer string.
  	strcat((char*)tbl_str,(char*)u2564);									// Copy top cell separator border character to buffer string.
  	for (i=1;i<74;i=i+2) strcat((char*)tbl_str,(char*)u2550);				// Repeatedly copy double-bar border character to buffer string.
  	strcat((char*)tbl_str,(char*)u2563);									// Copy left/right border character to buffer string.
	HAL_UART_Transmit(huart,tbl_str,strlen((char*)tbl_str),HAL_MAX_DELAY);// Send the text buffer over UART2 to the terminal.
	CursorNL(huart);
}

void DrwTblBase(UART_HandleTypeDef * huart) {
	uint8_t tbl_str[256];													// declare buffer string for printing to terminal.
	uint8_t u2550[5] = "\u2550\0";											// declare string containing Unicode character "�?"
	uint8_t u255A[5] = "\u255A\0";											// declare string containing Unicode character "╚"
	uint8_t u255D[5] = "\u255D\0";											// declare string containing Unicode character "�?"
	uint8_t u2567[5] = "\u2567\0";											// declare string containing Unicode character "╧"
	int i;																	// declare integer for loop counter.

  	strcpy((char*)tbl_str,"");												// Clear buffer.
  	strcat((char*)tbl_str,(char*)u255A);									// Copy bottom left border character to buffer string.
  	for (i=1;i<74;i=i+2) strcat((char*)tbl_str,(char*)u2550);				// Repeatedly copy double-bar border character to buffer string.
  	strcat((char*)tbl_str,(char*)u2567);									// Copy bottom cell separator border character to buffer string.
  	for (i=1;i<74;i=i+2) strcat((char*)tbl_str,(char*)u2550);				// Repeatedly copy double-bar border character to buffer string.
  	strcat((char*)tbl_str,(char*)u255D);									// Copy bottom right border character to buffer string.
	HAL_UART_Transmit(huart,tbl_str,strlen((char*)tbl_str),HAL_MAX_DELAY);// Send the text buffer over UART2 to the terminal.
	CursorNL(huart);
}

void DrwTblBarSngl(UART_HandleTypeDef * huart) {
	uint8_t tbl_str[256];													// declare buffer string for printing to terminal.
	uint8_t u2500[5] = "\u2500\0";											// declare string containing Unicode character "─"
	uint8_t u255F[5] = "\u255F\0";											// declare string containing Unicode character "╟"
	uint8_t u2562[5] = "\u2562\0";											// declare string containing Unicode character "╢"
	uint8_t u253C[5] = "\u253C\0";											// declare string containing Unicode character "┼"
	int i;																	// declare integer for loop counter.

  	strcpy((char*)tbl_str,"");												// Clear buffer.
  	strcat((char*)tbl_str,(char*)u255F);									// Copy left border hline character to buffer string.
  	for (i=1;i<74;i=i+2) strcat((char*)tbl_str,(char*)u2500);				// Repeatedly copy single-bar border character to buffer string.
  	strcat((char*)tbl_str,(char*)u253C);									// Copy cell separator crosshair character to buffer string.
  	for (i=1;i<74;i=i+2) strcat((char*)tbl_str,(char*)u2500);				// Repeatedly copy single-bar border character to buffer string.
  	strcat((char*)tbl_str,(char*)u2562);									// Copy right border hline character to buffer string.
	HAL_UART_Transmit(huart,tbl_str,strlen((char*)tbl_str),HAL_MAX_DELAY);// Send the text buffer over UART2 to the terminal.
	CursorNL(huart);
}

void DrwBlnkRow(UART_HandleTypeDef * huart) {
	uint8_t tbl_str[256];													// declare buffer string for printing to terminal.
	uint8_t u2551[5] = "\u2551\0";											// declare string containing Unicode character "║"
	uint8_t u2502[5] = "\u2502\0";											// declare string containing Unicode character "│"
	int i;																	// declare integer for loop counter.

	strcpy((char*)tbl_str,"");												// Clear buffer.
	strcat((char*)tbl_str,(char*)u2551);									// Copy left/right border character to buffer string.
	for (i=1;i<74;i=i+2) strcat((char*)tbl_str," ");						// Repeatedly copy "space" character to buffer string.
	strcat((char*)tbl_str,(char*)u2502);									// Copy vertical bar cell separator to buffer string.
	for (i=1;i<74;i=i+2) strcat((char*)tbl_str," ");						// Repeatedly copy "space" character to buffer string.
	strcat((char*)tbl_str,(char*)u2551);									// Copy left/right border character to buffer string.
	HAL_UART_Transmit(huart,tbl_str,strlen((char*)tbl_str),HAL_MAX_DELAY);// Send the text buffer over UART2 to the terminal.
	CursorNL(huart);
}

void DrwCellAt(uint8_t x,uint8_t y,char name_str[], char value_str[],UART_HandleTypeDef * huart) {
	uint8_t cell_str[128];
	uint8_t esc_str[11] = "\0";
	int i, name_len, value_len;

	/* work out name and value lengths to determine number of dots needed */
	name_len = strlen(name_str);
	value_len = strlen(value_str);

	/* construct cell string */
	strcpy((char*)cell_str,name_str);
	for (i=1;i<(76-((2*name_len)+(2*value_len)+4));i=i+2) strcat((char*)cell_str,".");
	strcat((char*)cell_str,value_str);

	/* Move to specified location */
	CursorHome(huart);
	snprintf((char*)esc_str,sizeof(esc_str),"%c[%d;%dH",ASCII_ESC,y,x);
	HAL_UART_Transmit(huart,esc_str,strlen((char*)esc_str),HAL_MAX_DELAY);

	/* print to terminal cell contents */
	HAL_UART_Transmit(huart,cell_str,strlen((char*)cell_str),HAL_MAX_DELAY);

	CursorPrompt(huart);
}

void DrwTextAt(uint8_t x,uint8_t y,char name_str[],UART_HandleTypeDef * huart) {
	uint8_t cell_str[128];
	uint8_t esc_str[11] = "\0";

	// construct cell string
	strcpy((char*)cell_str,name_str);

	// Move to specified location
	CursorHome(huart);
	snprintf((char*)esc_str,sizeof(esc_str),"%c[%d;%dH",ASCII_ESC,y,x);
	HAL_UART_Transmit(huart,esc_str,strlen((char*)esc_str),HAL_MAX_DELAY);

	// print to terminal cell contents
	HAL_UART_Transmit(huart,cell_str,strlen((char*)cell_str),HAL_MAX_DELAY);

	CursorPrompt(huart);
}

void DisplayPrompt(UART_HandleTypeDef * huart) {
	uint8_t prompt_str[80];

  	strcpy((char*)prompt_str,"");														// Clear buffer.
	sprintf((char*)prompt_str,"> ");													// Copy prompt text to buffer.
	HAL_UART_Transmit(huart,prompt_str,strlen((char*)prompt_str),HAL_MAX_DELAY);		// Send the prompt text to terminal.
}

uint8_t ReadTerm(UART_HandleTypeDef * huart) {
    uint8_t buffer[1];
    HAL_UART_Receive(huart, buffer, sizeof(buffer), HAL_MAX_DELAY);

    return(buffer[0]);
}

void SendTerm(uint8_t sendChar, UART_HandleTypeDef * huart) {
    uint8_t buffer[1];
    buffer[0] = sendChar;
    HAL_UART_Transmit(huart, buffer, sizeof(buffer), HAL_MAX_DELAY);
}

void EchoTerm(UART_HandleTypeDef * huart) {
    uint8_t echo_char;
    echo_char = ReadTerm(huart);
    SendTerm(echo_char,huart);
}

