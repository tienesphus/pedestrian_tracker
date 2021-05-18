cmake_minimum_required(VERSION 3.7)

project(bus_count VERSION 0.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_compile_options(
    -Wall -Wextra -Wno-psabi
    $<$<CONFIG:Debug>:-Wfatal-errors>
    $<$<NOT:$<CONFIG:Debug>>:-O3>
)

set(SOURCE_DIR "${bus_count_SOURCE_DIR}")
add_compile_options(-D SOURCE_DIR="${SOURCE_DIR}")

add_subdirectory(extras)
add_subdirectory(src)
