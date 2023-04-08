/*
 * button.h
 *
 * Button library for embedded systems
 *
 * USAGE:
 * - Include "stm32xxxx_hal.h" or a header file which includes it
 * - Create a button object
 * - Call button_init() to initialize button object
 * - Call button_scan() periodically (every 1-10ms recommended, adjust DEBOUNCE_TICS and LONG_PRESS_TICKS accordingly)
 * - Call button_event() to get button released event (this resets button event flags, so only read once per program loop)
 * - Call button_state() to get the current (debounced) button state without clearing the event flag
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 */

#ifndef BUTTON_H
#define BUTTON_H

/******    Includes    ******/
#include "stm32f0xx_hal.h"

/******    Constants and Objects    ******/
enum button_constants {
	NO_PRESS = 0,
	SHORT_PRESS,
	LONG_PRESS,

	INVERTED = 100, // GPIO pin low when button is pressed
	NON_INVERTED,	// GPIO pin high when button is pressed

	DEBOUNCE_TICKS = 3,	  // with a 10ms scan rate, "DEBOUNCE_TICKS = 3" equals 30ms debounce time
	LONG_PRESS_TICKS = 50 // with a 10ms scan rate, 50 ticks = 500ms press needed to activate LONG_PRESS
};

typedef struct {
	GPIO_TypeDef *port;
	uint16_t pin;
	uint8_t polarity;
	volatile uint8_t counter;
	volatile uint8_t state;
	volatile uint8_t release_state;
	volatile uint8_t release_flag;
	volatile uint16_t long_press_ticks;
} button;

/******    Function Declarations    ******/
void button_init(button *const self, GPIO_TypeDef *port, uint16_t pin, uint8_t polarity);
uint8_t button_event(button *const self);
uint8_t button_state(button *const self);
void button_scan(button *const self);

#endif
