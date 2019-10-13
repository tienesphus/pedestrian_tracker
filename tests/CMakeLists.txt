find_package(OpenCV 4.0 REQUIRED)

add_executable(tests
        tests_main.cpp
        buscount/testing_utils.cpp
        buscount/test_cv_utils.cpp
        buscount/test_detection.cpp
        buscount/test_detector_openvino.cpp
        buscount/test_tick_counter.cpp
        buscount/test_libbuscount.cpp
        buscount/test_video_sync.cpp
        buscount/test_tracker_comp.cpp
        buscount/test_affinity_features.cpp
        buscount/test_world.cpp
        buscount/test_detector_opencv.cpp # must be last
        common/test_geom.cpp
)

target_link_libraries(tests
    Catch2
    buscount_static
    opencv_highgui
)

target_include_directories(tests PUBLIC
        $<TARGET_PROPERTY:buscount_static,SOURCE_DIR>
        $<TARGET_PROPERTY:common,SOURCE_DIR>
        $<TARGET_PROPERTY:server_static,SOURCE_DIR>
)

# TODO make each test module should its own library
# trouble is that catch2 doesn't seem to pick up on the tests when in a library

#add_subdirectory(common)
#add_subdirectory(server)
#add_subdirectory(buscount)

#[[
target_link_libraries(tests
    tests_common
    tests_server
    tests_buscount
)
]]
