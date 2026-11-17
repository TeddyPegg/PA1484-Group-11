#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <curl/curl.h>
#include "../../secrets.h"

namespace {

const char kEndpoint[] = "https://realtime-api.trafiklab.se/v1/stops/list";
const char allStops[] = "allstops.json";
const char kalskronaStops[] = "stops.json";


bool file_has_data(const char* path)
{
	std::ifstream existing_file(path, std::ios::binary | std::ios::ate);
	return existing_file && existing_file.tellg() > 0;
}


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    // calculate the total size of the data
    size_t totalSize = size * nmemb;

    // cast the user pointer to a string pointer
    std::string* response = static_cast<std::string*>(userp);

    // append the data to the response string
    response->append(static_cast<char*>(contents), totalSize);

    return totalSize;
}


bool fetch_stops(const char* path)
{
    // initialize curl
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        std::cerr << "Failed to initialize curl" << std::endl;
        return false;
    }

    // make sure api key doesn't contain special characters that could break URL
    char* realTimeKey = curl_easy_escape(curl, RealTime_API_KEY, 0);
    if (realTimeKey == nullptr) {
        curl_easy_cleanup(curl);
        return false;
    }

    // add api key to endpoint
    std::string url_with_key = std::string(kEndpoint) + "?key=" + realTimeKey;
    curl_free(realTimeKey);

    // set URL
    curl_easy_setopt(curl, CURLOPT_URL, url_with_key.c_str());

    // collect response from API
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // perform the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Failed to perform curl request: "
                  << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    // check if API request was successful
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code < 200 || http_code >= 300) {
        std::cerr << "API returned HTTP status: " << http_code << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    // save response to file
    std::ofstream output(path);
    if (!output.is_open()) {
        std::cerr << "Failed to open output file: " << path << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }
    // write the response to the file
    output << response;

    if (!output.good()) {
        std::cerr << "Failed to write response to file" << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }
    // close the file
    output.close();
    // clean up curl
    curl_easy_cleanup(curl);
    return true;
}


}

int main()
{
    // initialize curl globally
    CURLcode init = curl_global_init(CURL_GLOBAL_DEFAULT);

    if (init != CURLE_OK) {
        std::cerr << "Failed to initialize curl globally" << std::endl;
        return 1;
    }

    // fetch all stops from the API
    bool success = fetch_stops(allStops);

    // clean up curl globally
    curl_global_cleanup();

    if (!success) {
        std::cerr << "Failed to fetch stops" << std::endl;
        return 1;
    }

    std::cout << "Successfully downloaded all stops!" << std::endl;

    return 0;
}


