#pragma once
#include <lvgl.h>

// callback function triggered when the header settings icon is clicked
typedef void (*header_settings_cb_t)(void);

// construct the header on the designated parent screen
void header_create(lv_obj_t*parent, header_settings_cb_t on_settings_clicked);