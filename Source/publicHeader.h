#ifndef NEMO_PUBLIC_HEADER_H
#define NEMO_PUBLIC_HEADER_H

#include "../JuceLibraryCode/JuceHeader.h"
//////////////////////////////////////////////////////////////////////////
#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4127)

#include <gst/gst.h>
#include <gst/gstplugin.h>
#include <gst/gstpluginfeature.h>
#include <gst/gstelementfactory.h>
#include <gst/video/videooverlay.h>
#include <gst/app/app.h>
#include <glib.h>

#pragma comment(lib, "glib-2.0.lib")
#pragma comment(lib, "gobject-2.0.lib")
#pragma comment(lib, "gstreamer-1.0.lib")
#pragma comment(lib, "gstapp-1.0.lib")
#pragma comment(lib, "gstvideo-1.0.lib")
#pragma comment(lib, "gstaudio-1.0.lib")
#pragma comment(lib, "gstbase-1.0.lib")
#pragma comment(lib, "gio-2.0.lib")
#pragma comment(lib, "gstpbutils-1.0.lib")



#pragma warning(pop)

//////////////////////////////////////////////////////////////////////////
// log
#define NEMO_LOG(x,l)	\
{	\
	String nemo_log_msg;	\
	nemo_log_msg << Time::getCurrentTime().formatted(L"%Y-%m-%d %H:%M:%S") << L"\t";	\
	nemo_log_msg << #l << L"\t" << x;	\
	Logger::writeToLog(nemo_log_msg);	\
}

#define NEMO_LOG_INFO(x)	\
	NEMO_LOG(x, INFO)

#define NEMO_LOG_ERROR(x)	\
	NEMO_LOG(x, ERROR)

#define NEMO_LOG_WARNING(x)	\
	NEMO_LOG(x, WARNING)

#define NEMO_LOG_DEBUG(x)	\
	NEMO_LOG(x, DEBUG)

//////////////////////////////////////////////////////////////////////////
// command line
#define CMD_ENCODER_CREATE							"create"
#define CMD_ENCODER_DESTROY							"destroy"
#define CMD_ENCODER_INIFILE							"inifile"
#define CMD_ENCODER_NAME							"name"
#define CMD_ENCODER_RESET							"reset"
#define CMD_ENCODER_BYE								"bye"
#define CMD_ENCODER_MEM_INIFILE						"memfile"

#define CMD_ENCODER_CREATE_TYPE_HARDWARE			"hardware"
#define CMD_ENCODER_CREATE_TYPE_SAVEFILE			"savefile"
#define CMD_ENCODER_CREATE_TYPE_RTMP				"rtmp"
#define CMD_ENCODER_CREATE_TYPE_RTP					"rtp"

//////////////////////////////////////////////////////////////////////////
// setting
#define KEY_ENCODER_NAME							"encoder_name"
#define KEY_ENCODER_TYPE							"encoder_type"

#define KEY_PROCESS_EVENT_NAME						"process_event_name"

#define KEY_ACCELERATION_TYPE						"acceleration type"

#define KEY_MEM_VIDEO_NAME							"mem_video_name"
#define KEY_MEM_VIDEO_WIDTH							"mem_video_width"
#define KEY_MEM_VIDEO_HEIGHT						"mem_video_height"
#define KEY_MEM_VIDEO_BUFFERS						"mem_video_buffers"

#define KEY_MEM_AUDIO_NAME							"mem_audio_name"
#define KEY_MEM_AUDIO_CHS							"mem_audio_channles"
#define KEY_MEM_AUDIO_FREQ							"mem_audio_frequency"
#define KEY_MEM_AUDIO_BLOCKSIZE						"mem_audio_blocksize"
#define KEY_MEM_AUDIO_BUFFERS						"mem_audio_buffers"
#define KEY_MEM_AUDIO_SAMPLE_FORMAT					"mem_audio_sample_format"

#define KEY_OUT_VIDEO_WIDTH							"output_video_width"
#define KEY_OUT_VIDEO_HEIGHT						"output_video_height"
#define KEY_OUT_VIDEO_BITRATE						"output_video_bitrate"

#define KEY_OUT_VIDEO_ANALYSE						"output_video_analyse"			//default: 0
#define KEY_OUT_VIDEO_AUD							"output_video_aud"				//default: true
#define KEY_OUT_VIDEO_B_ADAPT						"output_video_b_adapt"			//default: true
#define KEY_OUT_VIDEO_B_PYRAMID						"output_video_b_pyramid"		//default: false
#define KEY_OUT_VIDEO_B_FRAMES						"output_video_b_frames"			//default: 0		allowed: <= 4
#define KEY_OUT_VIDEO_CABAC							"output_video_cabac"			//default: true
#define KEY_OUT_VIDEO_DCT8X8						"output_video_dct8x8"			//default: false
#define KEY_OUT_VIDEO_INTERLACED					"output_video_interlaced"		//default: false
#define KEY_OUT_VIDEO_IP_FACTOR						"output_video_ip_factor"		//default: 1.4		allowed: [0, 2]
#define KEY_OUT_VIDEO_GOP							"output_video_gop"				//default: 0		allowed: [0, 2147483647]
#define KEY_OUT_VIDEO_ME							"output_video_me"				//default: 1		allowed: [0, 4]
#define KEY_OUT_VIDEO_SUBME							"output_video_subme"			//default: 1		allowed: [0, 10]
#define KEY_OUT_VIDEO_NOISE_REDUCTION				"output_video_noise_reduction"	//default: 0		allowed: <= 10000
#define KEY_OUT_VIDEO_PASS							"output_video_pass"				//default: 0		allowed: 0, 4, 5, 17, 18, 19
#define KEY_OUT_VIDEO_PB_FACTOR						"output_video_pb_factor"		//default: 1.3		allowed: [0, 2]
#define KEY_OUT_VIDEO_QP_MAX						"output_video_qp_max"			//default: 51		allowed: <= 51
#define KEY_OUT_VIDEO_QP_MIN						"output_video_qp_min"			//default: 10		allowed: <= 51
#define KEY_OUT_VIDEO_QP_STEP						"output_video_qp_step"			//default: 4		allowed: <= 50
#define KEY_OUT_VIDEO_QUANTIZER						"output_video_quantizer"		//default: 21		allowed: <= 50
#define KEY_OUT_VIDEO_REF							"output_video_ref"				//default: 1		allowed: [1,12]
#define KEY_OUT_VIDEO_SPS_ID						"output_video_sps_id"			//default: 0		allowed: <= 31
#define KEY_OUT_VIDEO_THREADS						"output_video_threads"			//default: 0		allowed: [0, 4]
#define KEY_OUT_VIDEO_TRELLIS						"output_video_trellis"			//default: true
#define KEY_OUT_VIDEO_VBV_BUF_CAPACITY				"output_video_vbv_buf_capacity" //default: 600		allowed: <= 10000
#define KEY_OUT_VIDEO_WEIGHTB						"output_video_weightb"			//default: false
#define KEY_OUT_VIDEO_INTRA_REFRESH					"output_video_intar_refresh"	//default: false
#define KEY_OUT_VIDEO_MB_TREE						"output_video_mb_tree"			//default: true
#define KEY_OUT_VIDEO_RC_LOOKAHEAD					"output_video_rc_lookahead"		//default: 0		allowed: [0,250]
#define KEY_OUT_VIDEO_SLICED_THREADS				"output_video_sliced_threads"	//default: true
#define KEY_OUT_VIDEO_SYNC_LOOKAHEAD				"output_video_sync_lookahead"	//default: -1		allowed: [-1,250]
#define KEY_OUT_VIDEO_PROFILE						"output_video_profile"			//default: main		allowed:
#define KEY_OUT_VIDEO_PSY_TUNE						"output_video_psy_tune"			//default: 0		allowed: [0, 5]
#define KEY_OUT_VIDEO_PRESET						"output_video_preset"			//default: 6		allowed: [0,10]
#define KEY_OUT_VIDEO_TUNE							"output_video_tune"				//default: 0		allowed: 0, 2, 4

#define KEY_OUT_AUDIO_CHS							"output_audio_channels"
#define KEY_OUT_AUDIO_FREQ							"output_audio_frequency"
#define KEY_OUT_AUDIO_BITRATE						"output_audio_bitrate"

#define KEY_OUT_URI									"output_uri"

#define KEY_OUT_HW_CARD								"output_hw_card"
#define KEY_OUT_HW_INDEX							"output_hw_index"
#define KEY_OUT_HW_KEYWORD							"output_hw_keyword"

#define KEY_OUT_DATA_INFO							"output_data_info"
#define KEY_OUT_DATA_INFO_HZ						"output_data_info_hz"
//////////////////////////////////////////////////////////////////////////
#define BYTES_G										0x4000000
#define BYTES_M										 0x100000
#define BYTES_K										    0x400
//////////////////////////////////////////////////////////////////////////
#endif