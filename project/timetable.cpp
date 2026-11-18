#include "timetable.h"
#include "stopInfo.h"
#include <vector>
#include <string.h>
#include <Arduino.h>
#include <WiFi.h>

LV_FONT_DECLARE(font_montserrat_16);

struct StopMapping {
    const char* name;
    const char* stop_id;
};

// 7 Karlskrona stops covering bus, train, and ferry
static const StopMapping KARLSKRONA_STOPS[] = {
    {"Campus Gräsvik",    "740032188"},
    {"Karlskrona C",      "740000230"},
    {"Kungsplan",         "740020510"},
    {"Bergåsa Station",   "740001537"},
    {"Lyckeby Centrum",   "740000663"},
    {"Handelshamnen",     "740020699"},
    {"Amiralitetstorget", "740040917"}
};

static const size_t STOPS_COUNT = sizeof(KARLSKRONA_STOPS) / sizeof(KARLSKRONA_STOPS[0]);

static std::vector<DepartureEntry> live_departures;
static lv_obj_t* list_container = nullptr;
static char active_stop[32] = "Campus Gräsvik";
static TransportType active_transport = TRANSPORT_ALL;
static char live_status_msg[48] = "Waiting for sync...";

static const char* get_stop_id_by_name(const char* name) {
    for (size_t i = 0; i < STOPS_COUNT; i++) {
        if (strcmp(KARLSKRONA_STOPS[i].name, name) == 0) {
            return KARLSKRONA_STOPS[i].stop_id;
        }
    }
    return KARLSKRONA_STOPS[0].stop_id; // Fallback to Campus Gräsvik
}

static TransportType parse_transport_mode(const String& mode) {
    if (mode.equalsIgnoreCase("TRAIN")) return TRANSPORT_TRAIN;
    if (mode.equalsIgnoreCase("FERRY") || mode.equalsIgnoreCase("BOAT") ||
        mode.equalsIgnoreCase("WATER") || mode.equalsIgnoreCase("SHIP")) {
        return TRANSPORT_FERRY;
    }
    return TRANSPORT_BUS;
}

static lv_color_t get_status_color(DepartureStatus status) {
    switch (status) {
        case STATUS_DELAYED:   return lv_color_hex(0xE67E22);
        case STATUS_CANCELLED: return lv_color_hex(0xE74C3C);
        case STATUS_ON_TIME:
        default:               return lv_color_hex(0x27AE60);
    }
}

// function to render each row in the timetable
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

    // Line badge
    lv_obj_t* badge = lv_obj_create(row);
    lv_obj_set_size(badge, 40, 28);
    lv_obj_set_style_bg_color(badge, get_status_color(item.status), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_radius(badge, 4, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* badge_lbl = lv_label_create(badge);
    lv_label_set_text(badge_lbl, item.line.c_str());
    lv_obj_set_style_text_color(badge_lbl, lv_color_white(), 0);
    lv_obj_center(badge_lbl);

    // Destination
    lv_obj_t* dest_lbl = lv_label_create(row);
    lv_label_set_text(dest_lbl, item.destination.c_str());
    lv_obj_set_style_text_color(dest_lbl, lv_color_white(), 0);
    lv_obj_set_flex_grow(dest_lbl, 1);
    lv_obj_set_style_pad_left(dest_lbl, 8, 0);

    // Time and status + color for status
    lv_obj_t* time_box = lv_obj_create(row);
    lv_obj_set_size(time_box, 64, 28);
    lv_obj_set_style_radius(time_box, 4, 0);
    lv_obj_set_style_pad_all(time_box, 0, 0);
    lv_obj_set_style_border_width(time_box, 0, 0);
    lv_obj_clear_flag(time_box, LV_OBJ_FLAG_SCROLLABLE);

    if (item.status == STATUS_DELAYED) {
        lv_obj_set_style_bg_color(time_box, lv_color_hex(0xE67E22), 0);
        lv_obj_set_style_bg_opa(time_box, LV_OPA_COVER, 0);
    } else if (item.status == STATUS_CANCELLED) {
        lv_obj_set_style_bg_color(time_box, lv_color_hex(0xE74C3C), 0);
        lv_obj_set_style_bg_opa(time_box, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_bg_opa(time_box, LV_OPA_TRANSP, 0);
    }

    lv_obj_t* time_lbl = lv_label_create(time_box);
    lv_label_set_text(time_lbl, item.time.c_str());
    lv_obj_set_style_text_color(time_lbl, lv_color_white(), 0);
    lv_obj_center(time_lbl);
}

// Fetches live departures from Trafiklab for the active stop
bool timetable_fetch_live_data(void) {
    if (WiFi.status() != WL_CONNECTED) {
        snprintf(live_status_msg, sizeof(live_status_msg), "Offline (No Wi-Fi)");
        timetable_refresh_view();
        return false;
    }   

    const char* stop_id = get_stop_id_by_name(active_stop);
    std::vector<ApiDeparture> fetched = FetchParsedDepartures(stop_id, 15);

    if (fetched.empty()) {
        snprintf(live_status_msg, sizeof(live_status_msg), "API Error / No Data");
        live_departures.clear();
        timetable_refresh_view();
        return false;
    }

    live_departures.clear();
    for (const auto& d : fetched) {
        DepartureEntry entry;
        entry.origin = active_stop;
        entry.destination = d.destination;
        entry.line = d.line;
        entry.time = d.time;
        entry.transport_type = parse_transport_mode(d.transportMode);

        if (d.isCancelled) {
            entry.status = STATUS_CANCELLED;
        } else if (d.isDelayed) {
            entry.status = STATUS_DELAYED;
        } else {
            entry.status = STATUS_ON_TIME;
        }
        live_departures.push_back(entry);
    }

    unsigned long secs = millis() / 1000;
    snprintf(live_status_msg, sizeof(live_status_msg), "Live OK (%u items @ %lus)", (unsigned)live_departures.size(), secs);
    timetable_refresh_view();
    return true;
}

const char* timetable_get_live_status(void) {
    return live_status_msg;
}

// function that redraws the timetable whenever it's called on filter changes
void timetable_refresh_view(void) {
    if (!list_container) return;
    lv_obj_clean(list_container);

    size_t matches = 0;
    for (size_t i = 0; i < live_departures.size(); i++) {
        bool match_type = (active_transport == TRANSPORT_ALL) ||
                          (live_departures[i].transport_type == active_transport);

        if (match_type) {
            render_row(list_container, live_departures[i]);
            matches++;
        }
    }

    if (matches == 0) {
        lv_obj_t* empty_lbl = lv_label_create(list_container);
        lv_label_set_text(empty_lbl, WiFi.status() == WL_CONNECTED ? "No departures found." : "Wi-Fi disconnected.");
        lv_obj_set_style_text_color(empty_lbl, lv_color_hex(0x64748B), 0);
        lv_obj_align(empty_lbl, LV_ALIGN_CENTER, 0, 0);
    }
}

// sets the filter to active choices and fetches new API data if stop changed
void timetable_set_filter(const char* stop_name, TransportType transport_type) {
    bool stop_changed = false;
    if (stop_name && strcmp(active_stop, stop_name) != 0) {
        strncpy(active_stop, stop_name, sizeof(active_stop) - 1);
        active_stop[sizeof(active_stop) - 1] = '\0';
        stop_changed = true;
    }
    active_transport = transport_type;

    if (stop_changed || live_departures.empty()) {
        timetable_fetch_live_data();
    } else {
        timetable_refresh_view();
    }
}

// original function to create the timetable screen
void timetable_create_screen(lv_obj_t* parent) {
    list_container = lv_obj_create(parent);
    lv_obj_set_size(list_container, lv_pct(100), lv_pct(86));
    lv_obj_align(list_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0x0F1117), 0);
    lv_obj_set_style_border_opa(list_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(list_container, 6, 0);
    lv_obj_set_style_pad_row(list_container, 6, 0);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN);

    timetable_refresh_view();
}