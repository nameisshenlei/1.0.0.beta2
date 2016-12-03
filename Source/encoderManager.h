#ifndef NEMO_ENCODER_MANAGER_H
#define NEMO_ENCODER_MANAGER_H

#include "publicHeader.h"
#include "encoderManagerListener.h"
#include "sharedMemoryReader.h"
#include "dataInfo.h"
#include "map"

class baseSilentEncoder;

struct MemMapEncoder
{
	String							strAudioMemoryName;
	String							strVideoMemoryName;
	sharedMemoryReader*				pVideoReader;
	sharedMemoryReader*				pAudioReader;
	//baseSilentEncoder*			pEncoder;
	CriticalSection					LockEncoder;
	std::list<baseSilentEncoder*>	listEncoder;
	CriticalSection					lockVideoBuffer;
	std::list<GstBuffer*>			listVideoBuffers;
	CriticalSection					lockAudioBuffer;
	std::list<GstBuffer*>			listAudioBuffers;

	dataInfo*						pDataInfo;
	std::map<String, bool>			mapEncNameAndInfo;

	MemMapEncoder(String VideoMemoryName, String AudioMemoryName);
	void DestroyMemMapEncoder();
	void pushEncoder(baseSilentEncoder* pEncoder, bool isEnableInfo = false);
	GstBuffer* GetVideoBuffer();
	void PushVideoBuffer(GstBuffer* pBuffer);
	GstBuffer* GetAudioBuffer();
	void PushAudioBuffer(GstBuffer* pBuffer);
};

class nemoEncoderManager : public Thread, public HighResolutionTimer, public MessageListener
{
public:
	nemoEncoderManager();
	virtual ~nemoEncoderManager();

	int start();
	void stop();

	void setListener(nemoEncoderManagerListener* l);

	int createEncoder(String encoderType, String encoderName,String iniFilePath, String memIniFilePath = L"");
	void destroyEncoder(String memVideoName, String memAudioName, String encoderName);
	void reset();

	void run();
	void hiResTimerCallback();
	virtual void handleMessage(const Message& message);
protected:
	bool createSharedMemoryReader(String iniFilePath);
	void destroySharedMemoryReader();

	bool checkEncoderName(String VideoMemName, String AudioMemName, String encoderName);

private:
	static void TimerRoutine(void* lpParam, bool TimerOrWaitFired);

private:
	nemoEncoderManagerListener*				m_listener;

// 	sharedMemoryReader*						m_videoSharedMemoryReader;
// 	sharedMemoryReader*						m_audioSharedMemoryReader;

// 	CriticalSection							m_lockEncoders;
// 	std::list<baseSilentEncoder*>			m_listEncoders;
	CriticalSection							m_lockMemMapEncoder;
	std::list<MemMapEncoder*>				m_listMemMapEncoder;

	GMainLoop*								m_gstMainLoop;

 	void*									m_hTimerQueue;
 	void*									m_hTimer;
};

#endif