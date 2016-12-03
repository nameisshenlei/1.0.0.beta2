#include "gstmfxenc.h"
#include "memory.h"
#include "string.h"

GST_DEBUG_CATEGORY_STATIC(mfxenc_debug);
#define GST_CAT_DEFAULT mfxenc_debug

enum
{
	/* FILL ME */
	LAST_SIGNAL
};

enum
{
	PROP_ZERO,
	PROP_CODEC_ID,
	PROP_CODEC_PROFILE,
	PROP_CODEC_LEVEL,
	PROP_TARGET_USAGE,
	PROP_GOP_PIC_SIZE,
	PROP_GOP_REF_DIST,
	PROP_GOP_OPT_FLAG,
	PROP_IDR_INTERVAL,
	PROP_RATE_CTL_METHOD,
	PROP_INIT_DELAY,
	PROP_BITRATE,
	PROP_MAX_BITRATE,
	PROP_QP_I,
	PROP_QP_P,
	PROP_QP_B,
	PROP_AVBR_ACCURACY,
	PROP_CONVERGENCE,
	PROP_ICQ_QUALITY,
	PROP_BUF_SIZE_IN_KB,
	PROP_NUM_SLICE,
	PROP_NUM_REF_FRAME,
	PROP_ENCODED_ORDER,
	PROP_BYTE_STREAM,
	N_PROPERTIES
};

#define GST_MFX_ENC_CODEC_ID_DEFAULT            MFX_CODEC_AVC
#define GST_MFX_ENC_CODEC_PROFILE_DEFAULT       MFX_PROFILE_AVC_MAIN
#define GST_MFX_ENC_CODEC_LEVEL_DEFAULT         MFX_LEVEL_AVC_3
#define GST_MFX_ENC_NUM_THREAD_DEFAULT          0
#define GST_MFX_ENC_TARGET_USAGE_DEFAULT        MFX_TARGETUSAGE_BALANCED
#define GST_MFX_ENC_GOP_PIC_SIZE_DEFAULT        0
#define GST_MFX_ENC_GOP_REF_DIST_DEFAULT        0
#define GST_MFX_ENC_GOP_OPT_FLAG_DEFAULT        0
#define GST_MFX_ENC_IDR_INTERVAL_DEFAULT        0
#define GST_MFX_ENC_RATE_CTL_METHOD_DEFAULT     MFX_RATECONTROL_VBR
#define GST_MFX_ENC_INIT_DELAY_DEFAULT          0
#define GST_MFX_ENC_BITRATE_DEFAULT             2048
#define GST_MFX_ENC_MAX_BITRATE_DEFAULT         0
#define GST_MFX_ENC_QP_I_DEFAULT				0
#define GST_MFX_ENC_QP_P_DEFAULT				0
#define GST_MFX_ENC_QP_B_DEFAULT				0
#define GST_MFX_ENC_ACCURACY_DEFAULT			0
#define GST_MFX_ENC_CONVERGENCE_DEFAULT			0
#define GST_MFX_ENC_ICQ_QUALITY_DEFAULT			0
#define GST_MFX_ENC_BUF_SIZE_IN_KB_DEFAULT		0
#define GST_MFX_ENC_NUM_SLICE_DEFAULT           0
#define GST_MFX_ENC_NUM_REF_FRAME_DEFAULT       1
#define GST_MFX_ENC_ENCODED_ORDER_DEFAULT       0
#define GST_MFX_ENC_ENCODED_BYTE_STREAM_DEFAULT FALSE

#define MSDK_SAFE_DELETE_ARRAY(P)       {if (P) {delete[] P; P = NULL;}}
#define MSDK_IGNORE_MFX_STS(P, X)       {if ((X) == (P)) {P = MFX_ERR_NONE;}}
#define MSDK_ALIGN32(X)					(((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define MSDK_ALIGN16(value)             (((value + 15) >> 4) << 4)

#define GST_TYPE_MFX_ENC_CODEC_ID	(gst_mfx_enc_codec_id_get_type())
static GType gst_mfx_enc_codec_id_get_type(void)
{
	static GType codec_id_type = 0;
	static const GEnumValue codec_id[] =
	{
		{ MFX_CODEC_AVC, "AVC", "avc" },
		{ MFX_CODEC_MPEG2, "MPEG-2", "mpeg2" },
		{ MFX_CODEC_VC1, "VC-1", "vc1" },
		{ MFX_CODEC_HEVC, "HEVC", "hevc" },
		{ 0, NULL, NULL }
	};
	if (!codec_id_type)
		codec_id_type = g_enum_register_static("GstMfxEncCodecIdType", codec_id);
	return codec_id_type;
}

#define GST_TYPE_MFX_ENC_CODEC_PROFILE	(gst_mfx_enc_codec_profile_get_type())
static GType gst_mfx_enc_codec_profile_get_type(void)
{
	static GType codec_profile_type = 0;
	static const GEnumValue codec_profile[] =
	{
		{ MFX_PROFILE_UNKNOWN, "Unknown", "unknown" },
		{ MFX_PROFILE_AVC_BASELINE, "AVC Baseline", "avc-baseline" },
		{ MFX_PROFILE_AVC_MAIN, "AVC Main", "avc-main" },
		{ MFX_PROFILE_AVC_EXTENDED, "AVC Extended", "avc-extended" },
		{ MFX_PROFILE_AVC_HIGH, "AVC High", "avc-high" },
		{ MFX_PROFILE_AVC_CONSTRAINED_BASELINE, "AVC Constrained Baseline", "avc-constrained-baseline" },
		{ MFX_PROFILE_AVC_CONSTRAINED_HIGH, "AVC Constrained High", "avc-constrained-high" },
		{ MFX_PROFILE_AVC_PROGRESSIVE_HIGH, "AVC Progressive High", "avc-progressive-high" },
		{ MFX_PROFILE_AVC_CONSTRAINT_SET0, "AVC Constraint Set0", "avc-constraint-set0" },
		{ MFX_PROFILE_AVC_CONSTRAINT_SET1, "AVC Constraint Set1", "avc-constraint-set1" },
		{ MFX_PROFILE_AVC_CONSTRAINT_SET2, "AVC Constraint Set2", "avc-constraint-set2" },
		{ MFX_PROFILE_AVC_CONSTRAINT_SET3, "AVC Constraint Set3", "avc-constraint-set3" },
		{ MFX_PROFILE_AVC_CONSTRAINT_SET4, "AVC Constraint Set4", "avc-constraint-set4" },
		{ MFX_PROFILE_AVC_CONSTRAINT_SET5, "AVC Constraint Set5", "avc-constraint-set5" },
		{ MFX_PROFILE_MPEG2_SIMPLE, "MPEG-2 Simple", "mpeg2-simple" },
		{ MFX_PROFILE_MPEG2_MAIN, "MPEG-2 Main", "mpeg2-main" },
		{ MFX_PROFILE_MPEG2_HIGH, "MPEG-2 High", "mpeg2-high" },
		{ MFX_PROFILE_VC1_SIMPLE, "VC-1 Simple", "vc1-simple" },
		{ MFX_PROFILE_VC1_MAIN, "VC-1 Main", "vc1-main" },
		{ MFX_PROFILE_VC1_ADVANCED, "VC-1 Advanced", "vc1-advanced" },
		{ MFX_PROFILE_HEVC_MAIN, "HEVC Main", "hevc-main" },
		{ MFX_PROFILE_HEVC_MAIN10, "HEVC Main10", "hevc-main10" },
		{ MFX_PROFILE_HEVC_MAINSP, "HEVC MainSP", "hevc-mainSP" },
		{ 0, NULL, NULL }
	};
	if (!codec_profile_type)
		codec_profile_type = g_enum_register_static("GstMfxEncCodecProfileType", codec_profile);
	return codec_profile_type;
}

#define GST_TYPE_MFX_ENC_CODEC_LEVEL	(gst_mfx_enc_codec_level_get_type())
static GType gst_mfx_enc_codec_level_get_type(void)
{
	static GType codec_level_type = 0;
	static const GEnumValue codec_level[] =
	{
		{ MFX_LEVEL_UNKNOWN, "Unknown", "unknown" },
		{ MFX_LEVEL_AVC_1, "AVC 1", "avc1" },
		{ MFX_LEVEL_AVC_1b, "AVC 1b", "avc1b" },
		{ MFX_LEVEL_AVC_11, "AVC 11", "avc11" },
		{ MFX_LEVEL_AVC_12, "AVC 12", "avc12" },
		{ MFX_LEVEL_AVC_13, "AVC 13", "avc13" },
		{ MFX_LEVEL_AVC_2, "AVC 2", "avc2" },
		{ MFX_LEVEL_AVC_21, "AVC 21", "avc21" },
		{ MFX_LEVEL_AVC_22, "AVC 22", "avc22" },
		{ MFX_LEVEL_AVC_3, "AVC 3", "avc3" },
		{ MFX_LEVEL_AVC_31, "AVC 31", "avc31" },
		{ MFX_LEVEL_AVC_32, "AVC 32", "avc32" },
		{ MFX_LEVEL_AVC_4, "AVC 4", "avc4" },
		{ MFX_LEVEL_AVC_41, "AVC 41", "avc41" },
		{ MFX_LEVEL_AVC_42, "AVC 42", "avc42" },
		{ MFX_LEVEL_AVC_5, "AVC 5", "avc5" },
		{ MFX_LEVEL_AVC_51, "AVC 51", "avc51" },
		{ MFX_LEVEL_AVC_52, "AVC 52", "avc52" },
		{ MFX_LEVEL_MPEG2_LOW, "MPEG-2 Low", "mpeg2-low" },
		{ MFX_LEVEL_MPEG2_MAIN, "MPEG-2 Main", "mpeg2-main" },
		{ MFX_LEVEL_MPEG2_HIGH, "MPEG-2 High", "mpeg2-high" },
		{ MFX_LEVEL_MPEG2_HIGH1440, "MPEG-2 High1440", "mpeg2-high1440" },
		{ MFX_LEVEL_VC1_LOW, "VC-1 Low", "vc1-low" },
		{ MFX_LEVEL_VC1_MEDIAN, "VC-1 Median", "vc1-median" },
		{ MFX_LEVEL_VC1_HIGH, "VC-1 High", "vc1-high" },
		{ MFX_LEVEL_VC1_0, "VC-1 0", "vc1-0" },
		{ MFX_LEVEL_VC1_1, "VC-1 1", "vc1-1" },
		{ MFX_LEVEL_VC1_2, "VC-1 2", "vc1-2" },
		{ MFX_LEVEL_VC1_3, "VC-1 3", "vc1-3" },
		{ MFX_LEVEL_VC1_4, "VC-1 4", "vc1-4" },
		{ MFX_LEVEL_HEVC_1, "HEVC 1", "hevc-1" },
		{ MFX_LEVEL_HEVC_2, "HEVC 2", "hevc-1" },
		{ MFX_LEVEL_HEVC_21, "HEVC 21", "hevc-1" },
		{ MFX_LEVEL_HEVC_3, "HEVC 3", "hevc-1" },
		{ MFX_LEVEL_HEVC_31, "HEVC 31", "hevc-1" },
		{ MFX_LEVEL_HEVC_4, "HEVC 4", "hevc-1" },
		{ MFX_LEVEL_HEVC_41, "HEVC 41", "hevc-1" },
		{ MFX_LEVEL_HEVC_5, "HEVC 5", "hevc-1" },
		{ MFX_LEVEL_HEVC_51, "HEVC 51", "hevc-1" },
		{ MFX_LEVEL_HEVC_52, "HEVC 52", "hevc-1" },
		{ MFX_LEVEL_HEVC_6, "HEVC 6", "hevc-1" },
		{ MFX_LEVEL_HEVC_61, "HEVC 61", "hevc-1" },
		{ MFX_LEVEL_HEVC_62, "HEVC 62", "hevc-1" },
		{ MFX_TIER_HEVC_MAIN, "HEVC main", "hevc-main" },
		{ MFX_TIER_HEVC_HIGH, "HEVC high", "hevc-high" },
		{ 0, NULL, NULL }
	};
	if (!codec_level_type)
		codec_level_type = g_enum_register_static("GstMfxEncCodecLevelType", codec_level);
	return codec_level_type;
}

#define GST_TYPE_MFX_ENC_TARGET_USAGE	(gst_mfx_enc_target_usage_get_type())
static GType gst_mfx_enc_target_usage_get_type(void)
{
	static GType target_usage_type = 0;
	static const GEnumValue target_usage[] =
	{
		{ MFX_TARGETUSAGE_1, "Quality 1", "quality-1" },
		{ MFX_TARGETUSAGE_2, "Quality 2", "quality-2" },
		{ MFX_TARGETUSAGE_3, "Quality 3", "quality-3" },
		{ MFX_TARGETUSAGE_4, "Quality 4", "quality-4" },
		{ MFX_TARGETUSAGE_5, "Quality 5", "quality-5" },
		{ MFX_TARGETUSAGE_6, "Quality 6", "quality-6" },
		{ MFX_TARGETUSAGE_7, "Quality 7", "quality-7" },
		{ MFX_TARGETUSAGE_UNKNOWN, "Unknown", "unknown" },
		{ MFX_TARGETUSAGE_BEST_QUALITY, "Best Quality", "best-quality" },
		{ MFX_TARGETUSAGE_BALANCED, "Balanced", "balanced" },
		{ MFX_TARGETUSAGE_BEST_SPEED, "Best Speed", "best-speed" },
		{ 0, NULL, NULL }
	};
	if (!target_usage_type)
		target_usage_type = g_enum_register_static("GstMfxEncTargetUsageType", target_usage);
	return target_usage_type;
}

#define GST_TYPE_MFX_ENC_GOP_OPT_FLAG	(gst_mfx_enc_gop_opt_flag_get_type())
static GType gst_mfx_enc_gop_opt_flag_get_type(void)
{
	static GType gop_opt_flag_type = 0;
	static const GEnumValue gop_opt_flag[] =
	{
		{ MFX_GOP_CLOSED, "Gop Closed", "closed" },
		{ MFX_GOP_STRICT, "Gop Strict", "strict" },
		{ 0, NULL, NULL }
	};
	if (!gop_opt_flag_type)
		gop_opt_flag_type = g_enum_register_static("GstMfxEncGopOptFlagType", gop_opt_flag);
	return gop_opt_flag_type;
}

#define GST_TYPE_MFX_ENC_RATE_CTL_METHOD	(gst_mfx_enc_rate_ctl_method_get_type())
static GType gst_mfx_enc_rate_ctl_method_get_type(void)
{
	static GType rate_ctl_method_type = 0;
	static const GEnumValue rate_ctl_method[] =
	{
		{ MFX_RATECONTROL_CBR, "CBR", "cbr" },
		{ MFX_RATECONTROL_VBR, "VBR", "vbr" },
		{ MFX_RATECONTROL_CQP, "CQP", "cqp" },
		{ MFX_RATECONTROL_AVBR, "AVBR", "avbr" },
		{ MFX_RATECONTROL_LA, "LA", "la" },
		{ MFX_RATECONTROL_ICQ, "ICQ", "icq" },
		{ MFX_RATECONTROL_VCM, "VCM", "vcm" },
		{ MFX_RATECONTROL_LA_ICQ, "LA-ICQ", "la-icq" },
		{ MFX_RATECONTROL_LA_EXT, "LA-EXT", "la-ext" },
		{ MFX_RATECONTROL_LA_HRD, "LA_HRD", "la-hrd" },
		{ MFX_RATECONTROL_QVBR, "QVBR", "qvbr" },
		{ 0, NULL, NULL }
	};
	if (!rate_ctl_method_type)
		rate_ctl_method_type = g_enum_register_static("GstMfxEncRateCtlMethodType",	rate_ctl_method);
	return rate_ctl_method_type;
}

static gboolean gst_mfx_encoder_allacate_surface(GstMFXEnc* enc);
static gboolean gst_mfx_encoder_open(GstMFXEnc* enc);
static int gst_mfx_get_free_surface_index(mfxFrameSurface1** pSurfacesPool, mfxU16 nPoolSize);
static gboolean gst_mfx_load_data(GstMFXEnc* mfxenc, GstVideoInfo *info, mfxFrameSurface1* pSurface, GstVideoCodecFrame* pBuffer);
static bool gst_mfx_extend_buffer(GstMFXEnc *mfxenc);

static void gst_mfxenc_finalize(GObject* object);
static void gst_mfxenc_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_mfxenc_get_property(GObject* object, guint prop_id,	GValue* value, GParamSpec* pspec);

static gboolean gst_mfxenc_start(GstVideoEncoder* benc);
static gboolean gst_mfxenc_stop(GstVideoEncoder* benc);
static gboolean gst_mfxenc_set_format(GstVideoEncoder* encoder, GstVideoCodecState* state);
static GstFlowReturn gst_mfxenc_handle_frame(GstVideoEncoder* encoder, GstVideoCodecFrame* frame);
static gboolean gst_mfxenc_propose_allocation(GstVideoEncoder* encoder,	GstQuery* query);

#define gst_mfx_encoder_parent_class parent_class
G_DEFINE_TYPE(GstMFXEnc, gst_mfx_encoder, GST_TYPE_VIDEO_ENCODER);

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("video/x-raw, "
	"format = (string) { NV12 }, "
	"framerate = (fraction) [0, MAX], "
	"width = (int) [ 16, MAX ], " "height = (int) [ 16, MAX ]")
	);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("video/x-h264, "
	"framerate = (fraction) [0/1, MAX], "
	"width = (int) [ 1, MAX ], " "height = (int) [ 1, MAX ], "
	"stream-format = (string) { byte-stream, avc }, "
	"alignment = (string) { au }")
	);

static void gst_mfx_encoder_class_init(GstMFXEncClass* klass)
{
	GObjectClass* gobject_class;
	GstElementClass* element_class;
	GstVideoEncoderClass* venc_class;

	gobject_class = (GObjectClass*)klass;
	element_class = (GstElementClass*)klass;
	venc_class = (GstVideoEncoderClass*)klass;

	parent_class = g_type_class_peek_parent(klass);

	gobject_class->finalize = gst_mfxenc_finalize;
	gobject_class->set_property = gst_mfxenc_set_property;
	gobject_class->get_property = gst_mfxenc_get_property;

	g_object_class_install_property(gobject_class, PROP_CODEC_ID,
		g_param_spec_enum("codec-id", "Codec Id",
		"Codec ID", GST_TYPE_MFX_ENC_CODEC_ID,
		GST_MFX_ENC_CODEC_ID_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_CODEC_PROFILE,
		//g_param_spec_enum("codec-profile", "Codec profile",
		g_param_spec_enum("profile", "Codec profile",
		"Codec Profile", GST_TYPE_MFX_ENC_CODEC_PROFILE,
		GST_MFX_ENC_CODEC_PROFILE_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_CODEC_LEVEL,
		//g_param_spec_enum("codec-level", "Codec level",
		g_param_spec_enum("level", "Codec level",
		"Codec Level", GST_TYPE_MFX_ENC_CODEC_LEVEL,
		GST_MFX_ENC_CODEC_LEVEL_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	//新版Intel Media SDK已经弃用
	// 	g_object_class_install_property(gobject_class, PROP_NUM_THREAD,
	// 		g_param_spec_uint("num-thread", "Num thread",
	// 		"Num Thread", 0, G_MAXUINT,
	// 		GST_MFX_ENC_NUM_THREAD_DEFAULT,
	// 		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_TARGET_USAGE,
		g_param_spec_enum("target-usage", "Target usage",
		"Target Usage", GST_TYPE_MFX_ENC_TARGET_USAGE,
		GST_MFX_ENC_TARGET_USAGE_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_GOP_PIC_SIZE,
		//g_param_spec_uint("gop-pic-size", "Gop pic size",
		g_param_spec_uint("key-int-max", "Gop pic size",
		"Gop Pic Size", 0, G_MAXUINT,
		GST_MFX_ENC_GOP_PIC_SIZE_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_GOP_REF_DIST,
		//g_param_spec_uint("gop-ref-dist", "Gop ref dist",
		g_param_spec_uint("bframes", "Gop ref dist",
		"Gop Ref Dist", 0, G_MAXUINT,
		GST_MFX_ENC_GOP_REF_DIST_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_GOP_OPT_FLAG,
		g_param_spec_uint("gop-opt-flag", "Gop opt flag",
		"Gop Opt Flag", 0, G_MAXUINT,
		GST_MFX_ENC_GOP_OPT_FLAG_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
	//IDR帧间隔，考虑屏蔽掉，永远采用0
	g_object_class_install_property(gobject_class, PROP_IDR_INTERVAL,
		g_param_spec_uint("idr-interval", "IDR interval",
		"IDR Interval", 0, G_MAXUINT,
		GST_MFX_ENC_IDR_INTERVAL_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_RATE_CTL_METHOD,
		g_param_spec_enum("rate-ctl-method", "Rate ctl method",
		"Rate Ctl Method", GST_TYPE_MFX_ENC_RATE_CTL_METHOD,
		GST_MFX_ENC_RATE_CTL_METHOD_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_INIT_DELAY,
		g_param_spec_uint("init-delay", "Init delay",
		"Init Delay (in KB)", 0, G_MAXUINT,
		GST_MFX_ENC_INIT_DELAY_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_BITRATE,
		g_param_spec_uint("bitrate", "TargetKbps",
		"Bitrate (in Kbps)", 0, G_MAXUINT,
		GST_MFX_ENC_BITRATE_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_MAX_BITRATE,
		g_param_spec_uint("vbv-buf-capacity", "Max bitrate",
		"Max bitrate (in Kbps)", 0, G_MAXUINT,
		GST_MFX_ENC_MAX_BITRATE_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_QP_I,
		g_param_spec_uint("QPI", "Quantization Parameters for I",
		"Quantization Parameters for I frame", 0, G_MAXUINT,
		GST_MFX_ENC_QP_I_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_QP_P,
		g_param_spec_uint("QPP", "Quantization Parameters for P",
		"Quantization Parameters for P frame", 0, G_MAXUINT,
		GST_MFX_ENC_QP_P_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_QP_B,
		g_param_spec_uint("QPB", "Quantization Parameters for B",
		"Quantization Parameters for B frame", 0, G_MAXUINT,
		GST_MFX_ENC_QP_B_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
	
	g_object_class_install_property(gobject_class, PROP_AVBR_ACCURACY,
		g_param_spec_uint("accuracy", "Accuracy",
		"AVBR Accuracy", 0, G_MAXUINT,
		GST_MFX_ENC_ACCURACY_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_CONVERGENCE,
		g_param_spec_uint("convergence", "Convergence",
		"Convergence", 0, G_MAXUINT,
		GST_MFX_ENC_CONVERGENCE_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_ICQ_QUALITY,
		g_param_spec_uint("icq-quality", "ICQQuality",
		"ICQQuality", 0, G_MAXUINT,
		GST_MFX_ENC_ICQ_QUALITY_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_BUF_SIZE_IN_KB,
		g_param_spec_uint("BufferSizeInKB", "BufferSizeInKB",
		"BufferSizeInKB", 0, G_MAXUINT,
		GST_MFX_ENC_BUF_SIZE_IN_KB_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_NUM_SLICE,
		g_param_spec_uint("num-slice", "Num slice",
		"Num slice", 0, G_MAXUINT,
		GST_MFX_ENC_NUM_SLICE_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_NUM_REF_FRAME,
		g_param_spec_uint("ref", "Num ref frame",
		"Num Ref Frame", 0, G_MAXUINT,
		GST_MFX_ENC_NUM_REF_FRAME_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_ENCODED_ORDER,
		g_param_spec_uint("encoded-order", "Encoded order",
		"Encoded Order", 0, G_MAXUINT,
		GST_MFX_ENC_ENCODED_ORDER_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	g_object_class_install_property(gobject_class, PROP_BYTE_STREAM,
		g_param_spec_boolean("byte-stream", "Byte Stream",
		"Generate byte stream format of NALU", GST_MFX_ENC_ENCODED_BYTE_STREAM_DEFAULT,
		GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

// 	g_object_class_install_property(gobject_class, PROP_WIDTH,
// 		g_param_spec_uint("encoded-width", "Encoded width",
// 		"Encoded width", 0, G_MAXUINT,
// 		GST_MFX_ENC_WIDTH_DEFAULT,
// 		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
// 
// 	g_object_class_install_property(gobject_class, PROP_HEIGHT,
// 		g_param_spec_uint("encoded-height", "Encoded height",
// 		"Encoded height", 0, G_MAXUINT,
// 		GST_MFX_ENC_HEIGHT_DEFAULT,
// 		GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_factory));
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&src_factory));
	gst_element_class_set_static_metadata(element_class, "mfxenc", "Codec/Encoder/Image", "Encode images in NV12 format", "Wim Taymans <wim.taymans@tvd.be>");

	venc_class->start = gst_mfxenc_start;
	venc_class->stop = gst_mfxenc_stop;
	venc_class->set_format = gst_mfxenc_set_format;
	venc_class->handle_frame = gst_mfxenc_handle_frame;
//	venc_class->propose_allocation = gst_mfxenc_propose_allocation;

	GST_DEBUG_CATEGORY_INIT(mfxenc_debug, "mfxenc", 0, "NV12 encoding element");
}

static void gst_mfx_encoder_init(GstMFXEnc* mfxenc)
{
	mfxenc->mfxEncParams = new mfxVideoParam;
	memset(mfxenc->mfxEncParams, 0, sizeof(mfxVideoParam));

//	g_mutex_init(&mfxenc->ListLock);
}

static void gst_mfxenc_finalize(GObject* object)
{
	GstMFXEnc* mfxenc = GST_MFX_ENCODER(object);
	if (mfxenc->mfxEncParams)
	{
		delete mfxenc->mfxEncParams;
		mfxenc->mfxEncParams = nullptr;
	}	
// 	if (mfxenc->listVideoCodecFrames)
// 	{
// 		g_mutex_lock(&mfxenc->ListLock);
// 		g_list_remove_all()
// 	}
	
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static gboolean gst_mfxenc_set_format(GstVideoEncoder* encoder, GstVideoCodecState* state)
{
	GstMFXEnc* enc = GST_MFX_ENCODER(encoder);
	GstVideoInfo* info = &state->info;

	if (enc->input_state)
		gst_video_codec_state_unref(enc->input_state);
	enc->input_state = gst_video_codec_state_ref(state);

	enc->mfxEncParams->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
	enc->mfxEncParams->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
	enc->mfxEncParams->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
	enc->mfxEncParams->mfx.FrameInfo.FrameRateExtN = GST_VIDEO_INFO_FPS_N(info);
	enc->mfxEncParams->mfx.FrameInfo.FrameRateExtD = GST_VIDEO_INFO_FPS_D(info);
	enc->mfxEncParams->mfx.FrameInfo.CropX = 0;
	enc->mfxEncParams->mfx.FrameInfo.CropY = 0;
	enc->mfxEncParams->mfx.FrameInfo.CropW = GST_VIDEO_INFO_WIDTH(info);
	enc->mfxEncParams->mfx.FrameInfo.CropH = GST_VIDEO_INFO_HEIGHT(info);
	enc->mfxEncParams->mfx.FrameInfo.Width = MSDK_ALIGN16(GST_VIDEO_INFO_WIDTH(info));
	enc->mfxEncParams->mfx.FrameInfo.Height = (MFX_PICSTRUCT_PROGRESSIVE == enc->mfxEncParams->mfx.FrameInfo.PicStruct) ?
		MSDK_ALIGN16(GST_VIDEO_INFO_HEIGHT(info)) : MSDK_ALIGN32(GST_VIDEO_INFO_HEIGHT(info));
	
	enc->mfxEncParams->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

	GstVideoCodecState *output_state = gst_video_encoder_set_output_state(encoder, gst_caps_new_empty_simple("video/x-h264"), state);
	gst_video_codec_state_unref(output_state);

	return gst_mfx_encoder_allacate_surface(enc) && gst_mfx_encoder_open(enc);
}

static gboolean gst_mfx_encoder_open(GstMFXEnc* enc)
{	
	mfxStatus sts = enc->pMfxENC->Init(enc->mfxEncParams);
	if (sts == MFX_WRN_PARTIAL_ACCELERATION || sts < MFX_ERR_NONE)
	{
		GST_ERROR_OBJECT(enc, "SW is used or Init encoder failed");
		return GST_STATE_CHANGE_FAILURE;
	}
// 	if (enc->isByteStream)
// 	{
// 		enc->m_extSPSPPS = new mfxExtCodingOptionSPSPPS;
// 		memset(enc->m_extSPSPPS, 0, sizeof(mfxExtCodingOptionSPSPPS));
// 		enc->m_extSPSPPS->Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
// 		enc->m_extSPSPPS->Header.BufferSz = sizeof(mfxExtCodingOptionSPSPPS);
// 		enc->m_extSPSPPS->PPSBuffer = enc->PPSBuffer;
// 		enc->m_extSPSPPS->SPSBuffer = enc->SPSBuffer;
// 		enc->m_extSPSPPS->PPSBufSize = MAXSPSPPSBUFFERSIZE;
// 		enc->m_extSPSPPS->SPSBufSize = MAXSPSPPSBUFFERSIZE;
// 		enc->mfxEncParams->ExtParam = (mfxExtBuffer**)(&enc->m_extSPSPPS);
// 		enc->mfxEncParams->NumExtParam = 1;
// 	}
	mfxVideoParam par = { 0 };
	if (enc->pMfxENC->GetVideoParam(&par) < MFX_ERR_NONE)
	{
		GST_ERROR_OBJECT(enc, "Retrieve video parameters selected by encoder failed");
		return GST_STATE_CHANGE_FAILURE;
	}
// 	if (enc->isByteStream)
// 	{
// 		sts = enc->pMfxENC->Reset(enc->mfxEncParams);
// 	}
	
	enc->pMfxBitStream = new mfxBitstream;
	memset(enc->pMfxBitStream, 0, sizeof(mfxBitstream));
	enc->pMfxBitStream->MaxLength = par.mfx.BufferSizeInKB * 1000;
	enc->pMfxBitStream->Data = new mfxU8[enc->pMfxBitStream->MaxLength];
	memset(enc->pMfxBitStream->Data, 0, enc->pMfxBitStream->MaxLength);
	return true;
}

static gboolean gst_mfx_encoder_allacate_surface(GstMFXEnc* enc)
{
	mfxStatus sts = enc->pMfxENC->Query(enc->mfxEncParams, enc->mfxEncParams);
	MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
	if (sts < MFX_ERR_NONE)
	{
		GST_ERROR_OBJECT(enc, "query params failed, error code = %d", sts);
		return false;
	}
	mfxFrameAllocRequest EncRequest;
	memset(&EncRequest, 0, sizeof(EncRequest));
	sts = enc->pMfxENC->QueryIOSurf(enc->mfxEncParams, &EncRequest);
	if (sts < MFX_ERR_NONE)
	{
		GST_ERROR_OBJECT(enc, "query IO surface failed, error code = %d", sts);
		return false;
	}

	enc->nEncSurfNum = EncRequest.NumFrameSuggested;

	mfxU16 width = (mfxU16)MSDK_ALIGN32(EncRequest.Info.Width);
	mfxU16 height = (mfxU16)MSDK_ALIGN32(EncRequest.Info.Height);
	mfxU8  bitsPerPixel = 12;  // NV12 format is a 12 bits per pixel format
	mfxU32 surfaceSize = width * height * bitsPerPixel / 8;
	enc->surfaceBuffers = (mfxU8 *)new mfxU8[surfaceSize * enc->nEncSurfNum];

	enc->ppEncSurfaces = new mfxFrameSurface1*[enc->nEncSurfNum];
	if (!enc->ppEncSurfaces)
	{
		GST_ERROR_OBJECT(sts, "Failed to allocate memory");
		return false;
	}
	for (int i = 0; i < enc->nEncSurfNum; i++)
	{
		enc->ppEncSurfaces[i] = new mfxFrameSurface1;
		memset(enc->ppEncSurfaces[i], 0, sizeof(mfxFrameSurface1));
		memcpy(&(enc->ppEncSurfaces[i]->Info), &(enc->mfxEncParams->mfx.FrameInfo), sizeof(mfxFrameInfo));
		enc->ppEncSurfaces[i]->Data.Y = &enc->surfaceBuffers[surfaceSize * i];
		enc->ppEncSurfaces[i]->Data.U = enc->ppEncSurfaces[i]->Data.Y + width * height;
		enc->ppEncSurfaces[i]->Data.V = enc->ppEncSurfaces[i]->Data.U + 1;
		enc->ppEncSurfaces[i]->Data.Pitch = width;
	}
	return true;
}

static int gst_mfx_get_free_surface_index(mfxFrameSurface1** pSurfacesPool, mfxU16 nPoolSize)
{
	if (pSurfacesPool)
		for (mfxU16 i = 0; i < nPoolSize; i++)
			if (0 == pSurfacesPool[i]->Data.Locked)
				return i;
	return MFX_ERR_NOT_FOUND;
}

static gboolean gst_mfx_load_data(GstMFXEnc* mfxenc, GstVideoInfo *info, mfxFrameSurface1* pSurface, GstVideoCodecFrame* frame)
{
	GstVideoFrame vframe;
	if (!gst_video_frame_map(&vframe, info, frame->input_buffer, GST_MAP_READ))
	{
		GST_ERROR_OBJECT(mfxenc, "Failed to map video frame, caps problem?");
		return false;
	}
// 	pSurface->Data.Y = (mfxU8 *)vframe.data[0];
// 	pSurface->Data.UV = (mfxU8 *)vframe.data[1];
	memcpy(pSurface->Data.Y, vframe.data[0], GST_VIDEO_INFO_WIDTH(info) * GST_VIDEO_INFO_HEIGHT(info));
	memcpy(pSurface->Data.UV, vframe.data[1], GST_VIDEO_INFO_WIDTH(info) * GST_VIDEO_INFO_HEIGHT(info) / 2);
	pSurface->Data.TimeStamp = frame->pts;
	gst_video_frame_unmap(&vframe);
	return true;
}

static GstFlowReturn gst_mfxenc_handle_frame(GstVideoEncoder* encoder, GstVideoCodecFrame* frame)
{
	GstMFXEnc *mfxenc = GST_MFX_ENCODER(encoder);
// 	g_mutex_lock(&mfxenc->ListLock);
// 	mfxenc->listVideoCodecFrames = g_list_append(mfxenc->listVideoCodecFrames, frame);
// 	g_mutex_unlock(&mfxenc->ListLock);

	g_mutex_lock(&mfxenc->pGMutex);
	guint32 nEncSurfIdx = gst_mfx_get_free_surface_index(mfxenc->ppEncSurfaces, mfxenc->nEncSurfNum);
	if (nEncSurfIdx == MFX_ERR_NOT_FOUND)
	{
		GST_LOG_OBJECT(mfxenc, "the specified surface is not found");
		g_mutex_unlock(&mfxenc->pGMutex);
		return GST_FLOW_OK;
	}
	GstVideoInfo *info = &mfxenc->input_state->info;
	if (!gst_mfx_load_data(mfxenc, info, mfxenc->ppEncSurfaces[nEncSurfIdx], frame))
	{
		g_mutex_unlock(&mfxenc->pGMutex);
		return GST_FLOW_ERROR;
	}
	gst_video_codec_frame_unref(frame);
	mfxStatus sts = MFX_ERR_NONE;
	mfxSyncPoint syncp = { 0 };
	for (;;)
	{
		sts = mfxenc->pMfxENC->EncodeFrameAsync(NULL, mfxenc->ppEncSurfaces[nEncSurfIdx], mfxenc->pMfxBitStream, &syncp);
		if (MFX_ERR_NONE < sts && !syncp)
		{
			if (MFX_WRN_DEVICE_BUSY == sts)
				g_usleep(10);
		}
		else if (MFX_ERR_NONE < sts && syncp)
		{
			sts = MFX_ERR_NONE;
			GST_LOG_OBJECT(mfxenc, "Ignore warnings if output is available");
			break;
		}
		else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
		{
			if (!gst_mfx_extend_buffer(mfxenc))
			{
				GST_ERROR_OBJECT(mfxenc, "extend memory failed");
				g_mutex_unlock(&mfxenc->pGMutex);
				return GST_FLOW_ERROR;
			}
		}
		else if (MFX_ERR_NONE > sts)
		{
			g_mutex_unlock(&mfxenc->pGMutex);
			if (sts == MFX_ERR_MORE_DATA)
				return GST_FLOW_OK;
			else
			{
				GST_WARNING_OBJECT(mfxenc, "encode hapend some error");
				return GST_FLOW_ERROR;
			}
		}
		else if (MFX_ERR_NONE == sts)
			break;
		else
		{
			GST_ERROR_OBJECT(mfxenc, "Unknow error, code = %d", sts);
			return GST_FLOW_OK;
		}
	}
	if ((sts = mfxenc->mfxSession->SyncOperation(syncp, 6000)) < MFX_ERR_NONE)
	{
		GST_ERROR_OBJECT(mfxenc, "Synchronize failed");
		g_mutex_unlock(&mfxenc->pGMutex);
		return GST_FLOW_FLUSHING;
	}
	GstBuffer* pBuffer = gst_buffer_new_and_alloc(mfxenc->pMfxBitStream->DataLength);
	if (!pBuffer)
	{
		GST_ERROR_OBJECT(mfxenc, "allocation buffer failed");
		g_mutex_unlock(&mfxenc->pGMutex);
		return GST_FLOW_ERROR;
	}
	GstMapInfo map = { 0 };
	if (!gst_buffer_map(pBuffer, &map, (GstMapFlags)GST_MAP_READWRITE))
	{
		GST_ERROR_OBJECT(mfxenc, "map buffer failed");
		gst_buffer_unref(pBuffer);
		g_mutex_unlock(&mfxenc->pGMutex);
		return GST_FLOW_ERROR;
	}
	memcpy(map.data, mfxenc->pMfxBitStream->Data, mfxenc->pMfxBitStream->DataLength);
	GST_BUFFER_PTS(pBuffer) = mfxenc->pMfxBitStream->TimeStamp;
	GST_BUFFER_DTS(pBuffer) = mfxenc->pMfxBitStream->DecodeTimeStamp;
	GST_BUFFER_OFFSET(pBuffer) = mfxenc->pMfxBitStream->DataOffset;
	gst_buffer_unmap(pBuffer, &map);
	mfxenc->pMfxBitStream->DataLength = 0;
// 	g_mutex_lock(&mfxenc->ListLock);
// 	GList* plFirst = g_list_first(mfxenc->listVideoCodecFrames);
// 	GstVideoCodecFrame* pFrame = (GstVideoCodecFrame*)plFirst->data;
// 	mfxenc->listVideoCodecFrames = g_list_remove_link(mfxenc->listVideoCodecFrames, plFirst);
// 	g_mutex_unlock(&mfxenc->ListLock);
	GstVideoCodecFrame* pFrame = gst_video_encoder_get_oldest_frame(encoder);
	if (pFrame->pts != GST_BUFFER_PTS(pBuffer))
		GST_WARNING_OBJECT(mfxenc, "pFrame's pts is not equality buffer'pts");
	pFrame->output_buffer = pBuffer;
	GstFlowReturn ret = gst_video_encoder_finish_frame(encoder, pFrame);
	if (ret != GST_FLOW_OK)
	{
		g_mutex_unlock(&mfxenc->pGMutex);
		return ret;
	}
	g_mutex_unlock(&mfxenc->pGMutex);
	return GST_FLOW_OK;
}

static bool gst_mfx_extend_buffer(GstMFXEnc *mfxenc)
{
	mfxU8* pData = mfxenc->pMfxBitStream->Data;
	mfxU8* pDataNew = new mfxU8[mfxenc->pMfxBitStream->MaxLength * 2];
	memset(pDataNew, 0, mfxenc->pMfxBitStream->MaxLength * 2);
	memmove(pDataNew, mfxenc->pMfxBitStream->Data + mfxenc->pMfxBitStream->DataOffset, mfxenc->pMfxBitStream->MaxLength);
	mfxenc->pMfxBitStream->Data = pDataNew;
	delete[] pData;
	mfxenc->pMfxBitStream->MaxLength *= 2;
	mfxenc->pMfxBitStream->DataOffset = 0;
	return true;
}

static gboolean gst_mfxenc_propose_allocation(GstVideoEncoder* encoder, GstQuery* query)
{
	gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);

	return GST_VIDEO_ENCODER_CLASS(parent_class)->propose_allocation(encoder, query);
}

static void gst_mfxenc_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
	GstMFXEnc *mfxenc = GST_MFX_ENCODER(object);
	GST_OBJECT_LOCK(mfxenc);
	switch (prop_id) {
	case PROP_CODEC_ID:
		mfxenc->mfxEncParams->mfx.CodecId = g_value_get_enum(value);
		break;
	case PROP_CODEC_PROFILE:
		mfxenc->mfxEncParams->mfx.CodecProfile = g_value_get_enum(value);
		break;
	case PROP_CODEC_LEVEL:
		mfxenc->mfxEncParams->mfx.CodecLevel = g_value_get_enum(value);
		break;
	case PROP_TARGET_USAGE:
		mfxenc->mfxEncParams->mfx.TargetUsage = g_value_get_enum(value);
		break;
	case PROP_GOP_PIC_SIZE:
		mfxenc->mfxEncParams->mfx.GopPicSize = g_value_get_uint(value);
		break;
	case PROP_GOP_REF_DIST:
		//此参数实际上可以控制B帧数，与x264enc的bframes一样的效果，但是后者0表示无B帧，此处1表示0个B帧，所以此处+1
		mfxenc->mfxEncParams->mfx.GopRefDist = g_value_get_uint(value) + 1;
		break;
	case PROP_GOP_OPT_FLAG:
		mfxenc->mfxEncParams->mfx.GopOptFlag = g_value_get_uint(value);
		break;
	case PROP_IDR_INTERVAL:
		mfxenc->mfxEncParams->mfx.IdrInterval = g_value_get_uint(value);
		break;
	case PROP_RATE_CTL_METHOD:
		mfxenc->mfxEncParams->mfx.RateControlMethod = g_value_get_enum(value);
		break;
	case PROP_INIT_DELAY:
		//InitialDelayInKB可能和vbv-buf-capacity是一样的作用，但是前者是kbps后者是KBps
		mfxenc->mfxEncParams->mfx.InitialDelayInKB = g_value_get_uint(value);
		break;
	case PROP_BITRATE:
		mfxenc->mfxEncParams->mfx.TargetKbps = g_value_get_uint(value);
		break;
	case PROP_MAX_BITRATE:
		mfxenc->mfxEncParams->mfx.MaxKbps = g_value_get_uint(value);
		break;
	case PROP_QP_I:
		mfxenc->mfxEncParams->mfx.QPI = g_value_get_uint(value);
		break;
	case PROP_QP_P:
		mfxenc->mfxEncParams->mfx.QPP = g_value_get_uint(value);
		break;
	case PROP_QP_B:
		mfxenc->mfxEncParams->mfx.QPB = g_value_get_uint(value);
		break;
	case PROP_AVBR_ACCURACY:
		mfxenc->mfxEncParams->mfx.Accuracy = g_value_get_uint(value);
		break;
	case PROP_CONVERGENCE:
		mfxenc->mfxEncParams->mfx.Convergence = g_value_get_uint(value);
		break;
	case PROP_ICQ_QUALITY:
		mfxenc->mfxEncParams->mfx.ICQQuality = g_value_get_uint(value);
		break;
	case PROP_BUF_SIZE_IN_KB:
		mfxenc->mfxEncParams->mfx.BufferSizeInKB = g_value_get_uint(value);
		break;
	case PROP_NUM_SLICE:
		mfxenc->mfxEncParams->mfx.NumSlice = g_value_get_uint(value);
		break;
	case PROP_NUM_REF_FRAME:
		mfxenc->mfxEncParams->mfx.NumRefFrame = g_value_get_uint(value);
		break;
	case PROP_ENCODED_ORDER:
		mfxenc->mfxEncParams->mfx.EncodedOrder = g_value_get_uint(value);
		break;
	case PROP_BYTE_STREAM:
		mfxenc->isByteStream = g_value_get_boolean(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
	GST_OBJECT_UNLOCK(mfxenc);
}

static void gst_mfxenc_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
	GstMFXEnc* mfxenc = GST_MFX_ENCODER(object);
	GST_OBJECT_LOCK(mfxenc);
	switch (prop_id) {
	case PROP_CODEC_ID:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.CodecId);
		break;
	case PROP_CODEC_PROFILE:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.CodecProfile);
		break;
	case PROP_CODEC_LEVEL:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.CodecLevel);
		break;
	case PROP_TARGET_USAGE:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.TargetUsage);
		break;
	case PROP_GOP_PIC_SIZE:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.GopPicSize);
		break;
	case PROP_GOP_REF_DIST:
		//理由同上
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.GopRefDist - 1);
		break;
	case PROP_GOP_OPT_FLAG:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.GopOptFlag);
		break;
	case PROP_IDR_INTERVAL:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.IdrInterval);
		break;
	case PROP_RATE_CTL_METHOD:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.RateControlMethod);
		break;
	case PROP_INIT_DELAY:
		//理由同上
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.InitialDelayInKB);
		break;
	case PROP_BITRATE:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.TargetKbps);
		break;
	case PROP_MAX_BITRATE:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.MaxKbps);
		break;
	case PROP_QP_I:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.QPI);
		break;
	case PROP_QP_P:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.QPP);
		break;
	case PROP_QP_B:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.QPB);
		break;
	case PROP_AVBR_ACCURACY:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.Accuracy);
		break;
	case PROP_CONVERGENCE:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.Convergence);
		break;
	case PROP_ICQ_QUALITY:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.ICQQuality);
		break;
	case PROP_BUF_SIZE_IN_KB:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.BufferSizeInKB);
		break;
	case PROP_NUM_SLICE:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.NumSlice);
		break;
	case PROP_NUM_REF_FRAME:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.NumRefFrame);
		break;
	case PROP_ENCODED_ORDER:
		g_value_set_uint(value, mfxenc->mfxEncParams->mfx.EncodedOrder);
		break;
	case PROP_BYTE_STREAM:
		g_value_set_boolean(value, mfxenc->isByteStream);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
	GST_OBJECT_UNLOCK(mfxenc);
}

static gboolean gst_mfxenc_start(GstVideoEncoder* benc)
{
	GstMFXEnc*enc = (GstMFXEnc*)benc;
	mfxVersion ver = { 0, 1 };
	mfxIMPL impl = MFX_IMPL_HARDWARE_ANY;
	mfxStatus sts = MFX_ERR_NONE;
	enc->mfxSession = new MFXVideoSession;
	sts = enc->mfxSession->Init(impl, &ver);
	if (sts < MFX_ERR_NONE)
	{
		GST_ERROR_OBJECT(enc, "Init intel session failed, error code : %d", sts);
		return false;
	}
	sts = enc->mfxSession->QueryIMPL(&impl);
	if (sts < MFX_ERR_NONE)
	{
		GST_WARNING_OBJECT(enc, "Could not query impl");
	}
	GST_LOG_OBJECT(enc, "Current IMPL = %d", impl);
	sts = enc->mfxSession->QueryVersion(&ver);
	if (sts < MFX_ERR_NONE)
	{
		GST_WARNING_OBJECT(enc, "Could not query version");
	}
	GST_LOG_OBJECT(enc, "Current version %d.%d", ver.Major, ver.Minor);

	enc->pMfxENC = new MFXVideoENCODE(*enc->mfxSession);
	if (!enc->pMfxENC)
	{
		GST_ERROR_OBJECT(enc, "create encoder failed");
		return FALSE;
	}
	g_mutex_init(&enc->pGMutex);
	return TRUE;
}

static gboolean gst_mfxenc_stop(GstVideoEncoder* benc)
{
	GstMFXEnc*enc = (GstMFXEnc*)benc;
	g_mutex_lock(&enc->pGMutex);
	if (enc->pMfxENC)
	{
		enc->pMfxENC->Close();
		delete enc->pMfxENC;
		enc->pMfxENC = nullptr;
	}
	if (enc->mfxSession)
	{
		enc->mfxSession->Close();
		delete enc->mfxSession;
		enc->mfxSession = nullptr;
	}
	for (int i = 0; i < enc->nEncSurfNum; i++)
		delete enc->ppEncSurfaces[i];
	if (enc->input_state)
	{
		gst_video_codec_state_unref(enc->input_state);
		enc->input_state = nullptr;
	}
	if (enc->pMfxBitStream)
	{
		delete enc->pMfxBitStream;
		enc->pMfxBitStream = nullptr;
	}
	g_mutex_unlock(&enc->pGMutex);
	g_mutex_clear(&enc->pGMutex);
	
	return TRUE;
}

gboolean gst_mfx_encoder_plugin_init(GstPlugin * plugin)
{
	return gst_element_register(plugin, "mfxenc", GST_RANK_PRIMARY, GST_TYPE_MFX_ENCODER);
}