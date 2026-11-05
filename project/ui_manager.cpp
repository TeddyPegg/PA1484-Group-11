#include "ui_manager.h"
#include "timetable.h"
#include "header.h"
#include <Preferences.h>

LV_FONT_DECLARE(font_montserrat_16);

// ESP32 Storage manager for preferences
static Preferences preferences;
static bool is_dark_mode = true;
static lv_obj_t* sw_darkmode = nullptr;

// Screen container references
static lv_obj_t* scr_group = nullptr;
static lv_obj_t* scr_timetable = nullptr;
static lv_obj_t* scr_settings  = nullptr;

// apply active theme colors across all the registered screens
void apply_theme(void) {
    lv_color_t bg_col   = is_dark_mode ? lv_color_hex(0x0F1117) : lv_color_hex(0xF1F5F9);
    lv_color_t text_col = is_dark_mode ? lv_color_white()       : lv_color_hex(0x0F172A);

    if (scr_timetable) {
        lv_obj_set_style_bg_color(scr_timetable, bg_col, 0);
        lv_obj_set_style_text_color(scr_timetable, text_col, 0);
    }
    if (scr_settings) {
        lv_obj_set_style_bg_color(scr_settings, bg_col, 0);
        lv_obj_set_style_text_color(scr_settings, text_col, 0);
    }
    if (scr_group) {
        lv_obj_set_style_bg_color(scr_group, bg_col, 0);
        lv_obj_set_style_text_color(scr_group, text_col, 0);
    }
}

// add users dark mode preference to ESP32 flash memory
static void darkmode_switch_event_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    is_dark_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);

    preferences.begin("settings", false);
    preferences.putBool("darkmode", is_dark_mode);
    preferences.end();

    Serial.printf("Dark mode sparades som: %s\n", is_dark_mode ? "ON" : "OFF");
    apply_theme();
}

static void back_to_timetable_btn_cb(lv_event_t* e) {
    (void)e;
    ui_switch_to_timetable(true);
}

// callback: Swipe left from group info to access departure dashboard
static void group_screen_swipe(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_LEFT) {
        ui_switch_to_timetable(false);
    }
}

// callback: swipe right from departure timetable to return to group info
static void timetable_screen_swipe(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        ui_switch_to_group();
    }
}

void ui_init(void) {
    // load user preferences on boot
    preferences.begin("settings", true);
    is_dark_mode = preferences.getBool("darkmode", true);
    preferences.end();

    // --- screen 0: Group Info Splash Screen ---
    scr_group = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_group, lv_color_hex(0x0F1117), 0);
    lv_obj_set_style_text_font(scr_group, &font_montserrat_16, 0);
    lv_obj_clear_flag(scr_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_group, group_screen_swipe, LV_EVENT_GESTURE, NULL);

    // Group info text box
    lv_obj_t* group_box = lv_obj_create(scr_group);
    lv_obj_set_size(group_box, lv_pct(90), lv_pct(80));
    lv_obj_center(group_box);
    lv_obj_set_style_bg_color(group_box, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(group_box, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_radius(group_box, 10, 0);
    lv_obj_clear_flag(group_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* info_label = lv_label_create(group_box);
    lv_label_set_text(info_label,
                      "PIBT, V1, G11\n\n"
                      "Noah Strandberg\n"
                      "Felicia Rönnberg\n"
                      "Shudhit Singh\n"
                      "Alex Erlandsson\n"
                      "Gabriel Eriksson");
    lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_center(info_label);

    // hint label at the bottom
    lv_obj_t* hint_label = lv_label_create(scr_group);
    lv_label_set_text(hint_label, "Swipe left for departures " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(hint_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(hint_label, lv_color_hex(0x6E7681), 0);
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -10);


    // --- Screen 1: Timetable Main View ---
    scr_timetable = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_timetable, lv_color_hex(0x0F1117), 0);
    lv_obj_set_style_text_font(scr_timetable, &font_montserrat_16, 0);
    lv_obj_add_event_cb(scr_timetable, timetable_screen_swipe, LV_EVENT_GESTURE, NULL);


    header_create(scr_timetable, ui_switch_to_settings);


    // Create timetable screen (bottom 70%)
    timetable_create_screen(scr_timetable);

    // --- Screen 2: Settings View ---
    scr_settings = lv_obj_create(NULL);
    lv_obj_set_style_text_font(scr_settings, &font_montserrat_16, 0);

    // Back button
    lv_obj_t* btn_back = lv_btn_create(scr_settings);
    lv_obj_set_size(btn_back, 90, 44);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 8);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x232936), 0);
    lv_obj_set_style_radius(btn_back, 8, 0);
    lv_obj_add_event_cb(btn_back, back_to_timetable_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* arrow_lbl = lv_label_create(btn_back);
    lv_label_set_text(arrow_lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(arrow_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(arrow_lbl, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t* back_text = lv_label_create(btn_back);
    lv_label_set_text(back_text, "Back");
    lv_obj_align(back_text, LV_ALIGN_RIGHT_MID, -4, 0);

    // Settings title
    lv_obj_t* title = lv_label_create(scr_settings);
    lv_label_set_text(title, "Settings & Network");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 20, 16);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    // Status container
    lv_obj_t* status_box = lv_obj_create(scr_settings);
    lv_obj_set_size(status_box, LV_PCT(92), LV_PCT(65));
    lv_obj_align(status_box, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_color(status_box, lv_color_hex(0x1F2430), 0);
    lv_obj_set_style_border_color(status_box, lv_color_hex(0x2D3748), 0);
    lv_obj_set_style_radius(status_box, 10, 0);
    lv_obj_set_style_pad_all(status_box, 16, 0);

    lv_obj_t* status_text = lv_label_create(status_box);
    lv_label_set_text(status_text, 
        "Wi-Fi: Connected\n\n"
        "Realtimedata: Active (API ready)\n\n"
        "Dark Mode:");
    lv_obj_align(status_text, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_color(status_text, lv_color_hex(0xCBD5E1), 0);

    // interactive switch controlling dark mode preferences
    sw_darkmode = lv_switch_create(status_box);
    lv_obj_align(sw_darkmode, LV_ALIGN_TOP_RIGHT, 0, 48);
    if (is_dark_mode) {
        lv_obj_add_state(sw_darkmode, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(sw_darkmode, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw_darkmode, darkmode_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // apply color theme according to initial state
    apply_theme();

    // start with group info active
    lv_scr_load(scr_group);
}

void ui_switch_to_group(void) {
    if (scr_group) {
        lv_scr_load_anim(scr_group, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
    }
}

void ui_switch_to_settings(void) {
    if (scr_settings) {
        lv_scr_load_anim(scr_settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
    }
}

void ui_switch_to_timetable(bool swipe_right) {
    if (scr_timetable) {
        lv_scr_load_anim(scr_timetable, swipe_right ? LV_SCR_LOAD_ANIM_MOVE_RIGHT : LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
    }
}
