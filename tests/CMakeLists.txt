find_package(OpenCV 4.0 REQUIRED)

add_executable(tests
    testing_utils.cpp
    test_utils.cpp
    tests_main.cpp
    test_detection.cpp
    test_detector_openvino.cpp
    test_tick_counter.cpp
    test_libbuscount.cpp
    test_video_sync.cpp
    test_tracker_comp.cpp
    test_affinity_features.cpp
    test_detector_opencv.cpp # must be last
        test_world.cpp)

target_link_libraries(tests buscount_static opencv_videoio)

target_include_directories(tests PUBLIC
    $<TARGET_PROPERTY:buscount_static,SOURCE_DIR>
    ${bus_count_SOURCE_DIR}/include/catch2
)