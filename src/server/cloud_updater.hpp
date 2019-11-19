#ifndef BUS_COUNT_CLOUD_UPDATER_HPP
#define BUS_COUNT_CLOUD_UPDATER_HPP

#include <event.hpp>
#include <data_fetch.hpp>

class CloudUpdater {

public:

    explicit CloudUpdater(DataFetch& data);

    /**
     * Sends all events that have occured since the last call to the server
     * @return true if successfully posted
     */
    bool send_events();

    /**
     * Resets the remote count
     * @return true if successfull
     */
    bool send_reset();

private:

    DataFetch& database;

    /**
     * Converts the given events to a json string object
     */
    std::string stringify_event(Event event, time_t timestamp, const std::string& bus_id, float latitude, float longitude);

    /**
     * Posts data to the server
     * @param data the data to send
     * @param server_address the address to send to
     * @return true if the data was posted. false if failure
     */
    bool post_to_server(const std::string& data);
};


#endif //BUS_COUNT_CLOUD_UPDATER_HPP
