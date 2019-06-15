#ifndef GST_BUSCOUNTFILTER_HPP
#define GST_BUSCOUNTFILTER_HPP

#include <gstreamermm.h>
#include <gstreamermm/private/element_p.h>

namespace GstBusCount {

// Static registration fuction, for any binaries this plugin is statically linked to.
void plugin_init_static();

class GstBusCountFilter : public Gst::Element
{
    static Gst::ValueList formats;

    Glib::RefPtr<Gst::Pad> video_in;
    Glib::RefPtr<Gst::Pad> video_out;

    Glib::Property<Glib::ustring> test_property;
public:
    /******** Static methods ********/

    // Class constructor
    static void class_init(Gst::ElementClass<GstBusCountFilter> *klass);

    // Registration function
    static bool register_buscount_filter(Glib::RefPtr<Gst::Plugin> plugin);
    static bool register_buscount_filter(GstPlugin *plugin);

    /******** Object methods ********/

    // Object constructor/instantiator
    explicit GstBusCountFilter(GstElement *gobj);

    // Flow control and events
    Gst::FlowReturn chain(const Glib::RefPtr<Gst::Pad> &, Glib::RefPtr<Gst::Buffer> &);
    bool sink_event(const Glib::RefPtr<Gst::Pad> &, Glib::RefPtr<Gst::Event> &);

};

}

#endif // GST_BUSCOUNTFILTER_HPP
