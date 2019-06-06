cmake_minimum_required(VERSION 3.7)

project(bus_count VERSION 0.2)

set(CMAKE_CXX_STANDARD 17)

add_compile_options(
    -Wall -Wextra
    $<$<CONFIG:Debug>:-Wfatal-errors>
    $<$<NOT:$<CONFIG:Debug>>:-O3>
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

add_subdirectory(src)