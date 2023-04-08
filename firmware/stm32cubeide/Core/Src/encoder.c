/*
 * encoder.c
 *
 * Rotary encoder library for embedded systems
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 */

#include "encoder.h"

/******    functions    ******/
void encoder_init(encoder *const self, TIM_TypeDef *timer) {
	self->timer = timer;
	self->state = NO_CHANGE;
	self->flag = RESET;
	self->delta = 0;
	self->value = ((self->timer->ARR + 1) / 2);
	self->timer->CNT = self->value;
	self->timer->SR &= ~(1); // Clear SR_UIF (update interrupt flag)
}

uint8_t encoder_event(encoder *const self) {
	uint32_t tmp_CNT = self->timer->CNT;
	if (tmp_CNT != self->value) {
		uint8_t overflow_state = encoder_overflow_check(self);
		if (overflow_state == OVERFLOW) {
			self->state = INCREASE;
		} else if (overflow_state == UNDERFLOW) {
			self->state = DECREASE;
		} else {
			if (tmp_CNT > self->value) {
				self->state = INCREASE;
			} else {
				self->state = DECREASE;
			}
		}
		self->delta = tmp_CNT - self->value;
		self->value = tmp_CNT;
		self->flag = SET;
	} else {
		self->state = NO_CHANGE;
	}
	return self->state;
}

uint32_t get_encoder_value(encoder *const self) {
	return self->value;
}

int32_t get_encoder_delta(encoder *const self) {
	int32_t delta = 0;
	if (self->flag) {
		delta = self->delta;
		self->flag = RESET;
	}
	return delta;
}

uint8_t encoder_overflow_check(encoder *const self) {
	uint8_t overflow_state = 0;
	if (self->timer->SR & 1) {							  // Check if SR_UIF (update interrupt flag) is set
		if (self->timer->CNT <= (self->timer->ARR / 2)) { // Check if counter is under or over the half point of timer ARR
			overflow_state = OVERFLOW;
		} else {
			overflow_state = UNDERFLOW;
		}
		self->timer->SR &= ~(1); // Clear SR_UIF (update interrupt flag)
	}
	return overflow_state;
}
