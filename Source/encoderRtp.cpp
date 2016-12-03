#include "encoderRtp.h"

#include "helpFunctions.h"
#include "errorCode.h"

encoderRtp::encoderRtp(String encoderName, String memVideoName, String memAudioName)
: baseSilentEncoder(encoderName, memVideoName, memAudioName)
{

}

encoderRtp::~encoderRtp()
{

}

GstStateChangeReturn encoderRtp::onPlay()
{
	String strURI = m_settingInfo->getValue(KEY_OUT_URI, "");
	if (strURI.isEmpty())
	{
		return GST_STATE_CHANGE_FAILURE;
	}
	strURI = strURI.fromFirstOccurrenceOf("rtp://", false, true);
	String strIp;
	int iPort = 0;
	strIp = strURI.upToFirstOccurrenceOf(":", false, true);
	iPort = strURI.fromFirstOccurrenceOf(":", false, true).getIntValue();
	if (strIp.isEmpty() || iPort <1024 || iPort > 65534)
	{
		return GST_STATE_CHANGE_FAILURE;
	}

	GstElement* elmtMPEGTSMux = nullptr;
	
	GstElement* elmtRtpMP2TPay = nullptr;
	GstElement* elmtUdpSink = nullptr;

	// mpeg-ts muxer
	elmtMPEGTSMux = gst_element_factory_make("mpegtsmux", "mpegtsMux");

	// rtp mp2tpay
	elmtRtpMP2TPay = gst_element_factory_make("rtpmp2tpay", "rtpMp2tPay");
	// udp sink
	elmtUdpSink = gst_element_factory_make("udpsink", "udpSink");

	if (!m_elmtVideoAppSrc || !m_elmtAudioAppSrc || !elmtMPEGTSMux || !elmtRtpMP2TPay || !elmtUdpSink)
	{
		return GST_STATE_CHANGE_FAILURE;
	}

	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		, elmtMPEGTSMux
		, elmtRtpMP2TPay
		, elmtUdpSink
		, nullptr);

	//////////////////////////////////////////////////////////////////////////

	{
		g_object_set(elmtMPEGTSMux,
			"alignment", 7,
			nullptr);
	}

	{
		// udp sink
		g_object_set(elmtUdpSink, "host", strIp.toUTF8(), nullptr);
		g_object_set(elmtUdpSink, "port", iPort, nullptr);
	}
	////////////////////////////////////////////////////////////////////////
	// video process
	if (video_h264_encoder(elmtMPEGTSMux) != 0)
	{
		return GST_STATE_CHANGE_FAILURE;
	}
	// audio process
	if (audio_aac_encoder(elmtMPEGTSMux) != 0)
	{
		return GST_STATE_CHANGE_FAILURE;
	}

	if (!gst_element_link(elmtMPEGTSMux, elmtRtpMP2TPay))
	{
		goto PLAY_FAILED;
	}
	if (!gst_element_link(elmtRtpMP2TPay, elmtUdpSink))
	{
		goto PLAY_FAILED;
	}

	GstStateChangeReturn stateRet = gst_element_set_state(m_elmtPipeline, GST_STATE_PLAYING);
	return stateRet;

PLAY_FAILED:
	return GST_STATE_CHANGE_FAILURE;
}

void encoderRtp::timerCallback()
{
	GstElement* elmt = gst_bin_get_by_name(GST_BIN(m_elmtPipeline), "udpSink");
	if (elmt)
	{
		guint64 total_bytes = 0;
		g_object_get(elmt, "bytes-served", &total_bytes, nullptr);
		gst_object_unref(elmt);

		writePropertiesInIniFile(m_iniFullPath, SE_ERROR_CODE_RUNNING_INFO,m_hGlobalEvent, String(L"ÒÑ·¢ËÍ ") + bytesToString(total_bytes));
	}
}