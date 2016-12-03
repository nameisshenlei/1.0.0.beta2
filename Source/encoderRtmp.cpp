#include "encoderRtmp.h"
#include "errorCode.h"
#include "helpFunctions.h"

encoderRtmp::encoderRtmp(String encoderName, String memVideoName, String memAudioName)
: baseSilentEncoder(encoderName, memVideoName, memAudioName)
{

}

encoderRtmp::~encoderRtmp()
{

}

GstStateChangeReturn encoderRtmp::onPlay()
{
	String strURI = m_settingInfo->getValue(KEY_OUT_URI, "");
	if (strURI.isEmpty())
	{
		return GST_STATE_CHANGE_FAILURE;
	}

	GstElement* elmtFlvMux = nullptr;
	GstElement* elmtRtmpSink2 = nullptr;

	// mpeg-ts muxer
	elmtFlvMux = gst_element_factory_make("flvmux", "flvMux");
	// rtmp protocol
	elmtRtmpSink2 = gst_element_factory_make("rtmp2sink", "rtmpSink2");


	if (!elmtFlvMux || !elmtRtmpSink2 )
	{
		return GST_STATE_CHANGE_FAILURE;
	}

	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		, elmtFlvMux
		, elmtRtmpSink2
		, nullptr);

	//////////////////////////////////////////////////////////////////////////

	{
		// set rtmp uri
		g_object_set(elmtRtmpSink2, "location", strURI.toRawUTF8(), nullptr);
	}

//////////////////////////////////////////////////////////////////////
	// video process
	if (video_h264_encoder(elmtFlvMux) != 0)
	{
		return GST_STATE_CHANGE_FAILURE;
	}
	// audio process
	if (audio_aac_encoder(elmtFlvMux) != 0)
	{
		return GST_STATE_CHANGE_FAILURE;
	}
	if (!gst_element_link(elmtFlvMux, elmtRtmpSink2))
	{
		goto PLAY_FAILED;
	}
	//////////////////////////////////////////////////////////////////////////

	GstStateChangeReturn stateRet = gst_element_set_state(m_elmtPipeline, GST_STATE_PLAYING);
	return stateRet;

PLAY_FAILED:
	return GST_STATE_CHANGE_FAILURE;
}

void encoderRtmp::timerCallback()
{
	// sent-bytes
	GstElement* elmt = gst_bin_get_by_name(GST_BIN(m_elmtPipeline), "rtmpSink2");
	if (elmt)
	{
		guint64 total_bytes = 0;
		g_object_get(elmt, "sent-bytes", &total_bytes, nullptr);
		gst_object_unref(elmt);

		writePropertiesInIniFile(m_iniFullPath, SE_ERROR_CODE_RUNNING_INFO, m_hGlobalEvent, String(L"ÒÑ·¢ËÍ ") + bytesToString(total_bytes));
	}
}