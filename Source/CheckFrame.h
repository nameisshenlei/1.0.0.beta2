#ifndef _CHECK_FRAME_H__
#define _CHECK_FRAME_H__
#include "windows.h"

enum my_color
{
	COLOR_0 = 0x000000,
	COLOR_1 = 0x0000ff,
	COLOR_2 = 0x00ff00,
	COLOR_3 = 0xff0000,
	COLOR_4 = 0xffffff,
};

void CheckFrame(const char* pBuffer, int iWidth, int iHeight, int* iDrop = nullptr, float* fDropPercent = nullptr, int* iRepeat = nullptr, float* fRepeatPercent = nullptr)
{
	static ULONGLONG lastFrame = 0, iTotalFrame = 1;
	static bool isFrist = true;
	ULONGLONG iCurrentIndex = 0;

	const char* pCenter = pBuffer + iWidth * (iHeight - 16) * 4 + 16 * 4;
	int B = 0, G = 0, R = 0;
	memcpy(&B, pCenter + 0, 1);
	memcpy(&G, pCenter + 1, 1);
	memcpy(&R, pCenter + 2, 1);
	if (abs(R - 0) < 10 && abs(G - 0) < 10 && abs(B - 0) < 10)
		iCurrentIndex = 0;
	else if (abs(R - 255) < 10 && abs(G - 0) < 10 && abs(B - 0) < 10)
		iCurrentIndex = 1;
	else if (abs(R - 0) < 10 && abs(G - 255) < 10 && abs(B - 0) < 10)
		iCurrentIndex = 2;
	else if (abs(R - 0) < 10 && abs(G - 0) < 10 && abs(B - 255) < 10)
		iCurrentIndex = 3;
	else if (abs(R - 255) < 10 && abs(G - 255) < 10 && abs(B - 255) < 10)
		iCurrentIndex = 4;
	else
		iCurrentIndex = -1;
	if (isFrist)
	{
		isFrist = false;
		lastFrame = iCurrentIndex;
	}
	int diff = iCurrentIndex - lastFrame;
	static ULONGLONG iDropFrame = 1, iRepeatFrame = 1;
	if (diff == 0)
	{
		*iDrop = iDropFrame;
		*fDropPercent = (float)iDropFrame / (float)iTotalFrame * 100.0;
		*iRepeat = iRepeatFrame;
		*fRepeatPercent = (float)iRepeatFrame / (float)iTotalFrame * 100.0;
		iRepeatFrame++;
	}
	else if (diff > 1)
	{
		*iDrop = iDropFrame;
		*fDropPercent = (float)iDropFrame / (float)iTotalFrame * 100.0;
		*iRepeat = iRepeatFrame;
		*fRepeatPercent = (float)iRepeatFrame / (float)iTotalFrame * 100.0;
		iDropFrame += (diff - 1);
	}
	lastFrame = iCurrentIndex;
	iTotalFrame++;
}

#endif//_CHECK_FRAME_H__