/*
 * opensolder.c
 *
 * This file contains the main functions and variables for the OpenSolder
 * firmware
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 *
 * TODO:
 * - implement better power regulation: PID or other alternave
 * - Better error handling
 * - Regular pcb temp readings for overheating protection and cold junction compensation
 * - Settings menu
 */

#include "opensolder.h"
#include "button.h"
#include "encoder.h"
#include "gui.h"
#include "temperature.h"

/******    Local Function Declarations    ******/
static void state_machine(void);
static void init_mmi(void);
static void read_mmi(void);

/******    File Scope Variables    ******/
static uint8_t system_state;

static button tool_holder_sensor; // Detects when tool is placed in holder
static button tip_change_sensor;  // Detects when tool touches the tip change bracket
static button mmi_button;		  // Front rotary encoder button
static encoder mmi_encoder;		  // Front rotary encoder

static uint8_t tool_holder_state;
static uint8_t tip_change_state;
static uint8_t mmi_button_event;
static uint8_t mmi_encoder_event;

/******    Init    ******/
void opensolder_init(void) {
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	HAL_I2C_Init(&hi2c1);
	HAL_ADCEx_Calibration_Start(&hadc);
	HAL_Delay(50); // Wait for calibration to finish
	init_mmi();
	init_display(SPLASHSCREEN_TIMEOUT_MS);
	system_state = INIT_STATE;
}

void init_mmi(void) {
	button_init(&tool_holder_sensor, STAND_GPIO_Port, STAND_Pin, INVERTED);
	button_init(&tip_change_sensor, TIP_REMOVER_GPIO_Port, TIP_REMOVER_Pin, INVERTED);
	button_init(&mmi_button, ENC_SW_GPIO_Port, ENC_SW_Pin, INVERTED);
	encoder_init(&mmi_encoder, TIM2);
}

/******    Main    ******/
void opensolder_main(void) {
	read_mmi();
	state_machine();
}

/******    State Machine    ******/
static void state_machine(void) {
	static uint32_t standby_timeout_tick_ms = 0;
	static uint32_t tip_insert_delay_tick_ms = 0;
	uint8_t tool_tip_state = get_tip_state();

	if (HAL_GetTick() > get_ac_delay_tick()) {
		error_handler();
		display_message(AC_NOT_DETECTED);
		system_state = ERROR_STATE;
		return;
	}

	switch (system_state) {
		case INIT_STATE:
			heater_off();
			draw_default_display();
			system_state = TIP_CHANGE_STATE;
			break;

		case TIP_CHANGE_STATE: {
			heater_off();

			if (tool_tip_state != TIP_DETECTED) {
				tip_insert_delay_tick_ms = HAL_GetTick() + TIP_CHANGE_DELAY_MS;
				display_message(tool_tip_state);
			} else if ((tool_tip_state == TIP_DETECTED) && (HAL_GetTick() > tip_insert_delay_tick_ms) && !tip_change_state) {
				draw_default_display();
				system_state = OFF_STATE;
			}
			update_display();
		} break;

		case OFF_STATE:
			heater_off();

			if (tip_change_state || (tool_tip_state != TIP_DETECTED)) {
				system_state = TIP_CHANGE_STATE;
			} else if (!tool_holder_state) {
				system_state = ON_STATE;
			}
			update_display();
			break;

		case ON_STATE:
			if (tip_change_state || (tool_tip_state != TIP_DETECTED)) {
				system_state = TIP_CHANGE_STATE;
			} else if (tool_holder_state) {
				standby_timeout_tick_ms = HAL_GetTick() + (STANDBY_TIME_S * 1000);
				system_state = STANDBY_STATE;
			}
			update_display();
			break;

		case STANDBY_STATE:
			if (tip_change_state || (tool_tip_state != TIP_DETECTED)) {
				system_state = TIP_CHANGE_STATE;
			} else if (!tool_holder_state) {
				system_state = ON_STATE;
			} else if (HAL_GetTick() > standby_timeout_tick_ms) {
				system_state = OFF_STATE;
			}
			update_display();
			break;

		case ERROR_STATE:
			error_handler();
			system_state = INIT_STATE;
			break;

		default:
			system_state = ERROR_STATE;
			break;
	}
}

/******    Other Functions   ******/
void read_mmi(void) {
	mmi_button_event = button_event(&mmi_button);
	mmi_encoder_event = encoder_event(&mmi_encoder);

	static uint32_t standby_delay_tick_ms = 0;
	static uint32_t tip_change_delay_tick_ms = 0;

	if (button_state(&tool_holder_sensor)) {
		tool_holder_state = SET;
		standby_delay_tick_ms = HAL_GetTick() + STANDBY_DELAY_MS;
	} else if (HAL_GetTick() > standby_delay_tick_ms) {
		tool_holder_state = RESET;
	}

	if (button_state(&tip_change_sensor)) {
		tip_change_state = SET;
		tip_change_delay_tick_ms = HAL_GetTick() + TIP_CHANGE_DELAY_MS;
	} else if (HAL_GetTick() > tip_change_delay_tick_ms) {
		tip_change_state = RESET;
	}

	if (mmi_encoder_event != NO_CHANGE) {
		int16_t new_temp = get_set_temp();
		new_temp += (TEMP_STEPS * get_encoder_delta(&mmi_encoder));

		if (new_temp > MAX_TEMP) {
			new_temp = MAX_TEMP;
		} else if (new_temp < MIN_TEMP) {
			new_temp = MIN_TEMP;
		}

		set_new_temp(new_temp);
	}
}

void sensor_scan(void) {
	button_scan(&tool_holder_sensor);
	button_scan(&tip_change_sensor);
	button_scan(&mmi_button);
}

uint8_t get_system_state(void) {
	return system_state;
}
