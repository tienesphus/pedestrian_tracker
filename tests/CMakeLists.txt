add_executable(tests
        testing_utils.cpp
        tests_main.cpp
        test_detection.cpp
        test_detector_opencv.cpp
        tick_counter.cpp)

target_compile_definitions(tests PRIVATE SOURCE_DIR="${bus_count_SOURCE_DIR}")

target_link_libraries(tests buscount_static)

target_include_directories(tests PUBLIC
        ${bus_count_SOURCE_DIR}/src
            ${bus_count_SOURCE_DIR}/include/catch2)