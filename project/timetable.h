#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>

enum DepartureStatus {
    STATUS_ON_TIME,
    STATUS_DELAYED,
    STATUS_CANCELLED
};

//Transport types
enum TransportType {
    TRANSPORT_ALL,
    TRANSPORT_BUS,
    TRANSPORT_TRAIN,
    TRANSPORT_FERRY
};

// Departuree entry
struct DepartureEntry {
    String origin;
    String destination;
    String line;
    String time;
    DepartureStatus status;
    TransportType transport_type;
};

// creates timetable screen
void timetable_create_screen(lv_obj_t* parent);

// sets the active filter based on whats chosen
void timetable_set_filter(const char* stop_name, TransportType transport_type);

// fetches fresh departures from Trafiklab for the active stop
bool timetable_fetch_live_data(void);

// returns readable live data status for Settings screen
const char* timetable_get_live_status(void);

// update view, called on filter changes
void timetable_refresh_view(void);