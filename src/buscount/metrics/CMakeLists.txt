add_library(metrics STATIC
        cached_detector.cpp
        cached_features.cpp
        timed_wrapper.cpp
        compare_gt.cpp
)
target_link_libraries(metrics PUBLIC buscount_static)


add_executable(metrics_pre metrics_preprocess.cpp)
target_link_libraries(metrics_pre PUBLIC metrics opencv_highgui)

add_executable(metrics_post metrics_postprocess.cpp)
target_link_libraries(metrics_post PUBLIC metrics opencv_highgui)