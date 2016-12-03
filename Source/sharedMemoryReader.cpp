#include "sharedMemoryReader.h"
#include <windows.h>

#define NEMO_SHARED_MEMORY_LIST_BUFFERS_MAX					50

sharedMemoryReader::sharedMemoryReader()
: m_buffers(0)
, m_bufferBlockSize(0)
, m_indexBufNow(0)
, m_handleMapFile(nullptr)
, m_bufferMapFile(nullptr)
, m_lastGetDataTime(-1)
, m_iWidth(0)
, m_iHeight(0)
, m_CheckFrame(false)
{

}

sharedMemoryReader::~sharedMemoryReader()
{

}

void sharedMemoryReader::SetCheckFrame(bool CheckFrame)
{
	m_CheckFrame = CheckFrame;
}

bool sharedMemoryReader::openSharedMemory(String sharedMemoryName, int buffers, int bufferBlockSize)
{
	m_bufferBlockSize = bufferBlockSize;
	m_buffers = buffers;
	m_sharedMemoryName = sharedMemoryName;

	int total_size = (m_bufferBlockSize + 20) * m_buffers;

	m_handleMapFile = OpenFileMapping(
		FILE_MAP_READ,
		FALSE,
		sharedMemoryName.toWideCharPointer()
		);

	if (!m_handleMapFile)
	{
		return false;
	}

	m_bufferMapFile = (char*)MapViewOfFile(m_handleMapFile, FILE_MAP_READ, 0, 0, total_size);
	if (!m_bufferMapFile)
	{
		CloseHandle(m_handleMapFile);
		m_handleMapFile = nullptr;

		return false;
	}

	for (int k = 0; k < m_buffers; ++k)
	{
		String lockName = sharedMemoryName + String(k);

		nemoWindowGlobalMutex* pMutex = new nemoWindowGlobalMutex(lockName);
		m_vectorLock.push_back(pMutex);
	}

	return true;
}

bool sharedMemoryReader::openSharedMemory(String sharedMemoryName, int buffers, int Width, int Height)
{
	m_iWidth = Width;
	m_iHeight = Height;
	m_bufferBlockSize = m_iHeight * m_iWidth * 4;
	return openSharedMemory(sharedMemoryName, buffers, m_bufferBlockSize);
}

void sharedMemoryReader::closeSharedMemory()
{

	for (size_t z = 0; z < m_vectorLock.size(); ++z)
	{
		delete m_vectorLock[z];
	}
	m_vectorLock.resize(0);

	if (m_bufferMapFile)
	{
		UnmapViewOfFile(m_bufferMapFile);
		m_bufferMapFile = nullptr;
	}
	if (m_handleMapFile)
	{
		CloseHandle(m_handleMapFile);
		m_handleMapFile = nullptr;
	}

	ScopedLock sl(m_lockGstBuffers);
	GstBuffer* oldBuf = nullptr;
	while (m_listGstBuffers.size() > 0)
	{
		oldBuf = m_listGstBuffers.front();
		m_listGstBuffers.pop_front();
		gst_buffer_unref(oldBuf);
	}
}

GstBuffer* sharedMemoryReader::readGstBuffer(bool isVideoBuffer)
{
	char* pBuffer = m_bufferMapFile;
	char* pBufferNow = nullptr;
	int64  blockTime = -1;

	pBufferNow = pBuffer + m_indexBufNow* (m_bufferBlockSize + 20);
	if (!m_vectorLock[m_indexBufNow] || !m_vectorLock[m_indexBufNow]->must_lock())
	{
		return nullptr;
	}

	blockTime = *((int64 *)(pBufferNow + 12));
	blockTime *= 1000000;
	if (m_lastGetDataTime == -1 || blockTime > m_lastGetDataTime)
	{
		GstBuffer *gstBuffer = nullptr;
		if (!m_CheckFrame)
		{
			gstBuffer = gst_buffer_new_and_alloc(m_bufferBlockSize);
			if (!isVideoBuffer)
			{
				gst_buffer_fill(gstBuffer, 0, pBufferNow + 20, m_bufferBlockSize);
			}
			else
			{
				GstMapInfo map;
				if (gst_buffer_map(gstBuffer, &map, GST_MAP_WRITE))
				{
					int iLenSize = m_iWidth * 4;
					char* pBuffer = pBufferNow + 20;
					for (int h = m_iHeight - 1; h >= 0; h--)
					{
						memcpy(map.data + h * iLenSize, pBuffer + (m_iHeight - h - 1) * iLenSize, iLenSize);
					}
					gst_buffer_unmap(gstBuffer, &map);
				}
			}

			GST_BUFFER_PTS(gstBuffer) = blockTime;
			GST_BUFFER_DTS(gstBuffer) = blockTime;
			GST_BUFFER_DURATION(gstBuffer) = GST_CLOCK_TIME_NONE;
			GST_BUFFER_OFFSET(gstBuffer) = 0;
			GST_BUFFER_OFFSET_END(gstBuffer) = GST_BUFFER_OFFSET_NONE;
		}
		else
		{
			if (m_iHeight != 0 && m_iWidth != 0)
				CheckFrame(pBufferNow + 20);
		}

		m_lastGetDataTime = blockTime;

		m_vectorLock[m_indexBufNow]->unlock();
		m_indexBufNow++;
		m_indexBufNow %= m_buffers;

		return gstBuffer;
	}
	else
	{
		m_vectorLock[m_indexBufNow]->unlock();
		return nullptr;
	}
}

String sharedMemoryReader::getSharedMemoryName()
{
	return m_sharedMemoryName;
}

void sharedMemoryReader::CheckFrame(char* pBuffer)
{
	static guint64 lastFrame = 0, iTotalFrame = 1;
	static bool isFrist = true;
	guint64 iCurrentIndex = 0;

	char* pCenter = pBuffer + m_iWidth * (m_iHeight - 16) * 4 + 16 * 4;
	int B = 0, G = 0, R = 0;
	memcpy(&B, pCenter + 0, 1);
	memcpy(&G, pCenter + 1, 1);
	memcpy(&R, pCenter + 2, 1);
	if (abs(R - 0) < 30 && abs(G - 0) < 30 && abs(B - 0) < 30)
		iCurrentIndex = 0;
	else if (abs(R - 255) < 30 && abs(G - 0) < 30 && abs(B - 0) < 30)
		iCurrentIndex = 1;
	else if (abs(R - 0) < 30 && abs(G - 255) < 30 && abs(B - 0) < 30)
		iCurrentIndex = 2;
	else if (abs(R - 0) < 30 && abs(G - 0) < 30 && abs(B - 255) < 30)
		iCurrentIndex = 3;
	else if (abs(R - 255) < 30 && abs(G - 255) < 30 && abs(B - 255) < 30)
		iCurrentIndex = 4;
	else
		iCurrentIndex = -1;
	if (isFrist)
	{
		isFrist = false;
		lastFrame = iCurrentIndex;
	}
	int diff = iCurrentIndex - lastFrame;
	static guint64 iDropFrame = 1, iRepeatFrame = 1;
	if (diff == 0)
	{
		char strLog[1024] = { 0 };
		sprintf_s(strLog, 1024, "Drop = %I64d  percent = %.2f   |   repeation = %I64d  percent = %.2f   |   total = %I64d", 
			iDropFrame, (float)iDropFrame / (float)iTotalFrame * 100.0, iRepeatFrame, (float)iRepeatFrame / (float)iTotalFrame * 100.0, iTotalFrame);
		GST_WARNING(strLog);
		iRepeatFrame++;
	}
	else if (diff > 1)
	{
		char strLog[1024] = { 0 };
		sprintf_s(strLog, 1024, "Drop = %I64d  percent = %.2f   |   repeation = %I64d  percent = %.2f   |   total = %I64d",
			iDropFrame, (float)iDropFrame / (float)iTotalFrame * 100.0, iRepeatFrame, (float)iRepeatFrame / (float)iTotalFrame * 100.0, iTotalFrame);
		GST_WARNING(strLog);
		iDropFrame += (diff - 1);
	}
	lastFrame = iCurrentIndex;
	iTotalFrame++;
}