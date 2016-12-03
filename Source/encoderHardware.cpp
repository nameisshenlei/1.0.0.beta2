#include "encoderHardware.h"
#include "helpFunctions.h"
#include "gst/base/gstbasetransform.h"

encoderHardware::encoderHardware(String encoderName, String memVideoName, String memAudioName)
: baseSilentEncoder(encoderName, memVideoName, memAudioName)
{	
}

encoderHardware::~encoderHardware()
{
}

GstStateChangeReturn encoderHardware::onPlay()
{
	GstElement* elmtAudioConvert = nullptr;
	GstElement* elmtAudioResample = nullptr;
	GstElement* elmtAudioSink = nullptr;
	GstElement* elmtVideoConvert = nullptr;
	GstElement* elmtVideoScale = nullptr;
	GstElement* elmtVideoRate = nullptr;
	GstElement* elmtVideoSink = nullptr;

// 	// audio convert
	elmtAudioConvert = gst_element_factory_make("audioconvert", "audioConvert");
	// audio resample
	elmtAudioResample = gst_element_factory_make("audioresample", "audioResample");
	// audio sink
	elmtAudioSink = gst_element_factory_make("decklinkaudiosink", "audioSink");

	// video convert
	elmtVideoConvert = gst_element_factory_make("videoconvert", "videoConvert");
	// video box
	elmtVideoScale = gst_element_factory_make("videoscale", "videoBox");
	elmtVideoRate = gst_element_factory_make("videorate", "videoRate");
	// video sink
	elmtVideoSink = gst_element_factory_make("decklinkvideosink", "videoSink");
	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		, elmtAudioConvert
		, elmtAudioResample
		, elmtAudioSink
		, elmtVideoConvert
		, elmtVideoRate
		, elmtVideoScale
		, elmtVideoSink
		, nullptr);
	{
		// video sink
		String hwKey = m_settingInfo->getValue(KEY_OUT_HW_KEYWORD, L"AUTO");
		int indexMode = gstHelpString2DecklinkModeIndex(hwKey);
		g_object_set(elmtVideoSink
			, "mode", indexMode
			, "device-number", m_settingInfo->getIntValue(KEY_OUT_HW_INDEX, 0)
			, nullptr);
		g_object_set(elmtVideoScale, "method", 0, /*"qos", false, */nullptr);
	}
	{
		// audio sink
		g_object_set(elmtAudioSink
			, "device-number", m_settingInfo->getIntValue(KEY_OUT_HW_INDEX, 0)
			, nullptr);
// 		gst_base_transform_set_qos_enabled(GST_BASE_TRANSFORM_CAST(elmtVideoScale), false);
// 		gst_base_transform_set_qos_enabled(GST_BASE_TRANSFORM_CAST(elmtVideoConvert), false);
	}
	if (!gst_element_link(m_elmtAudioAppSrc, elmtAudioConvert))
	{
		goto PLAY_FAILED;
	}
	if (!gst_element_link(elmtAudioConvert, elmtAudioResample))
	{
		goto PLAY_FAILED;
	}
	if (!gst_element_link(elmtAudioResample, elmtAudioSink))
	{
		goto PLAY_FAILED;
	}

	if (!gst_element_link(m_elmtVideoAppSrc, elmtVideoScale))
	{
		goto PLAY_FAILED;
	}
	if (!gst_element_link(elmtVideoScale, elmtVideoRate))
	{
		goto PLAY_FAILED;
	}
	if (!gst_element_link(elmtVideoRate, elmtVideoConvert))
	{
		goto PLAY_FAILED;
	}
	if (!gst_element_link(elmtVideoConvert, elmtVideoSink))
	{
		goto PLAY_FAILED;
	}

	GstStateChangeReturn stateRet = gst_element_set_state(m_elmtPipeline, GST_STATE_PAUSED);

	return stateRet;

PLAY_FAILED:
	return GST_STATE_CHANGE_FAILURE;
}

void encoderHardware::timerCallback()
{
	static int i = 0;
// 	if (i == 0)
// 	{
// 		gst_element_set_state(m_elmtPipeline, GST_STATE_PAUSED);
// 	}
	if (i == 3)
	{
		gst_element_set_state(m_elmtPipeline, GST_STATE_PLAYING);
	}
	i++;
}
