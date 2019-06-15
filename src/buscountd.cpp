// C++ includes
#include <gstreamermm.h>
#include <glibmm/main.h>

// C includes
#include <gst/rtsp-server/rtsp-server.h>

// Project includes
#include "gstbuscountfilter.hpp"

GstRTSPMediaFactory *create_test_factory()
{
    // Make a media factory for a test stream. The default media factory can use
    // gst-launch syntax to create pipelines. Any launch line works as long as the
    // endpoints are named pay%d where %d is any number from 0 to INT_MAX. Each
    // element named pay%d will be another stream for the given rtsp endpoint.

    // Test video
    GstRTSPMediaFactory *test_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(
        test_factory,
        "( "
            "videotestsrc is-live=1 ! buscountfilter ! clockoverlay ! videoconvert ! "
            "video/x-raw, format=I420 ! omxh264enc ! "
            "video/x-h264, profile=baseline ! rtph264pay name=pay0 pt=96 "
        ")"
    );

    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared(test_factory, TRUE);

    return test_factory;
}

GstRTSPMediaFactory *create_live_factory()
{
    // Live video
    GstRTSPMediaFactory *live_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(
        live_factory,
        "( "
            "v4l2src ! clockoverlay ! "
            "omxh264enc ! video/x-h264, profile=baseline ! "
            "rtph264pay name=pay0 pt=96 "
        ")"
    );

    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared(live_factory, TRUE);

    return live_factory;
}

int main (int argc, char *argv[])
{
    // Initialise gstreamermm
    Gst::init(argc, argv);

    // Register the gstreamer buscount plugin
    GstBusCount::plugin_init_static();

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

    // Attach the test factory to the /live URI
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
