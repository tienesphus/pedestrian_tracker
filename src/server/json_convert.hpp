#ifndef BUS_COUNT_JSON_CONVERT_HPP
#define BUS_COUNT_JSON_CONVERT_HPP

#include <json/json.h>

#include <optional.hpp>
#include <config.hpp>

Json::Value line_to_json(const geom::Line& l);

nonstd::optional<geom::Line> line_from_json(const Json::Value& json);

#endif