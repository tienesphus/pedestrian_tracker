/**
 * Buscountserver is designed to run locally on the pis. It handles all communication with the android application.
 */

#include "server.hpp"
#include <data_fetch.hpp>

#include <iostream>

int main()
{
    using namespace server;

    DataFetch data(SOURCE_DIR "/data/database.db");

    init_slave(data);
    init_master(data);

    start();

    std::cout << "Buscount server finished (does this ever get run?)" << std::endl;

    return 0;
}
