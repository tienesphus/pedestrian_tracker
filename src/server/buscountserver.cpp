#include "server.hpp"

int main()
{
    using namespace server;

    // fake slave state
    std::vector<Feed> feeds;
    feeds.emplace_back("live", "/live");
    feeds.emplace_back("test", "/test");
    feeds.emplace_back("dirty", "/dirty");
    Config config(
            OpenCVConfig(
                    Line(Point(0.4, 0), Point(0.4, 1)),
                    Line(Point(0.6, 0), Point(0.6, 1))
            ),
            feeds
    );

    init_master();
    init_slave(
            [&config]() -> Config {
                return config;
            },
            [&config](OpenCVConfig oldConfig) {
                config.cvConfig = oldConfig;
            });
    start();
}