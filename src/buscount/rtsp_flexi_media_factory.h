#ifndef BUSCOUNT_RTSP_HPP
#define BUSCOUNT_RTSP_HPP

#include <gst/rtsp-server/rtsp-media-factory.h>

G_BEGIN_DECLS

#define TYPE_RTSP_FLEXI_MEDIA_FACTORY \
    (rtsp_flexi_media_factory_get_type ())

#define RTSP_FLEXI_MEDIA_FACTORY(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RTSP_FLEXI_MEDIA_FACTORY, RtspFlexiMediaFactory))

typedef struct _RtspFlexiMediaFactory RtspFlexiMediaFactory;
typedef struct _RtspFlexiMediaFactoryClass RtspFlexiMediaFactoryClass;
typedef struct _RtspFlexiMediaFactoryPrivate RtspFlexiMediaFactoryPrivate;

struct _RtspFlexiMediaFactory
{
    GstRTSPMediaFactory parent;

    // Private
    RtspFlexiMediaFactoryPrivate *priv;
    gpointer _gst_reserved[GST_PADDING];
};

struct _RtspFlexiMediaFactoryClass
{
    GstRTSPMediaFactoryClass parent_class;

    // Private
    gpointer _gst_reserved[GST_PADDING];
};

typedef GstElement* (*RtspFlexiCreateElemFunc)(GstRTSPMediaFactory *, const GstRTSPUrl *);

GType rtsp_flexi_media_factory_get_type(void);
RtspFlexiMediaFactory *rtsp_flexi_media_factory_new(void);

// Getters and setters
void rtsp_flexi_media_factory_set_create_elem(RtspFlexiMediaFactory *, RtspFlexiCreateElemFunc);
void rtsp_flexi_media_factory_set_extra_data(RtspFlexiMediaFactory *self, GObject *data);
GObject *rtsp_flexi_media_factory_get_extra_data(RtspFlexiMediaFactory *self);


G_END_DECLS

#endif
