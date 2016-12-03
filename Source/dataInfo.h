#ifndef _DATA_INFO_H__
#define _DATA_INFO_H__

#include "publicHeader.h"
class dataInfo : public Timer
{
public:
	dataInfo(String strVMemoryName, String strAMemoryName);
	virtual ~dataInfo();

	void VUpdataData(guint64 iVCurrentPts);
	void AUpdataData(guint64 iACurrentPts);
	void BeginTimer(int intervalInMilliseconds);
	void EndTimer();
	void DirectStopTimer();

private:
	dataInfo();
	virtual void timerCallback() override;
	void startTimer(int intervalInMilliseconds) noexcept;
	void stopTimer() noexcept;

private:
	String			m_strVMemoryName;
	String			m_strAMemoryName;
	String			m_strFileName;
	int				m_iTimerRefCount;
	int				m_intervalInMilliseconds;

	guint64			m_uiVBeginPts;
	guint64			m_uiVCurrentPts;
	guint64			m_uiVOldPts;
	guint64			m_uiVIndex;
	gint64			m_iVMaxPtsDiff;
	gint64			m_iVVAGDiff;
	gint64			m_iVTotalDiff;

	guint64			m_uiABeginPts;
	guint64			m_uiACurrentPts;
	guint64			m_uiAOldPts;
	guint64			m_uiAIndex;
	gint64			m_iAMaxPtsDiff;
	gint64			m_iAVAGDiff;
	gint64			m_iATotalDiff;
};

#endif//_DATA_INFO_H__