#ifndef NEMO_ENCODER_RTMP_H
#define NEMO_ENCODER_RTMP_H

#include "baseSilentEncoder.h"

class encoderRtmp : public baseSilentEncoder
{
public:
	encoderRtmp(String encoderName, String memVideoName, String memAudioName);
	virtual ~encoderRtmp();

protected:
	virtual void timerCallback() override;
	virtual GstStateChangeReturn onPlay() override;
private:
};

#endif