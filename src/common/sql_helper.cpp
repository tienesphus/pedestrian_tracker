#include <spdlog/spdlog.h>
#include "sql_helper.hpp"

bool exec(sqlite3* db, const std::string& sql, std::function<int(int, char**, char**)> callback) {
    char* error;

    int result;

    do {
        result = sqlite3_exec(db, sql.c_str(),
                         [](void* function, int cols, char** values, char** headers) -> int {
                             auto* f_ptr = static_cast<std::function<int(int, char **, char **)> *>(function);
                             (*f_ptr)(cols, values, headers);
                             return 0;
                         }, &callback, &error);
    }
    while (result == SQLITE_BUSY);

    if (result != SQLITE_OK) {
        spdlog::error("SQL error: ({}), {}", result, error);
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool exec(sqlite3* db, const std::string& sql) {
    return exec(db, sql, [](auto...) -> auto {return 0;});
}
