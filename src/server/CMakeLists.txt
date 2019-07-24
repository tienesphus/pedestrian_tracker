add_library(server_static
        server.cpp
)

add_executable(buscountserver
        buscountserver.cpp
)

target_link_libraries(server_static PUBLIC buscount_static drogon trantor uuid pthread jsoncpp dl z)
target_link_libraries(buscountserver server_static)

target_compile_definitions(server_static PRIVATE SOURCE_DIR="${bus_count_SOURCE_DIR}")
