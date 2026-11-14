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
    const char* origin;
    const char* destination;
    const char* line;
    const char* time;
    DepartureStatus status;
    TransportType transport_type;
};

// creates timetable screen
void timetable_create_screen(lv_obj_t* parent);

// sets the active filter based on whats chosen
void timetable_set_filter(const char* stop_name, TransportType transport_type);

// update view, called on filter changes
void timetable_refresh_view(void);