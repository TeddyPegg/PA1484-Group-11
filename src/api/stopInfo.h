
#pragma once

#include <Arduino.h>
#include <vector>

struct ApiDeparture {
    String line;
    String destination;
    String time;
    String transportMode;
    bool isDelayed;
    bool isCancelled;
};

std::vector<ApiDeparture> FetchParsedDepartures(
    const char* stopId,
    size_t maxCount = 15
);
std::vector<ApiDeparture> FetchParsedArrivals(
    const char* stopId,
    size_t maxCount = 15
);