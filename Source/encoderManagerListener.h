#ifndef NEMO_ENCODER_MANAGER_LIENER_H
#define NEMO_ENCODER_MANAGER_LIENER_H

class nemoEncoderManager;
class nemoEncoderManagerListener
{
public:
	virtual void onVideoSharedMemoryCreate(String name, int w, int h, int bufs) = 0;
	virtual void onVideoSharedMemoryDestroyed() = 0;
	virtual void onAudioSharedMemoryCreated(String name, int chs, int freq, int blockSize, int bufs) = 0;
	virtual void onAudioSharedMemoryDestroyed() = 0;

	virtual void onEncoderCreated(PropertySet* settingInfo) = 0;
	virtual void onEncoderDestroyed(String strName) = 0;
};

#endif