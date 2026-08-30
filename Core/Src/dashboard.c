/*
 * dashboard.c
 *
 *  Created on: 18 Jul 2026
 *      Author: alan
 */

#include "dashboard.h"
#include "sht25.h"
#include "adc.h"
#include "i2c.h"
#include "rtc.h"
#include "gpio.h"
#include "eric_lora.h"
#include "water_level.h"
#include "pump_control.h"
#include "relay.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>

static uint32_t dashboardAdcValues[ADC_CHANS];

static uint16_t dashboard_water_count = 0;
static uint16_t dashboard_water_gate_us = 0;
static uint32_t dashboard_water_frequency_hz = 0;
static uint32_t dashboard_water_filtered_frequency_hz = 0;

void Dashboard_SetWaterValues(uint16_t count, uint16_t gate_us, uint32_t frequency_hz, uint32_t filtered_frequency_hz)
{
    dashboard_water_count = count;
    dashboard_water_gate_us = gate_us;
    dashboard_water_frequency_hz = frequency_hz;
    dashboard_water_filtered_frequency_hz = filtered_frequency_hz;
}

static void Dashboard_DisplayTable(UART_HandleTypeDef *huart);

void Dashboard_UpdateAdcValues(const uint32_t *values, uint8_t count)
{
    uint8_t i;

    if (values == NULL)
    {
        return;
    }

    if (count > ADC_CHANS)
    {
        count = ADC_CHANS;
    }

    for (i = 0U; i < count; i++)
    {
        dashboardAdcValues[i] = values[i];
    }
}
void Dashboard_Show(UART_HandleTypeDef * huart) {
	Dashboard_DisplayTable(huart);
	DisplayPrompt(huart);
}
static void Dashboard_DisplayTable(UART_HandleTypeDef * huart) {
	int i;

	ClrTerm(huart);
	CursorHome(huart);

	DrwTblTop(huart);			// Line 1
	DrwTtl(huart);				// Line 2
	DrwTblBarDbl(huart);		// Line 3

	for (i=0;i<15;i++) {
		DrwBlnkRow(huart);		// Lines 4-
	}

//	DrwTblBarSngl(huart);		// Line 8

//	DrwBlnkRow(huart);			// Line 9 (Left/Right Titles)

//	DrwTblBarSngl(huart);		// Line 10

//	for (i=0;i<8;i++) {
//		DrwBlnkRow(huart);		// Lines 11-18
//	}

	DrwTblBarSngl(huart);		// Line 19

	DrwBlnkRow(huart);			// Line 20

	DrwTblBase(huart);			// Line 21
}
void Dashboard_DisplayTemperature(UART_HandleTypeDef * huart) {
	char name_str[128] = "Temperature:\0";
	char value_str[128] = "\0";
	float temperature;

	if (SHT25_ReadTemperature(&temperature) == HAL_OK)
	{
		sprintf(value_str,"%.1f°C",temperature);

		DrwCellAt(RIGHT_COL,4,name_str,value_str,huart);
	}
}
void Dashboard_DisplayHumidity(UART_HandleTypeDef * huart) {
	char name_str[128] = "Relative Humidity:\0";
	char value_str[128] = "\0";
	float humidity;

	if (SHT25_ReadHumidity(&humidity) == HAL_OK)
	{
		sprintf(value_str,"%.1f%%",humidity);

		DrwCellAt(RIGHT_COL,5,name_str,value_str,huart);
	}
}
void Dashboard_Display9V(UART_HandleTypeDef * huart) {
	char name_str[128] = "V_+9V_Vin:\0";
	char value_str[128] = "\0";
	float vin9v = 0;

	vin9v = ( (float)dashboardAdcValues[ADC_9V] / 4095.0f ) * 3.3f * 3.0f;

	sprintf(value_str,"%.3fV",vin9v);

	DrwCellAt(LEFT_COL,4,name_str,value_str,huart);
}
void Dashboard_Display5V(UART_HandleTypeDef * huart) {
	char name_str[128] = "V_+5V Rail:\0";
	char value_str[128] = "\0";
	float nucleo5v = 0;

	nucleo5v = ( ( (float)dashboardAdcValues[ADC_5V] / 4095.0f ) * 3.3f * 1.667f );  // 1.667 = 10K / (10K + 15K)

	sprintf(value_str,"%.3fV",nucleo5v);

	DrwCellAt(LEFT_COL,5,name_str,value_str,huart);
}
void Dashboard_Display3V3(UART_HandleTypeDef * huart) {
	char name_str[128] = "V_+3V3 Rail:\0";
	char value_str[128] = "\0";
	float nucleo3v3v = 0;

	nucleo3v3v = (float)dashboardAdcValues[ADC_3V3V] / 4095.0f * 3.3f * 1.067f;
//	nucleo3v3v = (float)dashboardAdcValues[ADC_MAIN3V] * ADC_SCALE3V;

	sprintf(value_str,"%.2fV",nucleo3v3v);

	DrwCellAt(LEFT_COL,6,name_str,value_str,huart);
}
void Dashboard_DisplayLoRaCurrent(UART_HandleTypeDef * huart) {
	char name_str[128] = "eRIC 5V Rail Current:\0";
	char value_str[128] = "\0";
	float lorai = 0;

	lorai = ( ( (float)dashboardAdcValues[ADC_ILORA] / 4095.0f ) * 2660.0f );  // 2660 = 1000 * 3.3 / ( 0.1 * 0.0002 * 62000 ) ie Rs * 200µ * Rl

	sprintf(value_str,"%.2fmA",lorai);

	DrwCellAt(LEFT_COL,7,name_str,value_str,huart);
}
void Dashboard_DisplayLoRaEnable(UART_HandleTypeDef * huart) {
	char name_str[128] = "eRIC Enable:\0";
	char value_str[128] = "\0";

	if (HAL_GPIO_ReadPin(GPIOB, en_LoRa_Pin)) {
		sprintf(value_str,"High(1)");
	}
	else {
		sprintf(value_str,"Low(0)");
	}

	DrwCellAt(LEFT_COL,8,name_str,value_str,huart);
}
void Dashboard_DisplayEricBaud(UART_HandleTypeDef *huart)
{
    const ERIC_Settings_t *settings = ERIC_GetSettings();

    DrwCellAt(LEFT_COL,9,"eRIC Baud Rate:", settings->uart_baud, huart);
}
void Dashboard_DisplayEricAirRate(UART_HandleTypeDef *huart)
{
    const ERIC_Settings_t *settings = ERIC_GetSettings();

    DrwCellAt(LEFT_COL, 10,"eRIC Air Rate:",settings->air_data_rate,huart);
}
void Dashboard_DisplayEricChannel(UART_HandleTypeDef *huart)
{
    const ERIC_Settings_t *settings = ERIC_GetSettings();

    DrwCellAt(LEFT_COL,11,"eRIC Channel:",settings->channel,huart);
}
void Dashboard_DisplayDate(UART_HandleTypeDef *huart)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    char name_str[128] = "Date:\0";
    char value_str[128] = "\0";

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    sprintf(value_str,
            "20%02u-%02u-%02u",
            sDate.Year,
            sDate.Month,
            sDate.Date);

    DrwCellAt(RIGHT_COL, 20, name_str, value_str, huart);
}
void Dashboard_DisplayWeekday(UART_HandleTypeDef *huart)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    char name_str[128] = "Weekday:";
    char value_str[128] = "";

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    switch (sDate.WeekDay)
    {
        case RTC_WEEKDAY_MONDAY:
            strcpy(value_str, "Monday");
            break;

        case RTC_WEEKDAY_TUESDAY:
            strcpy(value_str, "Tuesday");
            break;

        case RTC_WEEKDAY_WEDNESDAY:
            strcpy(value_str, "Wednesday");
            break;

        case RTC_WEEKDAY_THURSDAY:
            strcpy(value_str, "Thursday");
            break;

        case RTC_WEEKDAY_FRIDAY:
            strcpy(value_str, "Friday");
            break;

        case RTC_WEEKDAY_SATURDAY:
            strcpy(value_str, "Saturday");
            break;

        case RTC_WEEKDAY_SUNDAY:
            strcpy(value_str, "Sunday");
            break;

        default:
            strcpy(value_str, "Unknown");
            break;
    }

    DrwCellAt(RIGHT_COL, 18, name_str, value_str, huart);
}
void Dashboard_DisplayTime(UART_HandleTypeDef *huart)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    char name_str[128] = "Time:\0";
    char value_str[128] = "\0";

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    /*
     * Important:
     * You must always read the date immediately after the time.
     */

    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    sprintf(value_str,
            "%02u:%02u:%02u",
            sTime.Hours,
            sTime.Minutes,
            sTime.Seconds);

    DrwCellAt(LEFT_COL, 20, name_str, value_str, huart);
}
void Dashboard_DisplayWaterCount(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Water Count:";
    char value_str[128] = "";

    sprintf(value_str,
            "%u",
            (unsigned int)dashboard_water_count);

    DrwCellAt(RIGHT_COL, 6, name_str, value_str, huart);
}


void Dashboard_DisplayWaterGate(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Gate Time:";
    char value_str[128] = "";

    sprintf(value_str,
            "%u µs",
            (unsigned int)dashboard_water_gate_us);

    DrwCellAt(RIGHT_COL, 7, name_str, value_str, huart);
}


void Dashboard_DisplayWaterFrequency(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Sensor Frequency:";
    char value_str[128] = "";

    float frequency_mhz =
        (float)dashboard_water_frequency_hz / 1000000.0f;

    sprintf(value_str, "%.6f MHz", frequency_mhz);

    DrwCellAt(RIGHT_COL, 8, name_str, value_str, huart);
}

void Dashboard_DisplayWaterFilteredFrequency(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Filtered Frequency:";
    char value_str[128] = "";

    float frequency_mhz = (float)dashboard_water_filtered_frequency_hz / 1000000.0f;

    sprintf(value_str, "%.6f MHz", frequency_mhz);

    DrwCellAt(RIGHT_COL, 9, name_str, value_str, huart);
}

void Dashboard_DisplayWaterDepth(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Water Depth:";
    char value_str[128] = "";

    float depth_cm;

    depth_cm = WaterLevel_FrequencyToCm(dashboard_water_frequency_hz);

    sprintf(value_str, "%.1f cm", depth_cm);

    DrwCellAt(RIGHT_COL, 10, name_str, value_str, huart);
}

void Dashboard_DisplayWaterPercent(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Water Level:";
    char value_str[128] = "";

    float depth_cm;
    float percent;

    depth_cm = WaterLevel_FrequencyToCm(dashboard_water_frequency_hz);

    percent = WaterLevel_DepthToPercent(depth_cm);

    sprintf(value_str, "%.1f%%", percent);

    DrwCellAt(RIGHT_COL, 11, name_str, value_str, huart);
}

void Dashboard_DisplayWaterLitres(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Usable Water:";
    char value_str[128] = "";

    float depth_cm;
    float litres;

    depth_cm = WaterLevel_FrequencyToCm(dashboard_water_frequency_hz);

    litres = WaterLevel_DepthToLitres(depth_cm);

    sprintf(value_str, "%.1f L", litres);

    DrwCellAt(RIGHT_COL, 12, name_str, value_str, huart);
}

void Dashboard_DisplayPumpRequest(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Pump Request:";
    char value_str[128] = "";

    if (PumpControl_IsRequested())
    {
        sprintf(value_str, "ON");
    }
    else
    {
        sprintf(value_str, "OFF");
    }

    DrwCellAt(RIGHT_COL, 13, name_str, value_str, huart);
}

void Dashboard_DisplayPumpMode(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Pump Mode:";
    char value_str[128] = "";

    if (PumpControl_IsAutomatic())
    {
        sprintf(value_str, "AUTO");
    }
    else
    {
        sprintf(value_str, "MANUAL");
    }

    DrwCellAt(RIGHT_COL, 14, name_str, value_str, huart);
}

void Dashboard_DisplayPumpRelay(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Pump Relay:";
    char value_str[128] = "";

    if (Relay_Get(RELAY_1) == RELAY_ON)
    {
        sprintf(value_str, "ON");
    }
    else
    {
        sprintf(value_str, "OFF");
    }

    DrwCellAt(RIGHT_COL, 15, name_str, value_str, huart);
}

void Dashboard_DisplayPumpLockout(UART_HandleTypeDef *huart)
{
    char name_str[128] = "Pump Lockout:";
    char value_str[128] = "";

    uint32_t remaining_ms;
    uint32_t remaining_seconds;

    remaining_ms =
        Relay1_GetLockoutRemainingMs();

    if (remaining_ms == 0U)
    {
        sprintf(value_str, "READY");
    }
    else
    {
        /*
         * Round upwards so that, for example,
         * 250 ms remaining still displays as 1 s.
         */
        remaining_seconds = (remaining_ms + 999U) / 1000U;

        sprintf(value_str, "%lu s", (unsigned long)remaining_seconds);
    }

    DrwCellAt(RIGHT_COL, 16, name_str, value_str, huart);
}

void Dashboard_StreamTemperature(UART_HandleTypeDef * huart) {
	char name_str[128] = "Temperature:\0";
	char value_str[128] = "\0";
	float sht_celsius = 0;
	uint8_t cell_str[128];
	int i;

	if (SHT25_ReadTemperature(&sht_celsius) != HAL_OK)
	{
	    return;
	}

	sprintf(value_str,"%.1f°C\n\r",sht_celsius);

	/* construct cell string */
	strcpy((char*)cell_str,name_str);
	for (i=1;i<4;i=i+2) strcat((char*)cell_str,".");
	strcat((char*)cell_str,value_str);

	HAL_UART_Transmit(huart,cell_str,strlen((char*)cell_str),HAL_MAX_DELAY);
}
void Dashboard_StreamHumidity(UART_HandleTypeDef * huart) {
	char name_str[128] = "Relative Humidity:\0";
	char value_str[128] = "\0";
	float sht_humid = 0;
	uint8_t cell_str[128];
	int i;

	if (SHT25_ReadHumidity(&sht_humid) != HAL_OK)
	{
		return;
	}

	sprintf(value_str,"%.1f%%\n\r",sht_humid);

	/* construct cell string */
	strcpy((char*)cell_str,name_str);
	for (i=1;i<4;i=i+2) strcat((char*)cell_str,".");
	strcat((char*)cell_str,value_str);

	HAL_UART_Transmit(huart,cell_str,strlen((char*)cell_str),HAL_MAX_DELAY);
}
void Dashboard_Refresh(UART_HandleTypeDef *huart)
{
    Dashboard_Display9V(huart);
    Dashboard_Display5V(huart);
    Dashboard_Display3V3(huart);
    Dashboard_DisplayLoRaCurrent(huart);

    Dashboard_DisplayLoRaEnable(huart);

    Dashboard_DisplayDate(huart);
    Dashboard_DisplayWeekday(huart);
    Dashboard_DisplayTime(huart);

    Dashboard_DisplayTemperature(huart);
    Dashboard_DisplayHumidity(huart);

    Dashboard_DisplayWaterCount(huart);
    Dashboard_DisplayWaterGate(huart);
    Dashboard_DisplayWaterFrequency(huart);
    Dashboard_DisplayWaterFilteredFrequency(huart);
    Dashboard_DisplayWaterDepth(huart);
    Dashboard_DisplayWaterPercent(huart);
    Dashboard_DisplayWaterLitres(huart);

    Dashboard_DisplayPumpRequest(huart);
    Dashboard_DisplayPumpMode(huart);
    Dashboard_DisplayPumpRelay(huart);
    Dashboard_DisplayPumpLockout(huart);

    Dashboard_DisplayEricBaud(huart);
    Dashboard_DisplayEricAirRate(huart);
    Dashboard_DisplayEricChannel(huart);
}

