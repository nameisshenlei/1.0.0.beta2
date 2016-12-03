#include "encoderSaveFile.h"
#include "helpFunctions.h"
#include "errorCode.h"

encoderSaveFile::encoderSaveFile(String encoderName, String memVideoName, String memAudioName)
: baseSilentEncoder(encoderName, memVideoName, memAudioName)
{

}

encoderSaveFile::~encoderSaveFile()
{

}

GstStateChangeReturn encoderSaveFile::onPlay()
{
	String strURI = m_settingInfo->getValue(KEY_OUT_URI, "");
	if (strURI.isEmpty())
	{
		return GST_STATE_CHANGE_FAILURE;
	}
	saveFileType saveType = saveFileTypeUnknown;
	{
		File fileDst(strURI);
		String strExten = fileDst.getFileExtension();
		if (strExten.equalsIgnoreCase(L".mp4"))
		{
			saveType = saveFileTypeMP4;
		}
		else if (strExten.equalsIgnoreCase(L".mkv"))
		{
			saveType = saveFileTypeMKV;
		}
		else if (strExten.equalsIgnoreCase(L".flv"))
		{
			saveType = saveFileTypeFLV;
		}
		else if (strExten.equalsIgnoreCase(L".mov"))
		{
			saveType = saveFileTypeMOV;
		}
		else if (strExten.equalsIgnoreCase(L".webm"))
		{
			saveType = saveFileTypeWebm;
		}
		else if (strExten.equalsIgnoreCase(L".avi"))
		{
			saveType = saveFileTypeAVI;
		}
		else
		{
			return GST_STATE_CHANGE_FAILURE;
		}
	}

	if (saveType == saveFileTypeUnknown)
	{
		return GST_STATE_CHANGE_FAILURE;
	}


	GstElement* elmtMux = nullptr;
	GstElement* elmtFileSink = nullptr;

	//
	elmtFileSink = gst_element_factory_make("filesink", "fileSink");
	
	if (saveType == saveFileTypeMP4)
	{
		elmtMux = gst_element_factory_make("mp4mux", "mp4Mux");
		g_object_set(elmtMux, "faststart", true, nullptr);
		g_object_set(elmtMux, "streamable", false, nullptr);
	}
	else if (saveType == saveFileTypeMKV)
	{
		elmtMux = gst_element_factory_make("matroskamux", "mkvMux");

		g_object_set(elmtMux, "writing-app", "tsingMedia", nullptr);
		g_object_set(elmtMux, "streamable", false, nullptr);
	}
	else if (saveType == saveFileTypeFLV)
	{
		elmtMux = gst_element_factory_make("flvmux", "flvMux");
	}
	else if (saveType == saveFileTypeMOV)
	{
		elmtMux = gst_element_factory_make("qtmux", "movMux");
	}
	else if (saveType == saveFileTypeWebm)
	{
		elmtMux = gst_element_factory_make("webmmux", "webmMux");
	}
	else if (saveType == saveFileTypeAVI)
	{
		elmtMux = gst_element_factory_make("avimux", "aviMux");
	}


	if (!elmtMux || !elmtFileSink)
	{
		return GST_STATE_CHANGE_FAILURE;
	}

	gst_bin_add_many(GST_BIN(m_elmtPipeline)
		, elmtMux
		, elmtFileSink
		, nullptr);

	//////////////////////////////////////////////////////////////////////////
	g_object_set(elmtFileSink, "location", strURI.toUTF8(), nullptr);
	//////////////////////////////////////////////////////////////////////
	if (!gst_element_link(elmtMux, elmtFileSink))
	{
		return GST_STATE_CHANGE_FAILURE;
	}
	
	if (saveType == saveFileTypeWebm)
	{
		if (video_vpx_encoder(elmtMux) != 0)
		{
			return GST_STATE_CHANGE_FAILURE;
		}
		if (audio_vorbis_encoder(elmtMux) != 0)
		{
			return GST_STATE_CHANGE_FAILURE;
		}
	}
// 	else if (saveType == saveFileTypeAVI)
// 	{
// 		// video process
// 		if (video_x264_encoder(elmtMux) != 0)
// 		{
// 			return GST_STATE_CHANGE_FAILURE;
// 		}
// 		// audio process
// 		if (audio_mp3_encoder(elmtMux) != 0)
// 		{
// 			return GST_STATE_CHANGE_FAILURE;
// 		}
// 	}
	else
	{
		// video process
		if (video_h264_encoder(elmtMux) != 0)
		{
			return GST_STATE_CHANGE_FAILURE;
		}
		// audio process
		if (audio_aac_encoder(elmtMux) != 0)
		{
			return GST_STATE_CHANGE_FAILURE;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	// 	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_elmtPipeline));
	// 	gst_bus_add_signal_watch(bus);
	// 	g_signal_connect(bus, "message", G_CALLBACK(nemogst_signal_rtmp_push_bus_watch), this);
	// 	gst_object_unref(bus);

	return gst_element_set_state(m_elmtPipeline, GST_STATE_PLAYING);
}

void encoderSaveFile::timerCallback()
{
	GstElement* elmt = gst_bin_get_by_name(GST_BIN(m_elmtPipeline), "fileSink");
	if (elmt)
	{
		gint64 n_pos = 0;
		GstQuery* q = nullptr;
		gboolean ret = false;
		q = gst_query_new_position(GST_FORMAT_TIME);
		ret = gst_element_query(elmt, q);
		if (!ret)
		{
			gst_object_unref(elmt);
			return;
		}
		gst_query_parse_position(q, nullptr, &n_pos);
		gst_query_unref(q);
		q = nullptr;

		String infoMsg;
		infoMsg << L"正在录制,已录制时长 " << String::formatted(L"%u:%02u:%02u", GST_TIME_ARGS(n_pos));
		writePropertiesInIniFile(m_iniFullPath, SE_ERROR_CODE_RUNNING_INFO, m_hGlobalEvent, infoMsg);
	}
}