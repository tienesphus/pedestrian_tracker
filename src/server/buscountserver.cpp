#include "server.hpp"
#include "server_client.hpp"

#include <thread>
#include <iostream>
#include <sqlite3.h>

int main()
{
    // TODO fix up server code
    // Note from Matt:
    //  I really screwed this code over (sorry). It was nice but then stuff broke and I needed to fix it quickly.
    //  Pretty much, it would seem that Drogon and OpenVino cannot be run in the same process because they
    //  require different arch build types to be run. Drogon will segfault randomly if not built with armv6 and
    //  OpenVino will segfault if not built with armv7. I tried rebuilding Drogon with armv7, but that didn't seem
    //  to work. Maybe something else can be done?
    //
    //  As such, I'm running buscountserver and buscountcli as separate processes with an sqlite server in the middle
    //  This actually isn't a terrible final solution; however, the way I've got the project source setup is really crap.
    //  (lots of duplicate/dead code, cross dependencies, unclear structure, much haphazard patch code, etc)

    using namespace server;

    sqlite3* db;
    std::string database_source = (std::string(SOURCE_DIR) + "/data/database.db");
    if (sqlite3_open(database_source.c_str(), &db) != SQLITE_OK) {
        throw std::logic_error("Cannot open database");
    }

    // fake slave state
    std::vector<Feed> feeds;
    feeds.emplace_back("live", "/live");
    feeds.emplace_back("test", "/test");
    feeds.emplace_back("dirty", "/dirty");

    init_slave(
            [feeds, &db]() -> Config {
                char* error;

                OpenCVConfig config(utils::Line(utils::Point(0, 0), utils::Point(0, 0)));

                if (sqlite3_exec(db, "SELECT Config FROM ConfigUpdate WHERE time = (SELECT max(Time) FROM ConfigUpdate)",
                        [](void* config, int, char** argv, char**) -> int {
                    Json::Reader reader;
                    Json::Value json;
                    reader.parse(argv[0], json);
                    nonstd::optional<OpenCVConfig> op_config = cvconfig_from_json(json);
                    *(OpenCVConfig*)config = *op_config;
                    return 0;
                }, &config, &error) != SQLITE_OK) {
                    std::cout << "SELECT config ERROR: " << error << std::endl;
                    sqlite3_free(error);
                }

                return Config(config, feeds);
            },
            [&db](OpenCVConfig new_config) {
                char* error;
                Json::StreamWriterBuilder builder;
                std::string output = Json::writeString(builder, to_json(new_config));
                if (sqlite3_exec(db, ("INSERT INTO ConfigUpdate(Config) VALUES ('" + output + "')").c_str(),
                            nullptr, nullptr, &error) != SQLITE_OK) {
                    std::cout << "SQL ERROR insert config update: " << error << std::endl;
                    sqlite3_free(error);
                }
            }
    );


    init_master(
            [&db]() -> int {
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
            },
            [&db](int delta) {
                char* error;

                if (sqlite3_exec(db, ("INSERT INTO CountEvents (name, DeltaIn) VALUES ('Manual', " + std::to_string(delta) + ")").c_str(),
                                 nullptr, nullptr, &error) != SQLITE_OK) {
                    std::cout << "INSERT count ERROR: " << error << std::endl;
                    sqlite3_free(error);
                }
            }
    );
    start();

    std::cout << "Buscount server finished (does this ever get run?)" << std::endl;

    return 0;
}
