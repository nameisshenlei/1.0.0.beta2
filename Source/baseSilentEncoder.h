#ifndef NEMO_BASE_SILENT_ENCODER_H
#define NEMO_BASE_SILENT_ENCODER_H

#include "publicHeader.h"
#include "helpFunctions.h"

enum ProFile
{
	PROFILE_AUTOSELECT_GUID = 0,
	PROFILE_BASELINE_GUID = 1,
	PROFILE_MAIN_GUID = 2,
	PROFILE_HIGH_GUID = 3,
	PROFILE_HIGH_444_GUID = 4,
	PROFILE_STEREO_GUID = 5,
	PROFILE_SVC_TEMPORAL_SCALABILTY = 6,
	PROFILE_CONSTRAINED_HIGH_GUID = 7,
};
enum PreSet
{
	PRESET_DEFAULT_GUID = 0,
	PRESET_HP_GUID = 1,
	PRESET_HQ_GUID = 2,
	PRESET_LOW_LATENCY_DEFAULT_GUID = 3,
	PRESET_LOW_LATENCY_HQ_GUID = 4,
	PRESET_LOW_LATENCY_HP_GUID = 5,
	PRESET_LOSSLESS_DEFAULT_GUID = 6,
	PRESET_LOSSLESS_HP_GUID = 7,
};

class nemoEncoderManager;

class baseSilentEncoder : public Timer
{
public:
	baseSilentEncoder(String encoderName, String memVideoName, String memAudioName);
	virtual ~baseSilentEncoder();

	int play(PropertySet* setting, vOutputCfg* pvOutputCfg, String iniFullPath, void* hGlobalEvent);

	void stop();

	void pushVideoBuffer(GstBuffer* b);
	void pushAudioBuffer(GstBuffer* b);

	const String& getEncoderName();
	void busMsgWatch(GstBus* bus, GstMessage* message);

	void SetManager(nemoEncoderManager* pEncoderManager);
protected:
	virtual GstStateChangeReturn onPlay() = 0;
	virtual void onStop();

	int initCommonEncoderElements();
	void writeStateIntoIni(String strCode, String strMsg);

	int video_h264_encoder(GstElement* mux);
	int video_vpx_encoder(GstElement* mux);
	int audio_aac_encoder(GstElement* mux);
	int audio_vorbis_encoder(GstElement* mux);
	int audio_mp3_encoder(GstElement* mux);
protected:
	String								m_memVideoName;
	String								m_memAudioName;
	String								m_encoderName;
	PropertySet*						m_settingInfo;
	vOutputCfg*							m_pvOutputCfg;
	String								m_iniFullPath;

	// pipeline
	GstElement*							m_elmtPipeline;
	// video app src
	GstElement*							m_elmtVideoAppSrc;
	// audio app src
	GstElement*							m_elmtAudioAppSrc;

	void*								m_hGlobalEvent;
private:
	bool								m_videoFirst;
	GstClockTime						m_timeVideoBeginPts;
	bool								m_audioFirst;
	GstClockTime						m_timeAudioBeginPts;

// 	GMutex								m_lockStateAsync;
// 	GCond								m_condStateAsync;
	nemoEncoderManager*					m_pEncoderManager;
};

#endif