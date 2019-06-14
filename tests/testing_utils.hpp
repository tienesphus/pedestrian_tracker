#ifndef BUS_COUNT_TESTING_UTILS_HPP
#define BUS_COUNT_TESTING_UTILS_HPP

#include <opencv2/core/types.hpp>
#include <detector.hpp>

cv::Mat load_test_image();

bool require_detections_in_spec(const Detections &result);

#endif //BUS_COUNT_TESTING_UTILS_HPP
