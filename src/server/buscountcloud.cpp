
#include <stdexcept>
#include <thread>
#include <zconf.h>
#include "cloud_updater.hpp"

int main() {

    DataFetch data(SOURCE_DIR "/data/database.db");
    CloudUpdater updater(data);
    // TOOD implement some way to kill cloud server
    std::atomic_bool running(true);

    std::thread thread([&]() {
        while (running) {
            updater.send_events();
            sleep(5);
        }
    });

    thread.join();
}
