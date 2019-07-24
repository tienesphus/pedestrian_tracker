add_library(server_static
        server.cpp
)

add_executable(buscountserver
        buscountserver.cpp
)

target_link_libraries(server_static PUBLIC buscount_static drogon trantor uuid pthread jsoncpp dl z)
target_link_libraries(buscountserver server_static)

configure_file(index.html index.html COPYONLY)