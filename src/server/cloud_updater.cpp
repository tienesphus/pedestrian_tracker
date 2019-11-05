#include "cloud_updater.hpp"

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
    bool success = true;

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
        if(res != CURLE_OK){
            spdlog::warn("Failed to post data: {}", curl_easy_strerror(res));
            success = false;
        }

        // cleanup curl
        curl_easy_cleanup(curl);
    }

    return success;
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
