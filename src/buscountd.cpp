#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>


int main (int argc, char *argv[])
{

    gst_init (&argc, &argv);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    // Create a server instance
    GstRTSPServer *server = gst_rtsp_server_new();

    // get the mount points for this server, every server has a default object
    // that be used to map uri mount points to media factories
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // make a media factory for a test stream. The default media factory can use
    // gst-launch syntax to create pipelines.
    // any launch line works as long as it contains elements named pay%d. Each
    // element with pay%d names will be a stream
    GstRTSPMediaFactory *test_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(
        test_factory,
        "( "
            "videotestsrc is-live=1 ! clockoverlay ! "
            "omxh264enc ! video/x-h264, profile=baseline ! "
            "rtph264pay name=pay0 pt=96 "
        ")"
    );
    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared (factory, TRUE);

    // attach the test factory to the /test url
    gst_rtsp_mount_points_add_factory (mounts, "/test", test_factory);
 

    // Live video
    GstRTSPMediaFactory *live_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch (
        live_factory,
        "( "
            "v4l2src ! clockoverlay ! "
            "omxh264enc ! video/x-h264, profile=baseline ! "
            "rtph264pay name=pay0 pt=96 "
        ")"
    );
    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared (factory, TRUE);
    // attach the test factory to the /live url
    gst_rtsp_mount_points_add_factory (mounts, "/live", factory);

    // don't need the ref to the mapper anymore
    g_object_unref (mounts);

    // attach the server to the default maincontext
    gst_rtsp_server_attach (server, NULL);

    // start serving
    g_print ("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run (loop);

    return 0;
}
