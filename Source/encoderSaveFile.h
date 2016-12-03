#ifndef NEMO_ENCODER_SAVE_FILE_H
#define NEMO_ENCODER_SAVE_FILE_H

#include "baseSilentEncoder.h"

class encoderSaveFile : public baseSilentEncoder
{
public:
	encoderSaveFile(String encoderName, String memVideoName, String memAudioName);
	virtual ~encoderSaveFile();

	enum saveFileType
	{
		saveFileTypeUnknown,
		saveFileTypeMP4,
		saveFileTypeMKV,
		saveFileTypeFLV,
		saveFileTypeMOV,
		saveFileTypeWebm,
		saveFileTypeAVI
	};

protected:
	virtual void timerCallback() override;
	virtual GstStateChangeReturn onPlay() override;
private:
};

#endif