#ifndef BUS_COUNT_SERVER_CLIENT_HPP
#define BUS_COUNT_SERVER_CLIENT_HPP

#include <json/json.h>

#include <optional.hpp>
#include <config.hpp>

nonstd::optional<WorldConfig> config_from_json(const Json::Value &data);

Json::Value to_json(const WorldConfig& config);

#endif