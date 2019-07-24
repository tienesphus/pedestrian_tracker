#ifndef BUS_COUNT_TESTING_UTILS_HPP
#define BUS_COUNT_TESTING_UTILS_HPP

#include <opencv2/core/types.hpp>
#include <detector.hpp>

/**
 * Loads a test image of the skier
 */
cv::Mat load_test_image();

/**
 * Check that the detection result of the test image are in spec
 */
void require_detections_in_spec(const Detections &result);

#endif //BUS_COUNT_TESTING_UTILS_HPP
