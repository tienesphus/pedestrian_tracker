#include "catch.hpp"

#include "libbuscount.hpp"
#include "detector_opencv.hpp"
#include "testing_utils.hpp"

TEST_CASE( "Bus Counter runs in serial", "[libbuscount]" ) {

    NetConfigOpenCV netConfig = load_test_config(cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);
    OpenCVDetector detector(netConfig);

    WorldConfig config = WorldConfig::from_file(std::string(SOURCE_DIR)+"/config.csv");
    Tracker tracker(config);

    BusCounter counter(detector, tracker, config,
            []() -> std::optional<cv::Mat> {
                return load_test_image();
            },
            [](const cv::Mat&) {
            },
            []() -> bool {
                return true;
            }
    );

    counter.run(BusCounter::RUN_SERIAL, false);
}
