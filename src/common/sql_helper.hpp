#ifndef BUS_COUNT_SQL_HELPER_HPP
#define BUS_COUNT_SQL_HELPER_HPP

#include <sqlite3.h>
#include <functional>

// TODO delete these functions and instead use sqlite3_prepare statements

bool exec(sqlite3* db, const std::string& sql, std::function<int(int, char**, char**)> callback);

bool exec(sqlite3* db, const std::string& sql);


#endif //BUS_COUNT_SQL_HELPER_HPP
