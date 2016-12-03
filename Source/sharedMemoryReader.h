#ifndef NEMO_SHARED_MEMORY_READER_H
#define NEMO_SHARED_MEMORY_READER_H

#include "publicHeader.h"
#include "nemoWindowGlobalMutex.h"
#include <list>

class sharedMemoryReader
{
public:
	sharedMemoryReader();
	virtual ~sharedMemoryReader();

	bool openSharedMemory(String sharedMemoryName, int buffers, int bufferBlockSize);
	bool openSharedMemory(String sharedMemoryName, int buffers, int Width, int Height);
	void closeSharedMemory();

	GstBuffer* readGstBuffer(bool isVideoBuffer = false);
	String getSharedMemoryName();

	void SetCheckFrame(bool CheckFrame);
	void CheckFrame(char* pBuffer);
protected:
	String m_sharedMemoryName;
	int m_buffers;
	int m_bufferBlockSize;
	int m_indexBufNow;
	int m_iWidth;
	int m_iHeight;

	std::vector<nemoWindowGlobalMutex*> m_vectorLock;
	void* m_handleMapFile;
	char* m_bufferMapFile;

	CriticalSection m_lockGstBuffers;
	std::list<GstBuffer*> m_listGstBuffers;

	int64 m_lastGetDataTime;

	bool m_CheckFrame;
};

#endif