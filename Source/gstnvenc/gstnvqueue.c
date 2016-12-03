#include "gstnvqueue.h"

void gst_nv_queue_destroy_buffers(GstNvQueue* pNvQueue)
{
	free(pNvQueue->m_pBuffer);
	pNvQueue->m_pBuffer = NULL;
}

void gst_nv_queue_initialize_queue(GstNvQueue* pNvQueue, EncodeBuffer* pEncBuffer, guint uSize)
{
	pNvQueue->m_uSize = uSize;
	pNvQueue->m_uPendingCount = 0;
	pNvQueue->m_uAvailableIdx = 0;
	pNvQueue->m_uPendingndex = 0;
	pNvQueue->m_pBuffer = malloc(sizeof(EncodeBuffer));
	memset(pNvQueue->m_pBuffer, 0, sizeof(EncodeBuffer));
	for (guint i = 0; i < pNvQueue->m_uSize; i++)
	{
		pNvQueue->m_pBuffer[i] = &pEncBuffer[i];
	}
}

EncodeBuffer* gst_nv_queue_get_available(GstNvQueue* pNvQueue)
{
	EncodeBuffer* pEncBuffer = NULL;
	if (pNvQueue->m_uPendingCount == pNvQueue->m_uSize)
	{
		return NULL;
	}
	pEncBuffer = pNvQueue->m_pBuffer[pNvQueue->m_uAvailableIdx];
	pNvQueue->m_uAvailableIdx = (pNvQueue->m_uAvailableIdx + 1) % pNvQueue->m_uSize;
	pNvQueue->m_uPendingCount += 1;
	return pEncBuffer;
}

EncodeBuffer* gst_nv_queue_get_pending(GstNvQueue* pNvQueue)
{
	if (pNvQueue->m_uPendingCount == 0)
	{
		return NULL;
	}

	EncodeBuffer* pEncBuffer = pNvQueue->m_pBuffer[pNvQueue->m_uPendingndex];
	pNvQueue->m_uPendingndex = (pNvQueue->m_uPendingndex + 1) % pNvQueue->m_uSize;
	pNvQueue->m_uPendingCount -= 1;
	return pEncBuffer;
}