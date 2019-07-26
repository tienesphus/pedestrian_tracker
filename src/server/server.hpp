#ifndef BUS_COUNT_SERVER_HPP
#define BUS_COUNT_SERVER_HPP

#include <functional>

#include <json/json.h>

#include "../optional.hpp"
#include "../utils.hpp"
#include "server_client.hpp"

namespace server {

    struct Feed {
        std::string name;
        std::string location;

        Feed(std::string name, std::string location);

    };

    struct Config {
        OpenCVConfig cvConfig;
        std::vector<Feed> feeds;

        Config(const OpenCVConfig &cvConfig, std::vector<Feed> feeds);
    };


    /**
     * Initialises the server with the master config
     * It is okay to initialise the server as both master and slave
     */
    void init_master();

    /**
     * Initialises the server as a slave
     * It is okay to initialise the server as both master and slave
     */
    void init_slave(const std::function<Config()> &getConfig, const std::function<void(OpenCVConfig)> &setConfig);

    /**
     * Starts the server (blocking).
     * Should only be run after init_master or init_slave has been called.
     * This *MUST* be run from the main thread. Doing anything else will
     * cause *everything* to stop dead (sometimes with strange usb error?)
     */
    void start();


    /**
     * Shut the server down.
     * TODO calling server::quit causes some error messages to appear.
     */
    void quit();
}

#endif //BUS_COUNT_SERVER_HPP
