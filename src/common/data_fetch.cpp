#include "data_fetch.hpp"
#include "optional.hpp"
#include <spdlog/spdlog.h>

#include <sqlite3.h>

#include <string>
#include <json/json.h>
#include <sstream>
#include <iostream>


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
        spdlog::debug("Closing SQL");
        sqlite3_close(db);
    }
}

int DataFetch::count() const
{
    sqlite3_stmt* stmt;

    const char* sql = "SELECT CurrentCount FROM ConfigUpdate";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot fetch counts: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot fetch count");
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        throw std::logic_error("Cannot fetch count");
    }

    auto count = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return count;
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
        spdlog::error("One of the points is not a float");
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
    Json::StreamWriterBuilder builder;
    std::string output = Json::writeString(builder, _to_json(config));
    sqlite3_stmt* stmt;

    const char* sql = "UPDATE ConfigUpdate SET Config = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot update world config: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot update world config");
    }

    sqlite3_bind_text(stmt, 1, output.c_str(), -1, nullptr);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot execute update world config");
    }

    sqlite3_finalize(stmt);

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
    spdlog::debug("EVENT: {}", name(event));
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

    // Store the event
    std::string sql = "INSERT INTO CountEvents(Name, DeltaIn, DeltaOut) VALUES ('" + std::to_string(event) + "', " +
                      std::to_string(deltaIn) + ", " + std::to_string(deltaOut) + ")";
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        spdlog::error("SQL ERROR: {}", error);
        sqlite3_free(error);
    }

    // Add the count
    sql = "UPDATE ConfigUpdate SET CurrentCount = CurrentCount + " + std::to_string(deltaIn - deltaOut);
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        std::cout << "SQL ERROR: " << error << std::endl;
        sqlite3_free(error);
    }
}

void DataFetch::remove_events(const std::vector<int>& entries)
{
    sqlite3_stmt* stmt;

    std::string sql = "DELETE FROM CountEvents WHERE id IN (";
    for (size_t i = 0; i < entries.size(); i++) {
        sql += std::to_string(entries[i]);
        if (i < entries.size()-1)
            sql += ", ";
    }
    sql += ")";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot prepare delete count events: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot select features");
    }

    int result = sqlite3_step(stmt);
    while (result == SQLITE_BUSY) {
        result = sqlite3_step(stmt);
    }
    if (result != SQLITE_DONE) {
        throw std::logic_error("Cannot delete count events");
    }

    sqlite3_finalize(stmt);
}

/**
 * Fetches all the events logged in the database
 * @return a list of (id, timestamp, Event)
 */
std::vector<std::tuple<int, time_t, Event>> DataFetch::fetch_events() const
{
    sqlite3_stmt* stmt;

    const char* sql = "SELECT id, time, name FROM CountEvents" ;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot select count events: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot select count events");
    }

    std::vector<std::tuple<int, time_t, Event>> events;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        time_t time = sqlite3_column_int(stmt, 1);
        auto event = static_cast<Event>(sqlite3_column_int(stmt, 2));
        events.emplace_back(id, time, event);
    }

    sqlite3_finalize(stmt);

    return events;
}

WorldConfig DataFetch::get_config() const
{
    sqlite3_stmt* stmt;

    const char* sql = "SELECT Config FROM ConfigUpdate LIMIT 1";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot fetch config: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot fetch config");
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        throw std::logic_error("Cannot fetch config");
    }
    const char* text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    std::string text_str(text);

    Json::Reader reader;
    Json::Value json;
    bool success = reader.parse(text, json);
    if (!success)
        throw std::logic_error("Cannot read json config from database");

    nonstd::optional<WorldConfig> config = _config_from_json(json);
    if (!config)
        throw std::logic_error("Cannot convert json config to config");

    sqlite3_finalize(stmt);

    return *config;
}

void DataFetch::set_count(int count)
{
    char* error;

    if (sqlite3_exec(db, ("UPDATE ConfigUpdate SET CurrentCount = " + std::to_string(count)).c_str(),
                     nullptr, nullptr, &error) != SQLITE_OK) {
        spdlog::error("INSERT count ERROR: {}", error);
        sqlite3_free(error);
    }
}


void DataFetch::set_name(const std::string& name)
{
    sqlite3_stmt* stmt;

    const char* sql = "UPDATE ConfigUpdate SET DeviceName = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot update device name: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot update device name");
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot execute update device name");
    }

    sqlite3_finalize(stmt);
}

std::string DataFetch::get_name()
{
    sqlite3_stmt* stmt;

    const char* sql = "SELECT DeviceName FROM ConfigUpdate LIMIT 1";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot fetch device name: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot fetch device name");
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        throw std::logic_error("Cannot fetch device name");
    }

    const char* text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    std::string text_str(text);

    sqlite3_finalize(stmt);

    return text_str;
}

void DataFetch::set_busid(const std::string& busid)
{
    sqlite3_stmt* stmt;

    const char* sql = "UPDATE ConfigUpdate SET BusID = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot update bus id: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot update bus id");
    }

    sqlite3_bind_text(stmt, 1, busid.c_str(), -1, nullptr);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot execute update bus id");
    }

    sqlite3_finalize(stmt);
}

std::string DataFetch::get_busid()
{
    sqlite3_stmt* stmt;

    const char* sql = "SELECT BusID FROM ConfigUpdate LIMIT 1";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot fetch server bus id: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot fetch bus id");
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        throw std::logic_error("Cannot fetch bus id");
    }

    const char* text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    std::string text_str(text);

    sqlite3_finalize(stmt);

    return text_str;
}

void DataFetch::set_remote_url(const std::string& remoteurl)
{
    sqlite3_stmt* stmt;

    const char* sql = "UPDATE ConfigUpdate SET ServerURL = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot update server url: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot update server url");
    }

    sqlite3_bind_text(stmt, 1, remoteurl.c_str(), -1, nullptr);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot execute update server url");
    }

    sqlite3_finalize(stmt);
}

std::string DataFetch::get_remote_url()
{
    sqlite3_stmt* stmt;

    const char* sql = "SELECT ServerURL FROM ConfigUpdate LIMIT 1";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot fetch server url: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot fetch server url");
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        throw std::logic_error("Cannot fetch server url");
    }

    const char* text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    std::string text_str(text);

    sqlite3_finalize(stmt);

    return text_str;
}
