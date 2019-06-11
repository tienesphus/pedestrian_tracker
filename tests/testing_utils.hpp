#ifndef BUS_COUNT_TESTING_UTILS_HPP
#define BUS_COUNT_TESTING_UTILS_HPP

#include <opencv2/core/types.hpp>
#include <detection.hpp>
#include <detector_opencv.hpp>

cv::Mat load_test_image();

NetConfigOpenCV load_test_config(int preferred_backend, int preferred_target);

bool require_detections_in_spec(const Detections &result);

#endif //BUS_COUNT_TESTING_UTILS_HPP
