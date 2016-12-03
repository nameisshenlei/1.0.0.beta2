#ifndef NEMO_ENCODER_HARDWARE_H
#define NEMO_ENCODER_HARDWARE_H

#include "baseSilentEncoder.h"
class encoderHardware : public baseSilentEncoder
{
public:
	encoderHardware(String encoderName, String memVideoName, String memAudioName);
	virtual ~encoderHardware();

protected:
	virtual void timerCallback() override;
	virtual GstStateChangeReturn onPlay() override;
private:
};


#endif