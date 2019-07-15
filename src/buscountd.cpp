#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <tuple>

int main (int argc, char *argv[])
{
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  // create a server instance
  server = gst_rtsp_server_new ();

  // get the mount points for this server, every server has a default object
  // that be used to map uri mount points to media factories
  mounts = gst_rtsp_server_get_mount_points (server);

  // make a media factory for a test stream. The default media factory can use
  // gst-launch syntax to create pipelines. 
  // any launch line works as long as it contains elements named pay%d. Each
  // element with pay%d names will be a stream
  factory = gst_rtsp_media_factory_new ();
  
  // test pattern
  gst_rtsp_media_factory_set_launch (factory,
      "( videotestsrc is-live=1 ! clockoverlay ! omxh264enc ! video/x-h264, profile=baseline ! rtph264pay name=pay0 pt=96 )");
  gst_rtsp_media_factory_set_shared (factory, TRUE);
  // attach the test factory to the /test url
  gst_rtsp_mount_points_add_factory (mounts, "/test", factory);
 
  // live video
  gst_rtsp_media_factory_set_launch (factory,
      "( v4l2src ! clockoverlay ! omxh264enc ! video/x-h264, profile=baseline ! rtph264pay name=pay0 pt=96 )");
  gst_rtsp_media_factory_set_shared (factory, TRUE);
  // attach the test factory to the /test url 
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

class Base {
    static std::vector<std::tuple<std::string, std::function>> children;
protected:

    template<typename T>
    class register {
    public:
        register(std::string name) {
            Base::children.emplace_back(name, T::init);
        }
    };

    template<typename T>
    friend register::register(std::string);
public:
    Base create(string name, ......);
};

class Intermediary : Base {
    static unique_ptr<Plugin>;
    Intermediary() {
        if(!unique_ptr) {
            unique_ptr = unique_ptr(new Plugin(aolijowiea));
        }
    }
};

class Derived : Intermediary {
    Derived();
    static Base::register<Derived> _register;
public:
    static Derived init();
};

auto Derived::_register("hjgjgj");

Derived Derived::init() {
    Derived();
}