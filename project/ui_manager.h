#pragma once

#include <Arduino.h>
#include <lvgl.h>

// initialize all screens, listeners and stored settings.
void ui_init(void);

// transition inbetween navigation screens
void ui_switch_to_settings(void);
void ui_switch_to_timetable(bool swipe_right);
void ui_switch_to_group(void);