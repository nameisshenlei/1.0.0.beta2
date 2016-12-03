#ifndef NEMO_WINDOW_GLOBAL_MUTEX_H
#define NEMO_WINDOW_GLOBAL_MUTEX_H

#include "../JuceLibraryCode/JuceHeader.h"

class nemoWindowGlobalMutex
{
public:
	nemoWindowGlobalMutex(String strName);
	virtual ~nemoWindowGlobalMutex();

	bool lock();
	bool must_lock();
	void unlock();
	bool isLocking();
protected:
	void* m_handle;
	bool m_isLocking;
private:
};

#endif