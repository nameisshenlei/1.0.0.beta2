#include "nemoWindowGlobalMutex.h"
#include <Windows.h>

nemoWindowGlobalMutex::nemoWindowGlobalMutex(String strName)
: m_isLocking(false)
{
	m_handle = ::CreateMutex(NULL,false,strName.toWideCharPointer());
}
nemoWindowGlobalMutex::~nemoWindowGlobalMutex()
{
	if(m_handle)
	{
		::ReleaseMutex(m_handle);
		::CloseHandle(m_handle);
		m_handle = NULL;
	}
}

bool nemoWindowGlobalMutex::lock()
{
	if(m_handle == NULL) return false;

	DWORD dwRe;
	dwRe = ::WaitForSingleObject(m_handle,50);
	if(dwRe == WAIT_OBJECT_0)
	{
		m_isLocking = true;
		return true;
	}
	else if(dwRe == WAIT_ABANDONED_0)
	{
		m_isLocking = true;
		return true;
	}
	else
	{
		return false;
	}
}

bool nemoWindowGlobalMutex::must_lock()
{
	if(m_handle == NULL) return false;

	DWORD dwRe;
	dwRe = ::WaitForSingleObject(m_handle,INFINITE);
	if(dwRe == WAIT_OBJECT_0)
	{
		m_isLocking = true;
		return true;
	}
	else if(dwRe == WAIT_ABANDONED_0)
	{
		m_isLocking = true;
		return true;
	}
	else
	{
		return false;
	}
}

void nemoWindowGlobalMutex::unlock()
{
	if(m_handle)
	{
		::ReleaseMutex(m_handle);
		m_isLocking = false;
	}
}

bool nemoWindowGlobalMutex::isLocking()
{
	return m_isLocking;
}