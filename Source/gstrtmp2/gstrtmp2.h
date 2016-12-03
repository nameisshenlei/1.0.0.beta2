#ifndef NEMO_GST_RTMP2_H
#define NEMO_GST_RTMP2_H


#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4127)
#include <gst/gst.h>
#pragma warning(pop)


gboolean gst_rtmp2_plugin_init(GstPlugin * plugin);

#endif