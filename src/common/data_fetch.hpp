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

    /**
     * Enters an event into the database at the current time
     */
    void enter_event(Event event);

    /**
     * Deletes the given ids from the database.
     * @param entries the ids to remove
     */
    void remove_events(const std::vector<int>& entries);

    /**
     * Fetches all the events logged in the database
     * @return a list of (id, timestamp, Event)
     */
    std::vector<std::tuple<int, time_t, Event>> fetch_events() const;

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
