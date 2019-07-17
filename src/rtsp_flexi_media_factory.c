#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-media-factory.h>

#include "rtsp_flexi_media_factory.h"

// Important definitions
#define RTSP_FLEXI_MEDIA_FACTORY_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_RTSP_FLEXI_MEDIA_FACTORY, RtspFlexiMediaFactoryPrivate))

struct _RtspFlexiMediaFactoryPrivate
{
    RtspFlexiCreateElemFunc create_element_func;
};

static GstElement *flexi_create_element(GstRTSPMediaFactory *, const GstRTSPUrl *);

// Define the gobject type
G_DEFINE_TYPE(RtspFlexiMediaFactory, rtsp_flexi_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY);

// Class initializer
static void rtsp_flexi_media_factory_class_init(RtspFlexiMediaFactoryClass *klass)
{
    GstRTSPMediaFactoryClass *mf_parent = GST_RTSP_MEDIA_FACTORY_CLASS(klass);
    mf_parent->create_element = flexi_create_element;
}

// Object initializer
static void rtsp_flexi_media_factory_init(RtspFlexiMediaFactory *self)
{
    RtspFlexiMediaFactoryPrivate *priv = RTSP_FLEXI_MEDIA_FACTORY_GET_PRIVATE(self);
    priv->create_element_func = NULL;
}

// Public object methods
RtspFlexiMediaFactory *rtsp_flexi_media_factory_new(void)
{
    return g_object_new(TYPE_RTSP_FLEXI_MEDIA_FACTORY, NULL);
}

void rtsp_flexi_media_factory_set_create_elem(RtspFlexiMediaFactory *self, RtspFlexiCreateElemFunc func)
{
    RtspFlexiMediaFactoryPrivate *priv = RTSP_FLEXI_MEDIA_FACTORY_GET_PRIVATE(self);
    priv->create_element_func = func;
}

// Private class methods
static GstElement* flexi_create_element(GstRTSPMediaFactory *self, const GstRTSPUrl *url)
{
    RtspFlexiMediaFactoryPrivate *priv = RTSP_FLEXI_MEDIA_FACTORY_GET_PRIVATE(self);
    if (priv->create_element_func == NULL)
        return GST_RTSP_MEDIA_FACTORY_CLASS(rtsp_flexi_media_factory_parent_class)->create_element(self, url);
    else
        return priv->create_element_func(self, url);
}

// Private object methods
