find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

find_package(jsoncpp REQUIRED)

add_library(common STATIC
        event.cpp
        geom.cpp
        data_fetch.cpp
        config.cpp
)

set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(common sqlite3 jsoncpp_lib spdlog::spdlog)

