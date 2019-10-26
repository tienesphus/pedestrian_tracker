// C++ includes
#include <gstreamermm.h>
#include <glibmm/main.h>
#include <spdlog/spdlog.h>

// Project includes
#include "detection/detector_opencv.hpp"
#include "detection/detector_openvino.hpp"
#include "rtsp_server.hpp"

int main(int argc, char *argv[])
{
    // Initialise gstreamermm
    Gst::init(argc, argv);

    //spdlog::default_logger()->set_level(spdlog::level::debug);

    /*
    DetectorOpenCV::NetConfig net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        1, //2/255.0,            // scale
        cv::Scalar(1,1,1),//cv::Scalar(127.5, 127.5, 127.5),     // mean
        SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
        //SOURCE_DIR "/models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
        //SOURCE_DIR "/models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
        cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
        cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
    };
    */

    DetectorOpenVino::NetConfig net_config {
            0.5f,               // thresh
            15,                 // clazz
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };

    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    //GstBusCount::GstBusCountFilter::detector_init(Detector::DETECTOR_OPENCV, &net_config, plugin);    
    GstBusCount::GstBusCountFilter::detector_init(Detector::DETECTOR_OPENVINO, &net_config, plugin);
    
    GstBusCount::GstBusCountFilter::tracker_init(plugin);

    // Create a main loop for the current thread
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();

    // Create an RTSP Server and attach it to the main-loop context
    RtspServer rtsp_server;
    rtsp_server.attach(loop->get_context());

    spdlog::info("Streams ready at rtsp://<IP>:8554/test and rtsp://<IP>:8554/live\n");

    // Start serving
    loop->run();

    return 0;
}

