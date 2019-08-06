#[[
add_library(tests_server STATIC
        dummy.cpp
)

target_link_libraries(tests_server server_static)

target_include_directories(tests_server PUBLIC
        $<TARGET_PROPERTY:common,SOURCE_DIR>
        ${bus_count_SOURCE_DIR}/include/catch2
)
]]