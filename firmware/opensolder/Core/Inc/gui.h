/*
 * gui.h
 *
 * License: GPL-3.0 or any later version
 * Copyright (c) 2022 Håvard Jakobsen
 */

#ifndef GUI_H
#define GUI_H

/******    Includes    ******/
#include "opensolder.h"
#include "ssd1306.h"

/******    Global Function Declarations    ******/
void init_display(uint16_t timeout);
void update_display(void);
void display_message(uint16_t message_code); // Use message code from opensolder_messages enum
void draw_default_display(void);

#endif
