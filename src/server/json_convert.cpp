#include <spdlog/spdlog.h>
#include "json_convert.hpp"

nonstd::optional<geom::Line> line_from_json(const Json::Value& json);

Json::Value to_json(const geom::Line& l)
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
        spdlog::info("One of the points is not a float");
        return nonstd::nullopt;
    }
}

nonstd::optional<WorldConfig> config_from_json(const Json::Value &data)
{
    nonstd::optional<geom::Line> crossing = line_from_json(data["crossing"]);

    if (crossing)
        return WorldConfig(*crossing, {});
    else
        return nonstd::nullopt;
}

// TODO delete Feed object
struct Feed {
    std::string name, location;

    Feed(std::string name, std::string location):
        name(std::move(name)), location(std::move(location))
    {}
};

Json::Value to_json(const Feed& feed)
{
    Json::Value json;
    json["name"] = feed.name;
    json["location"] = feed.location;
    return json;
}

Json::Value to_json(const WorldConfig& config)
{
    // TODO boundaries in json

    Json::Value json;
    json["crossing"] = to_json(config.crossing);

    return json;
}

Json::Value to_json_with_feeds(const WorldConfig& config) {
    // TODO don't send feeds to android
    // requires changing protocol between android and pi

    // fake slave state
    std::vector<Feed> feeds;
    feeds.emplace_back("live", "/live");
    feeds.emplace_back("test", "/test");
    feeds.emplace_back("dirty", "/dirty");

    Json::Value json;
    json["config"] = to_json(config);

    Json::Value feeds_json;
    for (const Feed& f : feeds) {
        feeds_json.append(to_json(f));
    }
    json["feeds"] = feeds_json;

    return json;
}
