#ifndef _SYSTEMUTILS_H
#define _SYSTEMUTILS_H

#include "QtilsApi.h"

#if defined (_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <Tlhelp32.h>
#include <vector>

//#define _WIN32_WINNT 0x0500
//#include "windows.h"

#define WINNT							0x11
#define WIN9X							0x12
#define EXITWINDOWS_FAILED				0x13
#define EXITWINDOWS_SUCESS				0x14
#define ADJUST_TOCKEN_SUCESS			0x15
#define ADJUST_TOCKEN_FAILED			0x15
#define ADJUST_PRIVILEGE_FAILED			0x16
#define OPENING_PROCESS_TOKEN_FAILED	0x100

class QTILS_API SystemUtils
{
	UINT			m_nVersionType;
	OSVERSIONINFO	m_osvi;

public:
	SystemUtils();
	virtual ~SystemUtils();

	static void		TakeItEasy(); // (tm)
	UINT			CheckNumProcessesRunning(wchar_t *processName);

	static void		Sleep(size_t milliseconds);

	static	BOOL	SetItAsLastShutDownProcess();
	BOOL			LogOff();
	BOOL			Restart();
	BOOL			ShutDown();

	BOOL			Lock();
	BOOL			IsLocked();

	OSVERSIONINFO	GetOsVersionInfo()const;
	UINT			GetOsVersion();

	BOOL			ForceShutDown();
	BOOL			ForceLogOff();
	BOOL			ForceReStart();

	static string	GetProcessIdString();

	// memory functions: all in bytes!
	static uint64	GetProcessMemUsage();
	static uint64	GetProcessPeakMemUsage();
	static uint64	GetAvailableMemory();
	static uint64	GetTotalMemory();
	static void		GetSystemMemoryInfo(uint64 &memAvailable, uint64 &memTotal);

protected:
	BOOL			ExitWindowsExt(UINT nFlag,DWORD dwType);
	BOOL			AdjustProcessTokenPrivilege();
};

#endif // defined(_WINDOWS)

#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))

class QTILS_API SystemUtils {
public:
	static void		TakeItEasy(); // (tm)

	static void		Sleep(size_t milliseconds);

	static string	GetProcessIdString();

	// memory functions: all in bytes!
	static uint64	GetProcessMemUsage();
	static uint64	GetProcessPeakMemUsage();
	static uint64	GetAvailableMemory();
	static uint64 GetTotalMemory();
	static void		GetSystemMemoryInfo(uint64 &memAvailable, uint64 &memTotal);
};

#endif // (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))

#endif // _SYSTEMUTILS_H
