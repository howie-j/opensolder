/*
 * opensolder.h
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 */

#ifndef OPENSOLDER_H
#define OPENSOLDER_H

/******    Includes    ******/
#include "main.h"
#include <stdint.h>

/******    Constants   ******/
enum PCT2075_constants {
	PCT2075_I2C_ADDR = 0x92U, // 1001 001 << 1
	PCT2075_TEMP_REG = 0x00U, // Temperature register pointer
	PCT2075_CONF_REG = 0x01U, // Configuration register pointer
	PCT2075_HYST_REG = 0x02U, // Hysteresis register pointer
	PCT2075_OS_REG = 0x03U,	  // Over-temperature shutdown register pointer
	PCT2075_IDLE_REG = 0x01U  // Idle register pointer
};

enum opensolder_constants {
	TIP_CHECK_INTERVAL = 50,		   // Number of half mains cycles between each tip check (value of 50 cycles * 10ms = 500ms tipcheck interval)
	TIP_CHANGE_DELAY_MS = 3000,		   // Delay after tip_change_sense is set before turning heater on
	DISPLAY_BRIGHTNESS = 255,		   // Sets display contrast/brightness, value 0-255
	DISPLAY_UPDATE_TICKS = 500,		   // Refresh rate for updating tip_temp, set higher to remove jitter
	DISPLAY_MESSAGE_TIMEOUT_MS = 3000, // How long to display messages
	STR_ARRAY_MAX_LEN = 128 / 6 + 1,	   // Display text string array max length (including '\0'.
	SPLASHSCREEN_TIMEOUT_MS = 1000,	   // Splash screen / boot screen, 0 to disable
	DEFAULT_TEMP = 300,				   // Default set temp on startup
	MAX_TEMP = 400,					   // Maximum allowable temperature
	MIN_TEMP = 30,					   // Minimum allowable temperature
	TEMP_STEPS = 5,					   // Number of degrees setTemp changes per encoder step
	MAX_ON_PERIODS = 4,				   // Maximum number if AC half cycles the power should be on before next temp reading
	STANDBY_TEMP = 50,				   // Tip temperature when handle is in holder
	STANDBY_TIME_S = 30,			   // Number of seconds to keep tip at elevated standby temperature, before turning heater off
	STANDBY_DELAY_MS = 500,			   // Delay from lifting the tool holder before turning heater on
	ADC_BUFFER_LENGTH = 50,			   // Number of ADC samples to take every reading
	ADC_MAX_DEVIATION = 200,		   // Maximum deviation allowed in the ADC sample buffer. Any value out of range gives the reading an error
	ADC_NO_TIP_MIN_VALUE = 4000,	   // Lowest expected temp reading with no tip inserted and TIP_CHECK pin high. Used for tip detection
	ADC_TIP_MAX_VALUE = 3800,		   // Max expected temp reading with tip inserted. Must be higher that MAX_TEMP reading. Used for tip detection
	AC_DETECTION_INTERVAL_MS = 12	   // Max expected time between each AC interrupt
};

enum opensolder_messages {
	OFF = 0,
	ON = 1,
	WAIT,
	TIP_DETECTED,
	TIP_NOT_DETECTED,
	TIP_CHECK_ERROR,
	AC_NOT_DETECTED,
	OVERHEATING,
	ADC_READING_ERROR = 999 // Constant to check tip_temp for an error. Also displays 999 on display in case of a reading error
};

enum my_states { INIT_STATE, TIP_CHANGE_STATE, OFF_STATE, ON_STATE, STANDBY_STATE, ERROR_STATE };

/******    Global Variables    ******/
extern ADC_HandleTypeDef hadc;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;

/******    Global Function Declarations    ******/
void opensolder_init(void);
void opensolder_main(void);
void sensor_scan(void);
uint8_t get_system_state(void);

#endif
