#ifndef BUS_COUNT_CLOUD_UPDATER_HPP
#define BUS_COUNT_CLOUD_UPDATER_HPP

#include <event.hpp>
#include <data_fetch.hpp>
#include <spdlog/spdlog.h>

#include <curl/curl.h>
#include <string>
#include <vector>

/**
 * DeviceRegister is designed to send heartbeat events to the master pi letting it know that this device
 * still exists. The master pi needs to know of the slave to let the Android device know where to find each device.
 */
class DeviceRegister {

public:

    explicit DeviceRegister(DataFetch& data);

    /**
     * Sends a registration event to the master pi. Should be called at regular intevals
     * @return true if successfully posted
     */
    bool send_registration();

private:

    DataFetch& database;
};


#endif //BUS_COUNT_CLOUD_UPDATER_HPP
