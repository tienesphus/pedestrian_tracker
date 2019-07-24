#include "server.hpp"
#include <thread>
#include <iostream>

int main()
{
    using namespace server;

//    // fake slave state
//    std::vector<Feed> feeds;
//    feeds.emplace_back("live", "/live");
//    feeds.emplace_back("test", "/test");
//    feeds.emplace_back("dirty", "/dirty");
//    Config config(
//            OpenCVConfig(
//                    utils::Line(utils::Point(0.5, 0.4), utils::Point(0.5, 0.6))
//            ),
//            feeds
//    );

//    init_slave(
//            [&config]() -> Config {
//                return config;
//            },
//            [&config](OpenCVConfig oldConfig) {
//                config.cvConfig = oldConfig;
//            });
//

    init_master();
    start();

    std::cout << "Buscount server does pass though" << std::endl;

    return 0;
}