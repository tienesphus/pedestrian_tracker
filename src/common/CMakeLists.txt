find_package(PkgConfig REQUIRED)
find_package(jsoncpp REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

add_library(common STATIC
    event.cpp
    geom.cpp
    data_fetch.cpp
    config.cpp
    sql_helper.cpp
    time_utils.cpp
    file_utils.cpp
)

set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(common
    sqlite3
    jsoncpp_lib
    spdlog::spdlog
)

