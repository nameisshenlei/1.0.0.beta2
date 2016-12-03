#ifndef NEMO_ENCODER_RTP_H
#define NEMO_ENCODER_RTP_H

#include "baseSilentEncoder.h"

class encoderRtp : public baseSilentEncoder
{
public:
	encoderRtp(String encoderName, String memVideoName, String memAudioName);
	virtual ~encoderRtp();
protected:
	virtual void timerCallback() override;
	virtual GstStateChangeReturn onPlay() override;
private:
};


#endif