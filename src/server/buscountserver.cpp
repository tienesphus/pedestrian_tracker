/**
 * Buscountserver is designed to run locally on all pis. It handles all communication with the android application,
 * handles inter-pi communication and updates the cloud server.
 *
 * It has two options to start up - slave or master. To start in slave mode, just run the program. To start in server
 * mode, run the program with "master" as the first argument (e.g. `./buscoundserver master`)
 */

#include "server.hpp"
#include "device_register.hpp"
#include "cloud_updater.hpp"
#include <data_fetch.hpp>
#include <spdlog/spdlog.h>

#include <iostream>
#include <cstring>
#include <thread>
#include <zconf.h>

int main(int argc, char** argv)
{
    using namespace server;

    DataFetch data(SOURCE_DIR "/data/database.db");
    CloudUpdater updater(data);

    bool isServer = false;
    if (argc >= 2 && std::strcmp(argv[1], "master") == 0) {
        spdlog::info("Running as master");
        isServer = true;
    }

    // Initialise
    init_slave(data, updater);
    if (isServer)
        init_master(data);

    std::atomic_bool running(true);

    // Run a seperate thread that talks to the master
    DeviceRegister registerator(data);
    std::thread device_register([&running, &registerator]() {
        usleep(500*1000); // allow server to startup
        while (running) {
            registerator.send_registration();
            usleep(2000*1000); // 2 seconds
        }
    });

    std::thread cloud_updater([&updater, &running]() {
        while (running) {
            updater.send_events();
            sleep(5);
        }
    });

    // Sit and wait for things
    start();

    // When the server is killed, stop the background tasks too
    running = false;
    device_register.join();
    cloud_updater.join();

    spdlog::info("Buscount server finished");

    return 0;
}
