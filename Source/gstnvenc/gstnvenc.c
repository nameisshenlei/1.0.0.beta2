#include "gstnvenc.h"
#include "memory.h"
#include "string.h"

GST_DEBUG_CATEGORY_STATIC(nvenc_debug);
#define GST_CAT_DEFAULT nvenc_debug

#define SET_VER(configStruct, type) {configStruct.version = type##_VER;}

typedef NVENCSTATUS(NVENCAPI *MYPROC)(NV_ENCODE_API_FUNCTION_LIST*);

enum
{
	/* FILL ME */
	LAST_SIGNAL
};

enum
{
	PROP_ZERO,
	PROP_PRESET_GUID,
	PROP_GOP_PIC_SIZE,
	PROP_B_FRAMS,
	PROP_RATE_MODE,
	PROP_CONST_QP_I,
	PROP_CONST_QP_P,
	PROP_CONST_QP_B,
	PROP_ADAPTIVE_QP,
	PROP_EXT_QP,
	PROP_MIN_QP,
	PROP_MIN_QP_I,
	PROP_MIN_QP_P,
	PROP_MIN_QP_B,
	PROP_MAX_QP,
	PROP_MAX_QP_I,
	PROP_MAX_QP_P,
	PROP_MAX_QP_B,
	PROP_INITIAL_QP,
	PROP_INITIAL_RCQP_I,
	PROP_INITIAL_RCQP_P,
	PROP_INITIAL_RCQP_B,
	PROP_BIT_RATE,
	RROP_MAX_BIT_RATE,
	PROP_VBV_SIZE,
	PROP_VBV_DELAY,
	PROP_REF_FRAME,
	PROP_PROFILE,
};

#define GST_NV_ENC_PRESET_GUID_DEFAULT		2
#define GST_NV_ENC_GOP_PIC_SIZE_DEFAULT		30
#define GST_NV_ENC_B_FRAMS_DEFAULT			0
#define GST_NV_ENC_RATE_MODE_DEFAULT		NV_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP
#define GST_NV_ENC_CONST_QP_I_DEFAULT		0
#define GST_NV_ENC_CONST_QP_P_DEFAULT		0
#define GST_NV_ENC_CONST_QP_B_DEFAULT		0
#define GST_NV_ENC_ADAPTIVE_QP_DEFAULT		FALSE
#define GST_NV_ENC_EXT_QP_DEFAULT			FALSE
#define GST_NV_ENC_MIN_QP_I_DEFAULT			0
#define GST_NV_ENC_MIN_QP_P_DEFAULT			0
#define GST_NV_ENC_MIN_QP_B_DEFAULT			0
#define GST_NV_ENC_MAX_QP_I_DEFAULT			0
#define GST_NV_ENC_MAX_QP_P_DEFAULT			0
#define GST_NV_ENC_MAX_QP_B_DEFAULT			0
#define GST_NV_ENC_INITIAL_QP_DEFAULT		0
#define GST_NV_ENC_MAX_QP_DEFAULT			0
#define GST_NV_ENC_MIN_QP_DEFAULT			0
#define GST_NV_ENC_INITIAL_QP_I_DEFAULT		0
#define GST_NV_ENC_INITIAL_QP_P_DEFAULT		0
#define GST_NV_ENC_INITIAL_QP_B_DEFAULT		0
#define GST_NV_ENC_BIT_RATE_DEFAULT			2048
#define GST_NV_ENC_VBV_SIZE_DEFAULT			0
#define GST_NV_ENC_VBV_DELAY_DEFAULT		0
#define GST_NV_ENC_REF_FRAME_DEFAULT		1
#define GST_NV_ENC_PROFILE_DEFAULT			2

static GUID PresetGUIDs[8] = { 0 };
static GUID ProfileGUIDs[8] = { 0 };

static gboolean gst_nv_enc_init_cuda(GstNVEnc* nvenc);
static gboolean gst_nv_enc_validate_preset_guid(GstNVEnc* nvenc, GUID inputPresetGUID, GUID inputCodecGUID);
static gboolean gst_nv_enc_validate_encode_guid(GstNVEnc* nvenc, GUID inputCodecGUID);
static inline gboolean compareGUIDs(GUID guid1, GUID guid2);
static gboolean gst_nv_enc_allocation_buffers(GstNVEnc* nvenc);
static gboolean gst_nv_enc_load_data(GstNVEnc* nvenc, GstVideoCodecFrame* frame, EncodeBuffer* pEncodeBuffer);
static GstFlowReturn gst_nv_enc_finish_frame(GstVideoEncoder* object, GstVideoCodecFrame* frame);

static void gst_nv_enc_finalize(GObject* object);
static void gst_nv_enc_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_nv_enc_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static gboolean gst_nv_enc_set_format(GstVideoEncoder* encoder, GstVideoCodecState* state);
static GstFlowReturn gst_nv_enc_handle_frame(GstVideoEncoder* encoder, GstVideoCodecFrame* frame);
static gboolean gst_nv_enc_start(GstVideoEncoder* benc);
static gboolean gst_nv_enc_stop(GstVideoEncoder* benc);

#define gst_nv_encoder_parent_class parent_class
G_DEFINE_TYPE(GstNVEnc, gst_nv_encoder, GST_TYPE_VIDEO_ENCODER);

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
	"stream-format = (string) { byte-stream }, "
	"alignment = (string) { au }")
	);

static void gst_nv_encoder_class_init(GstNVEncClass* klass)
{
	GObjectClass* gobject_class;
	GstElementClass* element_class;
	GstVideoEncoderClass* venc_class;

	gobject_class = (GObjectClass*)klass;
	element_class = (GstElementClass*)klass;
	venc_class = (GstVideoEncoderClass*)klass;

	parent_class = g_type_class_peek_parent(klass);

	gobject_class->finalize = gst_nv_enc_finalize;
	gobject_class->set_property = gst_nv_enc_set_property;
	gobject_class->get_property = gst_nv_enc_get_property;

	g_object_class_install_property(gobject_class, PROP_PRESET_GUID,
		g_param_spec_uint("preset", "preset GUIDs",
		"preset GUIDs", 0, G_MAXUINT,
		GST_NV_ENC_PRESET_GUID_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_GOP_PIC_SIZE,
		g_param_spec_uint("key-int-max", "Gop pic size",
		"Gop pic size", 0, G_MAXUINT,
		GST_NV_ENC_GOP_PIC_SIZE_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_B_FRAMS,
		g_param_spec_uint("bframes", "b frame count",
		"b frame count", 0, G_MAXUINT,
		GST_NV_ENC_B_FRAMS_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_RATE_MODE,
		g_param_spec_uint("pass", "Rate Control Modes",
		"Rate Control Modes", 0, G_MAXUINT,
		GST_NV_ENC_RATE_MODE_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_CONST_QP_I,
		g_param_spec_uint("constQP-I", "Const Quantization Parameters for I",
		"Const Quantization Parameters for I frame", 0, G_MAXUINT,
		GST_NV_ENC_CONST_QP_I_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_CONST_QP_P,
		g_param_spec_uint("constQP-P", "Const Quantization Parameters for P",
		"Const Quantization Parameters for P frame", 0, G_MAXUINT,
		GST_NV_ENC_CONST_QP_P_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_CONST_QP_B,
		g_param_spec_uint("constQP-B", "Const Quantization Parameters for B",
		"Const Quantization Parameters for B frame", 0, G_MAXUINT,
		GST_NV_ENC_CONST_QP_B_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_ADAPTIVE_QP,
		g_param_spec_boolean("AdaptiveQP", "Adaptive quantization",
		"Adaptive quantization",
		GST_NV_ENC_ADAPTIVE_QP_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_EXT_QP,
		g_param_spec_boolean("extQP", "Additional QP modifier",
		"Additional QP modifier",
		GST_NV_ENC_EXT_QP_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_MIN_QP,
		g_param_spec_boolean("minQP", "Additional minQP modifier",
		"Additional minQP modifier",
		GST_NV_ENC_EXT_QP_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_MIN_QP_I,
		g_param_spec_uint("minQP-I", "Min Quantization Parameters for I",
		"Min Quantization Parameters for I frame", 0, G_MAXUINT,
		GST_NV_ENC_MIN_QP_I_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_MIN_QP_P,
		g_param_spec_uint("minQP-P", "Min Quantization Parameters for P",
		"Min Quantization Parameters for P frame", 0, G_MAXUINT,
		GST_NV_ENC_MIN_QP_P_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_MIN_QP_B,
		g_param_spec_uint("minQP-B", "Min Quantization Parameters for B",
		"Min Quantization Parameters for I frame", 0, G_MAXUINT,
		GST_NV_ENC_MIN_QP_B_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_MAX_QP,
		g_param_spec_boolean("maxQP", "Additional maxQP modifier",
		"Additional maxQP modifier",
		GST_NV_ENC_EXT_QP_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_MAX_QP_I,
		g_param_spec_uint("maxQP-I", "Max Quantization Parameters for I",
		"Max Quantization Parameters for I frame", 0, G_MAXUINT,
		GST_NV_ENC_MAX_QP_I_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_MAX_QP_P,
		g_param_spec_uint("maxQP-P", "Max Quantization Parameters for P",
		"Max Quantization Parameters for P frame", 0, G_MAXUINT,
		GST_NV_ENC_MAX_QP_P_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_MAX_QP_B,
		g_param_spec_uint("maxQP-B", "Max Quantization Parameters for B",
		"Max Quantization Parameters for B frame", 0, G_MAXUINT,
		GST_NV_ENC_MAX_QP_B_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_INITIAL_QP,
		g_param_spec_boolean("initialQP", "Additional initialQP modifier",
		"Additional initialQP modifier",
		GST_NV_ENC_EXT_QP_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_INITIAL_RCQP_I,
		g_param_spec_uint("initialQP-I", "Initial Quantization Parameters for I",
		"Initial Quantization Parameters for I frame", 0, G_MAXUINT,
		GST_NV_ENC_INITIAL_QP_I_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_INITIAL_RCQP_P,
		g_param_spec_uint("initialQP-P", "Initial Quantization Parameters for P",
		"Initial Quantization Parameters for P frame", 0, G_MAXUINT,
		GST_NV_ENC_INITIAL_QP_P_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_INITIAL_RCQP_B,
		g_param_spec_uint("initialQP-B", "Initial Quantization Parameters for B",
		"Initial Quantization Parameters for B frame", 0, G_MAXUINT,
		GST_NV_ENC_INITIAL_QP_B_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_BIT_RATE,
		g_param_spec_uint("bitrate", "bitrate in bits/sec",
		"bitrate in bits/sec", 0, G_MAXUINT,
		GST_NV_ENC_BIT_RATE_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, RROP_MAX_BIT_RATE,
		g_param_spec_uint("max-bitrate", "max bitrate in bits/sec",
		"max bitrate in bits/sec", 0, G_MAXUINT,
		GST_NV_ENC_BIT_RATE_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_VBV_SIZE,
		g_param_spec_uint("vbv-size", "VBV(HRD) buffer size",
		"VBV(HRD) buffer size", 0, G_MAXUINT,
		GST_NV_ENC_VBV_SIZE_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_VBV_DELAY,
		g_param_spec_uint("vbv-delay", "VBV(HRD) initial delay in bits",
		"VBV(HRD) initial delay in bits", 0, G_MAXUINT,
		GST_NV_ENC_VBV_DELAY_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_PROFILE,
		g_param_spec_uint("profile", "profile",
		"profile", 0, G_MAXUINT,
		GST_NV_ENC_PROFILE_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class, PROP_REF_FRAME,
		g_param_spec_uint("ref", "Num ref frame",
		"Num ref frame", 0, G_MAXUINT,
		GST_NV_ENC_REF_FRAME_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_factory));
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&src_factory));
	gst_element_class_set_static_metadata(element_class, "mfxenc", "Codec/Encoder/Image", "Encode images in NV12 format", "Wim Taymans <wim.taymans@tvd.be>");

	venc_class->set_format = gst_nv_enc_set_format;
	venc_class->handle_frame = gst_nv_enc_handle_frame;
	venc_class->start = gst_nv_enc_start;
	venc_class->stop = gst_nv_enc_stop;

	GST_DEBUG_CATEGORY_INIT(nvenc_debug, "nvenc", 0, "NV12 encoding element");
}

static void gst_nv_encoder_init(GstNVEnc *self)
{
	self->m_pCreateEncodeParams = malloc(sizeof(NV_ENC_INITIALIZE_PARAMS));
	memset(self->m_pCreateEncodeParams, 0, sizeof(NV_ENC_INITIALIZE_PARAMS));
	NV_ENC_INITIALIZE_PARAMS p = { 0 };
	SET_VER(p, NV_ENC_INITIALIZE_PARAMS);
	memcpy(self->m_pCreateEncodeParams, &p, sizeof(NV_ENC_INITIALIZE_PARAMS));

	self->m_pEncodeConfig = malloc(sizeof(NV_ENC_CONFIG));
	memset(self->m_pEncodeConfig, 0, sizeof(NV_ENC_CONFIG));
	NV_ENC_CONFIG c = { 0 };
	SET_VER(c, NV_ENC_CONFIG);
	memcpy(self->m_pEncodeConfig, &c, sizeof(NV_ENC_CONFIG));

	self->m_pEncodeBufferQueue = malloc(sizeof(EncodeBuffer));
	memset(self->m_pEncodeBufferQueue, 0, sizeof(EncodeBuffer));

	PresetGUIDs[0] = NV_ENC_PRESET_DEFAULT_GUID;
	PresetGUIDs[1] = NV_ENC_PRESET_HP_GUID;
	PresetGUIDs[2] = NV_ENC_PRESET_HQ_GUID;
	PresetGUIDs[3] = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
	PresetGUIDs[4] = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
	PresetGUIDs[5] = NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
	PresetGUIDs[6] = NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID;
	PresetGUIDs[7] = NV_ENC_PRESET_LOSSLESS_HP_GUID;

	ProfileGUIDs[0] = NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID;
	ProfileGUIDs[1] = NV_ENC_H264_PROFILE_BASELINE_GUID;
	ProfileGUIDs[2] = NV_ENC_H264_PROFILE_MAIN_GUID;
	ProfileGUIDs[3] = NV_ENC_H264_PROFILE_HIGH_GUID;
	ProfileGUIDs[4] = NV_ENC_H264_PROFILE_HIGH_444_GUID;
	ProfileGUIDs[5] = NV_ENC_H264_PROFILE_STEREO_GUID;
	ProfileGUIDs[6] = NV_ENC_H264_PROFILE_SVC_TEMPORAL_SCALABILTY;
	ProfileGUIDs[7] = NV_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID;
}

static void gst_nv_enc_finalize(GObject* object)
{
	GstNVEnc* nvenc = GST_NV_ENCODER(object);
	if (nvenc->m_pCreateEncodeParams)
	{
		free(nvenc->m_pCreateEncodeParams);
		nvenc->m_pCreateEncodeParams = NULL;
	}
	if (nvenc->m_pEncodeConfig)
	{
		free(nvenc->m_pEncodeConfig);
		nvenc->m_pEncodeConfig = NULL;
	}
	if (nvenc->m_pEncodeBufferQueue)
	{
		free(nvenc->m_pEncodeBufferQueue);
		nvenc->m_pEncodeBufferQueue = NULL;
	}
	cuCtxDestroy(nvenc->m_pDevice);
	nvenc->m_pDevice = NULL;
}

static void gst_nv_enc_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
	GstNVEnc* nvenc = GST_NV_ENCODER(object);
	GST_OBJECT_LOCK(nvenc);
	switch (prop_id)
	{
	case PROP_PRESET_GUID:
		//nvenc->m_pCreateEncodeParams->presetGUID = PresetGUIDs[g_value_get_uint(value)];
		nvenc->m_nvParams.nvPreset = g_value_get_uint(value);
		break;
	case PROP_GOP_PIC_SIZE:
		//nvenc->m_pEncodeConfig->gopLength = g_value_get_uint(value);
		nvenc->m_nvParams.nvGOP = g_value_get_uint(value);
		break;
	case PROP_B_FRAMS:
		nvenc->m_uEncodeBufferCount = g_value_get_uint(value) + 4;
		//nvenc->m_pEncodeConfig->frameIntervalP = g_value_get_uint(value) + 1;
		nvenc->m_nvParams.nvBFrames = g_value_get_uint(value) + 1;
		break;
	case PROP_RATE_MODE:
		//nvenc->m_pEncodeConfig->rcParams.rateControlMode = g_value_get_uint(value);
		nvenc->m_nvParams.nvPass = g_value_get_uint(value);
		break;
	case PROP_CONST_QP_I:
		//nvenc->m_pEncodeConfig->rcParams.constQP.qpIntra = g_value_get_uint(value);
		nvenc->m_nvParams.nvConstQP.qpIntra = g_value_get_uint(value);
		break;
	case PROP_CONST_QP_P:
		//nvenc->m_pEncodeConfig->rcParams.constQP.qpInterP = g_value_get_uint(value);
		nvenc->m_nvParams.nvConstQP.qpInterP = g_value_get_uint(value);
		break;
	case PROP_CONST_QP_B:
		//nvenc->m_pEncodeConfig->rcParams.constQP.qpInterB = g_value_get_uint(value);
		nvenc->m_nvParams.nvConstQP.qpInterB = g_value_get_uint(value);
		break;
	case PROP_ADAPTIVE_QP:
		//nvenc->m_pEncodeConfig->rcParams.enableAQ = g_value_get_boolean(value);
		nvenc->m_nvParams.nvEnableAQ = g_value_get_boolean(value);
		break;
	case PROP_EXT_QP:
		nvenc->m_nvParams.nvEnableExtQPDeltaMap = g_value_get_boolean(value);
		break;
	case PROP_MIN_QP:
		nvenc->m_nvParams.nvEnableMinQP = g_value_get_boolean(value);
		break;
	case PROP_MIN_QP_I:
		nvenc->m_nvParams.nvMinQP.qpIntra = g_value_get_uint(value);
		break;
	case PROP_MIN_QP_P:
		nvenc->m_nvParams.nvMinQP.qpInterP = g_value_get_uint(value);
		break;
	case PROP_MIN_QP_B:
		nvenc->m_nvParams.nvMinQP.qpInterB = g_value_get_uint(value);
		break;
	case PROP_MAX_QP:
		nvenc->m_nvParams.nvEnableMaxQP = g_value_get_boolean(value);
		break;
	case PROP_MAX_QP_I:
		nvenc->m_nvParams.nvMaxQP.qpIntra = g_value_get_uint(value);
		break;
	case PROP_MAX_QP_P:
		nvenc->m_nvParams.nvMaxQP.qpInterP = g_value_get_uint(value);
		break;
	case PROP_MAX_QP_B:
		nvenc->m_nvParams.nvMaxQP.qpInterB = g_value_get_uint(value);
		break;
	case PROP_INITIAL_QP:
		nvenc->m_nvParams.nvEnableInitialRCQP = g_value_get_boolean(value);
		break;
	case PROP_INITIAL_RCQP_I:
		nvenc->m_nvParams.nvInitialRCQP.qpIntra = g_value_get_uint(value);
		break;
	case PROP_INITIAL_RCQP_P:
		nvenc->m_nvParams.nvInitialRCQP.qpInterP = g_value_get_uint(value);
		break;
	case PROP_INITIAL_RCQP_B:
		nvenc->m_nvParams.nvInitialRCQP.qpInterB = g_value_get_uint(value);
		break;
	case PROP_BIT_RATE:
		//nvenc->m_pEncodeConfig->rcParams.averageBitRate = g_value_get_uint(value) * 1024;
		//nvenc->m_pEncodeConfig->rcParams.maxBitRate = g_value_get_uint(value) * 1024 * 1.2;
		nvenc->m_nvParams.nvBitRate = g_value_get_uint(value) * 1000;
		break;
	case RROP_MAX_BIT_RATE:
		nvenc->m_nvParams.nvMaxBitRate = g_value_get_uint(value) * 1000;
		break;
	case PROP_VBV_SIZE:
		//nvenc->m_pEncodeConfig->rcParams.vbvBufferSize = g_value_get_uint(value);
		nvenc->m_nvParams.nvVBVSize = g_value_get_uint(value) * 1000;
		break;
	case PROP_VBV_DELAY:
		//nvenc->m_pEncodeConfig->rcParams.vbvInitialDelay = g_value_get_uint(value);
		nvenc->m_nvParams.nvVBVDelay = g_value_get_uint(value) * 1000;
		break;
	case PROP_PROFILE:
		//nvenc->m_pEncodeConfig->profileGUID = ProfileGUIDs[g_value_get_uint(value)];
		nvenc->m_nvParams.nvProfile = g_value_get_uint(value);
		break;
	case PROP_REF_FRAME:
		nvenc->m_nvParams.nvRef = g_value_get_uint(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
	GST_OBJECT_UNLOCK(nvenc);
}

static inline gboolean gst_nv_enc_compare_guids(GUID guid1, GUID guid2)
{
	if (guid1.Data1 == guid2.Data1 &&
		guid1.Data2 == guid2.Data2 &&
		guid1.Data3 == guid2.Data3 &&
		guid1.Data4[0] == guid2.Data4[0] &&
		guid1.Data4[1] == guid2.Data4[1] &&
		guid1.Data4[2] == guid2.Data4[2] &&
		guid1.Data4[3] == guid2.Data4[3] &&
		guid1.Data4[4] == guid2.Data4[4] &&
		guid1.Data4[5] == guid2.Data4[5] &&
		guid1.Data4[6] == guid2.Data4[6] &&
		guid1.Data4[7] == guid2.Data4[7])
	{
		return TRUE;
	}

	return FALSE;
}

static void gst_nv_enc_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
	GstNVEnc* nvenc = GST_NV_ENCODER(object);
	GST_OBJECT_LOCK(nvenc);
	switch (prop_id)
	{
	case PROP_PRESET_GUID:
		g_value_set_uint(value, nvenc->m_nvParams.nvPreset);
		break;
	case PROP_GOP_PIC_SIZE:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->gopLength);
		g_value_set_uint(value, nvenc->m_nvParams.nvGOP);
		break;
	case PROP_B_FRAMS:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->frameIntervalP - 1);
		g_value_set_uint(value, nvenc->m_nvParams.nvBFrames - 1);
		break;
	case PROP_RATE_MODE:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->rcParams.rateControlMode);
		g_value_set_uint(value, nvenc->m_nvParams.nvPass);
		break;
	case PROP_CONST_QP_I:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->rcParams.constQP.qpIntra);
		g_value_set_uint(value, nvenc->m_nvParams.nvConstQP.qpIntra);
		break;
	case PROP_CONST_QP_P:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->rcParams.constQP.qpInterP);
		g_value_set_uint(value, nvenc->m_nvParams.nvConstQP.qpInterP);
		break;
	case PROP_CONST_QP_B:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->rcParams.constQP.qpInterB);
		g_value_set_uint(value, nvenc->m_nvParams.nvConstQP.qpInterB);
		break;
	case PROP_ADAPTIVE_QP:
		//g_value_set_boolean(value, nvenc->m_pEncodeConfig->rcParams.enableAQ);
		g_value_set_boolean(value, nvenc->m_nvParams.nvEnableAQ);
		break;
	case PROP_EXT_QP:
		g_value_set_boolean(value, nvenc->m_nvParams.nvEnableExtQPDeltaMap);
		break;
	case PROP_MIN_QP_I:
		g_value_set_uint(value, nvenc->m_nvParams.nvMinQP.qpIntra);
		break;
	case PROP_MIN_QP_P:
		g_value_set_uint(value, nvenc->m_nvParams.nvMinQP.qpInterP);
		break;
	case PROP_MIN_QP_B:
		g_value_set_uint(value, nvenc->m_nvParams.nvMinQP.qpInterB);
		break;
	case PROP_MAX_QP_I:
		g_value_set_uint(value, nvenc->m_nvParams.nvMaxQP.qpIntra);
		break;
	case PROP_MAX_QP_P:
		g_value_set_uint(value, nvenc->m_nvParams.nvMaxQP.qpInterP);
		break;
	case PROP_MAX_QP_B:
		g_value_set_uint(value, nvenc->m_nvParams.nvMaxQP.qpInterB);
		break;
	case PROP_INITIAL_RCQP_I:
		g_value_set_uint(value, nvenc->m_nvParams.nvInitialRCQP.qpIntra);
		break;
	case PROP_INITIAL_RCQP_P:
		g_value_set_uint(value, nvenc->m_nvParams.nvInitialRCQP.qpInterP);
		break;
	case PROP_INITIAL_RCQP_B:
		g_value_set_uint(value, nvenc->m_nvParams.nvInitialRCQP.qpInterB);
		break;
	case PROP_BIT_RATE:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->rcParams.averageBitRate);
		g_value_set_uint(value, nvenc->m_nvParams.nvBitRate / 1024);
		break;
	case RROP_MAX_BIT_RATE:
		g_value_set_uint(value, nvenc->m_nvParams.nvMaxBitRate);
		break;
	case PROP_VBV_SIZE:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->rcParams.vbvBufferSize);
		g_value_set_uint(value, nvenc->m_nvParams.nvVBVSize / 1024);
		break;
	case PROP_VBV_DELAY:
		//g_value_set_uint(value, nvenc->m_pEncodeConfig->rcParams.vbvInitialDelay);
		g_value_set_uint(value, nvenc->m_nvParams.nvVBVDelay / 1024);
		break;
	case PROP_PROFILE:
		g_value_set_uint(value, nvenc->m_nvParams.nvProfile);
		break;
	case PROP_REF_FRAME:
		g_value_set_uint(value, nvenc->m_nvParams.nvRef);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
	GST_OBJECT_UNLOCK(nvenc);
}

static gboolean gst_nv_enc_init_cuda(GstNVEnc* nvenc)
{
	CUresult cuResult = cuInit(0);
	if (cuResult != CUDA_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Init cuda failed, code = %d", cuResult);
		return FALSE;
	}
	int deviceCount = 0;
	cuResult = cuDeviceGetCount(&deviceCount);
	if (cuResult != CUDA_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "get device count failed, code = %d", cuResult);
		return FALSE;
	}
	if (deviceCount <= 0)
	{
		GST_ERROR_OBJECT(nvenc, "could not found a GPU suported CUDA");
		return FALSE;
	}
	CUdevice device = 0;
	cuResult = cuDeviceGet(&device, 0);
	if (cuResult != CUDA_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Could not get device, code = ", cuResult);
		return FALSE;
	}
	int SMminor = 0, SMmajor = 0;
	cuResult = cuDeviceComputeCapability(&SMmajor, &SMminor, 0);
	if (cuResult != CUDA_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "cuDeviceComputeCapability return %d", cuResult);
		return FALSE;
	}
	if (((SMmajor << 4) + SMminor) < 0x30)
	{
		GST_ERROR_OBJECT(nvenc, "GPU 0 does not have NVENC capabilities exiting");
		return FALSE;
	}
	cuResult = cuCtxCreate((CUcontext*)(&nvenc->m_pDevice), 0, device);
	if (cuResult != CUDA_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "cuCtxCreate return %d", cuResult);
		return FALSE;
	}
	CUcontext cuContextCurr = { 0 };
	cuResult = cuCtxPopCurrent(&cuContextCurr);
	if (cuResult != CUDA_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "cuCtxPopCurrent return %d", cuResult);
		return FALSE;
	}	
	return TRUE;
}

static gboolean gst_nv_enc_start(GstVideoEncoder* benc)
{
	GstNVEnc* nvenc = GST_NV_ENCODER(benc);
	if (!gst_nv_enc_init_cuda(nvenc))
	{
		return FALSE;
	}
	nvenc->m_hinstLib = LoadLibrary(TEXT("nvEncodeAPI.dll"));
	if (nvenc->m_hinstLib == NULL)
	{
		GST_ERROR_OBJECT(nvenc, "Load nvEncodeAPI.dll failed");
		return FALSE;
	}
	MYPROC nvEncodeAPICreateInstance = (MYPROC)GetProcAddress(nvenc->m_hinstLib, "NvEncodeAPICreateInstance");
	if (nvEncodeAPICreateInstance == NULL)
	{
		GST_ERROR_OBJECT(nvenc, "Get process address failed");
		return FALSE;
	}
	nvenc->m_pEncodeAPI = malloc(sizeof(NV_ENCODE_API_FUNCTION_LIST));
	if (nvenc->m_pEncodeAPI == NULL)
	{
		GST_ERROR_OBJECT(nvenc, "Allocation encoder api list failed");
		return FALSE;
	}
	memset(nvenc->m_pEncodeAPI, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
	nvenc->m_pEncodeAPI->version = NV_ENCODE_API_FUNCTION_LIST_VER;
	NVENCSTATUS nvStatus = nvEncodeAPICreateInstance(nvenc->m_pEncodeAPI);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Create API instance failed, code = %d", nvStatus);
		return FALSE;
	}
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS openSessionExParams = { 0 };
	SET_VER(openSessionExParams, NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS);
	openSessionExParams.device = nvenc->m_pDevice;
	openSessionExParams.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
	openSessionExParams.reserved = NULL;
	openSessionExParams.apiVersion = NVENCAPI_VERSION;
	nvStatus = nvenc->m_pEncodeAPI->nvEncOpenEncodeSessionEx(&openSessionExParams, &nvenc->m_hEncoder);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Open encoder session failed, code = %d", nvStatus);
		return FALSE;
	}
	return TRUE;
}

static gboolean gst_nv_enc_stop(GstVideoEncoder* benc)
{
	GstNVEnc* nvenc = GST_NV_ENCODER(benc);
	for (guint i = 0; i < nvenc->m_uEncodeBufferCount; i++)
	{
		nvenc->m_pEncodeAPI->nvEncDestroyInputBuffer(nvenc->m_hEncoder, nvenc->m_stEncodeBuffer[i].stInputBfr.hInputSurface);
		nvenc->m_stEncodeBuffer[i].stInputBfr.hInputSurface = NULL;

		nvenc->m_pEncodeAPI->nvEncDestroyBitstreamBuffer(nvenc->m_hEncoder, nvenc->m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
		nvenc->m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;

		NV_ENC_EVENT_PARAMS eventParams = { 0 };
		SET_VER(eventParams, NV_ENC_EVENT_PARAMS);
		eventParams.completionEvent = nvenc->m_stEncodeBuffer[i].stOutputBfr.hOutputEvent;
		nvenc->m_pEncodeAPI->nvEncUnregisterAsyncEvent(nvenc->m_hEncoder, &eventParams);
		CloseHandle(nvenc->m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
		nvenc->m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
	}
	if (nvenc->m_stEOSOutputBfr.hOutputEvent)
	{
		NV_ENC_EVENT_PARAMS eventParams = { 0 };
		SET_VER(eventParams, NV_ENC_EVENT_PARAMS);
		eventParams.completionEvent = nvenc->m_stEOSOutputBfr.hOutputEvent;
		nvenc->m_pEncodeAPI->nvEncUnregisterAsyncEvent(nvenc->m_hEncoder, &eventParams);
		CloseHandle(nvenc->m_stEOSOutputBfr.hOutputEvent);
		nvenc->m_stEOSOutputBfr.hOutputEvent = NULL;
	}	

	nvenc->m_pEncodeAPI->nvEncDestroyEncoder(nvenc->m_hEncoder);
	nvenc->m_hEncoder = NULL;

	free(nvenc->m_pEncodeAPI);
	nvenc->m_pEncodeAPI = NULL;

	FreeLibrary(nvenc->m_hinstLib);
	nvenc->m_hinstLib = NULL;

	if (nvenc->m_pInputState)
	{
		gst_video_codec_state_unref(nvenc->m_pInputState);
		nvenc->m_pInputState = NULL;
	}
	
	return TRUE;
}
#pragma region
static gboolean gst_nv_enc_validate_preset_guid(GstNVEnc* nvenc, GUID inputPresetGUID, GUID inputCodecGUID)
{
	uint32_t i, presetFound, presetGUIDCount, presetGUIDArraySize;
	NVENCSTATUS nvStatus;
	GUID *presetGUIDArray;

	nvStatus = nvenc->m_pEncodeAPI->nvEncGetEncodePresetCount(nvenc->m_hEncoder, inputCodecGUID, &presetGUIDCount);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		return FALSE;
	}

	presetGUIDArray = malloc(sizeof(GUID)* presetGUIDCount);
	memset(presetGUIDArray, 0, sizeof(GUID)* presetGUIDCount);

	presetGUIDArraySize = 0;
	nvStatus = nvenc->m_pEncodeAPI->nvEncGetEncodePresetGUIDs(nvenc->m_hEncoder, inputCodecGUID, presetGUIDArray, presetGUIDCount, &presetGUIDArraySize);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		free(presetGUIDArray);
		presetGUIDArray = NULL;
		return FALSE;
	}

	g_assert(presetGUIDArraySize <= presetGUIDCount);

	presetFound = 0;
	for (i = 0; i < presetGUIDArraySize; i++)
	{
		if (gst_nv_enc_compare_guids(inputPresetGUID, presetGUIDArray[i]))
		{
			presetFound = 1;
			break;
		}
	}

	free(presetGUIDArray);
	presetGUIDArray = NULL;

	if (presetFound)
		return TRUE;
	else
		return FALSE;
}

static gboolean gst_nv_enc_validate_encode_guid(GstNVEnc* nvenc, GUID inputCodecGUID)
{
	unsigned int i, codecFound, encodeGUIDCount, encodeGUIDArraySize;
	NVENCSTATUS nvStatus;
	GUID *encodeGUIDArray;

	nvStatus = nvenc->m_pEncodeAPI->nvEncGetEncodeGUIDCount(nvenc->m_hEncoder, &encodeGUIDCount);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Could not get encode guid count, code = %d", nvStatus);
		return FALSE;
	}

	encodeGUIDArray = malloc(sizeof(GUID)* encodeGUIDCount);
	memset(encodeGUIDArray, 0, sizeof(GUID)* encodeGUIDCount);

	encodeGUIDArraySize = 0;
	nvStatus = nvenc->m_pEncodeAPI->nvEncGetEncodeGUIDs(nvenc->m_hEncoder, encodeGUIDArray, encodeGUIDCount, &encodeGUIDArraySize);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		free(encodeGUIDArray);
		encodeGUIDArray = NULL;
		return FALSE;
	}

	g_assert(encodeGUIDArraySize <= encodeGUIDCount);

	codecFound = 0;
	for (i = 0; i < encodeGUIDArraySize; i++)
	{
		if (gst_nv_enc_compare_guids(inputCodecGUID, encodeGUIDArray[i]))
		{
			codecFound = 1;
			break;
		}
	}

	free(encodeGUIDArray);
	if (codecFound)
		return TRUE;
	else
	{
		GST_ERROR_OBJECT(nvenc, "Could not found encode guid");
		return FALSE;
	}
	return TRUE;
}

static gboolean gst_nv_enc_set_format(GstVideoEncoder* encoder, GstVideoCodecState* state)
{
	GstNVEnc* nvenc = GST_NV_ENCODER(encoder);
	GstVideoInfo* info = &state->info;
	if (nvenc->m_pInputState)
		gst_video_codec_state_unref(nvenc->m_pInputState);
	nvenc->m_pInputState = gst_video_codec_state_ref(state);

	nvenc->m_pCreateEncodeParams->encodeGUID = NV_ENC_CODEC_H264_GUID;
	nvenc->m_pCreateEncodeParams->presetGUID = PresetGUIDs[nvenc->m_nvParams.nvPreset];

	if (!gst_nv_enc_validate_preset_guid(nvenc, nvenc->m_pCreateEncodeParams->presetGUID, nvenc->m_pCreateEncodeParams->encodeGUID))
	{
		GST_ERROR_OBJECT(nvenc, "not found preset guid");
		return FALSE;
	}
	if (!gst_nv_enc_validate_encode_guid(nvenc, nvenc->m_pCreateEncodeParams->encodeGUID))
	{
		return FALSE;
	}

	NV_ENC_PRESET_CONFIG stPresetCfg = { 0 };
	SET_VER(stPresetCfg, NV_ENC_PRESET_CONFIG);
	SET_VER(stPresetCfg.presetCfg, NV_ENC_CONFIG);

	NVENCSTATUS nvStatus = nvenc->m_pEncodeAPI->nvEncGetEncodePresetConfig(nvenc->m_hEncoder, nvenc->m_pCreateEncodeParams->encodeGUID, nvenc->m_pCreateEncodeParams->presetGUID, &stPresetCfg);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Could not get encoder preset config, code = %d", nvStatus);
		return FALSE;
	}

	memcpy(nvenc->m_pEncodeConfig, &stPresetCfg.presetCfg, sizeof(NV_ENC_CONFIG));

	nvenc->m_pCreateEncodeParams->encodeWidth = GST_VIDEO_INFO_WIDTH(info);
	nvenc->m_pCreateEncodeParams->encodeHeight = GST_VIDEO_INFO_HEIGHT(info);
	nvenc->m_pCreateEncodeParams->darWidth = GST_VIDEO_INFO_WIDTH(info);
	nvenc->m_pCreateEncodeParams->darHeight = GST_VIDEO_INFO_HEIGHT(info);
	nvenc->m_pCreateEncodeParams->maxEncodeWidth = GST_VIDEO_INFO_WIDTH(info);
	nvenc->m_pCreateEncodeParams->maxEncodeHeight = GST_VIDEO_INFO_HEIGHT(info);

	nvenc->m_pCreateEncodeParams->frameRateNum = GST_VIDEO_INFO_FPS_N(info);
	nvenc->m_pCreateEncodeParams->frameRateDen = GST_VIDEO_INFO_FPS_D(info);

	nvenc->m_pCreateEncodeParams->enableEncodeAsync = 1;
	nvenc->m_pCreateEncodeParams->enablePTD = 1;
// 	nvenc->m_pCreateEncodeParams->reportSliceOffsets = 0;
// 	nvenc->m_pCreateEncodeParams->enableSubFrameWrite = 0;
	nvenc->m_pCreateEncodeParams->encodeConfig = nvenc->m_pEncodeConfig;

// 	nvenc->m_pEncodeConfig->gopLength = nvenc->m_nvParams.nvGOP;
 	nvenc->m_pEncodeConfig->frameIntervalP = nvenc->m_nvParams.nvBFrames;
	nvenc->m_pEncodeConfig->rcParams.rateControlMode = nvenc->m_nvParams.nvPass;
// 	nvenc->m_pEncodeConfig->rcParams.constQP = nvenc->m_nvParams.nvConstQP;
// 	nvenc->m_pEncodeConfig->rcParams.enableAQ = nvenc->m_nvParams.nvEnableAQ;
// 	nvenc->m_pEncodeConfig->rcParams.enableExtQPDeltaMap = nvenc->m_nvParams.nvEnableExtQPDeltaMap;
// 	nvenc->m_pEncodeConfig->rcParams.enableMinQP = nvenc->m_nvParams.nvEnableMinQP;
// 	nvenc->m_pEncodeConfig->rcParams.minQP = nvenc->m_nvParams.nvMinQP;
// 	nvenc->m_pEncodeConfig->rcParams.enableMaxQP = nvenc->m_nvParams.nvEnableMaxQP;
// 	nvenc->m_pEncodeConfig->rcParams.maxQP = nvenc->m_nvParams.nvMaxQP;
// 	nvenc->m_pEncodeConfig->rcParams.enableInitialRCQP = nvenc->m_nvParams.nvEnableInitialRCQP;
// 	nvenc->m_pEncodeConfig->rcParams.initialRCQP = nvenc->m_nvParams.nvInitialRCQP;
	nvenc->m_pEncodeConfig->rcParams.averageBitRate = nvenc->m_nvParams.nvBitRate;
	nvenc->m_pEncodeConfig->rcParams.maxBitRate = nvenc->m_nvParams.nvMaxBitRate;
// 	nvenc->m_pEncodeConfig->rcParams.vbvBufferSize = nvenc->m_nvParams.nvVBVSize;
// 	nvenc->m_pEncodeConfig->rcParams.vbvInitialDelay = nvenc->m_nvParams.nvVBVDelay;
	nvenc->m_pEncodeConfig->profileGUID = ProfileGUIDs[nvenc->m_nvParams.nvProfile];

	nvenc->m_pEncodeConfig->frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;
	nvenc->m_pEncodeConfig->mvPrecision = NV_ENC_MV_PRECISION_FULL_PEL;
 
//	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.chromaFormatIDC = 1;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.disableDeblockingFilterIDC = 2;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.hierarchicalPFrames = 1;

//  	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.enableIntraRefresh = 1;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.idrPeriod = nvenc->m_pEncodeConfig->gopLength;
//	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.intraRefreshCnt = 1;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.intraRefreshPeriod = nvenc->m_pEncodeConfig->gopLength;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.level = NV_ENC_LEVEL_H264_31;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.disableSPSPPS = 0;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.repeatSPSPPS = 1;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.maxNumRefFrames = nvenc->m_nvParams.nvRef;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.adaptiveTransformMode = NV_ENC_H264_ADAPTIVE_TRANSFORM_ENABLE;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.fmoMode = NV_ENC_H264_FMO_DISABLE;

// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.h264VUIParameters.colourDescriptionPresentFlag = 1;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.h264VUIParameters.videoSignalTypePresentFlag = 1;
// 
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.h264VUIParameters.colourMatrix = info->colorimetry.matrix == GST_VIDEO_COLOR_MATRIX_BT601 ? 6 : 0;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.h264VUIParameters.colourPrimaries = info->colorimetry.primaries == GST_VIDEO_COLOR_PRIMARIES_BT470M ? 4 : 0;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.h264VUIParameters.transferCharacteristics = info->colorimetry.transfer == GST_VIDEO_TRANSFER_BT709 ? 1 : 0;
// 	nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.h264VUIParameters.videoFullRangeFlag = 0;


	nvStatus = nvenc->m_pEncodeAPI->nvEncInitializeEncoder(nvenc->m_hEncoder, nvenc->m_pCreateEncodeParams);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "init encoder failed, code = %d", nvStatus);
		return FALSE;
	}
	
	GstVideoCodecState *output_state = gst_video_encoder_set_output_state(encoder, gst_caps_new_empty_simple("video/x-h264"), state);
	gst_video_codec_state_unref(output_state);

	return gst_nv_enc_allocation_buffers(nvenc);
}
#pragma endregion
static gboolean gst_nv_enc_allocation_buffers(GstNVEnc* nvenc)
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	GstVideoInfo* info = &nvenc->m_pInputState->info;
	gst_nv_queue_initialize_queue(nvenc->m_pEncodeBufferQueue, nvenc->m_stEncodeBuffer, nvenc->m_uEncodeBufferCount);
	for (guint i = 0; i < nvenc->m_uEncodeBufferCount; i++)
	{
		NV_ENC_CREATE_INPUT_BUFFER createInputBufferParams;
		memset(&createInputBufferParams, 0, sizeof(createInputBufferParams));
		SET_VER(createInputBufferParams, NV_ENC_CREATE_INPUT_BUFFER);

		createInputBufferParams.width = GST_VIDEO_INFO_WIDTH(info);
		createInputBufferParams.height = GST_VIDEO_INFO_HEIGHT(info);
		createInputBufferParams.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;
		createInputBufferParams.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12_PL;

		nvStatus = nvenc->m_pEncodeAPI->nvEncCreateInputBuffer(nvenc->m_hEncoder, &createInputBufferParams);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			GST_ERROR_OBJECT(nvenc, "Create input buffer failed, code = %d", nvStatus);
			return FALSE;
		}
		nvenc->m_stEncodeBuffer[i].stInputBfr.hInputSurface = createInputBufferParams.inputBuffer;

		nvenc->m_stEncodeBuffer[i].stInputBfr.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12_PL;
		nvenc->m_stEncodeBuffer[i].stInputBfr.dwWidth = GST_VIDEO_INFO_WIDTH(info);
		nvenc->m_stEncodeBuffer[i].stInputBfr.dwHeight = GST_VIDEO_INFO_HEIGHT(info);


		NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBufferParams;
		memset(&createBitstreamBufferParams, 0, sizeof(createBitstreamBufferParams));
		SET_VER(createBitstreamBufferParams, NV_ENC_CREATE_BITSTREAM_BUFFER);

		createBitstreamBufferParams.size = 1024 * 1024;
		createBitstreamBufferParams.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

		nvStatus = nvenc->m_pEncodeAPI->nvEncCreateBitstreamBuffer(nvenc->m_hEncoder, &createBitstreamBufferParams);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			GST_ERROR_OBJECT(nvenc, "Create bitstream buffer failed, code = %d", nvStatus);
			return FALSE;
		}
		nvenc->m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = createBitstreamBufferParams.bitstreamBuffer;
		nvenc->m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = 1024 * 1024;

		NV_ENC_EVENT_PARAMS eventParams;
		memset(&eventParams, 0, sizeof(eventParams));
		SET_VER(eventParams, NV_ENC_EVENT_PARAMS);

		eventParams.completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		nvStatus = nvenc->m_pEncodeAPI->nvEncRegisterAsyncEvent(nvenc->m_hEncoder, &eventParams);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			GST_ERROR_OBJECT(nvenc, "Register event failed, code = %d", nvenc);
			return FALSE;
		}
		nvenc->m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = eventParams.completionEvent;
		nvenc->m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = TRUE;
	}

	nvenc->m_stEOSOutputBfr.bEOSFlag = TRUE;
	NV_ENC_EVENT_PARAMS eventParams;
	memset(&eventParams, 0, sizeof(eventParams));
	SET_VER(eventParams, NV_ENC_EVENT_PARAMS);

	eventParams.completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	nvStatus = nvenc->m_pEncodeAPI->nvEncRegisterAsyncEvent(nvenc->m_hEncoder, &eventParams);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Register event failed, code = %d", nvStatus);
		return FALSE;
	}
	nvenc->m_stEOSOutputBfr.hOutputEvent = eventParams.completionEvent;
	return TRUE;
}

static gboolean gst_nv_enc_load_data(GstNVEnc* nvenc, GstVideoCodecFrame* frame, EncodeBuffer *pEncodeBuffer)
{
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	NV_ENC_LOCK_INPUT_BUFFER lockInputBufferParams = { 0 };
	SET_VER(lockInputBufferParams, NV_ENC_LOCK_INPUT_BUFFER);

	lockInputBufferParams.inputBuffer = pEncodeBuffer->stInputBfr.hInputSurface;
	nvStatus = nvenc->m_pEncodeAPI->nvEncLockInputBuffer(nvenc->m_hEncoder, &lockInputBufferParams);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Lock input buffer failed, code = %d", nvStatus);
		return FALSE;
	}

	unsigned char* pInputSurface = lockInputBufferParams.bufferDataPtr;
	uint32_t lockedPitch = lockInputBufferParams.pitch;
	unsigned char* pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
	GstVideoInfo* info = &nvenc->m_pInputState->info;

	GstVideoFrame vframe = { 0 };
	if (!gst_video_frame_map(&vframe, info, frame->input_buffer, GST_MAP_READ))
	{
		GST_ERROR_OBJECT(nvenc, "Failed to map video frame, caps problem?");
		return FALSE;
	}

	memcpy(pInputSurface, vframe.data[0], GST_VIDEO_INFO_WIDTH(info) * GST_VIDEO_INFO_HEIGHT(info));
	memcpy(pInputSurfaceCh, vframe.data[1], GST_VIDEO_INFO_WIDTH(info) * GST_VIDEO_INFO_HEIGHT(info) / 2);
	gst_video_frame_unmap(&vframe);
	nvStatus = nvenc->m_pEncodeAPI->nvEncUnlockInputBuffer(nvenc->m_hEncoder, pEncodeBuffer->stInputBfr.hInputSurface);
	if (nvStatus != NV_ENC_SUCCESS)
	{
		GST_ERROR_OBJECT(nvenc, "Unlock input buffer failed, code = %d", nvStatus);
		return FALSE;
	}
	return TRUE;
}

static GstFlowReturn gst_nv_enc_handle_frame(GstVideoEncoder* encoder, GstVideoCodecFrame* frame)
{
	GstNVEnc* nvenc = GST_NV_ENCODER(encoder);
	GstVideoInfo *info = &nvenc->m_pInputState->info;
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	NV_ENC_PIC_PARAMS encPicParams = { 0 };

	EncodeBuffer* pEncodeBuffer = gst_nv_queue_get_available(nvenc->m_pEncodeBufferQueue);
	if (!pEncodeBuffer)
	{
		GstFlowReturn ret = gst_nv_enc_finish_frame(encoder, frame);
		if (ret != GST_FLOW_OK)
			return ret;
		pEncodeBuffer = gst_nv_queue_get_available(nvenc->m_pEncodeBufferQueue);
	}
	if (!gst_nv_enc_load_data(nvenc, frame, pEncodeBuffer))
		return GST_FLOW_ERROR;

	gst_video_codec_frame_unref(frame);

	SET_VER(encPicParams, NV_ENC_PIC_PARAMS);

	encPicParams.inputBuffer = pEncodeBuffer->stInputBfr.hInputSurface;
	encPicParams.bufferFmt = pEncodeBuffer->stInputBfr.bufferFmt;
	encPicParams.inputWidth = GST_VIDEO_INFO_WIDTH(info);
	encPicParams.inputHeight = GST_VIDEO_INFO_HEIGHT(info);
	encPicParams.outputBitstream = pEncodeBuffer->stOutputBfr.hBitstreamBuffer;
	encPicParams.completionEvent = pEncodeBuffer->stOutputBfr.hOutputEvent;
	encPicParams.encodePicFlags = 0;
	encPicParams.inputTimeStamp = frame->pts;
	encPicParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
	encPicParams.codecPicParams.h264PicParams.sliceMode = nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.sliceMode;
	encPicParams.codecPicParams.h264PicParams.sliceModeData = nvenc->m_pEncodeConfig->encodeCodecConfig.h264Config.sliceModeData;
	static guint64 index = 0;
// 	if (index % 30 == 0)
// 		encPicParams.encodePicFlags = NV_ENC_PIC_FLAG_OUTPUT_SPSPPS | NV_ENC_PIC_FLAG_FORCEIDR;
// 	else
// 		encPicParams.encodePicFlags = 0;
	index++;
	nvStatus = nvenc->m_pEncodeAPI->nvEncEncodePicture(nvenc->m_hEncoder, &encPicParams);
	if (nvStatus != NV_ENC_SUCCESS && nvStatus != NV_ENC_ERR_NEED_MORE_INPUT)
	{
		GST_ERROR_OBJECT(nvenc, "Encode picture failed, code = %d", nvStatus);
		return GST_FLOW_ERROR;
	}
	return GST_FLOW_OK;
}

static GstFlowReturn gst_nv_enc_finish_frame(GstVideoEncoder* object, GstVideoCodecFrame* frame)
{
	GstNVEnc* nvenc = GST_NV_ENCODER(object);
	NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
	NV_ENC_LOCK_BITSTREAM lockBitstreamData = { 0 };
	EncodeBuffer* pEncodeBuffer = gst_nv_queue_get_pending(nvenc->m_pEncodeBufferQueue);
	if (pEncodeBuffer->stOutputBfr.hBitstreamBuffer == NULL && pEncodeBuffer->stOutputBfr.bEOSFlag == FALSE)
	{
		return GST_FLOW_ERROR;
	}
	if (pEncodeBuffer->stOutputBfr.bWaitOnEvent == TRUE)
	{
		if (!pEncodeBuffer->stOutputBfr.hOutputEvent)
		{
			return GST_FLOW_ERROR;
		}
		WaitForSingleObject(pEncodeBuffer->stOutputBfr.hOutputEvent, INFINITE);
	}
	if (pEncodeBuffer->stOutputBfr.bEOSFlag)
		return GST_FLOW_OK;

	SET_VER(lockBitstreamData, NV_ENC_LOCK_BITSTREAM);
	lockBitstreamData.outputBitstream = pEncodeBuffer->stOutputBfr.hBitstreamBuffer;
	lockBitstreamData.doNotWait = TRUE;

	nvStatus = nvenc->m_pEncodeAPI->nvEncLockBitstream(nvenc->m_hEncoder, &lockBitstreamData);
	if (nvStatus == NV_ENC_SUCCESS)
	{
		GstBuffer* pBuffer = gst_buffer_new_and_alloc(lockBitstreamData.bitstreamSizeInBytes);
		GST_BUFFER_PTS(pBuffer) = lockBitstreamData.outputTimeStamp;
		GstMapInfo map = { 0 };
		if (!gst_buffer_map(pBuffer, &map, GST_MAP_READ))
		{
			GST_ERROR_OBJECT(nvenc, "Map buffer failed");
			return FALSE;
		}
		memcpy(map.data, lockBitstreamData.bitstreamBufferPtr, lockBitstreamData.bitstreamSizeInBytes);
		GstVideoCodecFrame* pFrame = gst_video_encoder_get_oldest_frame(object);
		pFrame->flags = lockBitstreamData.pictureType == NV_ENC_PIC_TYPE_IDR ? GST_VIDEO_CODEC_FRAME_FLAG_SYNC_POINT : 0;
		nvStatus = nvenc->m_pEncodeAPI->nvEncUnlockBitstream(nvenc->m_hEncoder, pEncodeBuffer->stOutputBfr.hBitstreamBuffer);
		gst_buffer_unmap(pBuffer, &map);
		if (pFrame->pts != GST_BUFFER_PTS(pBuffer))
			GST_WARNING_OBJECT(nvenc, "pFrame's pts is not equality buffer'pts");
		pFrame->output_buffer = pBuffer;
		return gst_video_encoder_finish_frame(object, pFrame);
	}
	return GST_FLOW_OK;
}

gboolean gst_nv_encoder_plugin_init(GstPlugin * plugin)
{
	return gst_element_register(plugin, "nvenc", GST_RANK_PRIMARY, GST_TYPE_NV_ENCODER);
}