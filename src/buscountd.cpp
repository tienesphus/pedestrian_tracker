// C++ includes
#include <gstreamermm.h>
#include <glibmm/main.h>

// C includes
#include <gst/rtsp-server/rtsp-server.h>

// Project includes
#include "rtsp_flexi_media_factory.h"
#include "detector_opencv.hpp"
#include "detector_openvino.hpp"
#include "gstbuscountfilter.hpp"

#define TEST_VIDEO "../../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4"

Glib::RefPtr<Gst::Pipeline> pipeline;

GstElement* create_elem_from_bin(GstRTSPMediaFactory *factory, const GstRTSPUrl *url)
{
    Glib::RefPtr<Gst::Bin> bin = Gst::Bin::create();

    bin->add(pipeline);

    return ((Glib::RefPtr<Gst::Element>)bin)->gobj();
}

GstRTSPMediaFactory *create_test_factory()
{
    // Make a media factory for a test stream. The default media factory can use
    // gst-launch syntax to create pipelines. Any launch line works as long as the
    // endpoints are named pay%d where %d is any number from 0 to INT_MAX. Each
    // element named pay%d will be another stream for the given rtsp endpoint.

    // Test video
    RtspFlexiMediaFactory *test_factory = rtsp_flexi_media_factory_new();
    GstRTSPMediaFactory *result = GST_RTSP_MEDIA_FACTORY(test_factory);
    //GstRTSPMediaFactory *result = gst_rtsp_media_factory_new();

    rtsp_flexi_media_factory_set_create_elem(test_factory, create_elem_from_bin);

    gst_rtsp_media_factory_set_launch(
        result,
        "( "
            "videotestsrc ! clockoverlay ! "
            "videoconvert ! video/x-raw, format=I420 ! omxh264enc ! "
            "video/x-h264, profile=baseline ! rtph264pay name=pay0 pt=96 "
        ")"
    );

    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared(result, TRUE);

    return result;
}

GstRTSPMediaFactory *create_file_factory()
{
    // Make a media factory for a test stream. The default media factory can use
    // gst-launch syntax to create pipelines. Any launch line works as long as the
    // endpoints are named pay%d where %d is any number from 0 to INT_MAX. Each
    // element named pay%d will be another stream for the given rtsp endpoint.

    // Test video
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(
        factory,
        "( "
            "filesrc location=" TEST_VIDEO " ! omxh264dec ! video/x-raw,framerate=8/1 ! "
            "buscountfilter ! clockoverlay ! videoconvert ! video/x-raw, format=I420 ! "
            "omxh264enc ! video/x-h264, profile=baseline ! rtph264pay name=pay0 pt=96 "
        ")"
    );

    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared(factory, TRUE);

    return factory;
}

GstRTSPMediaFactory *create_live_factory()
{
    // Live video
    GstRTSPMediaFactory *live_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(
        live_factory,
        "( "
            "v4l2src ! video/x-raw,framerate=8/1,width=800,height=600 ! "
            "buscountfilter ! clockoverlay ! videoconvert ! video/x-raw,format=I420 ! "
            "omxh264enc ! video/x-h264,profile=baseline ! "
            "rtph264pay name=pay0 pt=96 "
        ")"
    );

    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared(live_factory, TRUE);

    return live_factory;
}

int main(int argc, char *argv[])
{
    // Initialise gstreamermm
    Gst::init(argc, argv);

    DetectorOpenCV::NetConfig net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        1, //2/255.0,            // scale
        cv::Scalar(1,1,1),//cv::Scalar(127.5, 127.5, 127.5),     // mean
        "../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        "../models/MobileNetSSD_IE/MobileNetSSD.bin", // model
        //"../models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
        //"../models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
        cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
        cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
    };

    /*
    DetectorOpenVino::NetConfig net_config {
            0.5f,               // thresh
            15,                 // clazz
            "../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            "../models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };*/

    GstBusCount::GstBusCountFilter::detector_init(Detector::DETECTOR_OPENCV, &net_config);
    //GstBusCount::GstBusCountFilter::detector_init(Detector::DETECTOR_OPENVINO, &net_config);

    // Register the gstreamer buscount plugin
    GstBusCount::plugin_init_static();

    // Now create a base pipeline that others can read from, so that video only needs to be
    // processed once.
    Glib::RefPtr<Gst::Element> v4l2, bcfilt, vtee, btee, capsfilt;

    pipeline = Gst::Pipeline::create("base-pipeline");

    v4l2     = Gst::ElementFactory::create_element("v4l2src");
    capsfilt = Gst::ElementFactory::create_element("capsfilter");
    bcfilt   = Gst::ElementFactory::create_element("buscountfilter");
    vtee     = Gst::ElementFactory::create_element("tee");
    btee     = Gst::ElementFactory::create_element("tee");

    Glib::RefPtr<Gst::Caps> caps = Gst::Caps::create_simple(
        "video/x-raw",
        "framerate", Gst::Fraction(8, 1),
        "width",     800,
        "height",    600
    );

    capsfilt->property("caps", caps);

    pipeline->add(v4l2)->add(capsfilt)->add(vtee)->add(bcfilt)->add(btee);

    // Create a main loop for the current thread
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();

    // Create a new server instance
    GstRTSPServer *server = gst_rtsp_server_new();

    // Get a reference to the mount points object for this server. Every
    // server has a default object that can be used to map URI mount points
    // to media factories.
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // Attach the test factory to the /test URI
    gst_rtsp_mount_points_add_factory(mounts, "/test", create_test_factory());

    // Attach the file factory to the /file URI
    gst_rtsp_mount_points_add_factory(mounts, "/file", create_file_factory());

    // Attach the live factory to the /live URI
    gst_rtsp_mount_points_add_factory(mounts, "/live", create_live_factory());

    // Free the reference to the mount points object, not needed anymore
    g_object_unref(mounts);

    // Attach the server to the default MainLoop context
    gst_rtsp_server_attach(server, NULL);

    g_print("Streams ready at rtsp://<IP>:8554/test and rtsp://<IP>:8554/live\n");

    // Start serving
    loop->run();

    return 0;
}
