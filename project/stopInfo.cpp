#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string>
#include "secrets.h"
#include "stopInfo.h"
#include <vector>

const char departureUrl[] = "https://realtime-api.trafiklab.se/v1/departures/";
const char arrivalsUrl[] = "https://realtime-api.trafiklab.se/v1/arrivals/";

std::string MakeApiRequest(const char* endpoint, const char* stopId)
{
    //make the url with the stopId and api key
    std::string url = std::string(endpoint) + stopId + "?key=" + RealTime_API_KEY;
    
    // check if WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return "";
    }

    //initialize HTTPClient
    HTTPClient http;
    http.begin(url.c_str());

    //make headers for the request (ask for json and add the api key)
    http.addHeader("Accept", "application/json");
    //basically says that we are allowed to request information from the API with our api key
    http.addHeader("Authorization", (String("apikey ") + RealTime_API_KEY).c_str());

    //send the GET request
    int httpCode = http.GET();

    //get the response
    std::string response;

    //check if the request was successful
    if (httpCode > 0) {
        //get the response payload
        response = http.getString().c_str();
    } else {
        //print the error if the request failed
        Serial.print("HTTP request failed, error: ");
        Serial.println(http.errorToString(httpCode));
    }

    //end the HTTP connection
    http.end();

    //check if the we get a valid response from the API
    if (httpCode < 200 || httpCode >= 300) {
        Serial.print("API returned HTTP status: ");
        Serial.println(httpCode);
        return "";
    }

    return response;
}


// Funktion för att hämta och parsa ankomster för en given stopId där maxCount anger hur många ankomster som ska returneras
std::vector<ApiDeparture> FetchParsedArrivals(const char* stopId, size_t maxCount)
{
    std::vector<ApiDeparture> arrivalsList;

    // hämtar ankomst JSON
    std::string jsonResponse = FetchArrivals(stopId);

    // kollar om API-request gick igenom
    if (jsonResponse.empty()) {
        return arrivalsList;
    }

    // skapar ett json filter för tid, plannerad ankomsttid, förseningar, inställd, linje, nummer, riktning och transportmedel 
    JsonDocument filter;

    filter["arrivals"][0]["realtime"] = true;
    filter["arrivals"][0]["scheduled"] = true;
    filter["arrivals"][0]["delay"] = true;
    filter["arrivals"][0]["canceled"] = true;
    filter["arrivals"][0]["route"]["designation"] = true;
    filter["arrivals"][0]["route"]["direction"] = true;
    filter["arrivals"][0]["route"]["transport_mode"] = true;

    // skapar ett json dokument
    JsonDocument doc;

    // filterar json response med hjälp av filter
    DeserializationError error = deserializeJson(
        doc,
        jsonResponse,
        DeserializationOption::Filter(filter)
    );

    // kollar om json response gick igenom
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return arrivalsList;
    }

    // hämtar ankomster array
    JsonArray arrivals = doc["arrivals"].as<JsonArray>();

    // kollar om det finns några ankomster
    if (arrivals.isNull()) {
        Serial.println("No arrivals found");
        return arrivalsList;
    }

    // loopar igenom ankomsterna
    for (JsonObject arrival : arrivals) {

        // stoppar när vi har nått maxCount
        if (arrivalsList.size() >= maxCount) {
            break;
        }

        // hämtar route information
        JsonObject route = arrival["route"];

        // hämtar realtidstid eller planerad tid om realtid saknas
        const char* time = arrival["realtime"] | arrival["scheduled"] | "";

        // skapar en ankomst objekt
        ApiDeparture apiArrival;

        apiArrival.line = route["designation"] | "";
        apiArrival.destination = route["direction"] | "";
        apiArrival.transportMode = route["transport_mode"] | "";
        apiArrival.time = "";

        // tar bara ut HH:MM från tiden
        if (time != nullptr && time[0] != '\0') {
            String timeString(time);
            if (timeString.length() >= 16) {
                apiArrival.time = timeString.substring(11, 16);
            }
        }

        // hämtar försenings- och avbokningsinformation
        int delay = arrival["delay"] | 0;
        apiArrival.isDelayed = delay > 0;
        apiArrival.isCancelled = arrival["canceled"] | false;

        // lägger till ankomsten till listan
        arrivalsList.push_back(apiArrival);
    }

    return arrivalsList;
}

// Funktion för att hämta och parsa avgångar för en given stopId där maxCount anger hur många avgångar som ska returneras
std::vector<ApiDeparture> FetchParsedDepartures(const char* stopId, size_t maxCount)
{
    // skapar en lista för avgångar
    std::vector<ApiDeparture> departuresList;

    // hämtar avgångs JSON
    std::string jsonResponse = FetchDepartures(stopId);

    // kollar om API-request gick igenom
    if (jsonResponse.empty()) {
        return departuresList;
    }

    // skapar ett json filter för tid, plannerad ankomsttid, förseningar, inställd, linje, nummer, riktning och transportmedel 
    JsonDocument filter;

    filter["departures"][0]["realtime"] = true;
    filter["departures"][0]["scheduled"] = true;
    filter["departures"][0]["delay"] = true;
    filter["departures"][0]["canceled"] = true;
    filter["departures"][0]["route"]["designation"] = true;
    filter["departures"][0]["route"]["destination"]["name"] = true;
    filter["departures"][0]["route"]["transport_mode"] = true;

    // skapar en JSON document
    JsonDocument doc;

    // deserialize the JSON response using the filter
    DeserializationError error = deserializeJson(
        doc,
        jsonResponse,
        DeserializationOption::Filter(filter)
    );

    // kollar om json parsing gick igenom
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return departuresList;
    }

    // skapar en JSON array med avgångar
    JsonArray departures = doc["departures"].as<JsonArray>();

    // kollar om det finns några avgångar
    if (departures.isNull()) {
        Serial.println("No departures found");
        return departuresList;
    }

    // loopar igenom avgångarna
    for (JsonObject departure : departures) {

        // stoppar när vi har nått maxCount
        if (departuresList.size() >= maxCount) {
            break;
        }

        // plockar ut route information
        JsonObject route = departure["route"];

        // hämtar realtidstid eller planerad tid om realtid saknas
        const char* time = departure["realtime"] | departure["scheduled"] | "";

        // skapar ett departure objekt
        ApiDeparture apiDeparture;

        apiDeparture.line = route["designation"] | "";
        apiDeparture.destination = route["destination"]["name"] | "";
        apiDeparture.transportMode = route["transport_mode"] | "";
        apiDeparture.time = "";

        // ta bara ut HH:MM från tiden
        if (time != nullptr && time[0] != '\0' && strlen(time) >= 16) {
            apiDeparture.time = String(time).substring(11, 16);
        }

        // hämtar försenings- och avbokningsinformation
        int delay = departure["delay"] | 0;
        apiDeparture.isDelayed = delay > 0;
        apiDeparture.isCancelled = departure["canceled"] | false;

        // lägger till avgången till listan
        departuresList.push_back(apiDeparture);
    }
    
    return departuresList;
}


std::string FetchDepartures(const char* stopId)
{
    return MakeApiRequest(departureUrl, stopId);
}


std::string FetchArrivals(const char* stopId)
{
    return MakeApiRequest(arrivalsUrl, stopId);
}

