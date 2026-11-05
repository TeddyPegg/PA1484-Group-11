#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>


// trip states
enum DepartureStatus {
    STATUS_ON_TIME,
    STATUS_DELAYED,
    STATUS_CANCELLED
};

// transport types
enum TransportType {
    TRANSPORT_ALL,
    TRANSPORT_BUS,
    TRANSPORT_TRAIN,
    TRANSPORT_FERRY
};

// Represents a single departure row
struct DepartureEntry {
    const char* origin;
    const char* destination;
    const char* line;
    const char* time;
    DepartureStatus status;
    TransportType transport_type;
};

// Tracks active filter state
struct TripState {
    char from_stop[32];
    char to_stop[32];
    char when_time[32];
    TransportType transport_filter;
};

// start scrollable timetable list 
void timetable_create_screen(lv_obj_t* parent);

// apply filters to timetable
void timetable_filter(const char* from_stop, const char* to_stop, const char* when_time, TransportType transport_type = TRANSPORT_ALL);

// feed API data to UI renderer
void timetable_update_data(const DepartureEntry* entries, size_t count);

// retrieve currently active filter selections
const TripState& timetable_get_state(void);