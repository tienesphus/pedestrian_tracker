add_library(server_static
        server.cpp
        server_client.cpp
        ../utils.cpp
)

add_executable(buscountserver
        buscountserver.cpp
)


# TODO remove opencv_core from server (currently only linked for utils.cpp)
target_link_libraries(server_static PUBLIC drogon trantor uuid pthread jsoncpp dl z ssl crypto sqlite3 opencv_core opencv_highgui)

target_link_libraries(buscountserver server_static)
