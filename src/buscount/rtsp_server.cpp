// Standard includes
#include <functional>

// C++ includes
#include <gstreamermm.h>
#include <spdlog/spdlog.h>

// Project includes
#include "rtsp_server.hpp"

Glib::RefPtr<Gst::Element> EncoderBin::omx_in_filter()
{
    Glib::RefPtr<Gst::Caps> caps = Gst::Caps::create_simple(
        "video/x-raw",
        "format", "I420"
    );

    Glib::RefPtr<Gst::Element> elem = Gst::ElementFactory::create_element("capsfilter");
    elem->property("caps", caps);

    return elem;
}

EncoderBin::EncoderBin(const Glib::ustring& name, EncoderType type)
    : Bin(name)
{
    Glib::RefPtr<Gst::Element> to_link;
    Glib::RefPtr<Gst::Caps> caps;

    switch (type)
    {
        case ENC_OMXH264:
            add(in = omx_in_filter());
            add(to_link = Gst::ElementFactory::create_element("omxh264enc"));

            in->link(to_link);

            caps = Gst::Caps::create_simple(
                "video/x-h264",
                "profile", "baseline"
            );

            break;

        case ENC_X264:
            add(in = to_link = Gst::ElementFactory::create_element("x264enc"));
            break;
    }

    add(out = Gst::ElementFactory::create_element("rtph264pay"));

    to_link->link(out, caps);

    sink_pad = add_ghost_pad(in, "sink", "sink");
    src_pad = add_ghost_pad(out, "src", "src");
}

EncoderBin::~EncoderBin() {}

Glib::RefPtr<EncoderBin> EncoderBin::create(const Glib::ustring& name, EncoderType type)
{
    return Glib::RefPtr<EncoderBin>(new EncoderBin(name, type));
}

GstRTSPMediaFactory *RtspServer::create_live_factory(Glib::RefPtr<Gst::Element> proxysink)
{
    // Create factory
    RtspFlexiMediaFactory *factory = rtsp_flexi_media_factory_new();
    GstRTSPMediaFactory *factory_base = GST_RTSP_MEDIA_FACTORY(factory);

    // Attach "create_elem" callback
    rtsp_flexi_media_factory_set_create_elem(factory,
        [](GstRTSPMediaFactory *base, const GstRTSPUrl *url) -> GstElement* {
            RtspFlexiMediaFactory *self = RTSP_FLEXI_MEDIA_FACTORY(base);


            Glib::RefPtr<Gst::Element> proxysink, proxy, overlay, convert, encoder_bin;
            Glib::RefPtr<Gst::Bin> bin = Gst::Bin::create();

            spdlog::info("URL: [{}]", url->abspath);

            proxysink = Glib::wrap(GST_ELEMENT(rtsp_flexi_media_factory_get_extra_data(self)));

            bin->add(proxy = Gst::ElementFactory::create_element("proxysrc"))
               ->add(overlay = Gst::ElementFactory::create_element("clockoverlay"))
               ->add(convert = Gst::ElementFactory::create_element("videoconvert"))
               ->add(encoder_bin = EncoderBin::create("pay0"));

            proxy->link(overlay);
            overlay->link(convert);
            convert->link(encoder_bin);

            proxy->property("proxysink", proxysink);

            return ((Glib::RefPtr<Gst::Element>)bin)->gobj_copy();
        }
    );

    // Attach proxysink as extra data so that callback can access it later.
    rtsp_flexi_media_factory_set_extra_data(factory, G_OBJECT(proxysink->gobj()));

    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared(GST_RTSP_MEDIA_FACTORY(factory), TRUE);

    // Attach system clock
    Glib::RefPtr<Gst::Clock> clock = Gst::SystemClock::obtain();
    gst_rtsp_media_factory_set_clock(factory_base, clock->gobj());

    gst_rtsp_media_factory_set_latency(factory_base, 50);    // rtpbin jitterbuffer latency
    gst_rtsp_media_factory_set_buffer_size(factory_base, 0); // udpsink buffer size

    return GST_RTSP_MEDIA_FACTORY(factory);
}

GstRTSPMediaFactory *RtspServer::create_test_factory()
{
    // Test video
    RtspFlexiMediaFactory *factory = rtsp_flexi_media_factory_new();

    // Attach "create_elem" callback
    rtsp_flexi_media_factory_set_create_elem(factory,
        [](GstRTSPMediaFactory * /*factory*/, const GstRTSPUrl *url) -> GstElement* {
            Glib::RefPtr<Gst::Element> testvid, overlay, convert, encoder_bin;
            Glib::RefPtr<Gst::Bin> bin = Gst::Bin::create();

            spdlog::info("URL: [{}]", url->abspath);

            bin->add(testvid = Gst::ElementFactory::create_element("videotestsrc"))
               ->add(overlay = Gst::ElementFactory::create_element("clockoverlay"))
               ->add(convert = Gst::ElementFactory::create_element("videoconvert"))
               ->add(encoder_bin = EncoderBin::create("pay0"));

            testvid->link(overlay);
            overlay->link(convert);
            convert->link(encoder_bin);

            return ((Glib::RefPtr<Gst::Element>)bin)->gobj_copy();
        }
    );

    // Set shared so that multiple devices can connect to the same stream
    gst_rtsp_media_factory_set_shared(GST_RTSP_MEDIA_FACTORY(factory), TRUE);

    return GST_RTSP_MEDIA_FACTORY(factory);
}

void RtspServer::create_mounts()
{
    // Get a reference to the mount points object for this server. Every
    // server has a default object that can be used to map URI mount points
    // to media factories.
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // Attach the file factory to the /file URI
    //gst_rtsp_mount_points_add_factory(mounts, "/file", create_file_factory());

    // Attach the live factory to the /live URI
    gst_rtsp_mount_points_add_factory(mounts, "/live", create_live_factory(bprox));

    // Attach the test factory to the /test URI
    gst_rtsp_mount_points_add_factory(mounts, "/test", create_test_factory());


    // Free the reference to the mount points object, not needed anymore
    g_object_unref(mounts);
}

void RtspServer::init_buscount_pipeline()
{

    Glib::RefPtr<Gst::Element> v4l2, cnvt, bcfilt, vtee, btee;

    // Register the gstreamer buscount plugin
    GstBusCount::plugin_init_static();

    // Create pipeline
    buscountpipe = Gst::Pipeline::create("buscount-pipeline");

    Glib::RefPtr<Gst::Caps> caps = Gst::Caps::create_simple(
        "video/x-raw",
        "framerate", Gst::Fraction(10, 1),
        "width",     800,
        "height",    600
    );

    // Create/add elements to pipeline
    buscountpipe->add(v4l2 = Gst::ElementFactory::create_element("v4l2src"))
                    ->add(bcfilt = Gst::ElementFactory::create_element("buscountfilter"))
                    ->add(cnvt = Gst::ElementFactory::create_element("videoconvert"))
                    //->add(vtee = Gst::ElementFactory::create_element("tee"))
                    //->add(btee = Gst::ElementFactory::create_element("tee"))
                    //->add(vprox = Gst::ElementFactory::create_element("proxysink"))
                    ->add(bprox = Gst::ElementFactory::create_element("proxysink"));

    // Link elements together
    v4l2->link(cnvt, caps);
    cnvt->link(bcfilt);
    bcfilt->link(bprox);

    // Set clock
    Glib::RefPtr<Gst::Clock> clock = Gst::SystemClock::obtain();
    buscountpipe->use_clock(clock);
}

void RtspServer::init_rtsp_server()
{
    server = gst_rtsp_server_new();
    create_mounts();
}


// Public stuff
RtspServer::RtspServer()
{
    init_buscount_pipeline();
    init_rtsp_server();

    buscountpipe->set_state(Gst::STATE_PLAYING);
}

RtspServer::~RtspServer() {}

void RtspServer::attach()
{
    attach(Glib::RefPtr<Glib::MainContext>());
}

void RtspServer::attach(Glib::RefPtr<Glib::MainContext> context)
{
    // Attach the server to the default MainLoop context
    gst_rtsp_server_attach(server, (context ? context->gobj() : NULL));
}
