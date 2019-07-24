#ifndef BUS_COUNT_SERVER_HPP
#define BUS_COUNT_SERVER_HPP

#include <functional>

#include <json/json.h>

#include "../optional.hpp"

namespace server {

    struct Feed {
        std::string name;
        std::string location;

        Feed(std::string name, std::string location);

        Json::Value to_json() const;

        static nonstd::optional<Feed> from_json(const Json::Value &data);
    };

    struct Point {
        float x, y;

        Point(float x, float y);
    };

    struct Line {
        Point a, b;

        Line(const Point &a, const Point &b);

        Json::Value to_json() const;

        static nonstd::optional<Line> from_json(const Json::Value &json);
    };

    struct OpenCVConfig {
        Line crossing;

        explicit OpenCVConfig(const Line &crossing);
    };

    struct Config {
        OpenCVConfig cvConfig;
        std::vector<Feed> feeds;

        Config(const OpenCVConfig &cvConfig, std::vector<Feed> feeds);
    };


    /**
     * Initialises the server with the master config
     * It is okay to initialise the server as both master and slave
     */
    void init_master();

    /**
     * Initialises the server as a slave
     * It is okay to initialise the server as both master and slave
     */
    void init_slave(const std::function<Config()> &getConfig, const std::function<void(OpenCVConfig)> &setConfig);

    /**
     * Starts the server (blocking)
     * Should only be run after init_master or init_slave has been called
     */
    void start();
}

#endif //BUS_COUNT_SERVER_HPP
