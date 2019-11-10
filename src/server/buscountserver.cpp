/**
 * Buscountserver is designed to run locally on the pis. It handles all communication with the android application.
 */

#include "server.hpp"
#include "device_register.hpp"
#include <data_fetch.hpp>

#include <iostream>
#include <cstring>
#include <thread>
#include <zconf.h>

int main(int argc, char** argv)
{
    using namespace server;

    DataFetch data(SOURCE_DIR "/data/database.db");

    bool isServer = false;
    if (argc >= 2 && std::strcmp(argv[1], "master") == 0) {
        std::cout << "Running as master" << std::endl;
        isServer = true;
    }

    // Initialise
    init_slave(data);
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

    // Sit and wait for things
    start();

    running = false;
    device_register.join();

    std::cout << "Buscount server finished" << std::endl;

    return 0;
}
