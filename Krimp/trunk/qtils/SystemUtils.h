//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
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

#if defined (__unix__)

class QTILS_API SystemUtils {
public:
	static void		TakeItEasy(); // (tm)

	static string	GetProcessIdString();

	// memory functions: all in bytes!
	static uint64	GetProcessMemUsage();
	static uint64	GetProcessPeakMemUsage();
	static uint64	GetAvailableMemory();
	static uint64 GetTotalMemory();
	static void		GetSystemMemoryInfo(uint64 &memAvailable, uint64 &memTotal);
};

#endif // defined (__unix__)

#endif // _SYSTEMUTILS_H
