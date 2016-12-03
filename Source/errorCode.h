#ifndef SILENT_ENCODERS_ERROR_CODE_H
#define SILENT_ENCODERS_ERROR_CODE_H

#define SE_ERROR_CODE_SUCCESS								1000

#define SE_ERROR_CODE_STOPPED								1100
#define SE_ERROR_CODE_RUNNING_INFO							1200

#define SE_ERROR_CODE_FAILED								1201
#define SE_ERROR_CODE_INI_FILE_FAILED						1202
#define SE_ERROR_CODE_SHARED_MEMEORY_VIDEO_FAILED			1203
#define SE_ERROR_CODE_SHARED_MEMEORY_AUDIO_FAILED			1204
#define SE_ERROR_CODE_CODER_EXIST							1205
#define SE_ERROR_CODE_CODER_NOT_FOUND_TYPE					1206
#define SE_ERROR_CODE_PLAY_FAILED							1207

// save file
#define SE_ERROR_CODE_FILE_RECORING							2000
#define SE_ERROR_CODE_FILE_DISK_WILL_FULL					2001
#define SE_ERROR_CODE_FILE_DISK_FULL						2002
#define SE_ERROR_CODE_FILE_ERROR							2003

// rtp
#define SE_ERROR_CODE_RTP_NET_ERROR							2100
#define SE_ERROR_CODE_RTP_UPING								2101
#define SE_ERROR_CODE_RTP_NET_ERROR_PAUSE					2102
#define SE_ERROR_CODE_RTP_NET_ERROR_RESUME					2103
#define SE_ERROR_CODE_RTP_NET_TOO_SLOW						2104

// MPT
#define SE_ERROR_CODE_MPT_NET_ERROR							2200
#define SE_ERROR_CODE_MPT_RUNNING							2201
#define SE_ERROR_CODE_MPT_NET_ERROR_PAUSE					2202
#define SE_ERROR_CODE_MPT_NET_ERROR_RESUME					2203
#define SE_ERROR_CODE_MPT_NET_TOO_SLOW						2204
#define SE_ERROR_CODE_MPT_CREATE_CACHE_FAILED				2205
#define SE_ERROR_CODE_MPT_DISK_FULL							2206

// decklink
#define SE_ERROR_CODE_HARDWARE_NONE							2300
#define SE_ERROR_CODE_HARDWARE_RUNNING						2301
#define SE_ERROR_CODE_HARDWARE_PAUSE						2302
#define SE_ERROR_CODE_HARDWARE_RESUME						2303



#endif