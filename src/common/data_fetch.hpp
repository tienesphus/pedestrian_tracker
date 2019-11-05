#ifndef BUS_COUNT_DATA_FETCH_HPP
#define BUS_COUNT_DATA_FETCH_HPP

#include <sqlite3.h>

#include <functional>
#include <string>

#include "event.hpp"
#include "config.hpp"

/**
 * Gets configuration data/events to/from the database. This class is designed for one process (e.g. buscoundd) to be
 * reading data and another (e.g. buscountserver) to be writing the data
 */
class DataFetch {

public:
    /**
     * Contructs a reader from the given database path. The given file must point to an existing sqlite dabase
     * @param file the path of the database
     */
    explicit DataFetch(const std::string& file);

    ~DataFetch();

    /**
     * Logs an event into the database
     * @param event the event to log
     */
    void enter_event(Event event);

    /**
     * Gets the current local count. Note that if the server is running, it will reset this count every few seconds.
     * @return the current count in the database
     */
    int count() const;

    /**
     * Updates the configuration stored in the database
     * @param config the config to store
     */
    void update_config(const WorldConfig& config);

    /**
     * Gets the latest configuration stored
     * @return the latest config
     */
    WorldConfig get_latest_config() const;

    /**
     * Checks for any updates.
     */
    void check_config_update(const std::function<void(WorldConfig)>& updater);

    /**
     * Adds an offset to the count
     * @param delta the count offset
     */
    void add_count(int delta);

private:
    sqlite3* db;
    std::string last_count_update;
};

#endif //BUS_COUNT_DATA_APP_HPP
