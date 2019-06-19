cmake_minimum_required(VERSION 3.7)

project(bus_count VERSION 0.2)

set(CMAKE_CXX_STANDARD 14)

execute_process(COMMAND uname -m OUTPUT_VARIABLE SYS_ARCH)
string(STRIP "${SYS_ARCH}" SYS_ARCH)
add_compile_options(
    -Wall -Wextra -Wno-psabi
    $<$<STREQUAL:"${SYS_ARCH}","armv7l">:-march=armv7-a>
    $<$<CONFIG:Debug>:-Wfatal-errors>
    $<$<NOT:$<CONFIG:Debug>>:-O3>
)

add_subdirectory(src)
add_subdirectory(tests)
