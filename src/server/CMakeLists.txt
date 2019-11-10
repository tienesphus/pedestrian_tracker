find_package(Drogon REQUIRED)

add_library(server_static
    server.cpp
    json_convert.cpp
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

add_executable(buscountcloud
    buscountcloud.cpp
    cloud_updater.cpp
)
target_link_libraries(buscountcloud common curl pthread)
target_include_directories(buscountcloud PUBLIC
    $<TARGET_PROPERTY:common,SOURCE_DIR>
)
