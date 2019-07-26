#include "server_client.hpp"
#include <iostream>

Json::Value to_json(const utils::Line& l)
{
    Json::Value json;
    json["x1"] = l.a.x;
    json["y1"] = l.a.y;
    json["x2"] = l.b.x;
    json["y2"] = l.b.y;
    return json;
}

nonstd::optional<utils::Line> line_from_json(const Json::Value& json)
{
    Json::Value x1 = json["x1"];
    Json::Value y1 = json["y1"];
    Json::Value x2 = json["x2"];
    Json::Value y2 = json["y2"];

    if (x1.isDouble() && y1.isDouble() && x2.isDouble() && y2.isDouble())
    {
        utils::Line line(
                utils::Point(x1.asDouble(), y1.asDouble()),
                utils::Point(x2.asDouble(), y2.asDouble())
        );
        return line;
    } else {
        std::cout << "One of the points is not a float" << std::endl;
        return nonstd::nullopt;
    }
}

OpenCVConfig::OpenCVConfig(const utils::Line& crossing):
        crossing(crossing)
{}

nonstd::optional<OpenCVConfig> cvconfig_from_json(const Json::Value& json)
{
    nonstd::optional<utils::Line> crossing = line_from_json(json["crossing"]);

    if (crossing)
        return OpenCVConfig(*crossing);
    else
        return nonstd::nullopt;
}

Json::Value to_json(const OpenCVConfig& config)
{
    Json::Value json;
    json["crossing"] = to_json(config.crossing);
    return json;
}