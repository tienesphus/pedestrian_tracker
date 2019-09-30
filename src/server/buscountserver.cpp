#include "server.hpp"
#include <data_fetch.hpp>
#include <spdlog/spdlog.h>

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

    spdlog::info("Buscount server finished (does this ever get run?)");

    return 0;
}
