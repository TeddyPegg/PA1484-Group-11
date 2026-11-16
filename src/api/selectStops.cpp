
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <nlohmann/json.hpp>

// limit around 10km
const float lat_limit = 0.09;
const float lon_limit = 0.16;

// cordinates for Karlskrona
const float lat_ksk = 56.1616;
const float lon_ksk = 15.5866;



bool is_in_area(double lat, double lon)
{
    // calculate the absolute difference between the stop's coordinates and the reference coordinates
    float lat_diff = std::abs(lat - lat_ksk);
    float lon_diff = std::abs(lon - lon_ksk);

// check if the differences are within the defined limits
    return (lat_diff <= lat_limit) && (lon_diff <= lon_limit);
}


bool select_stops(const std::string& input_path, const std::string& output_path)
{
    // read the input JSON file
    std::ifstream input_file(input_path);
    if (!input_file.is_open()) {
        std::cerr << "Failed to open input file: " << input_path << std::endl;
        return false;
    }

    // parse the JSON data
    nlohmann::json data;
    try {
        input_file >> data;
    }
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return false;
    }

    // create a new JSON object for selected stops
    nlohmann::json selected_data = data;
    selected_data["stop_groups"] = nlohmann::json::array();

    int selected_count = 0;
    // check if the JSON contains stop_groups and if it's an array
    if (!data.contains("stop_groups") || !data["stop_groups"].is_array()) {
        std::cerr << "Invalid JSON: stop_groups is missing or is not an array"
                  << std::endl;
        return false;
    }

    // go through all stop groups
    for (const auto& group : data["stop_groups"]) {

        if (!group.contains("stops") || !group["stops"].is_array()) {
            continue;
        }
        // create a new group for selected stops
        nlohmann::json selected_group = group;
        selected_group["stops"] = nlohmann::json::array();

        // go through all individual stops in the group
        for (const auto& stop : group["stops"]) {
            // check if the stop has valid latitude and longitude
            if (!stop.contains("lat") || !stop.contains("lon") ||
                !stop["lat"].is_number() || !stop["lon"].is_number()) {
                continue;
            }
            // get the latitude and longitude of the stop
            double lat = stop["lat"].get<double>();
            double lon = stop["lon"].get<double>();

            // check if the stop is inside our selected area
            if (is_in_area(lat, lon)) {
                selected_group["stops"].push_back(stop);
                selected_count++;
                // print the name of the selected stop
                std::cout << "Selected: "
                          << stop.value("name", "Unknown stop")
                          << std::endl;
            }
        }

        // only save groups that contain selected stops
        if (!selected_group["stops"].empty()) {
            selected_data["stop_groups"].push_back(selected_group);
        }
    }

    // write the selected data to the output file
    std::ofstream output_file(output_path);
    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file: " << output_path << std::endl;
        return false;
    }
    // write the JSON data with indentation for readability
    output_file << selected_data.dump(4);
    // check if the write operation was successful
    if (!output_file.good()) {
        std::cerr << "Failed to write output file: "
                  << output_path << std::endl;
        return false;
    }
    // close the output file
    output_file.close();
    // print the number of selected stops
    std::cout << "Selected " << selected_count << " stops." << std::endl;

    return true;
}


int main()
{
    // define the input and output file paths
    const std::string input_path = "allstops.json";
    const std::string output_path = "stops.json";

    // select stops from the input file and write to the output file
    if (!select_stops(input_path, output_path)) {
        return 1;
    }

    // print a success message
    std::cout << "Successfully created " << output_path << std::endl;

    return 0;
}
