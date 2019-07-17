
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
    int cv_type;
};

static const PixelFormat format_descriptions[] = {
    { "BGR",  3, CV_8UC3 }//,
//    { "BGRx", 4, CV_8UC4 },
//    { "BGRA", 4, CV_8UC4 }
};

static const float default_threshold = 0.2;


// ******** Static members ******** //
const WorldConfig GstBusCountFilter::default_world_config = WorldConfig::from_file(
    cv::Size(640, 480), "../config.csv"
);
/*
(
    Line(cv::Point(0, 1), cv::Point(0, 1)),
    Line(cv::Point(0, 1), cv::Point(0, 1)),
    Line(cv::Point(0, 1), cv::Point(0, 1)),
    Line(cv::Point(0, 1), cv::Point(0, 1))
);
*/

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
    if (!detector)
    {
        GST_INFO("Initializing detector");
        switch (detector_type)
        {
            case Detector::DETECTOR_OPENCV:
                GST_DEBUG("Using OpenCV detector");
                detector = std::unique_ptr<DetectorOpenCV>(
                    new DetectorOpenCV(*static_cast<DetectorOpenCV::NetConfig*>(config))
                );
                break;

            case Detector::DETECTOR_OPENVINO:
                GST_DEBUG("Using OpenViNO detector");
                detector = std::unique_ptr<DetectorOpenVino>(
                    new DetectorOpenVino(*static_cast<DetectorOpenVino::NetConfig*>(config))
                );
                break;
        }
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
        tracker(world_config, default_threshold),
        buscounter(
                *detector, tracker, world_config,
                std::bind(&GstBusCountFilter::next_frame, this),
                std::bind(&GstBusCountFilter::push_frame, this, std::placeholders::_1),
                std::bind(&GstBusCountFilter::test_quit, this)
        ),
        buscount_running(false),
        pixel_size(format_descriptions[0].size),
        cv_type(format_descriptions[0].cv_type),
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

    GST_DEBUG("Getting next frame");

    if (frame_in_queue->length() == 0) {
        GST_DEBUG("Waiting for next frame");
        frame_queue_pushed.wait(lk);
    }

    GST_DEBUG("Popping next frame");
    cv::Mat ret(frame_in_queue->pop());
    frame_queue_popped.notify_one();

    return ret;
}

void GstBusCountFilter::push_frame(const cv::Mat &frame)
{
    Glib::RefPtr<Gst::Buffer> buf;
    Gst::MapInfo mapinfo;

    GST_DEBUG("Frame push requested");

    do
    {
        if (buf)
        {
            GST_DEBUG("Clearing buffer data");
            buf->unmap(mapinfo);
            buf.reset();
        }
        if (frame_out_queue->length() == 0)
        {
            GST_DEBUG("frame_out_queue exausted");
            break;
        }

        GST_DEBUG("Fetching next frame from frame_out_queue");
        buf = frame_out_queue->pop();
        buf->map(mapinfo, Gst::MAP_READ);
    }
    while (frame.data != mapinfo.get_data());

    buf->unmap(mapinfo);

    if (buf)
    {
        GST_DEBUG("Found frame. Pushing to video_out pad");
        video_out->push(std::move(buf));
    }
}

bool GstBusCountFilter::test_quit()
{
    return !buscount_running;
}

bool GstBusCountFilter::setup_caps(Glib::RefPtr<Gst::Event> &event)
{
    // Scratch value for fetching fields

    // Sink pad caps have been negotiated. Copy caps to source pad.
    auto caps_event = Glib::RefPtr<Gst::EventCaps>::cast_static(event);
    auto out_caps = caps_event->parse_caps()->copy();
    auto s = out_caps->get_structure(0);

    GST_DEBUG("### Set caps to %s", out_caps->to_string().c_str());

    cv_type = format_descriptions[0].cv_type;
    pixel_size = format_descriptions[0].size;
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
                cv_type = format_descriptions[i].cv_type;
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
    std::unique_lock<std::mutex> lk(cond_m);

    Gst::MapInfo mapinfo;
    buf->map(mapinfo, Gst::MAP_READ | Gst::MAP_WRITE);

    if (mapinfo.get_size() < (uint64_t)frame_width * (uint64_t)frame_height * pixel_size)
    {
        GST_ERROR("Buffer size (%u) did not match calculated size (%llu)",
                mapinfo.get_size(),
                (uint64_t)frame_width * (uint64_t)frame_height * pixel_size
        );
        return Gst::FLOW_NOT_SUPPORTED;
    }

    if (frame_in_queue->length() >= 16) {
        GST_DEBUG("Queue full, waiting for empty");
        frame_queue_popped.wait(lk);
    }

    GST_DEBUG("Pushing data to queue");
    frame_in_queue->push(cv::Mat(frame_height, frame_width, cv_type, mapinfo.get_data()));
    frame_out_queue->push(buf);
    frame_queue_pushed.notify_one();

    buf->unmap(mapinfo);

    return Gst::FLOW_OK;
}

Gst::StateChangeReturn GstBusCountFilter::change_state_vfunc(Gst::StateChange transition)
{
    Gst::StateChangeReturn result = Gst::STATE_CHANGE_SUCCESS;

    // Note: Changing upwards vs changing downwards are handled in separate switch statements due
    //       to multithreading considerations.

    // Changing upward
    switch (transition)
    {
        case Gst::STATE_CHANGE_NULL_TO_READY:
            GST_DEBUG("NULL -> READY");
            break;
        case Gst::STATE_CHANGE_READY_TO_PAUSED:
            GST_DEBUG("READY -> PAUSED");
            GST_DEBUG("Starting buscounter thread");
            buscount_running = true;
            buscount_thread = std::thread(
                [this]()
                {
                    try
                    {
                        GST_DEBUG("Started buscounter thread");
                        buscounter.run(BusCounter::RUN_PARALLEL, true);
                        GST_DEBUG("Buscounting finished");
                    }
                    catch (std::exception &e)
                    {
                        GST_ERROR("An exception occurred while running buscounter: %s", e.what());
                        set_state(Gst::STATE_NULL);
                    }
                }
            );
            break;
        case Gst::STATE_CHANGE_PAUSED_TO_PLAYING:
            GST_DEBUG("PAUSED -> PLAYING");
            break;
        default:
            break;
    }

    // Something bad happened in the parent...oops.
    result = Gst::Element::change_state_vfunc(transition);
    if (result == Gst::STATE_CHANGE_FAILURE)
        return result;

    // Changing downward
    switch (transition)
    {
        case Gst::STATE_CHANGE_PLAYING_TO_PAUSED:
            GST_DEBUG("PLAYING -> PAUSED");
            break;

        case Gst::STATE_CHANGE_PAUSED_TO_READY:
            GST_DEBUG("PAUSED -> READY");
            GST_DEBUG("Stopping buscounter thread");
            buscount_running = false;
            buscount_thread.join();
            GST_DEBUG("Stopped buscounter thread");
            break;

        case Gst::STATE_CHANGE_READY_TO_NULL:
            GST_DEBUG("READY -> NULL");
            break;

        default:
            break;
    }

    return result;
}

static gboolean plugin_init(GstPlugin* plugin)
{

    // Ensure Glib is a go
    Glib::init();

    // Initialize debug category
    GST_DEBUG_CATEGORY_INIT(buscount, "buscount", 0, "A debug category for the buscount plugin");

    DetectorOpenCV::NetConfig net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        1, //2/255.0,            // scale
        cv::Scalar(1,1,1),//cv::Scalar(127.5, 127.5, 127.5),     // mean
        "../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        "../models/MobileNetSSD_IE/MobileNetSSD.bin", // model
        cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
        cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
    };

    GstBusCount::GstBusCountFilter::detector_init(Detector::DETECTOR_OPENCV, &net_config);

    GST_DEBUG("Initializing plugin");

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
    static_cast<gboolean(*)(GstPlugin *plugin)>(&GstBusCount::plugin_init),
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
