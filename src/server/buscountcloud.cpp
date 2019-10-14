
#include <stdexcept>
#include <thread>
#include <zconf.h>
#include "cloud_updater.hpp"

int main() {

    // TODO Cloud Updater location should come from android
    CloudUpdater updater(SOURCE_DIR "/data/database.db", "http://buscount-cloud-dev.ap-southeast-2.elasticbeanstalk.com/status-update", "123");
    // TOOD implement some way to kill cloud server
    bool running = true;

    std::thread thread([&]() {
        while (running) {
            updater.send_events();
            sleep(5);
        }
    });

    thread.join();
}
