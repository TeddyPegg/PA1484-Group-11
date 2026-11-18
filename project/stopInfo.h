#pragma once

#include <Arduino.h>
#include <String>
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

std::string FetchDepartures(const char* stopId);
std::string FetchArrivals(const char* stopId);