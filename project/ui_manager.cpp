#include "ui_manager.h"
#include "timetable.h"
#include <Preferences.h>

// Custom font (supports Å, Ä, Ö)
LV_FONT_DECLARE(font_montserrat_16);

static Preferences preferences;

// Screen pointers
static lv_obj_t* scr_group     = nullptr;
static lv_obj_t* scr_timetable = nullptr;
static lv_obj_t* scr_settings  = nullptr;

// UI elements
static lv_obj_t* lbl_active_stop_header = nullptr;
static lv_obj_t* dd_settings_stop       = nullptr;
static lv_obj_t* dd_settings_transport  = nullptr;

// predefined 5 stops
static const char* STOPS_OPTIONS = 
    "Campus Gräsvik\n"
    "Karlskrona C\n"
    "Kungsplan\n"
    "Bergåsa Station\n"
    "Lyckeby Centrum";

//transport types
static const char* TRANSPORT_OPTIONS = 
    "All\n"
    "Bus\n"
    "Train\n"
    "Ferry";

// Synchronize filtered settings to the departure data screen
static void sync_settings_to_timetable(void) {
    char stop_buf[32];
    lv_dropdown_get_selected_str(dd_settings_stop, stop_buf, sizeof(stop_buf));
    uint16_t trans_idx = lv_dropdown_get_selected(dd_settings_transport);

    if (lbl_active_stop_header) {
        lv_label_set_text_fmt(lbl_active_stop_header, "Stop: %s", stop_buf);
    }
    timetable_set_filter(stop_buf, (TransportType)trans_idx);
}

// Save current settings to NVS
static void on_save_clicked(lv_event_t* e) {
    (void)e;
    uint16_t stop_idx  = lv_dropdown_get_selected(dd_settings_stop);
    uint16_t trans_idx = lv_dropdown_get_selected(dd_settings_transport);

    preferences.begin("transit", false);
    preferences.putUInt("stop_idx", stop_idx);
    preferences.putUInt("trans_idx", trans_idx);
    preferences.end();

    sync_settings_to_timetable();
    Serial.printf("US3.4: Stored defaults -> StopIdx: %u, TransIdx: %u\n", stop_idx, trans_idx);
}

// Reset dropdowns to the stored default settings
static void on_reset_clicked(lv_event_t* e) {
    (void)e;
    preferences.begin("transit", true);
    uint16_t saved_stop  = preferences.getUInt("stop_idx", 0);
    uint16_t saved_trans = preferences.getUInt("trans_idx", 0);
    preferences.end();

    lv_dropdown_set_selected(dd_settings_stop, saved_stop);
    lv_dropdown_set_selected(dd_settings_transport, saved_trans);

    sync_settings_to_timetable();
    Serial.println("Reset settings to stored defaults.");
}

// Gesture callbacks for swiping

static void group_screen_swipe(lv_event_t* e) {
    if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT) {
        ui_switch_to_timetable(false);
    }
}

static void timetable_screen_swipe(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        ui_switch_to_group();
    } else if (dir == LV_DIR_LEFT) {
        ui_switch_to_settings();
    }
}

static void settings_screen_swipe(lv_event_t* e) {
    if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT) {
        ui_switch_to_timetable(true);
    }
}

void ui_init(void) {
    // Screen 0: Start Screen (US1.1 / FR01)
    scr_group = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_group, lv_color_hex(0x0F1117), 0);
    lv_obj_set_style_text_font(scr_group, &font_montserrat_16, 0);
    lv_obj_clear_flag(scr_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_group, group_screen_swipe, LV_EVENT_GESTURE, NULL);

    lv_obj_t* group_box = lv_obj_create(scr_group);
    lv_obj_set_size(group_box, lv_pct(90), lv_pct(80));
    lv_obj_center(group_box);
    lv_obj_set_style_bg_color(group_box, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(group_box, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_radius(group_box, 10, 0);
    lv_obj_clear_flag(group_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* info_label = lv_label_create(group_box);
    lv_label_set_text(info_label,
                      "PIBT, V2, G11\n\n"
                      "Noah Strandberg\n"
                      "Felicia Rönnberg\n"
                      "Shudhit Singh\n"
                      "Alex Erlandsson\n"
                      "Gabriel Eriksson");
    lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_center(info_label);

    // Swipe hint container
    lv_obj_t* hint_box_group = lv_obj_create(scr_group);
    lv_obj_set_size(hint_box_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(hint_box_group, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(hint_box_group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(hint_box_group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(hint_box_group, 0, 0);
    lv_obj_set_flex_flow(hint_box_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hint_box_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* hint_group_txt = lv_label_create(hint_box_group);
    lv_label_set_text(hint_group_txt, "Swipe left for departures ");
    lv_obj_set_style_text_color(hint_group_txt, lv_color_hex(0x6E7681), 0);
    lv_obj_set_style_text_font(hint_group_txt, &font_montserrat_16, 0);

    lv_obj_t* hint_group_sym = lv_label_create(hint_box_group);
    lv_label_set_text(hint_group_sym, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(hint_group_sym, lv_color_hex(0x6E7681), 0);
    lv_obj_set_style_text_font(hint_group_sym, &lv_font_montserrat_16, 0); // built-in font so it shows symbols

    // Screen 1: Departures Screen (US1.2A / FR02)
    scr_timetable = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_timetable, lv_color_hex(0x0F1117), 0);
    lv_obj_set_style_text_font(scr_timetable, &font_montserrat_16, 0);
    lv_obj_add_event_cb(scr_timetable, timetable_screen_swipe, LV_EVENT_GESTURE, NULL);

    // Header bar
    lv_obj_t* top_bar = lv_obj_create(scr_timetable);
    lv_obj_set_size(top_bar, lv_pct(100), lv_pct(14));
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(top_bar, lv_color_hex(0x30363D), 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lbl_active_stop_header = lv_label_create(top_bar);
    lv_label_set_text(lbl_active_stop_header, "Stop: Campus Gräsvik");
    lv_obj_set_style_text_color(lbl_active_stop_header, lv_color_white(), 0);
    lv_obj_align(lbl_active_stop_header, LV_ALIGN_LEFT_MID, 6, 0);

    // Swipe hint container in timetable header
    lv_obj_t* hint_box_tt = lv_obj_create(top_bar);
    lv_obj_set_size(hint_box_tt, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(hint_box_tt, LV_ALIGN_RIGHT_MID, -6, 0);
    lv_obj_set_style_bg_opa(hint_box_tt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(hint_box_tt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(hint_box_tt, 0, 0);
    lv_obj_set_flex_flow(hint_box_tt, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hint_box_tt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* hint_tt_txt = lv_label_create(hint_box_tt);
    lv_label_set_text(hint_tt_txt, "Settings ");
    lv_obj_set_style_text_color(hint_tt_txt, lv_color_hex(0x6E7681), 0);
    lv_obj_set_style_text_font(hint_tt_txt, &font_montserrat_16, 0);

    lv_obj_t* hint_tt_sym = lv_label_create(hint_box_tt);
    lv_label_set_text(hint_tt_sym, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(hint_tt_sym, lv_color_hex(0x6E7681), 0);
    lv_obj_set_style_text_font(hint_tt_sym, &lv_font_montserrat_16, 0); // built-in font so it shows symbols

    timetable_create_screen(scr_timetable);

    // Screen 2: Settings Screen
    scr_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_settings, lv_color_hex(0x0F1117), 0);
    lv_obj_set_style_text_font(scr_settings, &font_montserrat_16, 0);
    lv_obj_clear_flag(scr_settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr_settings, settings_screen_swipe, LV_EVENT_GESTURE, NULL);

    // Top header
    lv_obj_t* settings_bar = lv_obj_create(scr_settings);
    lv_obj_set_size(settings_bar, lv_pct(100), lv_pct(14));
    lv_obj_align(settings_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(settings_bar, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(settings_bar, lv_color_hex(0x30363D), 0);
    lv_obj_clear_flag(settings_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Swipe hint container in settings header
    lv_obj_t* hint_box_st = lv_obj_create(settings_bar);
    lv_obj_set_size(hint_box_st, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(hint_box_st, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_bg_opa(hint_box_st, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(hint_box_st, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(hint_box_st, 0, 0);
    lv_obj_set_flex_flow(hint_box_st, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hint_box_st, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* hint_st_sym = lv_label_create(hint_box_st);
    lv_label_set_text(hint_st_sym, LV_SYMBOL_LEFT " ");
    lv_obj_set_style_text_color(hint_st_sym, lv_color_hex(0x6E7681), 0);
    lv_obj_set_style_text_font(hint_st_sym, &lv_font_montserrat_16, 0); // built-in font so it shows the symbols

    lv_obj_t* hint_st_txt = lv_label_create(hint_box_st);
    lv_label_set_text(hint_st_txt, "Departures");
    lv_obj_set_style_text_color(hint_st_txt, lv_color_hex(0x6E7681), 0);
    lv_obj_set_style_text_font(hint_st_txt, &font_montserrat_16, 0);

    lv_obj_t* title = lv_label_create(settings_bar);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    // Select stop dropdown
    lv_obj_t* lbl_stop = lv_label_create(scr_settings);
    lv_label_set_text(lbl_stop, "Select Stop (US3.2A):");
    lv_obj_align(lbl_stop, LV_ALIGN_TOP_LEFT, 16, 56);
    lv_obj_set_style_text_color(lbl_stop, lv_color_hex(0x94A3B8), 0);

    dd_settings_stop = lv_dropdown_create(scr_settings);
    lv_dropdown_set_options(dd_settings_stop, STOPS_OPTIONS);
    lv_dropdown_set_symbol(dd_settings_stop, ""); // fixes missing arrow symbol
    lv_obj_set_size(dd_settings_stop, 210, 36);
    lv_obj_align(dd_settings_stop, LV_ALIGN_TOP_LEFT, 16, 76);
    lv_obj_set_style_bg_color(dd_settings_stop, lv_color_hex(0x1F2430), 0);
    lv_obj_set_style_text_color(dd_settings_stop, lv_color_white(), 0);

    // Transport type dropdown
    lv_obj_t* lbl_trans = lv_label_create(scr_settings);
    lv_label_set_text(lbl_trans, "Transport Type (US3.3):");
    lv_obj_align(lbl_trans, LV_ALIGN_TOP_LEFT, 16, 120);
    lv_obj_set_style_text_color(lbl_trans, lv_color_hex(0x94A3B8), 0);

    dd_settings_transport = lv_dropdown_create(scr_settings);
    lv_dropdown_set_options(dd_settings_transport, TRANSPORT_OPTIONS);
    lv_dropdown_set_symbol(dd_settings_transport, ""); // fixes missing arrow symbol
    lv_obj_set_size(dd_settings_transport, 210, 36);
    lv_obj_align(dd_settings_transport, LV_ALIGN_TOP_LEFT, 16, 140);
    lv_obj_set_style_bg_color(dd_settings_transport, lv_color_hex(0x1F2430), 0);
    lv_obj_set_style_text_color(dd_settings_transport, lv_color_white(), 0);

    // Save button
    lv_obj_t* btn_save = lv_btn_create(scr_settings);
    lv_obj_set_size(btn_save, 96, 36);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_LEFT, 16, -14);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x2563EB), 0);
    lv_obj_add_event_cb(btn_save, on_save_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t* save_lbl = lv_label_create(btn_save);
    lv_label_set_text(save_lbl, "Save");
    lv_obj_center(save_lbl);

    // Reset button
    lv_obj_t* btn_reset = lv_btn_create(scr_settings);
    lv_obj_set_size(btn_reset, 96, 36);
    lv_obj_align(btn_reset, LV_ALIGN_BOTTOM_RIGHT, -16, -14);
    lv_obj_set_style_bg_color(btn_reset, lv_color_hex(0xDC2626), 0);
    lv_obj_add_event_cb(btn_reset, on_reset_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t* reset_lbl = lv_label_create(btn_reset);
    lv_label_set_text(reset_lbl, "Reset");
    lv_obj_center(reset_lbl);

    // Load initial values
    preferences.begin("transit", true);
    uint16_t init_stop  = preferences.getUInt("stop_idx", 0);
    uint16_t init_trans = preferences.getUInt("trans_idx", 0);
    preferences.end();

    lv_dropdown_set_selected(dd_settings_stop, init_stop);
    lv_dropdown_set_selected(dd_settings_transport, init_trans);
    sync_settings_to_timetable();

    // Start on group screen
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