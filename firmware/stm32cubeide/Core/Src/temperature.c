/*
 * temperature.c
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 */

/*
 * - NOTES -
 * temp	= temperature
 * tmp	= temporary
 * PA0	= THERMOCOUPLE_ADC
 * PA1	= TIP_CHECK
 * PA2	= TIP_CLAMP
 *
 * - TIMELINE -
 * 1. Zero Cross Interrupt happens some µs before the "true" zero cross because
 *    of optocoupler hysteresis. ZC interrupt starts TIM6
 * 2. TIM6 interrupt happens at true ZC. Two options:
 * 		A - turn heater on:
 * 			- Set TIP_CLAMP as output, pull low (prevents noise on thermocouple amplifier input)
 * 			- Turn heater on
 * 		B - turn heater off:
 * 			- Turn heater off
 * 			- Start TIM7 (2ms timer)
 * 3. TIM7 interrupt
 * 		- Check if this is the first or second TIM7 interrupt. Two options:
 * 		A - First interrupt (2ms after ZC):
 * 			- Set TIP_CLAMP pin to input state (high impedance)
 * 			- Do a tip_state check if TIP_CHECK_INTERVAL has passed (checks if tip is inserted)
 * 		B - Second interrupt (4ms after ZC):
 * 			- Start the ADC reading
 * 4. HAL_ADC_ConvCpltCallback() calls adc_complete() when the ADC conversion is done
 *
 * The reason for these delays are to delay the ADC reading until the thermocouple amplifier
 * and low-pass filter have reached steady state.
 */

#include "temperature.h"

/******    Local Function Declarations    ******/
static void start_adc(void);
static void adc_complete(void);
static void zerocross_interrupt(uint16_t GPIO_Pin);
static void timer_interrupt(TIM_HandleTypeDef *htim);
static void adc_calculate_buffer_average(void);
static void adc_to_temperature(void);
static void adc_deviation_check(void);
static void power_control(void);

/******    File Scope Variables    ******/
static uint16_t adc_buffer[ADC_BUFFER_LENGTH];
static uint32_t adc_buffer_average = 0;

static uint16_t set_temp = DEFAULT_TEMP;
static uint16_t tip_temp = 0;

static volatile uint8_t on_periods = 0;
static volatile uint8_t power_bar_value = 0;
static volatile uint32_t heater_power_history = 0; // DEBUG only - could be used for power histogram
static volatile uint8_t error_flag = RESET;
static volatile uint32_t ac_delay_tick_ms = 0;
static volatile uint16_t tip_state = TIP_NOT_DETECTED;
static volatile uint8_t tip_check_flag = RESET;
static volatile uint16_t tip_check_counter = 0;

/******    Callback Functions    ******/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	zerocross_interrupt(GPIO_Pin);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	timer_interrupt(htim);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	adc_complete();
}

/******    ISR Functions    ******/
// ISR: Rising edge is detected on ZERO_CROSS pin. Start TIM6, which is a delay for when the true AC zero cross happens
static void zerocross_interrupt(uint16_t GPIO_Pin) {
	if (GPIO_Pin == ZERO_CROSS_Pin) {
		HAL_TIM_Base_Start_IT(&htim6);
		ac_delay_tick_ms = HAL_GetTick() + AC_DETECTION_INTERVAL_MS;
	}
}

// ISR: Do heater and temperature reading tasks at specific points during the AC mains cycle
static void timer_interrupt(TIM_HandleTypeDef *htim) {

	// TIM6 interrupt, indicating true AC zero cross. This is where to turn the heater on/off to avoid inductive spikes
	if (htim == &htim6) {
		HAL_TIM_Base_Stop_IT(&htim6);

		heater_power_history <<= 1; // Records the tip power history of the past 32 AC half cycles. Power to tip = 1, no power = 0
		tip_check_counter++;		// Increase counter every AC half cycle

		// Switch heater on or off
		if ((on_periods >= 1) && (tip_temp < MAX_TEMP)) {
			// Drive TIP_CHECK pin LOW, this clamps thermo-couple signal to prevent transients and noise on the op-amp input
			TIP_CLAMP_GPIO_Port->BRR |= GPIO_BRR_BR_1;		   // Set PA2 LOW
			TIP_CLAMP_GPIO_Port->MODER |= GPIO_MODER_MODER2_0; // Set PA2 to push pull output mode

			// Turn heater on
			HAL_GPIO_WritePin(HEATER_GPIO_Port, HEATER_Pin, ON);
			on_periods--;
			heater_power_history++;

		} else {
			HAL_GPIO_WritePin(HEATER_GPIO_Port, HEATER_Pin, OFF); // Turn heater OFF
			HAL_TIM_Base_Start_IT(&htim7);						  // Start TIM7 to read tip temperature
		}

		sensor_scan(); // Scan buttons (zero cross happens at 100Hz, 10ms between each scan)

	} else if (htim == &htim7) {

		/*
		 * Wait 2ms (one TIM7 period) after power is turned off for transients
		 * to settle, then remove thermo-couple clamp (pin PA2). Wait another 2ms
		 * for the RC pre-amp filter to settle before starting the ADC conversion
		 */

		static uint8_t delay_flag = SET;

		// First period of TIM7, 2ms after true zero cross
		if (delay_flag) {
			delay_flag = RESET;

			// Set TIP_CLAMP pin to input state (high impedance)
			TIP_CLAMP_GPIO_Port->MODER &= ~GPIO_MODER_MODER2_0; // Set PA2 to input mode

			if (tip_check_counter > TIP_CHECK_INTERVAL) {
				heater_off();
				tip_check_flag = SET;

				// Drive TIP_CHECK pin HIGH, if no tip is inserted the op-amp will saturate and ADC will read close to 4096.
				TIP_CHECK_GPIO_Port->BSRR |= GPIO_BSRR_BS_1;	   // Set PA1 HIGH
				TIP_CHECK_GPIO_Port->MODER |= GPIO_MODER_MODER1_0; // Set PA1 to push pull output mode

				tip_check_counter = 0;
			}

			// Second period of TIM7, 4ms after true zero cross
		} else {
			delay_flag = SET;
			HAL_TIM_Base_Stop_IT(&htim7); // Stop TIM7
			start_adc();				  // Start ADC conversion in DMA mode
		}
	}
}

static void start_adc(void) {
	HAL_ADC_Start_DMA(&hadc, (uint32_t *)adc_buffer, (sizeof(adc_buffer) / sizeof(uint16_t)));
}

static void adc_complete(void) {
	adc_calculate_buffer_average();

	if (tip_check_flag == SET) {
		tip_check_flag = WAIT;
		TIP_CHECK_GPIO_Port->MODER &= ~GPIO_MODER_MODER1_0; // Set TIP_CHECK pin PA1 to input mode
		tip_state = tip_check();
	} else if ((tip_check_flag == RESET) && (tip_state == TIP_DETECTED)) {
		adc_to_temperature();
		adc_deviation_check();
		if (error_flag == SET) {
			tip_temp = ADC_READING_ERROR;
			error_handler();
		} else if ((get_system_state() == ON_STATE) || (get_system_state() == STANDBY_STATE)) {
			power_control();
		}
	}
	power_bar_value = on_periods;
}

static void adc_to_temperature(void) {
	// Calculate tip temperature in Celsius
	tip_temp = (adc_buffer_average * 100 / 750) + 25;
}

static void adc_calculate_buffer_average(void) {
	// Calculate the average value of adc_buffer
	adc_buffer_average = 0;
	for (uint16_t i = 0; i < ADC_BUFFER_LENGTH; i++) {
		adc_buffer_average += adc_buffer[i];
	}
	adc_buffer_average /= ADC_BUFFER_LENGTH;
}

static void adc_deviation_check(void) {
	// Check if any values in adc_buffer deviates more than expected, if so set error_flag
	int16_t upper_limit = adc_buffer_average + ADC_MAX_DEVIATION;
	int16_t lower_limit = adc_buffer_average - ADC_MAX_DEVIATION;

	for (uint16_t i = 0; i < ADC_BUFFER_LENGTH; i++) {
		if ((adc_buffer[i] > upper_limit) || (adc_buffer[i] < lower_limit)) {
			error_flag = SET;
		}
	}
}

static void power_control(void) {
	/*
	 * Decide if power should be delivered to tip next half period, and
	 * how many periods until next temp read (how much power to apply).
	 *
	 * Full power is one read period (OFF) per 4 power periods (ON).
	 * Number of power periods before next read period decreases as
	 * temperature closes in on set_temp to prevent too much overshoot.
	 */

	uint16_t tmp_set_temp = set_temp;
	if ((get_system_state() == STANDBY_STATE) && (tmp_set_temp > STANDBY_TEMP)) {
		tmp_set_temp = STANDBY_TEMP;
	}

	// Turn heater ON/OFF
	if ((tip_temp + 3) < tmp_set_temp) {
		uint16_t temperature_error = tmp_set_temp - tip_temp;
		on_periods = temperature_error / 10;
		if (on_periods > MAX_ON_PERIODS) {
			on_periods = MAX_ON_PERIODS;
		} else if (on_periods == 0) {
			on_periods = 1;
		}
	} else {
		heater_off();
	}
}

uint8_t tip_check(void) {
	/*
	 * - TIP CHECK -
	 * 1. tip_check_flag is set every TIP_CHECK_INTERVAL AC half cycles, and TIP_CHECK_PIN is pulled high
	 * 2. When ADC is finished, adc_complete() reset TIP_CHECK_PIN,
	 *    calculates the average value of the ADC buffer reading and calls tip_check() to update tip_state
	 */

	uint8_t tmp_return = TIP_NOT_DETECTED;

	if (tip_check_flag != WAIT) {
		error_handler();
		tmp_return = TIP_CHECK_ERROR;
	} else if (adc_buffer_average > ADC_NO_TIP_MIN_VALUE) {
		tmp_return = TIP_NOT_DETECTED;
	} else if (adc_buffer_average < ADC_TIP_MAX_VALUE) {
		tmp_return = TIP_DETECTED;
	} else {
		tmp_return = TIP_CHECK_ERROR;
	}

	tip_check_flag = RESET;
	return tmp_return;
}

/******    Other Functions   ******/
int16_t read_pcb_temperature(void) {
	int16_t temp_register = 0;
	uint8_t tmp_buffer[2];

	if (HAL_I2C_Master_Receive(&hi2c1, PCT2075_I2C_ADDR, tmp_buffer, sizeof(tmp_buffer), 100) != HAL_ERROR) {
		temp_register = (tmp_buffer[0] << 8);
		temp_register |= tmp_buffer[1];
		return ((temp_register >> 5) * 0.125); // Returns 11 bit signed (2s compl.) temperature in Celsius.
	} else {
		return ADC_READING_ERROR;
	}
}

uint32_t get_ac_delay_tick(void) {
	return ac_delay_tick_ms;
}

void heater_off(void) {
	on_periods = 0;
}

void set_new_temp(uint16_t new_temp) {
	set_temp = new_temp;
}

uint16_t get_set_temp(void) {
	return set_temp;
}

uint16_t get_tip_temp(void) {
	return tip_temp;
}

uint8_t get_tip_state(void) {
	return tip_state;
}

void error_handler(void) {
	// Turn heater hard OFF
	HAL_GPIO_WritePin(HEATER_GPIO_Port, HEATER_Pin, OFF);
	heater_off();
	tip_state = TIP_CHECK_ERROR;
	error_flag = RESET;
}

uint8_t get_power_bar_value(void) {
	return power_bar_value;
}
