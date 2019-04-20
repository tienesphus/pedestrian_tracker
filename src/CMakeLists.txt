project(bus_count_prog)

find_package(PkgConfig REQUIRED)

find_package(OpenCV 4.0 REQUIRED)
pkg_check_modules(GSTRTSPSRV REQUIRED gstreamer-rtsp-server-1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(bus_count bus_count.cpp detection.cpp)
add_executable(buscountd buscountd.cpp)

target_include_directories(bus_count PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(buscountd PRIVATE ${GSTRTSPSRV_INCLUDE_DIRS})

target_link_libraries(bus_count ${OpenCV_LIBS})
target_link_libraries(buscountd ${GSTRTSPSRV_LIBRARIES})

