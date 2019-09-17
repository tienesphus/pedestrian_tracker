find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

add_library(common SHARED
        event.cpp
        geom.cpp
        data_fetch.cpp
        config.cpp
        sql_helper.cpp
)

target_link_libraries(common sqlite3 jsoncpp)

