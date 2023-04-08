/*
 * encoder.h
 *
 * Rotary encoder library for embedded systems
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 */

#ifndef ENCODER_H
#define ENCODER_H

/******    Includes    ******/
#include "stm32f0xx_hal.h"

/******    Constants and Objects    ******/
enum encoder_constants { INCREASE = 50, NO_CHANGE, DECREASE, OVERFLOW, UNDERFLOW };

typedef struct {
	TIM_TypeDef *timer;
	volatile uint8_t state;
	volatile uint8_t flag;
	volatile uint32_t value;
	volatile int32_t delta;
} encoder;

/******    Function Declarations   ******/
void encoder_init(encoder *const self, TIM_TypeDef *timer);
uint8_t encoder_event(encoder *const self);
uint8_t encoder_overflow_check(encoder *const self);
uint32_t get_encoder_value(encoder *const self);
int32_t get_encoder_delta(encoder *const self);

#endif
