#include "juceMessage.h"

juceMessage::juceMessage(MsgType msgType, String encoderName, String memVideoName, String memAudioName)
: m_MsgType(msgType)
, m_encoderName(encoderName)
, m_memVideoName(memVideoName)
, m_memAudioName(memAudioName)
{
}

juceMessage::~juceMessage()
{

}

MsgType juceMessage::GetMsgType()
{
	return m_MsgType;
}

String juceMessage::GetEncoderName()
{
	return	m_encoderName;
}

String juceMessage::GetMemVideoName()
{
	return m_memVideoName;
}

String juceMessage::GetMemAudioName()
{
	return m_memAudioName;
}
