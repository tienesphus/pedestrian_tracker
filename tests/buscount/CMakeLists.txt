#[[
find_package(OpenCV 4.0 REQUIRED)

add_library(tests_buscount STATIC
        testing_utils.cpp
        test_cv_utils.cpp
        test_detection.cpp
        test_detector_openvino.cpp
        test_tick_counter.cpp
        test_libbuscount.cpp
        test_video_sync.cpp
        test_tracker_comp.cpp
        test_affinity_features.cpp
        test_world.cpp
        test_detector_opencv.cpp # must be last
)

target_link_libraries(tests_buscount buscount_static opencv_videoio)

target_include_directories(tests_buscount PUBLIC
    $<TARGET_PROPERTY:buscount_static,SOURCE_DIR>
    $<TARGET_PROPERTY:common,SOURCE_DIR>
    ${bus_count_SOURCE_DIR}/include/catch2
)
]]