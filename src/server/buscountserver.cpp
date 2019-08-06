#include "server.hpp"
#include <data_fetch.hpp>

#include <iostream>

int main()
{
    using namespace server;

    DataFetch data(SOURCE_DIR "/data/database.db");

    init_slave(
            [&data]() -> WorldConfig {
                return data.get_latest_config();
            },
            [&data](WorldConfig new_config) {
                data.update_config(new_config);
            }
    );

    init_master(
            [&data]() -> int {
                return data.count();
            },
            [&data](int delta) {
                data.add_count(delta);
            }
    );

    start();

    std::cout << "Buscount server finished (does this ever get run?)" << std::endl;

    return 0;
}
