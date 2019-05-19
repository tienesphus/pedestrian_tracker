cmake_minimum_required(VERSION 3.7)
project(bus_count)

set(CMAKE_CXX_STANDARD 14)

add_compile_options(
    -Wall -Wextra
    $<$<CONFIG:Debug>:-Wfatal-errors>
    $<$<NOT:$<CONFIG:Debug>>:-O3>
)

add_subdirectory(src)
