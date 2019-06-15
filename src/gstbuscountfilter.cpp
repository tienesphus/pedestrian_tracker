
#include "gstbuscountfilter.hpp"

#include <gstreamermm/wrap_init.h>
#include <glib.h>

namespace GstBusCount {

Gst::ValueList GstBusCountFilter::formats;

static void value_list_string_init(Gst::ValueList &list, ...)
{
    va_list args;
    va_start(args, list);

    char *str;
    while (str = va_arg(args, char*))
    {
        Glib::Value<Glib::ustring> val;
        Glib::ustring ustr(str);

        val.init(val.value_type());
        val.set(ustr);

        list.append(val);
    }

    va_end(args);
}

void GstBusCountFilter::class_init(Gst::ElementClass<GstBusCountFilter> *klass)
{
    formats.init(formats.get_type());
    value_list_string_init(formats,
        "BGRx", "RGBx", "xRGB", "xBGR", "RGBA",
        "BGRA", "ARGB", "RGB", "BGR", NULL
    );

    klass->set_metadata(
            "Bus Counter Filter",
            "Processor/Video/",
            "Processes incoming video from a camera and extract person entry count",
            "Alastair Knowles <2062674@student.swin.edu.au>");

    // Looks ugly, but we can't use template/variadic constructor because it implicitly copies
    // everything, and there's a bug when copying Glib::Value objects (they aren't fully
    // initialised).
    auto caps_struct = Gst::Structure("video/x-raw");

    caps_struct.set_field("format", static_cast<Glib::ValueBase&>(formats));
    caps_struct.set_field("width", Gst::Range<int>(1, G_MAXINT32));
    caps_struct.set_field("height", Gst::Range<int>(1, G_MAXINT32));
    caps_struct.set_field("framerate", Gst::Range<Gst::Fraction>(
            Gst::Fraction(0, 1),
            Gst::Fraction(G_MAXINT32, 1)
    ));

    klass->add_pad_template(
        Gst::PadTemplate::create(
            "video_sink",
            Gst::PAD_SINK,
            Gst::PAD_ALWAYS,
            Gst::Caps::create(caps_struct)
        )
    );

    klass->add_pad_template(
        Gst::PadTemplate::create(
            "video_src",
            Gst::PAD_SRC,
            Gst::PAD_ALWAYS,
            Gst::Caps::create(caps_struct)
        )
    );
}

bool GstBusCountFilter::register_buscount_filter(Glib::RefPtr<Gst::Plugin> plugin)
{
    return Gst::ElementFactory::register_element(
        plugin,
        "buscountfilter",
        10,
        Gst::register_mm_type<GstBusCountFilter>("buscountfilter")
    );
}

bool GstBusCountFilter::register_buscount_filter(GstPlugin *plugin)
{
    return gst_element_register(
        plugin,
        "buscountfilter",
        10,
        Gst::register_mm_type<GstBusCountFilter>("buscountfilter")
    );
}



GstBusCountFilter::GstBusCountFilter(GstElement *gobj):
        Glib::ObjectBase(typeid(GstBusCountFilter)),
        Gst::Element(gobj),
        test_property(*this, "test_property", "default_val")
{
    add_pad(video_in = Gst::Pad::create(get_pad_template("video_sink"), "video_sink"));
    add_pad(video_out = Gst::Pad::create(get_pad_template("video_src"), "video_src"));

    GST_PAD_SET_PROXY_CAPS(video_in->gobj());
    GST_PAD_SET_PROXY_CAPS(video_out->gobj());

    video_in->set_chain_function(sigc::mem_fun(*this, &GstBusCountFilter::chain));
    video_in->set_event_function(sigc::mem_fun(*this, &GstBusCountFilter::sink_event));
}

// Events
bool GstBusCountFilter::sink_event(const Glib::RefPtr<Gst::Pad> &pad, Glib::RefPtr<Gst::Event> &event)
{
    bool result;

    switch (event->get_event_type())
    {
        case GST_EVENT_CAPS:
        {
            // Sink pad caps have been negotiated. Copy caps to source pad.
            auto caps_event = Glib::RefPtr<Gst::EventCaps>::cast_static(event);
            auto out_caps = caps_event->parse_caps()->copy();

            g_print("### Set caps to %s", out_caps->to_string().c_str());

            result = gst_pad_set_caps(video_out->gobj(), out_caps->gobj());
            result &= pad->event_default(Glib::RefPtr<Gst::Event>(event));

            break;
        }

        default:
            result = pad->event_default(Glib::RefPtr<Gst::Event>(event));
            break;

    }

    return result;
}

// Flow control
Gst::FlowReturn GstBusCountFilter::chain(const Glib::RefPtr<Gst::Pad> &pad, Glib::RefPtr<Gst::Buffer> &buf)
{
    buf = buf->create_writable();

    Gst::MapInfo mapinfo;
    buf->map(mapinfo, Gst::MAP_WRITE);

    std::sort(mapinfo.get_data(), mapinfo.get_data() + mapinfo.get_size());
    buf->unmap(mapinfo);

    return video_out->push(std::move(buf));
}

static bool plugin_init(const Glib::RefPtr<Gst::Plugin> &plugin)
{
    return GstBusCountFilter::register_buscount_filter(plugin->gobj());
}

static gboolean plugin_init(GstPlugin* plugin)
{
    // Ensure Glib is a go
    Glib::init();
    return GstBusCountFilter::register_buscount_filter(plugin);
}

} // end namespace GstBusCount

extern "C"
const GstPluginDesc gst_plugin_desc = {
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "gstbuscount", "Project specific plugin for defining new gstreamer elements",
    GstBusCount::plugin_init,
    "1.0", "Proprietary",
    "gstbuscountfilter.cpp", "buscountd",
    "https://swinburne.com.au",
    __GST_PACKAGE_RELEASE_DATETIME,
    GST_PADDING_INIT
};

namespace GstBusCount {
void plugin_init_static(void)
{
    Gst::Plugin::register_static(
        gst_plugin_desc.major_version, gst_plugin_desc.minor_version,
        gst_plugin_desc.name, gst_plugin_desc.description,
        sigc::ptr_fun<const Glib::RefPtr<Gst::Plugin>&, bool>(&plugin_init),
        gst_plugin_desc.version, gst_plugin_desc.license,
        gst_plugin_desc.source, gst_plugin_desc.package,
        gst_plugin_desc.origin
    );
}
}
