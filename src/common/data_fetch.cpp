#include "data_fetch.hpp"
#include "optional.hpp"

#include <sqlite3.h>

#include <string>
#include <iostream>
#include <json/json.h>

DataFetch::DataFetch(const std::string& file):
    db(nullptr), last_count_update("")
{
    if (sqlite3_open(file.c_str(), &db) != SQLITE_OK) {
        throw std::logic_error("Cannot open database");
    }
}

DataFetch::~DataFetch()
{
    if (db) {
        std::cout << "Closing SQL" << std::endl;
        sqlite3_close(db);
    }
}

int DataFetch::count() const
{
    char* error;

    int in_out[] = {0, 0};
    if (sqlite3_exec(db, "SELECT SUM(DeltaIn) as 'CountIn', SUM(DeltaOut) as 'CountOut' FROM CountEvents",
                     [](void* in_out, int, char** argv, char**) -> int {
                         std::string in = argv[0];
                         std::string out = argv[1];
                         int in_count = std::stoi(argv[0]);
                         int out_count = std::stoi(argv[1]);
                         ((int*)in_out)[0] = in_count;
                         ((int*)in_out)[1] = out_count;
                         return 0;
                     }, in_out, &error) != SQLITE_OK) {
        std::cout << "SELECT count ERROR: " << error << std::endl;
        sqlite3_free(error);
    }

    return in_out[0] - in_out[1];
}

// TODO Remove copy paste of JSON passers in data_fetch.cpp
// These to/from_json functions are copied from json_convert.cpp in the server project.
// We cannot directly use server code because it is not a library dependencies.
// Json is only used here to store data inside the database which should be changed.

Json::Value _to_json(const geom::Line& l)
{
    Json::Value json;
    json["x1"] = l.a.x;
    json["y1"] = l.a.y;
    json["x2"] = l.b.x;
    json["y2"] = l.b.y;
    return json;
}

nonstd::optional<geom::Line> _line_from_json(const Json::Value& json)
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


Json::Value _to_json(const WorldConfig& config)
{
    // TODO boundaries in json

    Json::Value json;
    json["crossing"] = _to_json(config.crossing);

    return json;
}

void DataFetch::update_config(const WorldConfig& config)
{
    char* error;
    Json::StreamWriterBuilder builder;
    std::string output = Json::writeString(builder, _to_json(config));
    if (sqlite3_exec(db, ("INSERT INTO ConfigUpdate(Config) VALUES ('" + output + "')").c_str(),
                     nullptr, nullptr, &error) != SQLITE_OK) {
        std::cout << "SQL ERROR insert config update: " << error << std::endl;
        sqlite3_free(error);
    }
}

nonstd::optional<WorldConfig> _config_from_json(const Json::Value &data)
{
    nonstd::optional<geom::Line> crossing = _line_from_json(data["crossing"]);

    if (crossing)
        return WorldConfig(*crossing, {});
    else
        return nonstd::nullopt;
}

void DataFetch::enter_event(Event event)
{
    std::cout << "EVENT: " << name(event) << std::endl;
    char *error = nullptr;
    int deltaIn = 0, deltaOut = 0;
    switch (event) {
        case Event::COUNT_IN:
            deltaIn = 1;
            break;
        case Event::COUNT_OUT:
            deltaOut = 1;
            break;
        case Event::BACK_IN:
            deltaOut = -1;
            break;
        case Event::BACK_OUT:
            deltaIn = -1;
            break;
        default:
            throw std::logic_error("Unknown event type");
    }
    std::string sql = "INSERT INTO CountEvents(Name, DeltaIn, DeltaOut) VALUES ('" + name(event) + "', " +
                      std::to_string(deltaIn) + ", " + std::to_string(deltaOut) + ")";
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        std::cout << "SQL ERROR: " << error << std::endl;
        sqlite3_free(error);
    }
}

WorldConfig DataFetch::get_latest_config() const
{
    char* error;

    WorldConfig config(geom::Line(geom::Point(0, 0), geom::Point(0, 0)), {});

    if (sqlite3_exec(db, "SELECT Config FROM ConfigUpdate WHERE time = (SELECT max(Time) FROM ConfigUpdate)",
                     [](void* config, int, char** argv, char**) -> int {
                         // TODO store config as proper data instead of JSON
                         Json::Reader reader;
                         Json::Value json;
                         reader.parse(argv[0], json);
                         nonstd::optional<WorldConfig> op_config = _config_from_json(json);
                         *(WorldConfig*)config = *op_config;
                         return 0;
                     }, &config, &error) != SQLITE_OK) {
        std::cout << "SELECT config ERROR: " << error << std::endl;
        sqlite3_free(error);
    }

    return config;
}

void DataFetch::check_config_update(const std::function<void(WorldConfig)>& updater)
{
    char *error = nullptr;

    // fetch the latest update timestamp from the database
    std::string latest_update = "?";
    if (sqlite3_exec(db, "SELECT MAX(time) FROM ConfigUpdate",
                     [](void* latest_update, int argc, char** argv, char**) -> int {
                         if (argc != 1)
                             throw std::logic_error("Required one result");
                         *(std::string*)latest_update = argv[0];
                         return 0;
                     }, &latest_update, &error) != SQLITE_OK) {
        std::cout << "SELECT ERROR: " << error << std::endl;
        sqlite3_free(error);
        return;
    }

    if (latest_update == "?") {
        throw std::logic_error("Latest update was not updated");
    }

    WorldConfig world_config = WorldConfig(geom::Line(geom::Point(0, 0), geom::Point(1, 1)), {});

    // fetch the config and update (if needed)
    if (last_count_update < latest_update) {
        if (sqlite3_exec(db, ("SELECT Config FROM ConfigUpdate WHERE Time='"+latest_update+"'").c_str(),
                         [](void* world_config, int argc, char** data, char**) -> int {
                             if (argc != 1)
                                 throw std::logic_error("SELECT config must return one column");
                             Json::Reader reader;
                             Json::Value json;
                             bool success = reader.parse(data[0], json);
                             if (!success)
                                 throw std::logic_error("Cannot read json config from database");
                             nonstd::optional<WorldConfig> config = _config_from_json(json);
                             if (!config)
                                 throw std::logic_error("Cannot convert json config to config");
                             *(WorldConfig*)world_config = *config;
                             return 0;
                         }, &world_config,
                         &error) != SQLITE_OK) {
            std::cout << "SELECT ERROR: " << error << std::endl;
            sqlite3_free(error);
        } else {
            last_count_update = std::move(latest_update);
            updater(world_config);
        }
    }
}

void DataFetch::add_count(int delta)
{
    char* error;

    if (sqlite3_exec(db, ("INSERT INTO CountEvents (name, DeltaIn) VALUES ('Manual', " + std::to_string(delta) + ")").c_str(),
                     nullptr, nullptr, &error) != SQLITE_OK) {
        std::cout << "INSERT count ERROR: " << error << std::endl;
        sqlite3_free(error);
    }
}