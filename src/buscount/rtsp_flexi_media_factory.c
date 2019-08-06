#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-media-factory.h>

#include "rtsp_flexi_media_factory.h"

// Important definitions
struct _RtspFlexiMediaFactoryPrivate
{
    RtspFlexiCreateElemFunc create_element_func;
};

static GstElement *flexi_create_element(GstRTSPMediaFactory *, const GstRTSPUrl *);

// Define the gobject type
G_DEFINE_TYPE_WITH_PRIVATE(RtspFlexiMediaFactory, rtsp_flexi_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY);

// Class initializer
static void rtsp_flexi_media_factory_class_init(RtspFlexiMediaFactoryClass *klass)
{
    GstRTSPMediaFactoryClass *parent_class;

    //g_type_class_add_private(klass, sizeof(RtspFlexiMediaFactoryPrivate));

    parent_class = GST_RTSP_MEDIA_FACTORY_CLASS(klass);
    parent_class->create_element = flexi_create_element;
}

// Object initializer
static void rtsp_flexi_media_factory_init(RtspFlexiMediaFactory *self)
{
    RtspFlexiMediaFactoryPrivate *priv = rtsp_flexi_media_factory_get_instance_private(self);
    priv->create_element_func = NULL;
}

// Public object methods
RtspFlexiMediaFactory *rtsp_flexi_media_factory_new(void)
{
    return g_object_new(TYPE_RTSP_FLEXI_MEDIA_FACTORY, NULL);
}

void rtsp_flexi_media_factory_set_create_elem(RtspFlexiMediaFactory *self, RtspFlexiCreateElemFunc func)
{
    RtspFlexiMediaFactoryPrivate *priv = rtsp_flexi_media_factory_get_instance_private(self);
    priv->create_element_func = func;
}

// Private class methods
static GstElement* flexi_create_element(GstRTSPMediaFactory *base, const GstRTSPUrl *url)
{
    RtspFlexiMediaFactory *self = RTSP_FLEXI_MEDIA_FACTORY(base);
    RtspFlexiMediaFactoryPrivate *priv = rtsp_flexi_media_factory_get_instance_private(self);

    if (priv->create_element_func == NULL)
        return GST_RTSP_MEDIA_FACTORY_CLASS(rtsp_flexi_media_factory_parent_class)->create_element(base, url);
    else
        return priv->create_element_func(base, url);
}

// Private object methods
