#include <vector>
#include <lvgl.h>
#include <header.h>
#include <Arduino.h>
#include "timetable.h"

// interactive dropdown pointers for route and transport filtering
static lv_obj_t* dd_from = nullptr;
static lv_obj_t* dd_to   = nullptr;
static lv_obj_t* dd_when = nullptr;
static lv_obj_t* dd_transport = nullptr;

// predefined stops
static const char* STOP_OPTIONS = 
    "All\n"
    "Campus Gräsvik\n"
    "Karlskrona C\n"
    "Kungsplan\n"
    "Lyckeby Centrum\n"
    "Bergåsa Station";

// predefined offsets
static const char* WHEN_OPTIONS =
    "Now\n"
    "+15 min\n"
    "+30 min\n"
    "+1 hour\n"
    "+2 hours";

// predefined modes
static const char* TRANSPORT_OPTIONS =
    "All\n"
    "Bus\n"
    "Train\n"
    "Ferry";

// Compact preview single letters shown on the dropdown preview
static const char* TRANSPORT_LETTERS[] = {"A", "B", "T", "F"};

// map the selected transport dropdown index to the transportType enum
static TransportType parse_transport_type(uint16_t index) {
    switch (index) {
        case 1:  return TRANSPORT_BUS;
        case 2:  return TRANSPORT_TRAIN;
        case 3:  return TRANSPORT_FERRY;
        case 0:
        default: return TRANSPORT_ALL;
    }
}

// callback triggered whenever any of the primary route dropdowns change
static void on_filter_changed(lv_event_t* e) {
    (void)e;
    if (!dd_from || !dd_to || !dd_when || !dd_transport) return;

    char from_buf[32];
    char to_buf[32];
    char when_buf[32];

    lv_dropdown_get_selected_str(dd_from, from_buf, sizeof(from_buf));
    lv_dropdown_get_selected_str(dd_to,   to_buf,   sizeof(to_buf));
    lv_dropdown_get_selected_str(dd_when, when_buf, sizeof(when_buf));

    uint16_t trans_idx = lv_dropdown_get_selected(dd_transport);
    TransportType t_type = parse_transport_type(trans_idx);

    Serial.printf("Filter: From='%s', To='%s', When='%s', TypeIdx=%u\n", from_buf, to_buf, when_buf, trans_idx);
    timetable_filter(from_buf, to_buf, when_buf, t_type);
}

// send the settings button click event to ui_manager
static void on_settings_btn_clicked(lv_event_t* e) {
    header_settings_cb_t cb = (header_settings_cb_t)lv_event_get_user_data(e);
    if (cb) {
        cb();
    }
}

// select which trailing element to attach to the rgith side of a row
enum TrailingType {
    TRAILING_NONE,
    TRAILING_SETTINGS,
    TRAILING_TRANSPORT
};


// modular row container class managing prefix labels, dropdowns and trailing actions
class SubBox {
public:
    lv_obj_t* row_cont;
    lv_obj_t* label_obj;
    lv_obj_t* dropdown_obj;
    lv_obj_t* btn_settings = nullptr;
    lv_obj_t* dd_trailing  = nullptr;

    SubBox(lv_obj_t* parent, const char* label_text, const char* options, TrailingType trailing, header_settings_cb_t on_settings_clicked, lv_obj_t** out_dd_ptr) {
        // horizontal flex row
        row_cont = lv_obj_create(parent);
        lv_obj_set_width(row_cont, lv_pct(98));
        lv_obj_set_flex_grow(row_cont, 1);
        lv_obj_clear_flag(row_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_set_style_pad_all(row_cont, 0, 0);
        lv_obj_set_style_pad_column(row_cont, 6, 0);
        lv_obj_set_style_bg_opa(row_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(row_cont, LV_OPA_TRANSP, 0);

        lv_obj_set_flex_flow(row_cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // fixed-width prefix label on the left
        label_obj = lv_label_create(row_cont);
        lv_label_set_text(label_obj, label_text);
        lv_obj_set_width(label_obj, 56);
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_color(label_obj, lv_color_white(), 0);

        // main dropdown selector
        dropdown_obj = lv_dropdown_create(row_cont);
        lv_dropdown_set_options(dropdown_obj, options);
        lv_dropdown_set_symbol(dropdown_obj, ""); // "" supresses a missing arrow glyph in the font

        lv_obj_set_flex_grow(dropdown_obj, 1);
        lv_obj_set_height(dropdown_obj, lv_pct(95));
        lv_obj_set_style_bg_color(dropdown_obj, lv_color_hex(0x232936), 0);
        lv_obj_set_style_bg_opa(dropdown_obj, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(dropdown_obj, lv_color_hex(0x3B4252), 0);
        lv_obj_set_style_text_color(dropdown_obj, lv_color_white(), 0);
        lv_obj_set_style_radius(dropdown_obj, 6, 0);
        lv_obj_set_style_pad_ver(dropdown_obj, 2, 0);
        lv_obj_set_style_pad_hor(dropdown_obj, 6, 0);
        lv_obj_add_event_cb(dropdown_obj, on_filter_changed, LV_EVENT_VALUE_CHANGED, NULL);

        if (out_dd_ptr) {
            *out_dd_ptr = dropdown_obj;
        }

        // list menu styling
        lv_obj_t* list = lv_dropdown_get_list(dropdown_obj);
        lv_obj_set_style_bg_color(list, lv_color_hex(0x1F2430), 0);
        lv_obj_set_style_border_color(list, lv_color_hex(0x3B4252), 0);
        lv_obj_set_style_text_color(list, lv_color_white(), 0);

        // row 1: settings icon button on the far right
        if (trailing == TRAILING_SETTINGS) {
            btn_settings = lv_btn_create(row_cont);
            lv_obj_set_size(btn_settings, 38, lv_pct(95));
            lv_obj_set_style_bg_color(btn_settings, lv_color_hex(0x232936), 0);
            lv_obj_set_style_border_color(btn_settings, lv_color_hex(0x3B4252), 0);
            lv_obj_set_style_border_width(btn_settings, 1, 0);
            lv_obj_set_style_radius(btn_settings, 6, 0);
            lv_obj_set_style_pad_all(btn_settings, 0, 0);
            lv_obj_add_event_cb(btn_settings, on_settings_btn_clicked, LV_EVENT_CLICKED, (void*)on_settings_clicked);

            lv_obj_t* btn_lbl = lv_label_create(btn_settings);
            lv_label_set_text(btn_lbl, LV_SYMBOL_SETTINGS);
            lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_16, 0);
            lv_obj_center(btn_lbl);
        }
        // row 2: transport dropdown button on the far right
        else if (trailing == TRAILING_TRANSPORT) {
            // 1. Hidden dropdown to manage the popup list and choices
            dd_trailing = lv_dropdown_create(row_cont);
            lv_dropdown_set_options(dd_trailing, TRANSPORT_OPTIONS);
            lv_obj_add_flag(dd_trailing, LV_OBJ_FLAG_HIDDEN); // Keep the ugly box hidden
            
            // 2. Visible button matching settings gear exactly (38px x 95%)
            lv_obj_t* btn_trans = lv_btn_create(row_cont);
            lv_obj_set_size(btn_trans, 38, lv_pct(95));
            lv_obj_set_style_bg_color(btn_trans, lv_color_hex(0x232936), 0);
            lv_obj_set_style_bg_opa(btn_trans, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(btn_trans, lv_color_hex(0x3B4252), 0);
            lv_obj_set_style_border_width(btn_trans, 1, 0);
            lv_obj_set_style_radius(btn_trans, 6, 0);
            lv_obj_set_style_pad_all(btn_trans, 0, 0);

            // 3. Label that is guaranteed dead-centered on both X and Y
            lv_obj_t* btn_lbl = lv_label_create(btn_trans);
            lv_label_set_text(btn_lbl, "A");
            lv_obj_set_style_text_color(btn_lbl, lv_color_white(), 0);
            lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_16, 0);
            lv_obj_center(btn_lbl); // Pure dead-center alignment

            // Clicking our centered button triggers the dropdown menu to open
            lv_obj_add_event_cb(btn_trans, [](lv_event_t* e) {
                lv_obj_t* dd = (lv_obj_t*)lv_event_get_user_data(e);
                lv_dropdown_open(dd);
                
                // Align popup list to anchor to the right edge and expand leftward instead of going out of screen right
                lv_obj_t* list = lv_dropdown_get_list(dd);
                if (list) {
                    lv_obj_set_width(list, 110);
                    lv_obj_align_to(list, lv_event_get_target(e), LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 4);
                }
            }, LV_EVENT_CLICKED, dd_trailing);

            // Hook dropdown selection change to update the button's centered label and filter
            lv_obj_add_event_cb(dd_trailing, [](lv_event_t* e) {
                lv_obj_t* dd = lv_event_get_target(e);
                lv_obj_t* lbl = (lv_obj_t*)lv_event_get_user_data(e);
                
                uint16_t selected_index = lv_dropdown_get_selected(dd);
                if (selected_index < 4) {
                    lv_label_set_text(lbl, TRANSPORT_LETTERS[selected_index]);
                    lv_obj_center(lbl); // Keep centered when letter updates
                }

                char from_buf[32];
                char to_buf[32];
                char when_buf[32];

                lv_dropdown_get_selected_str(dd_from, from_buf, sizeof(from_buf));
                lv_dropdown_get_selected_str(dd_to,   to_buf,   sizeof(to_buf));
                lv_dropdown_get_selected_str(dd_when, when_buf, sizeof(when_buf));

                TransportType t_type = parse_transport_type(selected_index);
                Serial.printf("Filter: From='%s', To='%s', When='%s', TypeIdx=%u\n", from_buf, to_buf, when_buf, selected_index);
                timetable_filter(from_buf, to_buf, when_buf, t_type);
            }, LV_EVENT_VALUE_CHANGED, btn_lbl);

            // Style popup list
            lv_obj_t* trans_list = lv_dropdown_get_list(dd_trailing);
            lv_obj_set_style_bg_color(trans_list, lv_color_hex(0x1F2430), 0);
            lv_obj_set_style_border_color(trans_list, lv_color_hex(0x3B4252), 0);
            lv_obj_set_style_text_color(trans_list, lv_color_white(), 0);
            lv_obj_set_style_text_align(trans_list, LV_TEXT_ALIGN_LEFT, 0);

            dd_transport = dd_trailing;
        }
    }
};

// main container allocating the top 40% vertical screen space
class Maincontainor {
public:
    lv_obj_t* main_obj;
    std::vector<SubBox> childBox;

    Maincontainor(lv_obj_t* parent) {
        main_obj = lv_obj_create(parent);
        lv_obj_set_width(main_obj, lv_pct(100));
        lv_obj_set_height(main_obj, lv_pct(40));
        lv_obj_align(main_obj, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_set_style_pad_all(main_obj, 3, 0);
        lv_obj_set_style_pad_row(main_obj, 2, 0);
        lv_obj_set_style_bg_color(main_obj, lv_color_hex(0x161B22), 0);
        lv_obj_set_style_border_color(main_obj, lv_color_hex(0x30363D), 0);

        lv_obj_set_flex_flow(main_obj, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(main_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(main_obj, LV_OBJ_FLAG_SCROLLABLE);
    }

    void addSubBox(const char* label_text, const char* options, TrailingType trailing, header_settings_cb_t on_settings_clicked, lv_obj_t** out_dd_ptr) {
        childBox.emplace_back(main_obj, label_text, options, trailing, on_settings_clicked, out_dd_ptr);
    }
};

void header_create(lv_obj_t* parent, header_settings_cb_t on_settings_clicked) {
    static Maincontainor topContainer(parent);

    // builds the 3 filter rows
    topContainer.addSubBox("From:", STOP_OPTIONS, TRAILING_SETTINGS,  on_settings_clicked, &dd_from);
    topContainer.addSubBox("To:",   STOP_OPTIONS, TRAILING_TRANSPORT, nullptr,             &dd_to);
    topContainer.addSubBox("When:", WHEN_OPTIONS, TRAILING_NONE,      nullptr,             &dd_when);
}