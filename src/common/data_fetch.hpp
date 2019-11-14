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
     * Enters an event into the database at the current time
     * @param event the event to log
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
     * Gets the world configuration stored
     */
    WorldConfig get_config() const;

    void set_name(const std::string& name);

    std::string get_name();

    void set_busid(const std::string& busid);

    std::string get_busid();

    void set_remote_url(const std::string& remoteurl);

    std::string get_remote_url();

    /**
     * Resets the local count
     * @param count the new value
     */
    void set_count(int count);

private:
    sqlite3* db;
    std::string last_count_update;
};

#endif //BUS_COUNT_DATA_APP_HPP
