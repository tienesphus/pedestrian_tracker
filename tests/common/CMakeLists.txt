#[[
add_library(tests_common STATIC
        test_geom.cpp
)

target_link_libraries(tests_common common)

target_include_directories(tests_common PUBLIC
        $<TARGET_PROPERTY:common,SOURCE_DIR>
        ${bus_count_SOURCE_DIR}/include/catch2
)
]]