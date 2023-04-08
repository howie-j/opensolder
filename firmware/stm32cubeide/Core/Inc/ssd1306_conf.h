/**
 * Private configuration file for the SSD1306 library.
 * This example is configured for STM32F0, I2C and including all fonts.
 */

#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

#include "main.h"

// Choose a microcontroller family
#define STM32F0
//#define STM32F1
//#define STM32F4
//#define STM32L0
//#define STM32L1
//#define STM32L4
//#define STM32F3
//#define STM32H7
//#define STM32F7
//#define STM32G0

// Choose a bus
//#define SSD1306_USE_I2C
#define SSD1306_USE_SPI

// I2C Configuration
//#define SSD1306_I2C_PORT        hi2c1
//#define SSD1306_I2C_ADDR        (0x3C << 1)

// SPI Configuration
#define SSD1306_SPI_PORT        hspi1
#define SSD1306_CS_Port         CS_GPIO_Port
#define SSD1306_CS_Pin          CS_Pin
#define SSD1306_DC_Port         DC_GPIO_Port
#define SSD1306_DC_Pin          DC_Pin
#define SSD1306_Reset_Port      RES_GPIO_Port
#define SSD1306_Reset_Pin       RES_Pin

// Mirror the screen if needed
// #define SSD1306_MIRROR_VERT
// #define SSD1306_MIRROR_HORIZ

// Set inverse color if needed
// # define SSD1306_INVERSE_COLOR

// Include only needed fonts
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26

// Some OLEDs don't display anything in first two columns.
// In this case change the following macro to 130.
// The default value is 128.
// #define SSD1306_WIDTH           130

// The height can be changed as well if necessary.
// It can be 32, 64 or 128. The default value is 64.
// #define SSD1306_HEIGHT          64

#endif /* __SSD1306_CONF_H__ */
