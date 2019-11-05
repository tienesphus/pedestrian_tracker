#ifndef BUS_COUNT_SERVER_HPP
#define BUS_COUNT_SERVER_HPP

#include <functional>

#include <json/json.h>

#include "json_convert.hpp"

#include <optional.hpp>
#include <geom.hpp>
#include <config.hpp>
#include <data_fetch.hpp>

namespace server {

    /**
     * Initialises the server with the master config
     * It is okay to initialise the server as both master and slave
     */
    void init_master(DataFetch& data);

    /**
     * Initialises the server as a slave
     * It is okay to initialise the server as both master and slave
     */
    void init_slave(DataFetch& data);

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
