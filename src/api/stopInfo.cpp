#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string>
#include "../../secreats.h"
#include <iostream>


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


std::string GetArrivalTime(const std::string& jsonResponse)
{
    // create a JSON document
    JsonDocument doc;

    // deserialize the JSON response
    DeserializationError error = deserializeJson(doc, jsonResponse);

    // check if the JSON was read correctly
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return "";
    }

    // get the arrivals array
    JsonArray arrivals = doc["arrivals"].as<JsonArray>();

    // check if there are any arrivals
    if (arrivals.isNull() || arrivals.size() == 0) {
        Serial.println("No arrivals found");
        return "";
    }

    // get the first arrival
    JsonObject arrival = arrivals[0];

    // get the realtime time, or scheduled time if realtime is missing
    const char* time = arrival["realtime"] | arrival["scheduled"] | "";

    // check if a time was found
    if (time == nullptr || time[0] == '\0') {
        Serial.println("No arrival time found");
        return "";
    }

    // return only HH:MM
    return std::string(time).substr(11, 5);
}


std::string GetDepartureTime(const std::string& jsonResponse)
{
    // create a JSON document
    JsonDocument doc;

    // deserialize the JSON response
    DeserializationError error = deserializeJson(doc, jsonResponse);

    // check if the JSON was read correctly
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return "";
    }

    // get the departures array
    JsonArray departures = doc["departures"].as<JsonArray>();

    // check if there are any departures
    if (departures.isNull() || departures.size() == 0) {
        Serial.println("No departures found");
        return "";
    }

    // get the first departure
    JsonObject departure = departures[0];

    // get the realtime time, or scheduled time if realtime is missing
    const char* time = departure["realtime"] | departure["scheduled"] | "";

    // check if a time was found
    if (time == nullptr || time[0] == '\0') {
        Serial.println("No departure time found");
        return "";
    }

    // return only HH:MM
    return std::string(time).substr(11, 5);
}


std::string FetchDepartures(const char* stopId)
{
    return MakeApiRequest(departureUrl, stopId);
}
/*
std::string response = FetchDepartures("EXAMPLE_STOP_ID");

std::string departureTime = GetDepartureTime(response);

Serial.println(departureTime.c_str());

*/

std::string FetchArrivals(const char* stopId)
{
    return MakeApiRequest(arrivalsUrl, stopId);
}
/*
std::string response = FetchArrivals("EXAMPLE_STOP_ID");

std::string arrivalTime = GetArrivalTime(response);

Serial.println(arrivalTime.c_str());

*/
