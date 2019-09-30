cmake_minimum_required(VERSION 3.7)

project(bus_count VERSION 0.2)

set(CMAKE_CXX_STANDARD 14)

add_compile_options(
    -Wall -Wextra -Wno-psabi
    $<$<CONFIG:Debug>:-Wfatal-errors>
    $<$<NOT:$<CONFIG:Debug>>:-O3>
)

add_compile_options(-D SOURCE_DIR="${bus_count_SOURCE_DIR}")

add_subdirectory(include/spdlog)
add_subdirectory(src)
add_subdirectory(tests)
