#include "json_convert.hpp"
#include <iostream>

Json::Value line_to_json(const geom::Line& l)
{
    Json::Value json;
    json["x1"] = l.a.x;
    json["y1"] = l.a.y;
    json["x2"] = l.b.x;
    json["y2"] = l.b.y;
    return json;
}

nonstd::optional<geom::Line> line_from_json(const Json::Value& json)
{
    Json::Value x1 = json["x1"];
    Json::Value y1 = json["y1"];
    Json::Value x2 = json["x2"];
    Json::Value y2 = json["y2"];

    if (x1.isDouble() && y1.isDouble() && x2.isDouble() && y2.isDouble())
    {
        geom::Line line(
                geom::Point(x1.asDouble(), y1.asDouble()),
                geom::Point(x2.asDouble(), y2.asDouble())
        );
        return line;
    } else {
        std::cout << "One of the points is not a float" << std::endl;
        return nonstd::nullopt;
    }
}