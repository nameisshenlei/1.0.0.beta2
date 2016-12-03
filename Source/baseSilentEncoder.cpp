#include "baseSilentEncoder.h"
#include "errorCode.h"
#include "helpFunctions.h"
#include <windows.h>
#include "juceMessage.h"
#include "encoderManager.h"
#include "mfxvideo++.h"
#include "gstnvenc\nvEncodeAPI.h"

#define APPSRC_QUEUE_MAX_BYTES			400 * 1024 * 1024

static void nemogst_signal_bus_watch(GstBus* bus, GstMessage* message, gpointer user_data);


baseSilentEncoder::baseSilentEncoder(String encoderName, String memVideoName, String memAudioName)
: m_encoderName(encoderName)
, m_memVideoName(memVideoName)
, m_memAudioName(memAudioName)
, m_settingInfo(nullptr)
, m_elmtPipeline(nullptr)
, m_elmtAudioAppSrc(nullptr)
, m_elmtVideoAppSrc(nullptr)
, m_hGlobalEvent(nullptr)
, m_pvOutputCfg(nullptr)
{
}

baseSilentEncoder::~baseSilentEncoder()
{
	if (isTimerRunning())
	{
		stopTimer();
	}
	if (m_hGlobalEvent)
	{
		::CloseHandle(m_hGlobalEvent);
		m_hGlobalEvent = nullptr;
	}
	deleteAndZero(m_settingInfo);
	deleteAndZero(m_pvOutputCfg);
}


const String& baseSilentEncoder::getEncoderName()
{
	return m_encoderName;
}

void baseSilentEncoder::writeStateIntoIni(String strCode, String strMsg)
{
	WritePrivateProfileString(L"states", L"stscode", strCode.toWideCharPointer(), m_iniFullPath.toWideCharPointer());
	WritePrivateProfileString(L"states", L"stscode", strMsg.toWideCharPointer(), m_iniFullPath.toWideCharPointer());
}

int baseSilentEncoder::play(PropertySet* setting, vOutputCfg* pvOutputSfc, String iniFullPath, void* hGlobalEvent)
{
	m_settingInfo = setting;
	m_pvOutputCfg = pvOutputSfc;
	m_iniFullPath = iniFullPath;
	m_hGlobalEvent = hGlobalEvent;

	m_elmtPipeline = nullptr;
	m_elmtAudioAppSrc = nullptr;
	m_elmtVideoAppSrc = nullptr;

	m_audioFirst = false;
	m_timeAudioBeginPts = 0;
	m_videoFirst = false;
	m_timeVideoBeginPts = 0;
	// video app src
	m_elmtVideoAppSrc = gst_element_factory_make("appsrc", "videoAppSrc");
	// audio app src
	m_elmtAudioAppSrc = gst_element_factory_make("appsrc", "audioAppSrc");

	if (!m_elmtAudioAppSrc || !m_elmtVideoAppSrc)
	{
		goto PLAY_FAILED;
	}
	m_elmtPipeline = gst_pipeline_new(nullptr);
	if (!m_elmtPipeline)
	{
		goto PLAY_FAILED;
	}

	// set properties
	{
		// video raw caps
		GstCaps* videoCaps = gst_caps_new_simple("video/x-raw",
			"format", G_TYPE_STRING, "BGRA",
			"framerate", GST_TYPE_FRACTION, 30, 1,
			"pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
			"width", G_TYPE_INT, m_settingInfo->getIntValue(KEY_MEM_VIDEO_WIDTH, 1280),
			"height", G_TYPE_INT, m_settingInfo->getIntValue(KEY_MEM_VIDEO_HEIGHT, 720),
			nullptr);

		g_object_set(m_elmtVideoAppSrc, "caps", videoCaps, nullptr);
		g_object_set(m_elmtVideoAppSrc, "format", GST_FORMAT_TIME, nullptr);
		//g_object_set(m_elmtVideoAppSrc, "max-bytes", 0, nullptr);
		gst_app_src_set_max_bytes((GstAppSrc*)m_elmtVideoAppSrc, 0);
	}
	{
		GstCaps* audioCaps = gst_caps_new_simple("audio/x-raw", \
			"format", G_TYPE_STRING, m_settingInfo->getValue(KEY_MEM_AUDIO_SAMPLE_FORMAT).toUTF8(), \
			"layout", G_TYPE_STRING, "interleaved", \
			"rate", G_TYPE_INT, m_settingInfo->getIntValue(KEY_MEM_AUDIO_FREQ, 44100), \
			"channels", G_TYPE_INT, m_settingInfo->getIntValue(KEY_MEM_AUDIO_CHS, 2), \
			"channel-mask", GST_TYPE_BITMASK, (guint64)3, \
			nullptr);
		g_object_set(m_elmtAudioAppSrc, "caps", audioCaps, nullptr);
		g_object_set(m_elmtAudioAppSrc, "format", GST_FORMAT_TIME, nullptr);
		//g_object_set(m_elmtAudioAppSrc, "max-bytes", 0, nullptr);
		gst_app_src_set_max_bytes((GstAppSrc*)m_elmtAudioAppSrc, 0);
	}

	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		, m_elmtVideoAppSrc
		, m_elmtAudioAppSrc
		, nullptr);

	GstStateChangeReturn ret = onPlay();
	{
		//////////////////////////////////////////////////////////////////////////
		GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_elmtPipeline));
		gst_bus_add_signal_watch(bus);
		g_signal_connect(bus, "message", G_CALLBACK(nemogst_signal_bus_watch), this);
		gst_object_unref(bus);
	}

	if (ret == GST_STATE_CHANGE_ASYNC || ret == GST_STATE_CHANGE_SUCCESS)
	{
		startTimer(1000);
		return 0;
	}

PLAY_FAILED:
	m_settingInfo = nullptr;
	m_iniFullPath = String::empty;
	m_hGlobalEvent = nullptr;
	return -1;
}

void baseSilentEncoder::stop()
{
	if (isTimerRunning())
	{
		stopTimer();
	}
	onStop();

	gst_element_send_event(m_elmtPipeline, gst_event_new_eos());
// 	gst_app_src_end_of_stream((GstAppSrc*)m_elmtAudioAppSrc);
// 	gst_app_src_end_of_stream((GstAppSrc*)m_elmtVideoAppSrc);

	gst_bus_remove_signal_watch(GST_ELEMENT_BUS(m_elmtPipeline));

	gst_bus_timed_pop_filtered(GST_ELEMENT_BUS(m_elmtPipeline), 1 * GST_SECOND, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

	gst_element_set_state(m_elmtPipeline, GST_STATE_NULL);

	if (m_elmtPipeline)
	{
		gst_object_unref(m_elmtPipeline);
		m_elmtPipeline = nullptr;
	}

	m_elmtAudioAppSrc = nullptr;
	m_elmtVideoAppSrc = nullptr;

	if (m_hGlobalEvent)
	{
		::CloseHandle(m_hGlobalEvent);
		m_hGlobalEvent = nullptr;
	}
}

void baseSilentEncoder::onStop()
{

}

int baseSilentEncoder::video_h264_encoder(GstElement* mux)
{
	//GstElement* elmtVideoFlip = nullptr;
	GstElement* elmtVideoConvert = nullptr;
	GstElement* elmtVideoScale = nullptr;
	GstElement* elmtVideoRate = nullptr;
	GstElement* elmtVideoH264Enc = nullptr;
	GstElement* elmtVideoH264Parse = nullptr;

	// video flip
	//elmtVideoFlip = gst_element_factory_make("videoflip", "videoFlip");
	// video convert
	elmtVideoConvert = gst_element_factory_make("videoconvert", "videoConvert");
	//video scale
	elmtVideoScale = gst_element_factory_make("videoscale", "videoScale");
	// video rate
	elmtVideoRate = gst_element_factory_make("videorate", "videoRate");
	// video x264 encoder
	m_settingInfo->setValue(KEY_ACCELERATION_TYPE, "none");
	if (m_settingInfo->getValue(KEY_ACCELERATION_TYPE).equalsIgnoreCase("intel"))
		elmtVideoH264Enc = gst_element_factory_make("mfxenc", "mfxenc");
	else if (m_settingInfo->getValue(KEY_ACCELERATION_TYPE).equalsIgnoreCase("cuda"))
		elmtVideoH264Enc = gst_element_factory_make("nvenc", "nvenc");
	else
		elmtVideoH264Enc = gst_element_factory_make("x264enc", "x264Enc");
	// h264 parse
	elmtVideoH264Parse = gst_element_factory_make("h264parse", "h264Parse");

	if (/*!elmtVideoFlip || */!elmtVideoConvert || !elmtVideoRate || !elmtVideoH264Enc || !elmtVideoH264Parse)
	{
		return -1;
	}

	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		//, elmtVideoFlip
		, elmtVideoConvert
		, elmtVideoScale
		, elmtVideoRate
		, elmtVideoH264Enc
		, elmtVideoH264Parse
		, nullptr);

	{
		// flip
		//g_object_set(elmtVideoFlip, "method", 5, nullptr);
	}
// 	if (!gst_element_link(m_elmtVideoAppSrc, elmtVideoFlip))
// 	{
// 		return -1;
// 	}
// 	if (!gst_element_link(elmtVideoFlip, elmtVideoConvert))
// 	{
// 		return -1;
// 	}
	if (!gst_element_link(m_elmtVideoAppSrc, elmtVideoConvert))
	{
		return -1;
	}
	char colorspase[10] = { 0 };
	if (m_settingInfo->getValue(KEY_ACCELERATION_TYPE).equalsIgnoreCase("intel") || m_settingInfo->getValue(KEY_ACCELERATION_TYPE).equalsIgnoreCase("cuda"))
		sprintf_s(colorspase, 10, "%s", "NV12");
	else
		sprintf_s(colorspase, 10, "%s", "I420");
	if (!gst_element_link_filtered(elmtVideoConvert, elmtVideoScale, 
		gst_caps_new_simple("video/x-raw",
		"format", G_TYPE_STRING, colorspase,
		nullptr
		)))
	{
		return -1;
	}
	if (!gst_element_link_filtered(elmtVideoScale, elmtVideoRate,
		gst_caps_new_simple("video/x-raw", 
		"width", G_TYPE_INT, m_settingInfo->getIntValue(KEY_OUT_VIDEO_WIDTH, 1280),
		"height", G_TYPE_INT, m_settingInfo->getIntValue(KEY_OUT_VIDEO_HEIGHT, 720),
		nullptr)))
	{
		return -1;
	}
	if (!gst_element_link_filtered(elmtVideoRate, elmtVideoH264Enc,
		gst_caps_new_simple("video/x-raw",
		"framerate", GST_TYPE_FRACTION, 30, 1,
		nullptr
		)))
	{
		return -1;
	}
	if (!gst_element_link_filtered(elmtVideoH264Enc, elmtVideoH264Parse,
		gst_caps_new_simple("video/x-h264",
		"profile", G_TYPE_STRING, (/*m_pvOutputCfg->strProfile.isEmpty() ? */"high"/* : m_pvOutputCfg->strProfile.toRawUTF8()*/),
		nullptr)))
	{
		return -1;
	}
// 	if (!gst_element_link(elmtVideoH264Enc, elmtVideoH264Parse))
// 	{
// 		return -1;
// 	}

	if (!gst_element_link(elmtVideoH264Parse, mux))
	{
		return -1;
	}
	{
		//x264 default setting
		bool isRtp = m_settingInfo->getValue(KEY_ENCODER_TYPE).equalsIgnoreCase(CMD_ENCODER_CREATE_TYPE_RTP);
		int gop = isRtp ? 60 : 300;
		map<String, int>& mapIntCfg = m_pvOutputCfg->mapIntCfgs;
		map<String, int>::iterator iter = mapIntCfg.find(String("bitrate"));
		if (m_settingInfo->getValue(KEY_ACCELERATION_TYPE).equalsIgnoreCase("intel"))
		{
			g_object_set(elmtVideoH264Enc, "profile", MFX_PROFILE_AVC_HIGH, nullptr);
			g_object_set(elmtVideoH264Enc, "level", MFX_LEVEL_AVC_5, nullptr);
			g_object_set(elmtVideoH264Enc, "target-usage", MFX_TARGETUSAGE_1, nullptr);
			g_object_set(elmtVideoH264Enc, "key-int-max", gop, nullptr);
			g_object_set(elmtVideoH264Enc, "bframes", 0, nullptr);
			g_object_set(elmtVideoH264Enc, "gop-opt-flag", MFX_GOP_STRICT, nullptr);
			g_object_set(elmtVideoH264Enc, "idr-interval", 30, nullptr);
			g_object_set(elmtVideoH264Enc, "rate-ctl-method", MFX_RATECONTROL_AVBR, nullptr);
			g_object_set(elmtVideoH264Enc, "init-delay", iter->second / 1000 / 8, nullptr);
			//g_object_set(elmtVideoH264Enc, "icq-quality", 18, nullptr);
			g_object_set(elmtVideoH264Enc, "vbv-buf-capacity", iter->second / 1000 * 3 / 2, nullptr);
			g_object_set(elmtVideoH264Enc, "ref", 1, nullptr);
			g_object_set(elmtVideoH264Enc, "bitrate", iter->second / 1000, nullptr);
			//g_object_set(elmtVideoH264Enc, "byte-stream", true, nullptr);
		}
		else if (m_settingInfo->getValue(KEY_ACCELERATION_TYPE).equalsIgnoreCase("cuda"))
		{
			iter->second = 2048 * 1000;
			g_object_set(elmtVideoH264Enc, "profile", PROFILE_AUTOSELECT_GUID, nullptr);
			g_object_set(elmtVideoH264Enc, "preset", PRESET_HQ_GUID, nullptr);
			g_object_set(elmtVideoH264Enc, "bitrate", iter->second / 1000, nullptr);
			g_object_set(elmtVideoH264Enc, "max-bitrate", iter->second / 1000 * 3 / 2, nullptr);
			g_object_set(elmtVideoH264Enc, "vbv-size", iter->second / 1000 * 2, nullptr);
			g_object_set(elmtVideoH264Enc, "vbv-delay", iter->second / 1000 * 2 * 9 / 10, nullptr);
			g_object_set(elmtVideoH264Enc, "pass", NV_ENC_PARAMS_RC_2_PASS_VBR, nullptr);
//			g_object_set(elmtVideoH264Enc, "AdaptiveQP", TRUE, nullptr);
			g_object_set(elmtVideoH264Enc, "key-int-max", 30, nullptr);
			g_object_set(elmtVideoH264Enc, "bframes", 0, nullptr);
			g_object_set(elmtVideoH264Enc, "ref", 3, nullptr);
			g_object_set(elmtVideoH264Enc, "minQP", TRUE, nullptr);
			g_object_set(elmtVideoH264Enc, "minQP-I", 0, nullptr);
			g_object_set(elmtVideoH264Enc, "minQP-P", 0, nullptr);
			g_object_set(elmtVideoH264Enc, "minQP-B", 0, nullptr);
			g_object_set(elmtVideoH264Enc, "maxQP", TRUE, nullptr);
			g_object_set(elmtVideoH264Enc, "maxQP-I", 40, nullptr);
			g_object_set(elmtVideoH264Enc, "maxQP-P", 40, nullptr);
			g_object_set(elmtVideoH264Enc, "maxQP-B", 40, nullptr);
		}
		else
		{
			g_object_set(elmtVideoH264Enc,
				"speed-preset", 3,
				"psy-tune", 3,
				"pass", 5,
				"bitrate", iter->second / 1000,
				"key-int-max", gop,
				"byte-stream", true,
				"vbv-buf-capacity", iter->second / 1000 * 3 / 2,
				"sliced-threads", false,
				"rc-lookahead", 40,
				"me", 1,
				"quantizer", 18,
				"bframes", 0,
				nullptr);
		}
	}
	{
 		// x264
		map<String, float>& mapFloatCfg = m_pvOutputCfg->mapFloatCfgs;
		map<String, int>& mapIntCfg = m_pvOutputCfg->mapIntCfgs;
		map<String, int>::iterator iter = mapIntCfg.find(String("preset"));
		if (iter != mapIntCfg.end()) g_object_set(elmtVideoH264Enc, iter->first.toRawUTF8(), iter->second, nullptr);
		iter = mapIntCfg.find(String("tune"));
		if (iter != mapIntCfg.end()) g_object_set(elmtVideoH264Enc, iter->first.toRawUTF8(), iter->second, nullptr);
		iter = mapIntCfg.find(String("psy-tune"));
		if (iter != mapIntCfg.end()) g_object_set(elmtVideoH264Enc, iter->first.toRawUTF8(), iter->second, nullptr);
		for (map<String, float>::iterator iterFloat = mapFloatCfg.begin(); iterFloat != mapFloatCfg.end(); iterFloat++)
		{
			g_object_set(elmtVideoH264Enc, iterFloat->first.toRawUTF8(), iterFloat->second, nullptr);
		}
		for (iter = mapIntCfg.begin(); iter != mapIntCfg.end(); iter++)
		{
			if (!iter->first.equalsIgnoreCase("preset") && !iter->first.equalsIgnoreCase("tune") && !iter->first.equalsIgnoreCase("psy-tune"))
			{
				if (iter->first.equalsIgnoreCase("bitrate"))
					g_object_set(elmtVideoH264Enc, "bitrate", iter->second / 1000, nullptr);
				else
					g_object_set(elmtVideoH264Enc, iter->first.toRawUTF8(), iter->second, nullptr);
			}
		}
	}
	return 0;
}

int baseSilentEncoder::video_vpx_encoder(GstElement* mux)
{
	//GstElement* elmtVideoFlip = nullptr;
	GstElement* elmtVideoConvert = nullptr;
	GstElement* elmtVideoRate = nullptr;
	GstElement* elmtVideoVpxEnc = nullptr;

	// video flip
	//elmtVideoFlip = gst_element_factory_make("videoflip", "videoFlip");
	// video convert
	elmtVideoConvert = gst_element_factory_make("videoconvert", "videoConvert");
	// video rate
	elmtVideoRate = gst_element_factory_make("videorate", "videoRate");
	// video x264 encoder
	elmtVideoVpxEnc = gst_element_factory_make("vp8enc", "vp8Enc");

	if (/*!elmtVideoFlip || */!elmtVideoConvert || !elmtVideoRate || !elmtVideoVpxEnc)
	{
		return -1;
	}

	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		//, elmtVideoFlip
		, elmtVideoConvert
		, elmtVideoRate
		, elmtVideoVpxEnc
		, nullptr);

	{
		// flip
		//g_object_set(elmtVideoFlip, "method", 5, nullptr);
	}
	{
		// x264
		g_object_set(elmtVideoVpxEnc,
			"target-bitrate", m_settingInfo->getIntValue(KEY_OUT_VIDEO_BITRATE, 600000),
			nullptr);
	}
// 	if (!gst_element_link(m_elmtVideoAppSrc, elmtVideoFlip))
// 	{
// 		return -1;
// 	}
// 	if (!gst_element_link(elmtVideoFlip, elmtVideoConvert))
// 	{
// 		return -1;
// 	}
	if (!gst_element_link(m_elmtVideoAppSrc, elmtVideoConvert))
	{
		return -1;
	}
	if (!gst_element_link_filtered(elmtVideoConvert, elmtVideoRate,
		gst_caps_new_simple("video/x-raw",
		"format", G_TYPE_STRING, "I420",
		nullptr
		)))
	{
		return -1;
	}
	if (!gst_element_link_filtered(elmtVideoRate, elmtVideoVpxEnc,
		gst_caps_new_simple("video/x-raw",
		"framerate", GST_TYPE_FRACTION, 30, 1,
		nullptr
		)))
	{
		return -1;
	}
	if (!gst_element_link(elmtVideoVpxEnc, mux))
	{
		return -1;
	}

	return 0;
}

int baseSilentEncoder::audio_aac_encoder(GstElement* mux)
{
	GstElement* elmtAudioAACEnc = nullptr;
	GstElement* elmtAudioAACParse = nullptr;
	GstElement* elmtAudioConvert = nullptr;
	GstElement* elmtAudioRate = nullptr;
	GstElement* elmtAudioResample = nullptr;
	//audio sample format convert
	elmtAudioConvert = gst_element_factory_make("audioconvert", "audioconvert");
	elmtAudioRate = gst_element_factory_make("audiorate", "audiorate");
	elmtAudioResample = gst_element_factory_make("audioresample", "audioresample");
	// audio AAC encoder
	elmtAudioAACEnc = gst_element_factory_make("voaacenc", "aacEnc");
	// aac parse
	elmtAudioAACParse = gst_element_factory_make("aacparse", "aacParse");

	if (!elmtAudioAACEnc || !elmtAudioAACParse || !elmtAudioConvert || !elmtAudioRate)
	{
		return -1;
	}
	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		, elmtAudioAACEnc
		, elmtAudioAACParse
		, elmtAudioConvert
		, elmtAudioRate
		, elmtAudioResample
		, nullptr);
	{
		// aac
		g_object_set(elmtAudioAACEnc, "bitrate", m_settingInfo->getIntValue(KEY_OUT_AUDIO_BITRATE, 96000), nullptr);
	}

	if (!gst_element_link_many(m_elmtAudioAppSrc, elmtAudioConvert, elmtAudioResample, nullptr))
	{
		return -1;
	}
	if (!gst_element_link_filtered(elmtAudioResample, elmtAudioRate,
		gst_caps_new_simple("audio/x-raw", "rate", G_TYPE_INT, m_settingInfo->getIntValue(KEY_OUT_AUDIO_FREQ, 44100), nullptr)))
	{
		return -1;
	}
	if (!gst_element_link_many(elmtAudioRate, elmtAudioAACEnc, elmtAudioAACParse, nullptr))
	{
		return -1;
	}

	if (!gst_element_link(elmtAudioAACParse, mux))
	{
		return -1;
	}

	return 0;
}

int baseSilentEncoder::audio_vorbis_encoder(GstElement* mux)
{
	GstElement* elmtAudioConvert = nullptr;
	GstElement* elmtAudioResample = nullptr;
	GstElement* elmtAudioVorbisEnc = nullptr;
	GstElement* elmtAudioVorbisParse = nullptr;
	// convert
	elmtAudioConvert = gst_element_factory_make("audioconvert", "audioConvert");
	// resample
	elmtAudioResample = gst_element_factory_make("audioresample", "audioResample");
	// audio AAC encoder
	elmtAudioVorbisEnc = gst_element_factory_make("vorbisenc", "vorbisEnc");
	// aac parse
	elmtAudioVorbisParse = gst_element_factory_make("vorbisparse", "vorbisParse");

	if (!elmtAudioConvert || !elmtAudioResample || !elmtAudioVorbisEnc || !elmtAudioVorbisParse)
	{
		return -1;
	}
	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		, elmtAudioConvert
		, elmtAudioResample
		, elmtAudioVorbisEnc
		, elmtAudioVorbisParse
		, nullptr);
	{
		// aac
		g_object_set(elmtAudioVorbisEnc, "bitrate", m_settingInfo->getIntValue(KEY_OUT_AUDIO_BITRATE, 96000), nullptr);
	}

	if (!gst_element_link(m_elmtAudioAppSrc, elmtAudioConvert))
	{
		return -1;
	}
	if (!gst_element_link(elmtAudioConvert, elmtAudioResample))
	{
		return -1;
	}

	if (!gst_element_link(elmtAudioResample, elmtAudioVorbisEnc))
	{
		return -1;
	}
	if (!gst_element_link(elmtAudioVorbisEnc, elmtAudioVorbisParse))
	{
		return -1;
	}
	if (!gst_element_link(elmtAudioVorbisParse, mux))
	{
		return -1;
	}

	return 0;
}

int baseSilentEncoder::audio_mp3_encoder(GstElement* mux)
{
	GstElement* elmtAudioLameMP3Enc = nullptr;
	GstElement* elmtAudioMPEGParse = nullptr;
	// audio AAC encoder
	elmtAudioLameMP3Enc = gst_element_factory_make("lamemp3enc", "mp3Enc");
	// aac parse
	elmtAudioMPEGParse = gst_element_factory_make("mpegaudioparse", "mp3Parse");

	if (!elmtAudioLameMP3Enc || !elmtAudioMPEGParse)
	{
		return -1;
	}
	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		, elmtAudioLameMP3Enc
		, elmtAudioMPEGParse
		, nullptr);
	{
		// aac
		g_object_set(elmtAudioLameMP3Enc, "bitrate", m_settingInfo->getIntValue(KEY_OUT_AUDIO_BITRATE, 96000), nullptr);
	}

	if (!gst_element_link(m_elmtAudioAppSrc, elmtAudioLameMP3Enc))
	{
		return -1;
	}
	if (!gst_element_link(elmtAudioLameMP3Enc, elmtAudioMPEGParse))
	{
		return -1;
	}

	if (!gst_element_link(elmtAudioMPEGParse, mux))
	{
		return -1;
	}

	return 0;
}

int baseSilentEncoder::initCommonEncoderElements()
{
	return 0;
}

void baseSilentEncoder::pushVideoBuffer(GstBuffer* b)
{
	if (!m_elmtVideoAppSrc || gst_app_src_get_current_level_bytes((GstAppSrc*)m_elmtVideoAppSrc) >= APPSRC_QUEUE_MAX_BYTES)
		return;

// 	GstBuffer* gstBuffer = gst_buffer_copy(b);
// 	if (!m_audioFirst)
// 	{
// 		// preroll video for app-src
// 		GST_BUFFER_PTS(gstBuffer) = 0;
// 		m_timeAudioBeginPts = GST_BUFFER_PTS(b);
// 		m_audioFirst = true;
// 	}
// 	else
// 	{
// 		GST_BUFFER_PTS(gstBuffer) = GST_BUFFER_PTS(b) - m_timeAudioBeginPts;
// 	}
// 
// 	GST_BUFFER_DTS(gstBuffer) = GST_BUFFER_PTS(gstBuffer);
// 	GST_BUFFER_DURATION(gstBuffer) = GST_BUFFER_DURATION(b);
// 	GST_BUFFER_OFFSET(gstBuffer) = 0;
// 	GST_BUFFER_OFFSET_END(gstBuffer) = GST_BUFFER_OFFSET_NONE;
// 
// 	GstFlowReturn ret = gst_app_src_push_buffer((GstAppSrc*)m_elmtVideoAppSrc, gstBuffer);
// 	if (ret != GST_FLOW_OK)
// 	{
// 		gst_buffer_unref(gstBuffer);
// 	}

	if (m_audioFirst && GST_BUFFER_PTS(b) > m_timeAudioBeginPts)
	{
		GstBuffer* gstBuffer = gst_buffer_copy(b);
		GST_BUFFER_PTS(gstBuffer) = GST_BUFFER_PTS(b) - m_timeAudioBeginPts;
		GST_BUFFER_DTS(gstBuffer) = GST_BUFFER_PTS(gstBuffer);
		GST_BUFFER_DURATION(gstBuffer) = GST_BUFFER_DURATION(b);
		GST_BUFFER_OFFSET(gstBuffer) = 0;
		GST_BUFFER_OFFSET_END(gstBuffer) = GST_BUFFER_OFFSET_NONE;

		GstFlowReturn ret = gst_app_src_push_buffer((GstAppSrc*)m_elmtVideoAppSrc, gstBuffer);
		if (ret != GST_FLOW_OK)
		{
			gst_buffer_unref(gstBuffer);
		}
	}
}

void baseSilentEncoder::pushAudioBuffer(GstBuffer* b)
{
	if (!m_elmtAudioAppSrc || gst_app_src_get_current_level_bytes((GstAppSrc*)m_elmtAudioAppSrc) >= APPSRC_QUEUE_MAX_BYTES / 5)
		return;

	if (m_audioFirst && GST_BUFFER_PTS(b) < m_timeAudioBeginPts)
		return;

	GstBuffer* gstBuffer = gst_buffer_copy(b);

	if (!m_audioFirst)
	{
		// preroll audio for app-src
		GST_BUFFER_PTS(gstBuffer) = 0;
		m_timeAudioBeginPts = GST_BUFFER_PTS(b);
		m_audioFirst = true;
	}
	else
	{
		GST_BUFFER_PTS(gstBuffer) = GST_BUFFER_PTS(b) - m_timeAudioBeginPts;
	}

	GST_BUFFER_DTS(gstBuffer) = GST_BUFFER_PTS(gstBuffer);
	GST_BUFFER_DURATION(gstBuffer) = GST_BUFFER_DURATION(b);
	GST_BUFFER_OFFSET(gstBuffer) = 0;
	GST_BUFFER_OFFSET_END(gstBuffer) = GST_BUFFER_OFFSET_NONE;

	GstFlowReturn ret = gst_app_src_push_buffer((GstAppSrc*)m_elmtAudioAppSrc, gstBuffer);
	if (ret != GST_FLOW_OK)
	{
		gst_buffer_unref(gstBuffer);
	}
}


void baseSilentEncoder::busMsgWatch(GstBus*, GstMessage* message)
{
	GstState old_state, new_state, pending_state;
	GError* err;
	String strMsg;
	GstStreamStatusType statusType;
	GstElement          *elemOwner;

	switch (GST_MESSAGE_TYPE(message))
	{
	case GST_MESSAGE_ERROR:
		{
			gst_message_parse_error(message, &err, nullptr);
			strMsg = String::fromUTF8(err->message);
			GST_ERROR_OBJECT(m_elmtPipeline, err->message);
			NEMO_LOG_ERROR(strMsg);
			NEMO_LOG_ERROR(L"Destroy local encoder");
			writePropertiesInIniFile(m_iniFullPath, SE_ERROR_CODE_FAILED, m_hGlobalEvent, strMsg.toWideCharPointer());
			juceMessage* msg = new juceMessage(gst_msg_type_error, m_encoderName, m_memVideoName, m_memAudioName);
			m_pEncoderManager->postMessage(msg);
			break;
		}
	case GST_MESSAGE_EOS:
		writePropertiesInIniFile(m_iniFullPath, SE_ERROR_CODE_STOPPED, m_hGlobalEvent);
		break;
	case GST_MESSAGE_STATE_CHANGED:
		gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
// 		GST_DEBUG_BIN_TO_DOT_FILE((GstBin *)m_elmtPipeline, GST_DEBUG_GRAPH_SHOW_ALL, "silentEncoderRtp");
		break;
	case GST_MESSAGE_STREAM_STATUS:
		gst_message_parse_stream_status(message, &statusType, &elemOwner);
		break;
	case GST_MESSAGE_ASYNC_START:
		elemOwner = nullptr;
	case GST_MESSAGE_ASYNC_DONE:
		elemOwner = nullptr;
		break;
	}
}

static void nemogst_signal_bus_watch(GstBus* bus, GstMessage* message, gpointer user_data)
{
	baseSilentEncoder* pEncoder = static_cast<baseSilentEncoder*>(user_data);
	if (pEncoder)
	{
		pEncoder->busMsgWatch(bus, message);
	}
}

void baseSilentEncoder::SetManager(nemoEncoderManager* pEncoderManager)
{
	m_pEncoderManager = pEncoderManager;
}