
#include "gstbuscountfilter.hpp"
#include "detector_opencv.hpp"
#include "detector_openvino.hpp"

#include <gstreamermm/wrap_init.h>
#include <glib.h>

GST_DEBUG_CATEGORY_STATIC(buscount); // Define a new debug category for gstreamer output
#define GST_CAT_DEFAULT buscount     // Set new category as the default category

namespace GstBusCount {

// ******** Compilation unit specific data ******** //
struct PixelFormat
{
    Glib::ustring name;
    uint8_t size;
};

static const PixelFormat format_descriptions[] = {
    { "BGR",  3 },
    { "BGRx", 4 },
    { "BGRA", 4 }
};


// ******** Static members ******** //
const WorldConfig GstBusCountFilter::default_world_config(
    Line(cv::Point(0, 1), cv::Point(0, 1)),
    Line(cv::Point(0, 1), cv::Point(0, 1)),
    Line(cv::Point(0, 1), cv::Point(0, 1)),
    Line(cv::Point(0, 1), cv::Point(0, 1))
);

Gst::ValueList GstBusCountFilter::formats;
std::unique_ptr<Detector> GstBusCountFilter::detector;


// ******** Function definitions ******** //

void GstBusCountFilter::class_init(Gst::ElementClass<GstBusCountFilter> *klass)
{
    // Metadata for the class
    klass->set_metadata(
            "Bus Counter Filter",
            "Processor/Video/",
            "Processes incoming video from a camera and extract person entry count",
            "Alastair Knowles <2062674@student.swin.edu.au>");

    // Static object initialisation. Done here instead of at the compilation unit level
    // due to requiring initialization of gstreamer stuff.
    formats.init(formats.get_type());

    for (auto i = 0u; i < sizeof(format_descriptions) / sizeof(PixelFormat); i++)
    {
        Glib::Value<Glib::ustring> val;

        val.init(val.value_type());
        val.set(Glib::ustring(format_descriptions[i].name));

        formats.append(val);
    }

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

void GstBusCountFilter::detector_init(Detector::Type detector_type, void *config)
{
    switch (detector_type)
    {
        case Detector::DETECTOR_OPENCV:
            detector = std::unique_ptr<DetectorOpenCV>(
                new DetectorOpenCV(*static_cast<DetectorOpenCV::NetConfig*>(config))
            );
            break;

        case Detector::DETECTOR_OPENVINO:
            detector = std::unique_ptr<DetectorOpenVino>(
                new DetectorOpenVino(*static_cast<DetectorOpenVino::NetConfig*>(config))
            );
            break;
    }
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
        //line_inside_property(*this, "line_inside_property", "default_val")
        //line_outside_property(*this, "line_outside_property", "default_val")
        //line_bounds_a_property(*this, "line_bounds_a_property", "default_val")
        //line_bounds_b_property(*this, "line_bounds_b_property", "default_val")
        test_property(*this, "test_property", "default_val"),
        frame_in_queue(Gst::AtomicQueue<cv::Mat>::create(10)),
        frame_out_queue(Gst::AtomicQueue<Glib::RefPtr<Gst::Buffer>>::create(10)),
        world_config(default_world_config),
        tracker(world_config),
        buscounter(
                *detector, tracker, world_config,
                std::bind(&GstBusCountFilter::next_frame, this),
                std::bind(&GstBusCountFilter::push_frame, this, std::placeholders::_1),
                std::bind(&GstBusCountFilter::test_quit, this)
        ),
        pixel_size(3),
        frame_width(1),
        frame_height(1),
        fps(1.0)
{
    add_pad(video_in = Gst::Pad::create(get_pad_template("video_sink"), "video_sink"));
    add_pad(video_out = Gst::Pad::create(get_pad_template("video_src"), "video_src"));

    GST_PAD_SET_PROXY_CAPS(video_in->gobj());
    GST_PAD_SET_PROXY_CAPS(video_out->gobj());

    video_in->set_chain_function(sigc::mem_fun(*this, &GstBusCountFilter::chain));
    video_in->set_event_function(sigc::mem_fun(*this, &GstBusCountFilter::sink_event));
}

// Private methods
nonstd::optional<cv::Mat> GstBusCountFilter::next_frame()
{
    std::unique_lock<std::mutex> lk(cond_m);

    if (frame_in_queue->length() == 0) {
        frame_queue_cond.wait(lk);
    }

    return frame_in_queue->pop();
}

void GstBusCountFilter::push_frame(const cv::Mat &frame)
{
    Glib::RefPtr<Gst::Buffer> buf;
    Gst::MapInfo mapinfo;
    do
    {
        if (buf)
        {
            buf->unmap(mapinfo);
            buf.reset();
        }
        if (frame_out_queue->length() == 0)
        {
            break;
        }

        buf = frame_out_queue->pop();
        buf->map(mapinfo, Gst::MAP_READ);
    }
    while (frame.data != mapinfo.get_data());

    if (buf)
        video_out->push(std::move(buf));
}

bool GstBusCountFilter::test_quit()
{
    return false;
}

bool GstBusCountFilter::setup_caps(Glib::RefPtr<Gst::Event> &event)
{
    // Scratch value for fetching fields

    // Sink pad caps have been negotiated. Copy caps to source pad.
    auto caps_event = Glib::RefPtr<Gst::EventCaps>::cast_static(event);
    auto out_caps = caps_event->parse_caps()->copy();
    auto s = out_caps->get_structure(0);

    GST_DEBUG("### Set caps to %s", out_caps->to_string().c_str());

    pixel_size = 3;
    frame_width = 1;
    frame_height = 1;
    fps = 1.0;

    if (s.get_field_type("format") != Glib::Value<Glib::ustring>::value_type())
        GST_DEBUG("Format is not of a valid type, defaulting pixel size to %i", pixel_size);
    else
    {
        Glib::Value<Glib::ustring> value;
        s.get_field("format", value);

        bool format_exists = false;
        for (auto i = 0u; i < sizeof(format_descriptions) / sizeof(PixelFormat); i++)
        {
            if (value.get() == format_descriptions[i].name)
            {
                format_exists = true;
                pixel_size = format_descriptions[i].size;
                GST_DEBUG("Pixel size set to %i", pixel_size);
            }
        }

        if (!format_exists)
            GST_DEBUG("Format is not of a valid type, defaulting pixel size to %i", pixel_size);
    }

    if (s.get_field_type("width") != Glib::Value<int>::value_type())
        GST_DEBUG("Width is not of a valid type, defaulting to %i", frame_width);
    else
    {
        Glib::Value<int> value;
        s.get_field("width", value);

        frame_width = value.get();

        GST_DEBUG("Width set to %i", frame_width);
    }

    if (s.get_field_type("height") != Glib::Value<int>::value_type())
        GST_DEBUG("Height is not of a valid type, defaulting to %i", frame_height);
    else
    {
        Glib::Value<int> value;
        s.get_field("height", value);

        frame_height = value.get();

        GST_DEBUG("Height set to %i", frame_height);
    }

    if (s.get_field_type("framerate") != Glib::Value<Gst::Fraction>::value_type())
        GST_DEBUG("Framerate is not of a valid type, defaulting to %f", fps);
    else
    {
        Glib::Value<Gst::Fraction> value;
        s.get_field("framerate", value);

        Gst::Fraction f = value.get();
        fps = f.num / f.denom;

        GST_DEBUG("Framerate set to %f", fps);
    }

    return gst_pad_set_caps(video_out->gobj(), out_caps->gobj());
}

// Events
bool GstBusCountFilter::sink_event(const Glib::RefPtr<Gst::Pad> &pad, Glib::RefPtr<Gst::Event> &event)
{
    bool result;

    switch (event->get_event_type())
    {
        case GST_EVENT_CAPS:
        {
            result = setup_caps(event);
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

    Gst::MapInfo mapinfo;
    buf->map(mapinfo, Gst::MAP_READ | Gst::MAP_WRITE);

    if (mapinfo.get_size() != (uint64_t)frame_width * (uint64_t)frame_height * pixel_size)
    {
        GST_ERROR("Buffer size (%u) did not match calculated size (%llu)",
                mapinfo.get_size(),
                (uint64_t)frame_width * (uint64_t)frame_height * pixel_size
        );
        return Gst::FLOW_NOT_SUPPORTED;
    }

    frame_in_queue->push(cv::Mat(frame_height, frame_width, CV_8UC3, mapinfo.get_data(), pixel_size));
    frame_out_queue->push(buf);
    frame_queue_cond.notify_one();

    buf->unmap(mapinfo);

    return Gst::FLOW_OK;
}

static gboolean plugin_init(GstPlugin* plugin)
{
    // Ensure Glib is a go
    Glib::init();

    // Initialize debug category
    GST_DEBUG_CATEGORY_INIT(buscount, "buscount", 0, "A debug category for the buscount plugin");

    // Register elements (we've only got 1 element at the moment)
    return GstBusCountFilter::register_buscount_filter(plugin);

}

static bool plugin_init(const Glib::RefPtr<Gst::Plugin> &plugin)
{
    return plugin_init(plugin->gobj());
}

} // end namespace GstBusCount

// Struct for plugin registration. Used for debugging with gst-inspect-1.0 and gst-launch-1.0
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
