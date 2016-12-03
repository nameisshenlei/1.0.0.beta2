#ifndef _JUCE_MESSAGE_H__
#define _JUCE_MESSAGE_H__

#include "../JuceLibraryCode/JuceHeader.h"

typedef enum{
	gst_msg_type_error,
}MsgType;

class nemoEncoderManager;

class juceMessage
	: public Message
{
public:
	juceMessage(MsgType msgType, String encoderName, String memVideoName, String memAudioName);
	~juceMessage();
	MsgType GetMsgType();
	String GetEncoderName();
	String GetMemVideoName();
	String GetMemAudioName();

private:
	MsgType				m_MsgType;
	String				m_encoderName;
	String				m_memVideoName;
	String				m_memAudioName;
};

#endif//_JUCE_MESSAGE_H__