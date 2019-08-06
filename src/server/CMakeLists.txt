add_library(server_static
        server.cpp
        json_convert.cpp
)

target_link_libraries(server_static PUBLIC
        common
        drogon
        trantor
        uuid
        pthread
        jsoncpp
        dl
        z
        ssl
        crypto
)

# allow common header files to be used
target_include_directories(server_static PUBLIC
        $<TARGET_PROPERTY:common,SOURCE_DIR>
)

add_executable(buscountserver
        buscountserver.cpp
        )

target_link_libraries(buscountserver server_static)

