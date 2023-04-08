/*
 * gui.c
 *
 * This file contains all OLED display functionality
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 */

#include "gui.h"
#include "ssd1306.h"
#include "temperature.h"
#include <stdio.h>
#include <string.h>

/******    Struct Declaration    ******/
typedef struct {
	uint8_t x;							   // Upper left x starting position of string
	uint8_t y;							   // Upper left y starting position of string
	FontDef *font;						   // Font type (Font_7x10 etc.)
	SSD1306_COLOR color;				   // Font color [Black, White]
	char string[STR_ARRAY_MAX_LEN]; // Currently fixed value, no need for malloc()
	uint8_t length;						   // Max number of characters. If strlen(string) is shorter than length, blank characters are appended to string
} ssd1306_string;

/******    Local Function Declarations    ******/
static void draw_init_display(void);
static void write_string(ssd1306_string string);

/******    File Scope Variables    ******/
enum display_constants {
	EDGE_OFFSET = 2,
	TEXT_OFFSET = 5,
	DISPLAY_WIDTH = 128,
	DISPLAY_WIDTH_POS = DISPLAY_WIDTH - 1,
	DISPLAY_HEIGHT = 64,
	DISPLAY_HEIGHT_POS = DISPLAY_HEIGHT - 1,

	// Splashscreen rectangle
	INIT_R_X1 = EDGE_OFFSET,
	INIT_R_Y1 = EDGE_OFFSET,
	INIT_R_X2 = DISPLAY_WIDTH_POS - EDGE_OFFSET,
	INIT_R_Y2 = ((TEXT_OFFSET * 2) + EDGE_OFFSET + 18),
	INIT_TEXT_X = 9,
	INIT_TEXT_Y = EDGE_OFFSET + TEXT_OFFSET,

	// Set temp rectangle
	SET_R_X1 = EDGE_OFFSET,
	SET_R_Y1 = EDGE_OFFSET,
	SET_R_X2 = (DISPLAY_WIDTH_POS - EDGE_OFFSET) / 2,
	SET_R_Y2 = EDGE_OFFSET + TEXT_OFFSET * 3 + 10 + 18,

	// Tip temp rectangle
	TIP_R_X1 = SET_R_X2 + EDGE_OFFSET + 1,
	TIP_R_Y1 = SET_R_Y1,
	TIP_R_X2 = DISPLAY_WIDTH_POS - EDGE_OFFSET,
	TIP_R_Y2 = SET_R_Y2,

	// Power bar rectangle
	PB_R_X1 = SET_R_X1,
	PB_R_Y1 = SET_R_Y2 + EDGE_OFFSET + 1,
	PB_R_X2 = TIP_R_X2,
	PB_R_Y2 = DISPLAY_HEIGHT_POS - EDGE_OFFSET,

	// Set temp text/value pos
	SET_TEXT_X = SET_R_X1 + TEXT_OFFSET,
	SET_TEXT_Y = SET_R_Y1 + TEXT_OFFSET,
	SET_VAL_X = SET_TEXT_X,
	SET_VAL_Y = SET_TEXT_Y + TEXT_OFFSET + 10,

	// Tip temp text/value pos
	TIP_TEXT_X = TIP_R_X1 + TEXT_OFFSET,
	TIP_TEXT_Y = TIP_R_Y1 + TEXT_OFFSET,
	TIP_VAL_X = TIP_TEXT_X,
	TIP_VAL_Y = TIP_TEXT_Y + TEXT_OFFSET + 10,

	// Power bar text pos
	PB_TEXT_X = SET_TEXT_X,
	PB_TEXT_Y = ((PB_R_Y2 + PB_R_Y1) / 2) - (8 / 2) + 1,
	PB_TEXT_MAX_LEN = (DISPLAY_WIDTH - 2 * SET_TEXT_X) / 6,

	// Message rectangle/text pos
	MSG_OFFSET = 5,
	MSG_R_X1 = MSG_OFFSET,
	MSG_R_Y1 = (DISPLAY_HEIGHT_POS - 10) / 2 - MSG_OFFSET,
	MSG_R_X2 = DISPLAY_WIDTH_POS - MSG_OFFSET,
	MSG_R_Y2 = (DISPLAY_HEIGHT_POS + 10) / 2 + MSG_OFFSET + 1,
	MSG_TEXT_X = MSG_R_X1 + MSG_OFFSET,
	MSG_TEXT_Y = MSG_R_Y1 + MSG_OFFSET + 1,
	MSG_TEXT_MAX_LEN = (DISPLAY_WIDTH - 4 * MSG_OFFSET - 2) / 7,
};

static ssd1306_string s_opensolder = {INIT_TEXT_X, INIT_TEXT_Y, &Font_11x18, White, "OpenSolder\0", 10};
static ssd1306_string s_firmware = {INIT_TEXT_X, INIT_R_Y2 + TEXT_OFFSET + 2, &Font_7x10, White, "Firmware:   v0.9\0", 16};
static ssd1306_string s_ambient = {INIT_TEXT_X, INIT_R_Y2 + TEXT_OFFSET * 2 + 10, &Font_7x10, White, "Ambient:    \0", 16};

static ssd1306_string set_temp_text = {SET_TEXT_X, SET_TEXT_Y, &Font_7x10, White, "Set\0", 3};
static ssd1306_string set_temp_val = {SET_VAL_X, SET_VAL_Y, &Font_11x18, White, "300'\0", 4};
static ssd1306_string tip_temp_text = {TIP_TEXT_X, TIP_TEXT_Y, &Font_7x10, White, "Tip\0", 3};
static ssd1306_string tip_temp_val = {TIP_VAL_X, TIP_VAL_Y, &Font_11x18, White, "302'\0", 4};

static ssd1306_string power_bar_text = {PB_TEXT_X, PB_TEXT_Y, &Font_6x8, White, "\0", PB_TEXT_MAX_LEN};
static ssd1306_string message_text = {MSG_TEXT_X, MSG_TEXT_Y, &Font_7x10, White, "\0", MSG_TEXT_MAX_LEN};

/******    Functions    ******/
// Draw the default display image
void init_display(uint16_t timeout) {
	ssd1306_Init();
	ssd1306_SetContrast(DISPLAY_BRIGHTNESS);
	draw_init_display();
	HAL_Delay(timeout);
}

// Draw the splash screen during initialization
void draw_init_display(void) {
	ssd1306_Fill(Black);
	ssd1306_DrawRectangle(INIT_R_X1, INIT_R_Y1, INIT_R_X2, INIT_R_Y2, White);
	write_string(s_opensolder);

	// Read ambient temperature and concaternate to s_ambient.string
	char s_buffer[STR_ARRAY_MAX_LEN];
	snprintf(s_buffer, sizeof(s_buffer), "%d'C", read_pcb_temperature());
	strcat(s_ambient.string, s_buffer);

	write_string(s_firmware);
	write_string(s_ambient);
	ssd1306_UpdateScreen();
}

void write_string(ssd1306_string string) {
	// Pad string.string if shorter than string.length to clear previous characters
	ssd1306_SetCursor(string.x, string.y);
	if (strlen(string.string) < string.length) {
		for (uint8_t i = strlen(string.string); i < string.length; i++) {
			strcat(string.string, " ");
		}
	} else if (strlen(string.string) > string.length) {
		string.string[string.length] = '\0';
	}

	// Write new string
	ssd1306_SetCursor(string.x, string.y);
	ssd1306_WriteString(string.string, *string.font, string.color);
}

void draw_default_display(void) {
	ssd1306_Fill(Black);
	ssd1306_DrawRectangle(SET_R_X1, SET_R_Y1, SET_R_X2, SET_R_Y2, White);
	ssd1306_DrawRectangle(TIP_R_X1, TIP_R_Y1, TIP_R_X2, TIP_R_Y2, White);
	ssd1306_DrawRectangle(PB_R_X1, PB_R_Y1, PB_R_X2, PB_R_Y2, White);
	write_string(set_temp_text);
	write_string(set_temp_val);
	write_string(tip_temp_text);
	write_string(tip_temp_val);
	ssd1306_UpdateScreen();
}

void update_display(void) {
	static uint32_t display_update_tick = 0;
	static uint16_t prev_tip_temp = 0;
	static uint16_t power_bar_value = 0;

	// Update set_temp
	snprintf(set_temp_val.string, set_temp_val.length + 1, "%d'C", get_set_temp());
	write_string(set_temp_val);

	// Keep rapid changing elements like tip_temp from creating display jitter
	if ((HAL_GetTick() > display_update_tick)
			|| (get_tip_temp() < prev_tip_temp - 1)
			|| (get_tip_temp() > prev_tip_temp + 1)) {
		display_update_tick = HAL_GetTick() + DISPLAY_UPDATE_TICKS;
		prev_tip_temp = get_tip_temp();

		snprintf(tip_temp_val.string, tip_temp_val.length + 1, "%d'C", get_tip_temp());
		write_string(tip_temp_val);

		// Clear powerbar
		ssd1306_DrawFilledRectangle(PB_R_X1 + 1, PB_R_Y1 + 1, PB_R_X2 - 1, PB_R_Y2 - 1, Black);

		// Draw powerbar
		power_bar_value = (uint16_t)get_power_bar_value() * (PB_R_X2 - PB_R_X1 - 1) / MAX_ON_PERIODS + PB_R_X1;
		ssd1306_DrawFilledRectangle(PB_R_X1, PB_R_Y1 + 1, power_bar_value, PB_R_Y2 - 1, White);
	}

	// DEBUG - display current state
	switch (get_system_state()) {
		case INIT_STATE:
			snprintf(power_bar_text.string, power_bar_text.length + 1, "Initial");
			break;
		case TIP_CHANGE_STATE:
			snprintf(power_bar_text.string, power_bar_text.length + 1, "Tip change");
			break;
		case OFF_STATE:
			snprintf(power_bar_text.string, power_bar_text.length + 1, "OFF state");
			break;
		case ON_STATE:
			snprintf(power_bar_text.string, power_bar_text.length + 1, "ON state");
			break;
		case STANDBY_STATE:
			snprintf(power_bar_text.string, power_bar_text.length + 1, "Standby");
			break;
		case ERROR_STATE:
			snprintf(power_bar_text.string, power_bar_text.length + 1, "Error");
			break;
		default:
			break;
	}
	write_string(power_bar_text);
	// DEBUG END - display current state

	// Update the display with the new values
	ssd1306_UpdateScreen();
}

void display_message(uint16_t message_code) {
	switch (message_code) {
		case TIP_NOT_DETECTED:
			snprintf(message_text.string, message_text.length + 1, "Insert tip");
			break;
		case TIP_CHECK_ERROR:
			snprintf(message_text.string, message_text.length + 1, "Tip check error");
			break;
		case AC_NOT_DETECTED:
			snprintf(message_text.string, message_text.length + 1, "AC not detected");
			break;
		case OVERHEATING:
			snprintf(message_text.string, message_text.length + 1, "! Overheating !");
			break;
		default:
			snprintf(message_text.string, message_text.length + 1, "Unknown error");
			break;
	}

	ssd1306_Fill(Black);
	ssd1306_DrawRectangle(MSG_R_X1, MSG_R_Y1, MSG_R_X2, MSG_R_Y2, White);
	write_string(message_text);
	ssd1306_UpdateScreen();
}
