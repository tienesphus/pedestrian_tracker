find_package(Drogon REQUIRED)

add_library(server_static
    server.cpp
    device_register.cpp
    json_convert.cpp
    cloud_updater.cpp
)

target_link_libraries(server_static PUBLIC
    ${DROGON_LIBRARIES}
    common
    trantor
    uuid
    pthread
    jsoncpp
    dl
    z
    ssl
    crypto
    atomic
    curl
)

# allow common header files to be used
target_include_directories(server_static PUBLIC
    $<TARGET_PROPERTY:common,SOURCE_DIR>
    ${DROGON_INCLUDE_DIRS}
)

add_executable(buscountserver
    buscountserver.cpp
)
target_link_libraries(buscountserver server_static)
