#ifndef BUS_COUNT_DATA_FETCH_HPP
#define BUS_COUNT_DATA_FETCH_HPP

#include <sqlite3.h>

#include <functional>
#include <string>

#include "event.hpp"
#include "config.hpp"

class DataFetch {

public:
    explicit DataFetch(const std::string& file);

    ~DataFetch();

    void enter_event(Event event);

    int count() const;

    void update_config(const WorldConfig& config);

    WorldConfig get_latest_config() const;

    void check_config_update(const std::function<void(WorldConfig)>& updater);

    void add_count(int delta);

private:
    sqlite3* db;
    std::string last_count_update;
};

#endif //BUS_COUNT_DATA_APP_HPP
