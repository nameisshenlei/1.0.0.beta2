#ifndef _GST_NV_ENC_H__
#include <gst/gst.h>
#include <gst/video/gstvideoencoder.h>

#include "cuda.h"

#include "nvEncodeAPI.h"

#include "gstnvqueue.h"

G_BEGIN_DECLS

typedef struct _EncodeFrameConfig
{
	uint8_t  *yuv[3];
	uint32_t stride[3];
	uint32_t width;
	uint32_t height;
}EncodeFrameConfig;

typedef struct
{
	guint		nvPreset;
	guint		nvGOP;
	guint		nvBFrames;
	guint		nvPass;
	NV_ENC_QP	nvConstQP;
	gboolean	nvEnableAQ;
	gboolean	nvEnableExtQPDeltaMap;
	gboolean	nvEnableInitialRCQP;
	gboolean	nvEnableMinQP;
	gboolean	nvEnableMaxQP;
	guint		nvBitRate;
	guint		nvMaxBitRate;
	guint		nvVBVSize;
	guint		nvVBVDelay;
	guint		nvProfile;
	NV_ENC_QP	nvMinQP;
	NV_ENC_QP	nvMaxQP;
	NV_ENC_QP	nvInitialRCQP;
	guint		nvRef;
}nvParams;

#define GST_MAX_ENCODE_QUEUE		32

#define GST_TYPE_NV_ENCODER \
	(gst_nv_encoder_get_type())
#define GST_NV_ENCODER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NV_ENCODER, GstNVEnc))
#define GST_NV_ENCODER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_NV_ENCODER, GstNVEncClass))
#define GST_IS_NV_ENCODER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_NV_ENCODER))
#define GST_IS_NV_ENCODER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_NV_ENCODER))

typedef struct _GstNVEnc  GstNVEnc;
typedef struct _GstNVEncClass GstNVEncClass;

struct _GstNVEnc
{
	GstVideoEncoder encoder;

	GstVideoCodecState*				m_pInputState;

	HINSTANCE						m_hinstLib;
	NV_ENCODE_API_FUNCTION_LIST*	m_pEncodeAPI;
	void*							m_pDevice;
	void*							m_hEncoder;
	NV_ENC_INITIALIZE_PARAMS*		m_pCreateEncodeParams;
	NV_ENC_CONFIG*					m_pEncodeConfig;

	guint							m_uEncodeBufferCount;

	EncodeBuffer					m_stEncodeBuffer[GST_MAX_ENCODE_QUEUE];
	GstNvQueue*						m_pEncodeBufferQueue;
	EncodeOutputBuffer				m_stEOSOutputBfr;
	nvParams						m_nvParams;
};

struct _GstNVEncClass
{
	GstVideoEncoderClass parent_class;
};

GType gst_nv_encoder_get_type(void);
gboolean gst_nv_encoder_plugin_init(GstPlugin * plugin);

G_END_DECLS

#define _GST_NV_ENC_H__
#endif // !_GST_NV_ENC_H__