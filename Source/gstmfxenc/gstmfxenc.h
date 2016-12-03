#include <gst/gst.h>
#include <gst/video/gstvideoencoder.h>
#include "mfxvideo++.h"

G_BEGIN_DECLS

#define MAXSPSPPSBUFFERSIZE 1000

#define GST_TYPE_MFX_ENCODER \
	(gst_mfx_encoder_get_type())
#define GST_MFX_ENCODER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MFX_ENCODER, GstMFXEnc))
#define GST_MFX_ENCODER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MFX_ENCODER, GstMFXEncClass))
#define GST_IS_MFX_ENCODER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MFX_ENCODER))
#define GST_IS_MFX_ENCODER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MFX_ENCODER))

typedef struct _GstMFXEnc  GstMFXEnc;
typedef struct _GstMFXEncClass GstMFXEncClass;

struct _GstMFXEnc
{
	GstVideoEncoder encoder;

	GstVideoCodecState*			input_state;

	MFXVideoSession*			mfxSession;
	mfxVideoParam*				mfxEncParams;

	MFXVideoENCODE*		pMfxENC;
	mfxFrameSurface1**	ppEncSurfaces;
	mfxU16				nEncSurfNum;
	mfxU8*				surfaceBuffers;
	mfxBitstream*		pMfxBitStream;
	GMutex				pGMutex;
	gboolean			isByteStream;

// 	GList*				listVideoCodecFrames;
// 	GMutex				ListLock;
};

struct _GstMFXEncClass
{
	GstVideoEncoderClass parent_class;
};

GType gst_mfx_encoder_get_type(void);
gboolean gst_mfx_encoder_plugin_init(GstPlugin * plugin);

G_END_DECLS