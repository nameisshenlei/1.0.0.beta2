#include "CrashRpt.h"
#include "encoderManager.h"
#include "encoderSaveFile.h"
#include "encoderHardware.h"
#include "encoderRtmp.h"
#include "encoderRtp.h"
#include "helpFunctions.h"
#include "errorCode.h"
#include <windows.h>
#include "juceMessage.h"
#include "baseSilentEncoder.h"

//void* g_hDoneEvent = NULL;

MemMapEncoder::MemMapEncoder(String VideoMemoryName, String AudioMemoryName)
{
	pVideoReader = nullptr;
	pAudioReader = nullptr;
	strVideoMemoryName = VideoMemoryName;
	strAudioMemoryName = AudioMemoryName;
	pDataInfo = new dataInfo(VideoMemoryName, AudioMemoryName);
	OutputDebugString(L"new dataInfo");
}

void MemMapEncoder::DestroyMemMapEncoder()
{
	if (pVideoReader)
	{
		pVideoReader->closeSharedMemory();
		deleteAndZero(pVideoReader);
	}
	if (pAudioReader)
	{
		pAudioReader->closeSharedMemory();
		deleteAndZero(pAudioReader);
	}
	baseSilentEncoder* pEncoder = nullptr;
	{
		ScopedLock lock(LockEncoder);
		while (!listEncoder.empty())
		{
			pEncoder = listEncoder.front();
			listEncoder.pop_front();
			pEncoder->stop();
			deleteAndZero(pEncoder);
		}
	}
	GstBuffer* pBuffer = nullptr;
	{
		ScopedLock lock(lockVideoBuffer);
		while (!listVideoBuffers.empty())
		{
			pBuffer = listVideoBuffers.front();
			listVideoBuffers.pop_front();
			gst_buffer_unref(pBuffer);
		}
	}
	{
		ScopedLock lock(lockAudioBuffer);
		while (!listAudioBuffers.empty())
		{
			pBuffer = listAudioBuffers.front();
			listAudioBuffers.pop_front();
			gst_buffer_unref(pBuffer);
		}
	}
	if (pDataInfo)
	{
		pDataInfo->DirectStopTimer();
		mapEncNameAndInfo.clear();
		deleteAndZero(pDataInfo);
	}
}

void MemMapEncoder::pushEncoder(baseSilentEncoder* pEncoder, bool isEnableInfo)
{
	ScopedLock lock(LockEncoder);
	listEncoder.push_back(pEncoder);
	mapEncNameAndInfo.insert(make_pair(pEncoder->getEncoderName(), isEnableInfo));
}

GstBuffer* MemMapEncoder::GetVideoBuffer()
{
	if (listVideoBuffers.empty())
	{
		return nullptr;
	}
	ScopedLock lock(lockVideoBuffer);
	GstBuffer* pBuffer = listVideoBuffers.front();
	listVideoBuffers.pop_front();
	return pBuffer;
}

void MemMapEncoder::PushVideoBuffer(GstBuffer* pBuffer)
{
	ScopedLock lock(lockVideoBuffer);
	listVideoBuffers.push_back(pBuffer);
}

GstBuffer* MemMapEncoder::GetAudioBuffer()
{
	if (listAudioBuffers.empty())
	{
		return nullptr;
	}
	ScopedLock lock(lockAudioBuffer);
	GstBuffer* pBuffer = listAudioBuffers.front();
	listAudioBuffers.pop_front();
	return pBuffer;
}

void MemMapEncoder::PushAudioBuffer(GstBuffer* pBuffer)
{
	ScopedLock lock(lockAudioBuffer);
	listAudioBuffers.push_back(pBuffer);
}


nemoEncoderManager::nemoEncoderManager()
: Thread(L"gstMainLoop")
, m_listener(nullptr)
// , m_videoSharedMemoryReader(nullptr)
// , m_audioSharedMemoryReader(nullptr)
, m_gstMainLoop(nullptr)
//, m_hTimer(NULL)
//, m_hTimerQueue(NULL)
{

}

nemoEncoderManager::~nemoEncoderManager()
{

}

int nemoEncoderManager::start()
{
	m_gstMainLoop = g_main_loop_new(nullptr, false);
	startThread();

	return 0;
}

void nemoEncoderManager::stop()
{
	if (isTimerRunning())
		stopTimer();

// 	if (WaitForSingleObject(g_hDoneEvent, INFINITE) == WAIT_OBJECT_0 && m_hTimer)
// 	{
// 		CloseHandle(g_hDoneEvent);
// 		DeleteTimerQueue(m_hTimerQueue);
// 	}
	{
// 		baseSilentEncoder* e = nullptr;
// 		ScopedLock lock(m_lockEncoders);
// 		while (m_listEncoders.size() > 0)
// 		{
// 			e = m_listEncoders.front();
// 			m_listEncoders.pop_front();
// 
// 			e->stop();
// 			deleteAndZero(e);
// 		}
		MemMapEncoder* pMemMapEncoder = nullptr;
		ScopedLock lock(m_lockMemMapEncoder);
		while (m_listMemMapEncoder.size())
		{
			pMemMapEncoder = m_listMemMapEncoder.front();
			m_listMemMapEncoder.pop_front();
			pMemMapEncoder->DestroyMemMapEncoder();
			deleteAndZero(pMemMapEncoder);
		}
	}
	if (m_gstMainLoop)
	{
		g_main_loop_quit(m_gstMainLoop);
		signalThreadShouldExit();
		waitForThreadToExit(1000);
		g_main_loop_unref(m_gstMainLoop);
		m_gstMainLoop = nullptr;
	}

// 	if (m_videoSharedMemoryReader)
// 	{
// 		m_videoSharedMemoryReader->closeSharedMemory();
// 		deleteAndZero(m_videoSharedMemoryReader);
// 	}
// 	if (m_audioSharedMemoryReader)
// 	{
// 		m_audioSharedMemoryReader->closeSharedMemory();
// 		deleteAndZero(m_audioSharedMemoryReader);
// 	}
}

void nemoEncoderManager::setListener(nemoEncoderManagerListener* l)
{
	m_listener = l;
}

int nemoEncoderManager::createEncoder(String encoderType, String encoderName, String iniFilePath, String memIniFilePath)
{
	PropertySet* newSetting = nullptr;
	vOutputCfg* newOutputCfg = nullptr;
// 	sharedMemoryReader* videoReader = nullptr;
// 	sharedMemoryReader* audioReader = nullptr;
	baseSilentEncoder* newEncoder = nullptr;

	MemMapEncoder* pMemMapEncoder = NULL;
	bool bMemAlreadyExist = false;
	std::list<MemMapEncoder*>::iterator iter;

	int errorCode = SE_ERROR_CODE_SUCCESS;
	void* hGlobalEvent = nullptr;

	newSetting = readPropertiesFromIni(iniFilePath, memIniFilePath, &newOutputCfg);
	if (!newSetting)
	{
		errorCode = SE_ERROR_CODE_INI_FILE_FAILED;
		goto CREATE_FAILED;
	}
	newSetting->setValue(KEY_ENCODER_NAME, encoderName);
	newSetting->setValue(KEY_ENCODER_TYPE, encoderType);

	{
		String globalEventName;
		globalEventName = newSetting->getValue(KEY_PROCESS_EVENT_NAME, L"");
		if (!globalEventName.isEmpty())
		{
			hGlobalEvent = ::OpenEvent(EVENT_ALL_ACCESS, NULL, globalEventName.toWideCharPointer());
			if (!hGlobalEvent)
			{
				hGlobalEvent = ::CreateEvent(NULL, FALSE, FALSE, globalEventName.toWideCharPointer());
			}
		}
	}

	for (iter = m_listMemMapEncoder.begin(); iter != m_listMemMapEncoder.end(); ++iter)
	{
		if ((*iter)->strAudioMemoryName == newSetting->getValue(KEY_MEM_AUDIO_NAME, L"") && (*iter)->strVideoMemoryName == newSetting->getValue(KEY_MEM_VIDEO_NAME, L""))
		{
			bMemAlreadyExist = true;
			pMemMapEncoder = *iter;
			break;
		}
	}

	if (m_listMemMapEncoder.empty() || !bMemAlreadyExist)
	{
		pMemMapEncoder = new MemMapEncoder(newSetting->getValue(KEY_MEM_VIDEO_NAME, L""), newSetting->getValue(KEY_MEM_AUDIO_NAME, L""));
	}
	
	if (!bMemAlreadyExist)
	{
		pMemMapEncoder->pVideoReader = new sharedMemoryReader();
// 		if (!pMemMapEncoder->pVideoReader->openSharedMemory(
// 			newSetting->getValue(KEY_MEM_VIDEO_NAME),
// 			newSetting->getIntValue(KEY_MEM_VIDEO_BUFFERS),
// 			newSetting->getIntValue(KEY_MEM_VIDEO_WIDTH) * newSetting->getIntValue(KEY_MEM_VIDEO_HEIGHT) * 4))
		if (!pMemMapEncoder->pVideoReader->openSharedMemory(
			newSetting->getValue(KEY_MEM_VIDEO_NAME),
			newSetting->getIntValue(KEY_MEM_VIDEO_BUFFERS),
			newSetting->getIntValue(KEY_MEM_VIDEO_WIDTH),
			newSetting->getIntValue(KEY_MEM_VIDEO_HEIGHT)))
		{
			errorCode = SE_ERROR_CODE_SHARED_MEMEORY_VIDEO_FAILED;
			goto CREATE_FAILED;
		}
		pMemMapEncoder->pVideoReader->SetCheckFrame((bool)newOutputCfg->mapIntCfgs[String("check-frame")]);
	}

	if (!bMemAlreadyExist)
	{
		pMemMapEncoder->pAudioReader = new sharedMemoryReader();
		if (!pMemMapEncoder->pAudioReader->openSharedMemory(
			newSetting->getValue(KEY_MEM_AUDIO_NAME),
			newSetting->getIntValue(KEY_MEM_AUDIO_BUFFERS),
			newSetting->getIntValue(KEY_MEM_AUDIO_BLOCKSIZE)))
		{
			errorCode = SE_ERROR_CODE_SHARED_MEMEORY_AUDIO_FAILED;
			goto CREATE_FAILED;
		}
	}

	// create silent encoder
	if (checkEncoderName(newSetting->getValue(KEY_MEM_VIDEO_NAME, L""), newSetting->getValue(KEY_MEM_AUDIO_NAME, L""), encoderName))
	{
		errorCode = SE_ERROR_CODE_CODER_EXIST;
		goto CREATE_FAILED;
	}
	
	if (encoderType.equalsIgnoreCase(CMD_ENCODER_CREATE_TYPE_HARDWARE))
	{
		// hardware
		newEncoder = new encoderHardware(encoderName, pMemMapEncoder->strVideoMemoryName, pMemMapEncoder->strAudioMemoryName);
	}
	else if (encoderType.equalsIgnoreCase(CMD_ENCODER_CREATE_TYPE_SAVEFILE))
	{
		// save file
		newEncoder = new encoderSaveFile(encoderName, pMemMapEncoder->strVideoMemoryName, pMemMapEncoder->strAudioMemoryName);
	}
	else if (encoderType.equalsIgnoreCase(CMD_ENCODER_CREATE_TYPE_RTMP))
	{
		// rtmp
		newEncoder = new encoderRtmp(encoderName, pMemMapEncoder->strVideoMemoryName, pMemMapEncoder->strAudioMemoryName);
	}
	else if (encoderType.equalsIgnoreCase(CMD_ENCODER_CREATE_TYPE_RTP))
	{
		// rtp
		newEncoder = new encoderRtp(encoderName, pMemMapEncoder->strVideoMemoryName, pMemMapEncoder->strAudioMemoryName);
	}

	if (!newEncoder)
	{
		errorCode = SE_ERROR_CODE_CODER_NOT_FOUND_TYPE;
		goto CREATE_FAILED;
	}

	newEncoder->SetManager(this);
	if (newEncoder->play(newSetting, newOutputCfg, iniFilePath, hGlobalEvent) != 0)
	{
		errorCode = SE_ERROR_CODE_PLAY_FAILED;
		goto CREATE_FAILED;
	}
	pMemMapEncoder->pushEncoder(newEncoder, (bool)newSetting->getIntValue(KEY_OUT_DATA_INFO));
	if (!bMemAlreadyExist)
	{
// 		ScopedLock lock(m_lockEncoders);
// 		m_listEncoders.push_back(newEncoder);
		ScopedLock lock(m_lockMemMapEncoder);
		m_listMemMapEncoder.push_back(pMemMapEncoder);
	}

	if (m_listener)
	{
		m_listener->onEncoderCreated(newSetting);
	}

	if (pMemMapEncoder->pVideoReader)
	{
		if (m_listener)
		{
			m_listener->onVideoSharedMemoryCreate(
				newSetting->getValue(KEY_MEM_VIDEO_NAME),
				newSetting->getIntValue(KEY_MEM_VIDEO_WIDTH),
				newSetting->getIntValue(KEY_MEM_VIDEO_HEIGHT),
				newSetting->getIntValue(KEY_MEM_VIDEO_BUFFERS)
				);
		}

		//m_videoSharedMemoryReader = videoReader;
	}
	if (pMemMapEncoder->pAudioReader)
	{
		if (m_listener)
		{
			m_listener->onAudioSharedMemoryCreated(
				newSetting->getValue(KEY_MEM_AUDIO_NAME),
				newSetting->getIntValue(KEY_MEM_AUDIO_CHS), 
				newSetting->getIntValue(KEY_MEM_AUDIO_FREQ),
				newSetting->getIntValue(KEY_MEM_AUDIO_BLOCKSIZE),
				newSetting->getIntValue(KEY_MEM_AUDIO_BUFFERS)
				);
		}
		//m_audioSharedMemoryReader = audioReader;
	}

	if (pMemMapEncoder->pAudioReader && pMemMapEncoder->pVideoReader && !isTimerRunning())
	{
		startTimer(20);
	}
	if (pMemMapEncoder->pDataInfo && newSetting->getIntValue(KEY_OUT_DATA_INFO))
	{
		pMemMapEncoder->pDataInfo->BeginTimer(1000 * newSetting->getIntValue(KEY_OUT_DATA_INFO_HZ));
	}

//	static bool isFirst = true;
//	if (isFirst)
//	{
// 		g_hDoneEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
// 		m_hTimerQueue = CreateTimerQueue();
// 		if (!m_hTimerQueue || !g_hDoneEvent)
// 		{
// 			goto CREATE_FAILED;
// 		}
// 		if (!CreateTimerQueueTimer(&m_hTimer, m_hTimerQueue, (WAITORTIMERCALLBACK)TimerRoutine, this, 3000, 20, WT_EXECUTEDEFAULT | WT_EXECUTEINTIMERTHREAD))
// 		{
// 			goto CREATE_FAILED;
// 		}
//		isFirst = false;
//	}

	errorCode = SE_ERROR_CODE_SUCCESS;
	writePropertiesInIniFile(iniFilePath, errorCode, hGlobalEvent);

	return 0;

CREATE_FAILED:
	if (newEncoder)
	{
		deleteAndZero(newEncoder);
	}
	if (newSetting)
	{
		deleteAndZero(newSetting);
	}
	if (pMemMapEncoder && pMemMapEncoder->pVideoReader && pMemMapEncoder->listEncoder.empty())
	{
		pMemMapEncoder->pVideoReader->closeSharedMemory();
		deleteAndZero(pMemMapEncoder->pVideoReader);
	}
	if (pMemMapEncoder && pMemMapEncoder->pAudioReader && pMemMapEncoder->listEncoder.empty())
	{
		pMemMapEncoder->pAudioReader->closeSharedMemory();
		deleteAndZero(pMemMapEncoder->pAudioReader);
	}
// 	if (g_hDoneEvent)
// 	{
// 		CloseHandle(g_hDoneEvent);
// 	}
// 	if (m_hTimerQueue)
// 	{
// 		CloseHandle(m_hTimerQueue);
// 	}

	writePropertiesInIniFile(iniFilePath, errorCode, hGlobalEvent);
	if (hGlobalEvent)
	{
		::CloseHandle(hGlobalEvent);
		hGlobalEvent = nullptr;
	}

	return -1;
}

void nemoEncoderManager::destroyEncoder(String memVideoName, String memAudioName, String encoderName)
{
// 	baseSilentEncoder* encoderFound = nullptr;
// 	{
// 		ScopedLock lock(m_lockEncoders);
// 		baseSilentEncoder* e = nullptr;
// 		std::list<baseSilentEncoder*>::iterator iterEncoder;
// 		for (iterEncoder = m_listEncoders.begin(); iterEncoder != m_listEncoders.end(); ++iterEncoder)
// 		{
// 			e = *iterEncoder;
// 			if (e->getEncoderName().equalsIgnoreCase(encoderName))
// 			{
// 				encoderFound = e;
// 				m_listEncoders.erase(iterEncoder);
// 				break;
// 			}
// 		}
// 	}
// 	if (encoderFound)
// 	{
// 		encoderFound->stop();
// 		if (m_listener)
// 		{
// 			m_listener->onEncoderDestroyed(encoderName);
// 		}
// 		deleteAndZero(encoderFound);
// 	}
	baseSilentEncoder* pEncoder = nullptr;
	{
		ScopedLock lock(m_lockMemMapEncoder);
		std::list<MemMapEncoder*>::iterator iterMemMapEncoder;
		MemMapEncoder* m = nullptr;
		for (iterMemMapEncoder = m_listMemMapEncoder.begin(); iterMemMapEncoder != m_listMemMapEncoder.end(); ++iterMemMapEncoder)
		{
			m = *iterMemMapEncoder;
			std::list<baseSilentEncoder*>::iterator iterEncoder;
			for (iterEncoder = m->listEncoder.begin(); iterEncoder != m->listEncoder.end(); ++iterEncoder)
			{
				if (m->strVideoMemoryName.equalsIgnoreCase(memVideoName) && m->strAudioMemoryName.equalsIgnoreCase(memAudioName) && (*iterEncoder)->getEncoderName().equalsIgnoreCase(encoderName))
				{
					pEncoder = *iterEncoder;
					m->listEncoder.erase(iterEncoder);
					pEncoder->stop();
					if (m->mapEncNameAndInfo[encoderName])
					{
						m->pDataInfo->EndTimer();
					}
					m->mapEncNameAndInfo.erase(encoderName);
					if (m_listener)
					{
						m_listener->onEncoderDestroyed(encoderName);
					}
					deleteAndZero(pEncoder);
					if (m->listEncoder.empty())
					{
						m->pVideoReader->closeSharedMemory();
						m->pAudioReader->closeSharedMemory();
						deleteAndZero(m->pVideoReader);
						deleteAndZero(m->pAudioReader);
						m_listMemMapEncoder.erase(iterMemMapEncoder);
						deleteAndZero(m);
					}
					return;
				}
			}
		}
	}
}

void nemoEncoderManager::reset()
{
// 	ScopedLock lock(m_lockEncoders);
// 	baseSilentEncoder* e = nullptr;
// 	std::list<baseSilentEncoder*>::iterator iterEncoder;
// 	for (iterEncoder = m_listEncoders.begin(); iterEncoder != m_listEncoders.end();)
// 	{
// 		e = *iterEncoder;
// 		iterEncoder = m_listEncoders.erase(iterEncoder);
// 		e->stop();
// 		if (m_listener)
// 		{
// 			m_listener->onEncoderDestroyed(e->getEncoderName());
// 		}
// 		deleteAndZero(e);
// 	}
	ScopedLock lock(m_lockMemMapEncoder);
	MemMapEncoder* pMemMapEncoder = nullptr;
	std::list<MemMapEncoder*>::iterator iterMemMapEncoder;
	for (iterMemMapEncoder = m_listMemMapEncoder.begin(); iterMemMapEncoder != m_listMemMapEncoder.end();)
	{
		pMemMapEncoder = *iterMemMapEncoder;
		iterMemMapEncoder = m_listMemMapEncoder.erase(iterMemMapEncoder);
		std::list<baseSilentEncoder*>::iterator iter;
		if (m_listener)
		{
			for (iter = pMemMapEncoder->listEncoder.begin(); iter != pMemMapEncoder->listEncoder.end(); ++iter)
			{
				m_listener->onEncoderDestroyed((*iter)->getEncoderName());
			}
		}
		pMemMapEncoder->DestroyMemMapEncoder();
		deleteAndZero(pMemMapEncoder);
	}
}

bool nemoEncoderManager::checkEncoderName(String VideoMemName, String AudioMemName, String encoderName)
{
// 	ScopedLock lock(m_lockEncoders);
// 
// 	baseSilentEncoder* e = nullptr;
// 	std::list<baseSilentEncoder*>::iterator iterEncoder;
// 	for (iterEncoder = m_listEncoders.begin(); iterEncoder != m_listEncoders.end(); ++iterEncoder)
// 	{
// 		e = *iterEncoder;
// 		if (e->getEncoderName().equalsIgnoreCase(encoderName))
// 			return true;
// 	}
	ScopedLock lock(m_lockMemMapEncoder);
	MemMapEncoder* pMemMapEncoder = nullptr;
	std::list<MemMapEncoder*>::iterator iter;
	for (iter = m_listMemMapEncoder.begin(); iter != m_listMemMapEncoder.end(); ++iter)
	{
		pMemMapEncoder = *iter;
		if (pMemMapEncoder->strVideoMemoryName.equalsIgnoreCase(VideoMemName) && pMemMapEncoder->strAudioMemoryName.equalsIgnoreCase(AudioMemName))
		{
			std::list<baseSilentEncoder*>::iterator iterEncoder;
			for (iterEncoder = pMemMapEncoder->listEncoder.begin(); iterEncoder != pMemMapEncoder->listEncoder.end(); ++iterEncoder)
			{
				if ((*iterEncoder)->getEncoderName().equalsIgnoreCase(encoderName))
					return true;
			}
		}
	}

	return false;
}

void nemoEncoderManager::run()
{
	CrThreadAutoInstallHelper cr_install_helper(0);
	g_main_loop_run(m_gstMainLoop);
}

void nemoEncoderManager::hiResTimerCallback()
{
// 	GstBuffer* gstBuffer = nullptr;
// 	baseSilentEncoder* e = nullptr;
// 	std::list<baseSilentEncoder*>::iterator iterEncoder;
// 	// video
// 	gstBuffer = m_videoSharedMemoryReader->readGstBuffer();
// 	while (gstBuffer)
// 	{
// 		{
// 			ScopedLock lock(m_lockEncoders);
// 			for (iterEncoder = m_listEncoders.begin(); iterEncoder != m_listEncoders.end(); ++iterEncoder)
// 			{
// 				e = *iterEncoder;
// 				e->pushVideoBuffer(gstBuffer);
// 			}
// 		}
// 		gst_buffer_unref(gstBuffer);
// 		gstBuffer = m_videoSharedMemoryReader->readGstBuffer();
// 	}
// 	// audio
// 	gstBuffer = m_audioSharedMemoryReader->readGstBuffer();
// 	while (gstBuffer)
// 	{
// 		ScopedLock lock(m_lockEncoders);
// 		for (iterEncoder = m_listEncoders.begin(); iterEncoder != m_listEncoders.end(); ++iterEncoder)
// 		{
// 			e = *iterEncoder;
// 			e->pushAudioBuffer(gstBuffer);
// 		}
// 		gst_buffer_unref(gstBuffer);
// 		gstBuffer = m_audioSharedMemoryReader->readGstBuffer();
// 	}
	GstBuffer* gstBuffer = nullptr;
	MemMapEncoder* pMemMapEncoder = nullptr;
	std::list<MemMapEncoder*>::iterator iter;
	std::list<baseSilentEncoder*>::iterator iterEncoder;
	ScopedLock lock(m_lockMemMapEncoder);
	for (iter = m_listMemMapEncoder.begin(); iter != m_listMemMapEncoder.end(); ++iter)
	{
		pMemMapEncoder = *iter;
		{
			gstBuffer = pMemMapEncoder->pAudioReader->readGstBuffer();
			while (gstBuffer)
			{
				pMemMapEncoder->pDataInfo->AUpdataData(GST_BUFFER_PTS(gstBuffer));
				//pMemMapEncoder->PushAudioBuffer(gstBuffer);
	 			for (iterEncoder = pMemMapEncoder->listEncoder.begin(); iterEncoder != pMemMapEncoder->listEncoder.end(); ++iterEncoder)
	 			{
	 				(*iterEncoder)->pushAudioBuffer(gstBuffer);
	 			}
	 			gst_buffer_unref(gstBuffer);
				gstBuffer = pMemMapEncoder->pAudioReader->readGstBuffer();
			}
		}
		{
			gstBuffer = pMemMapEncoder->pVideoReader->readGstBuffer(true);
			while (gstBuffer)
			{
				pMemMapEncoder->pDataInfo->VUpdataData(GST_BUFFER_PTS(gstBuffer));
				//pMemMapEncoder->PushVideoBuffer(gstBuffer);
	 			for (iterEncoder = pMemMapEncoder->listEncoder.begin(); iterEncoder != pMemMapEncoder->listEncoder.end(); ++iterEncoder)
	 			{
	 				(*iterEncoder)->pushVideoBuffer(gstBuffer);
	 			}
	 			gst_buffer_unref(gstBuffer);
				gstBuffer = pMemMapEncoder->pVideoReader->readGstBuffer(true);
			}
		}
	}
}

void nemoEncoderManager::handleMessage(const Message& message)
{
	juceMessage* msg = dynamic_cast<juceMessage*>((Message*)(&message));
	switch (msg->GetMsgType())
	{
	case gst_msg_type_error:
		destroyEncoder(msg->GetMemVideoName(), msg->GetMemAudioName(), msg->GetEncoderName());
		break;
	default:
		break;
	}
}

void nemoEncoderManager::TimerRoutine(void* lpParam, bool TimerOrWaitFird)
{
// 	WaitForSingleObject(g_hDoneEvent, INFINITE);
// 	nemoEncoderManager* pThis = static_cast<nemoEncoderManager*>(lpParam);
// 	GstBuffer* gstBuffer = nullptr;
// 	MemMapEncoder* pMemMapEncoder = nullptr;
// 	std::list<MemMapEncoder*>::iterator iter;
// 	std::list<baseSilentEncoder*>::iterator iterEncoder;
// 	ScopedLock lock(pThis->m_lockMemMapEncoder);
// 	for (iter = pThis->m_listMemMapEncoder.begin(); iter != pThis->m_listMemMapEncoder.end(); ++iter)
// 	{
// 		pMemMapEncoder = *iter;
// 		if (!pMemMapEncoder->listAudioBuffers.empty())
// 		{
// 			gstBuffer = pMemMapEncoder->GetAudioBuffer();
// 			for (iterEncoder = pMemMapEncoder->listEncoder.begin(); iterEncoder != pMemMapEncoder->listEncoder.end(); ++iterEncoder)
// 			{
// 				(*iterEncoder)->pushAudioBuffer(gstBuffer);
// 			}
// 			gst_buffer_unref(gstBuffer);
// 		}
// 		if (!pMemMapEncoder->listVideoBuffers.empty())
// 		{
// 			gstBuffer = pMemMapEncoder->GetVideoBuffer();
// 			for (iterEncoder = pMemMapEncoder->listEncoder.begin(); iterEncoder != pMemMapEncoder->listEncoder.end(); ++iterEncoder)
// 			{
// 				(*iterEncoder)->pushVideoBuffer(gstBuffer);
// 			}
// 			gst_buffer_unref(gstBuffer);
// 		}
// 	}
// 	SetEvent(g_hDoneEvent);
}