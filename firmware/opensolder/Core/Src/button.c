/*
 * button.c
 *
 * Button library for embedded systems
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 */

#include "button.h"

/******    functions    ******/
void button_init(button *const self, GPIO_TypeDef *port, uint16_t pin, uint8_t polarity) {
	self->port = port;
	self->pin = pin;
	self->polarity = polarity;
	self->counter = 0;
	self->state = NO_PRESS;
	self->release_state = NO_PRESS;
	self->release_flag = RESET;
	self->long_press_ticks = 0;
}

uint8_t button_event(button *const self) {
	uint8_t event = NO_PRESS;
	if (self->release_flag) {
		event = self->release_state;
		self->release_state = NO_PRESS;
		self->release_flag = RESET;
	}
	return event;
}

uint8_t button_state(button *const self) {
	return self->state;
}

void button_scan(button *const self) {
	uint8_t button_pressed = HAL_GPIO_ReadPin(self->port, self->pin);

	if (self->polarity == INVERTED) {
		button_pressed = !button_pressed;
	}

	if (button_pressed) {
		if (self->counter == DEBOUNCE_TICKS) {
			if (self->long_press_ticks == LONG_PRESS_TICKS) {
				self->state = LONG_PRESS;
			} else {
				self->state = SHORT_PRESS;
				self->long_press_ticks++;
			}
		} else {
			self->counter++;
		}

	} else {
		if (self->counter == 0) {
			self->long_press_ticks = 0;
			if (self->state != NO_PRESS) {
				self->release_flag = SET;
				self->release_state = self->state;
				self->state = NO_PRESS;
			}
		} else {
			self->counter--;
		}
	}
}
