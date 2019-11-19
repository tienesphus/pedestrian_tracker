#include "cloud_updater.hpp"

#include <spdlog/spdlog.h>

#include <curl/curl.h>
#include <vector>

CloudUpdater::CloudUpdater(DataFetch& data)
    :database(data)
{}

std::string CloudUpdater::stringify_event(Event event, time_t timestamp, const std::string& bus_id, float latitude, float longitude)
{
    std::string server_type = delta_count(event) < 0 ? "out" : "in";

    return "{"
           "\"event_timestamp\": " + std::to_string(timestamp) + ", "
            "\"busid\": \"" + bus_id + "\", "
            "\"latitude\": " + std::to_string(latitude) + ", "
            "\"longitude\": " + std::to_string(longitude) + ", "
            "\"event_count\": 1,"
            "\"event_type\": \"" + server_type + "\""
      "}";
}

bool CloudUpdater::post_to_server(const std::string& data)
{
    bool success = false;

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_URL, (database.get_remote_url() + "/status-update").c_str());

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK){
            spdlog::warn("Failed to post data: {}", curl_easy_strerror(res));
            success = false;
        } else {
            long http_code = 0;
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 200) {
                success = true;
            } else {
                spdlog::warn("Response {} on cloud update", http_code);
                success = false;
            }
        }

        // cleanup curl
        curl_easy_cleanup(curl);
    }

    return success;
}

bool CloudUpdater::send_reset()
{

    auto start = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(start);

    // Send the data
    return post_to_server("{"
           "\"event_timestamp\": " + std::to_string(time) + ", "
            "\"busid\": \"" + database.get_busid() + "\", "
            "\"latitude\": 0, "
            "\"longitude\": 0, "
            "\"event_count\": 0,"
            "\"event_type\": count"
      "}");
}

bool CloudUpdater::send_events()
{
    // Build the events to send
    auto events = database.fetch_events();
    if (events.empty()) {
        return true;
    }

    std::string bus_id = database.get_busid();

    // Build the server's json string
    std::string data = "[";
    for (size_t i = 0; i < events.size(); ++i) {
        const auto& tuple = events[i];
        time_t time = std::get<1>(tuple);
        Event event = std::get<2>(tuple);
        // TODO add longitude and latitude
        data += stringify_event(event, time, bus_id, 0, 0);
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
