#include "timetable.h"
#include <vector>
#include <string.h>
#include <Arduino.h>

LV_FONT_DECLARE(font_montserrat_16);



// Static mock data representing demo departures in Karlskrona
static const DepartureEntry DEMO_DEPARTURES[] = {
    {"Campus Gräsvik",   "Karlskrona C",     "1", "12:15", STATUS_ON_TIME,   TRANSPORT_BUS},
    {"Campus Gräsvik",   "Lyckeby Centrum",  "2", "12:22", STATUS_DELAYED,   TRANSPORT_BUS},
    {"Karlskrona C",     "Bergåsa Station",  "5", "12:30", STATUS_ON_TIME,   TRANSPORT_BUS},
    {"Kungsplan",        "Campus Gräsvik",   "1", "12:35", STATUS_CANCELLED, TRANSPORT_BUS},
    {"Bergåsa Station",  "Karlskrona C",     "7", "12:42", STATUS_DELAYED,   TRANSPORT_TRAIN},
    {"Lyckeby Centrum",  "Kungsplan",        "2", "12:50", STATUS_ON_TIME,   TRANSPORT_BUS},
    {"Campus Gräsvik",   "Bergåsa Station",  "3", "12:58", STATUS_ON_TIME,   TRANSPORT_BUS}
};

static const size_t DEMO_COUNT = sizeof(DEMO_DEPARTURES) / sizeof(DEMO_DEPARTURES[0]);

static const DepartureEntry* active_entries = DEMO_DEPARTURES;
static size_t active_count = DEMO_COUNT;

static lv_obj_t* list_container = nullptr;
static TripState current_state = {"All", "All", "Now", TRANSPORT_ALL};

// map status to color palette: Green = on time, Orange = delayed, Red = cancelled
static lv_color_t get_status_color(DepartureStatus status) {
    switch (status) {
        case STATUS_DELAYED:   return lv_color_hex(0xE67E22); // Solid Orange
        case STATUS_CANCELLED: return lv_color_hex(0xE74C3C); // Solid Red
        case STATUS_ON_TIME:
        default:               return lv_color_hex(0x27AE60); // Solid Green
    }
}

// render an individual departure item row
static void render_row(lv_obj_t* parent, const DepartureEntry& item) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 44);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1F2430), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x2D3748), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_set_style_pad_hor(row, 8, 0);
    lv_obj_set_style_pad_ver(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 1. Line badge on left
    lv_obj_t* badge = lv_obj_create(row);
    lv_obj_set_size(badge, 40, 28);
    lv_obj_set_style_bg_color(badge, get_status_color(item.status), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_radius(badge, 4, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* badge_lbl = lv_label_create(badge);
    lv_label_set_text(badge_lbl, item.line);
    lv_obj_set_style_text_color(badge_lbl, lv_color_white(), 0);
    lv_obj_center(badge_lbl);

    // 2. Destination in middle
    lv_obj_t* dest_lbl = lv_label_create(row);
    lv_label_set_text(dest_lbl, item.destination);
    lv_obj_set_style_text_color(dest_lbl, lv_color_white(), 0);
    lv_obj_set_flex_grow(dest_lbl, 1);
    lv_obj_set_style_pad_left(dest_lbl, 8, 0);

    // 3. Time on right with status background
    lv_obj_t* time_box = lv_obj_create(row);
    lv_obj_set_size(time_box, 64, 28);
    lv_obj_set_style_radius(time_box, 4, 0);
    lv_obj_set_style_pad_all(time_box, 0, 0);
    lv_obj_set_style_border_width(time_box, 0, 0);
    lv_obj_clear_flag(time_box, LV_OBJ_FLAG_SCROLLABLE);

    if (item.status == STATUS_DELAYED) {
        lv_obj_set_style_bg_color(time_box, lv_color_hex(0xE67E22), 0); // Orange
        lv_obj_set_style_bg_opa(time_box, LV_OPA_COVER, 0);
    } else if (item.status == STATUS_CANCELLED) {
        lv_obj_set_style_bg_color(time_box, lv_color_hex(0xE74C3C), 0); // Red
        lv_obj_set_style_bg_opa(time_box, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_bg_opa(time_box, LV_OPA_TRANSP, 0); // Transparent for On Time
    }

    lv_obj_t* time_lbl = lv_label_create(time_box);
    lv_label_set_text(time_lbl, item.time);
    lv_obj_set_style_text_color(time_lbl, lv_color_white(), 0);
    lv_obj_center(time_lbl);
}

// reconstruct the scrollabel departure list with the active filter criteria
static void rebuild_timetable_view(void) {
    if (!list_container) return;

    lv_obj_clean(list_container);

    size_t matches = 0;
    for (size_t i = 0; i < active_count; i++) {
        bool match_from = (strcmp(current_state.from_stop, "All") == 0) || 
                          (strcmp(active_entries[i].origin, current_state.from_stop) == 0);
        bool match_to   = (strcmp(current_state.to_stop, "All") == 0) || 
                          (strcmp(active_entries[i].destination, current_state.to_stop) == 0);
        bool match_type = (current_state.transport_filter == TRANSPORT_ALL) || 
                          (active_entries[i].transport_type == current_state.transport_filter);

        if (match_from && match_to && match_type) {
            render_row(list_container, active_entries[i]);
            matches++;
        }
    }

    // fallback when no departures match the route criteria
    if (matches == 0) {
        lv_obj_t* empty_lbl = lv_label_create(list_container);
        lv_label_set_text(empty_lbl, "No departures found matching route.");
        lv_obj_set_style_text_color(empty_lbl, lv_color_hex(0x64748B), 0);
        lv_obj_align(empty_lbl, LV_ALIGN_CENTER, 0, 0);
    }
}

void timetable_create_screen(lv_obj_t* parent) {
    list_container = lv_obj_create(parent);
    
    // allocate exacly 60% vertical height to match with header taking 40% height on top
    lv_obj_set_size(list_container, lv_pct(100), lv_pct(60));
    lv_obj_align(list_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0x0F1117), 0);
    lv_obj_set_style_border_opa(list_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(list_container, 6, 0);
    lv_obj_set_style_pad_row(list_container, 6, 0);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN);

    rebuild_timetable_view();
}

void timetable_filter(const char* from_stop, const char* to_stop, const char* when_time, TransportType transport_type) {
    if (from_stop) strncpy(current_state.from_stop, from_stop, sizeof(current_state.from_stop) - 1);
    if (to_stop)   strncpy(current_state.to_stop,   to_stop,   sizeof(current_state.to_stop) - 1);
    if (when_time) strncpy(current_state.when_time, when_time, sizeof(current_state.when_time) - 1);
    current_state.transport_filter = transport_type;

    rebuild_timetable_view();
}

void timetable_update_data(const DepartureEntry* entries, size_t count) {
    if (entries && count > 0) {
        active_entries = entries;
        active_count = count;
    } else {
        active_entries = DEMO_DEPARTURES;
        active_count = DEMO_COUNT;
    }
    rebuild_timetable_view();
}

const TripState& timetable_get_state(void) {
    return current_state;
}