#ifndef GST_BUSCOUNTFILTER_HPP
#define GST_BUSCOUNTFILTER_HPP

#include "libbuscount.hpp"
#include "detector.hpp"
#include "tracker.hpp"
#include "world.hpp"

#include <gstreamermm.h>
#include <gstreamermm/private/element_p.h>

namespace GstBusCount {

// Static registration fuction, for any binaries this plugin is statically linked to.
void plugin_init_static();

class GstBusCountFilter : public Gst::Element
{
    /******** Static members ********/

    static const WorldConfig default_world_config;

    static Gst::ValueList formats;
    static std::unique_ptr<Detector> detector;

    /******** Object members ********/

    // Pads
    Glib::RefPtr<Gst::Pad> video_in;
    Glib::RefPtr<Gst::Pad> video_out;

    // Properties
    Glib::Property<Glib::ustring> test_property;
    //Glib::Property<Glib::ustring> line_inside_property;
    //Glib::Property<Glib::ustring> line_outside_property;
    //Glib::Property<Glib::ustring> line_bounds_a_property;
    //Glib::Property<Glib::ustring> line_bounds_b_property;

    // Owned objects
    Glib::RefPtr<Gst::AtomicQueue<cv::Mat>> frame_in_queue;
    Glib::RefPtr<Gst::AtomicQueue<Glib::RefPtr<Gst::Buffer>>> frame_out_queue;
    std::condition_variable frame_queue_pushed;
    std::condition_variable frame_queue_popped;
    std::mutex cond_m;

    WorldConfig world_config;
    Tracker tracker;
    BusCounter buscounter;

    std::thread buscount_thread;
    bool buscount_running;

    uint8_t pixel_size;
    int cv_type;
    int frame_width;
    int frame_height;
    float fps;


    /******** Object methods ********/

    nonstd::optional<cv::Mat> next_frame();
    void push_frame(const cv::Mat &frame);
    bool test_quit();

    bool setup_caps(Glib::RefPtr<Gst::Event> &event);

public:
    /******** Static methods ********/

    // Class constructor
    static void class_init(Gst::ElementClass<GstBusCountFilter> *klass);
    static void detector_init(Detector::Type detector_type, void *config);

    // Registration function
    static bool register_buscount_filter(GstPlugin *plugin);

    /******** Object methods ********/

    // Object constructor/instantiator
    explicit GstBusCountFilter(GstElement *gobj);

    // Element flow control, events, messages
    Gst::StateChangeReturn change_state_vfunc(Gst::StateChange) override;

    // Pad flow control, events, messages
    Gst::FlowReturn chain(const Glib::RefPtr<Gst::Pad> &pad, Glib::RefPtr<Gst::Buffer> &buf);
    bool pad_event(const Glib::RefPtr<Gst::Pad> &pad, Glib::RefPtr<Gst::Event> &event);
    bool pad_query(const Glib::RefPtr<Gst::Pad> &pad, Glib::RefPtr<Gst::Query> &query);


};

}

#endif // GST_BUSCOUNTFILTER_HPP
