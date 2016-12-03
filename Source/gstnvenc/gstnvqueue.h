#ifndef _CNV_QUEUE_H__
#define _CNV_QUEUE_H__

#include <gst/gst.h>
#include "cuda.h"
#include "nvEncodeAPI.h"

G_BEGIN_DECLS

typedef struct _EncodeOutputBuffer
{
	unsigned int          dwBitstreamBufferSize;
	NV_ENC_OUTPUT_PTR     hBitstreamBuffer;
	HANDLE                hOutputEvent;
	BOOL                  bWaitOnEvent;
	BOOL                  bEOSFlag;
}EncodeOutputBuffer;

typedef struct _EncodeInputBuffer
{
	unsigned int      dwWidth;
	unsigned int      dwHeight;
	CUdeviceptr       pNV12devPtr;
	uint32_t          uNV12Stride;
	CUdeviceptr       pNV12TempdevPtr;
	uint32_t          uNV12TempStride;
	void*             nvRegisteredResource;
	NV_ENC_INPUT_PTR  hInputSurface;
	NV_ENC_BUFFER_FORMAT bufferFmt;
}EncodeInputBuffer;

typedef struct _EncodeBuffer
{
	EncodeOutputBuffer      stOutputBfr;
	EncodeInputBuffer       stInputBfr;
}EncodeBuffer;

typedef struct
{
	EncodeBuffer** m_pBuffer;
	unsigned int m_uSize;
	unsigned int m_uPendingCount;
	unsigned int m_uAvailableIdx;
	unsigned int m_uPendingndex;
}GstNvQueue;

void gst_nv_queue_destroy_buffers(GstNvQueue* pNvQueue);
void gst_nv_queue_initialize_queue(GstNvQueue* pNvQueue, EncodeBuffer* pEncBuffer, guint uSize);
EncodeBuffer* gst_nv_queue_get_available(GstNvQueue* pNvQueue);
EncodeBuffer* gst_nv_queue_get_pending(GstNvQueue* pNvQueue);

G_END_DECLS

#endif//_CNV_QUEUE_H__