#ifndef BUS_COUNT_SERVER_CLIENT_HPP
#define BUS_COUNT_SERVER_CLIENT_HPP

#include <functional>
#include <json/json.h>

#include "../optional.hpp"
#include "../utils.hpp"


struct OpenCVConfig {
    utils::Line crossing;

    explicit OpenCVConfig(const utils::Line &crossing);
};

nonstd::optional<utils::Line> line_from_json(const Json::Value& json);

nonstd::optional<OpenCVConfig> cvconfig_from_json(const Json::Value& data);

Json::Value to_json(const OpenCVConfig& config);

#endif