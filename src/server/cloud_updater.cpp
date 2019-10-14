#include <utility>

#include "cloud_updater.hpp"

CloudUpdater::CloudUpdater(const std::string& database, std::string server, std::string bus_id)
    :server_address(std::move(server)), bus_id(std::move(bus_id)), database(database)
{}

std::string CloudUpdater::stringify_event(Event event, time_t timestamp, float latitude, float longitude)
{
    return "{"
           "\"event_timestamp\": " + std::to_string(timestamp) + ", "
            "\"busid\": \"" + bus_id + "\", "
            "\"latitude\": " + std::to_string(latitude) + ", "
            "\"longitude\": " + std::to_string(longitude) + ", "
            "\"event_count\": " + std::to_string(delta_count(event)) + ","
            "\"event_type\": " + name(event) + ""
      "}";
}

bool CloudUpdater::post_to_server(const std::string& data)
{
    bool success = true;

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, server_address.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

        // strlen is default
        // curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);

        // Check for errors
        if(res != CURLE_OK){
            spdlog::warn("Failed to post data: {}", curl_easy_strerror(res));
            success = false;
        }

        // cleanup curl
        curl_easy_cleanup(curl);
    }

    return success;
}

bool CloudUpdater::send_events() {
    // Build the events to send
    auto events = database.fetch_events();
    std::string data = "[";
    for (size_t i = 0; i < events.size(); ++i) {
        const auto& tuple = events[i];
        time_t time = std::get<1>(tuple);
        Event event = std::get<2>(tuple);
        // TODO add longitude and latitude
        data += stringify_event(event, time, 0, 0);
        if (i != events.size()-1)
            data += ", ";
    }
    data += "]";

    // Send the data
    if (post_to_server(data)) {

        // delete the posted events
        std::vector<int> remove_ids;
        for (const auto &tuple : events) {
            remove_ids.push_back(std::get<0>(tuple));
        }
        database.remove_events(remove_ids);
        return true;
    } else {
        return false;
    }
}
