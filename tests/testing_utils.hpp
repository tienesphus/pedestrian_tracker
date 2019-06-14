#ifndef BUS_COUNT_TESTING_UTILS_HPP
#define BUS_COUNT_TESTING_UTILS_HPP

#include <opencv2/core/types.hpp>
#include <detector.hpp>

cv::Mat load_test_image();

NetConfig load_test_config(int preferred_backend, int preferred_target);

NetConfig load_test_config_ie();

bool require_detections_in_spec(const Detections &result);

#endif //BUS_COUNT_TESTING_UTILS_HPP
