project(bus_count_prog)

find_package(OpenCV 4.0 REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(bus_count bus_count.cpp detection.cpp)

target_include_directories(bus_count PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bus_count ${OpenCV_LIBS})