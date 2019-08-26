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
        common/test_geom.cpp
        metrics/test_read_simulator.cpp
        metrics/test_detection_cache.cpp
        metrics/test_feature_cache.cpp
        # all opencv detection must be after openvino detection
        buscount/test_detector_opencv.cpp
)

# TODO why do I need to include opencv_imgcodecs here but not in buscountcli?
# and why did I only start needing it when I restructured the project?
target_link_libraries(tests buscount_static server_static metrics opencv_videoio opencv_imgcodecs)

target_include_directories(tests PUBLIC
        $<TARGET_PROPERTY:buscount_static,SOURCE_DIR>
        $<TARGET_PROPERTY:common,SOURCE_DIR>
        $<TARGET_PROPERTY:server_static,SOURCE_DIR>
        ${bus_count_SOURCE_DIR}/include/catch2
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