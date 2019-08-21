#ifndef RTSP_SERVER_HPP
#define RTSP_SERVER_HPP

// C++ includes
#include <gstreamermm.h>
#include <glibmm/main.h>

// C includes
#include <gst/rtsp-server/rtsp-server.h>

// Project includes
#include "gstbuscountfilter.hpp"
#include "rtsp_flexi_media_factory.h"

class EncoderBin : public Gst::Bin
{
public:
    enum EncoderType
    {
        ENC_OMXH264,
        ENC_X264
    };

private:
    Glib::RefPtr<Gst::Element> in;
    Glib::RefPtr<Gst::Element> out;

    Glib::RefPtr<Gst::GhostPad> sink_pad;
    Glib::RefPtr<Gst::GhostPad> src_pad;

    Glib::RefPtr<Gst::Element> omx_in_filter();

protected:
    explicit EncoderBin(const Glib::ustring& name, EncoderType type);

public:
    virtual ~EncoderBin();
    static Glib::RefPtr<EncoderBin> create(const Glib::ustring& name, EncoderType type = DEFAULT_ENC_TYPE);
};

class RtspServer
{
    GstRTSPServer *server;
    Glib::RefPtr<Gst::Pipeline> buscountpipe;
    Glib::RefPtr<Gst::Element> vprox, bprox;

    GstRTSPMediaFactory *create_test_factory();
    GstRTSPMediaFactory *create_live_factory(Glib::RefPtr<Gst::Element> proxysink);

    void create_mounts();
    void init_buscount_pipeline();
    void init_rtsp_server();

public:
    RtspServer();
    ~RtspServer();

    void attach();
    void attach(Glib::RefPtr<Glib::MainContext> context);
};

#endif
