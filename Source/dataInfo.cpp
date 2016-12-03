#include "dataInfo.h"
#include "windows.h"

#ifdef UNICODE
#define fc_fopen _wfopen
#else
#define fc_fopen fopen
#endif

dataInfo::dataInfo(){}

dataInfo::dataInfo(String strVMemoryName, String strAMemoryName)
: m_strVMemoryName(strVMemoryName)
, m_strAMemoryName(strAMemoryName)
, m_iTimerRefCount(0)
, m_intervalInMilliseconds(0)

, m_uiVBeginPts(0)
, m_uiVCurrentPts(0)
, m_uiVOldPts(0)
, m_uiVIndex(0)
, m_iVMaxPtsDiff(0)
, m_iVVAGDiff(0)
, m_iVTotalDiff(0)

, m_uiABeginPts(0)
, m_uiACurrentPts(0)
, m_uiAOldPts(0)
, m_uiAIndex(0)
, m_iAMaxPtsDiff(0)
, m_iAVAGDiff(0)
, m_iATotalDiff(0)
{
	String strPluginsPath = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory().getFullPathName() + L"\\logs";
	m_strFileName = strPluginsPath + L"\\" + strVMemoryName + L"_info.log";
	FILE* pFile = fc_fopen(m_strFileName.toWideCharPointer(), L"wt");
	if (pFile)
		fclose(pFile);
}

dataInfo::~dataInfo()
{

}

void dataInfo::VUpdataData(guint64 iVCurrentPts)
{
	m_uiVCurrentPts = iVCurrentPts;
	if (!m_uiVIndex)
		m_uiVBeginPts = m_uiVCurrentPts;
	m_uiVIndex++;
	gint64 CurrentDiff = m_uiVOldPts ? m_uiVCurrentPts - m_uiVOldPts : m_uiVOldPts;
	m_iVMaxPtsDiff = m_iVMaxPtsDiff > CurrentDiff ? m_iVMaxPtsDiff : CurrentDiff;
	m_iVTotalDiff += CurrentDiff;
	m_uiVOldPts = m_uiVCurrentPts;
}

void dataInfo::AUpdataData(guint64 iACurrentPts)
{
	m_uiACurrentPts = iACurrentPts;
	if (!m_uiAIndex)
		m_uiABeginPts = m_uiACurrentPts;
	m_uiAIndex++;
	gint64 CurrentDiff = m_uiAOldPts ? m_uiACurrentPts - m_uiAOldPts : m_uiAOldPts;
	m_iAMaxPtsDiff = m_iAMaxPtsDiff > CurrentDiff ? m_iAMaxPtsDiff : CurrentDiff;
	m_iATotalDiff += CurrentDiff;
	m_uiAOldPts = m_uiACurrentPts;
}

void dataInfo::startTimer(int intervalInMilliseconds)
{

}

void dataInfo::stopTimer()
{

}

void dataInfo::BeginTimer(int intervalInMilliseconds)
{
	if (m_iTimerRefCount == 0 || intervalInMilliseconds != m_intervalInMilliseconds)
		Timer::startTimer(intervalInMilliseconds);
	m_iTimerRefCount++;
	m_intervalInMilliseconds = intervalInMilliseconds;
}

void dataInfo::EndTimer()
{
	m_iTimerRefCount--;
	if (m_iTimerRefCount <= 0)
		Timer::stopTimer();
}

void dataInfo::DirectStopTimer()
{
	m_iTimerRefCount = 0;
	if (isTimerRunning())
		Timer::stopTimer();
}

void dataInfo::timerCallback()
{
	String strLog = "";
	FILE* pFile = fc_fopen(m_strFileName.toWideCharPointer(), L"a+t");
	if (!pFile)
	{
		strLog << "Could not open file : " << m_strFileName;
		GST_WARNING(strLog.toRawUTF8());
		return;
	}
	SYSTEMTIME tm = { 0 };
	GetLocalTime(&tm);
	strLog << (int)tm.wYear << "-" << (int)tm.wMonth << "-" << (int)tm.wDay << "-" << (int)tm.wHour << ":" << (int)tm.wMinute << ":" << (int)tm.wSecond << "\r\n";
	strLog << "video max diff = " << m_iVMaxPtsDiff / 1000000 << "\r\n"
		<< "video VAG diff = " << m_iVTotalDiff / 1000000 / m_uiVIndex << "\r\n"
		<< "audio max diff = " << m_iAMaxPtsDiff / 1000000 << "\r\n"
		<< "audio VAG diff = " << m_iATotalDiff / 1000000 / m_uiAIndex << "\r\n"
		<< "========================\r\n";
	fwrite(strLog.toRawUTF8(), 1, strLog.length(), pFile);
	fclose(pFile);
}