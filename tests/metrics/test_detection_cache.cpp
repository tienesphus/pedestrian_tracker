#include <metrics/cached_detector.hpp>
#include <detection/detector_openvino.hpp>
#include <opencv2/core/mat.hpp>
#include <metrics/read_simulator.hpp>
#include "catch.hpp"
#include "../buscount/testing_utils.hpp"

TEST_CASE( "Detections are cached and retrieved", "[detection_cache]" )
{
    // Setup the writer
    DetectorOpenVino::NetConfig net_config {
            0.0f,               // thresh
            15,                 // clazz
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };
    CacheDetections cache_write(SOURCE_DIR "/data/metrics.db", "unit_tests");
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");
    DetectorOpenVino detector(net_config, plugin);
    CachedDetectorWriter writer(cache_write, detector);

    // Write the detections
    cache_write.clear();
    cv::Mat input = load_test_image();
    Detections write_detections_0 = writer.process(input);
    Detections write_detections_1 = writer.process(input);


    // Setup the reader
    CacheDetections cache_read(SOURCE_DIR "/data/metrics.db", "unit_tests");
    CachedDetectorReader reader(cache_read, 0.0);
    double time = 0;
    ReadSimulator<> sym([]() -> cv::Mat {return cv::Mat(2, 2, CV_32F); }, 1, &time);

    // Read the detections back
    cv::Mat fake_input = *sym.next();
    Detections read_detections_0 = reader.process(fake_input);
    time += 1;
    Detections read_detections_1 = reader.process(fake_input);

    int read_size_0 = read_detections_0.get_detections().size();
    int read_size_1 = read_detections_1.get_detections().size();
    int write_size_0 = write_detections_0.get_detections().size();
    int write_size_1 = write_detections_1.get_detections().size();


    REQUIRE(read_size_0 == write_size_0);
    REQUIRE(read_size_1 == write_size_1);
    for (int i = 0; i < read_size_0; ++i) {
        Detection read = read_detections_0.get_detections()[i];
        Detection write = write_detections_0.get_detections()[i];
        REQUIRE(read == write);
    }
}
