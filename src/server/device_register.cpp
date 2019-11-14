#include "device_register.hpp"
#include <data_fetch.hpp>

DeviceRegister::DeviceRegister(DataFetch& data)
    :database(data)
{}


bool DeviceRegister::send_registration()
{
    auto name = database.get_name();

    bool success = true;

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        // TODO don't hardcode master device's IP address
        curl_easy_setopt(curl, CURLOPT_URL, ("192.168.82.1:8080/register"));

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        std::string data = R"({"name": ")" + name + "\"}";
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


