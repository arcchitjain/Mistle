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
#include "qlobal.h"

#include "SystemUtils.h"

#if defined (_WINDOWS)

#include <iostream>
#include <stdio.h>
#include "psapi.h"
#pragma comment( lib, "psapi.lib" )


SystemUtils::SystemUtils() {
	m_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	BOOL bval=GetVersionEx(&m_osvi);

	if(m_osvi.dwPlatformId==VER_PLATFORM_WIN32_NT)
		m_nVersionType=WINNT;
	else if(m_osvi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
		m_nVersionType=WIN9X;
}

SystemUtils::~SystemUtils() {

}
void SystemUtils::TakeItEasy() {
	SetPriorityClass(GetCurrentThread(), BELOW_NORMAL_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
}

UINT SystemUtils::CheckNumProcessesRunning(wchar_t *processName) {
	HANDLE hSnap = INVALID_HANDLE_VALUE;
	PROCESSENTRY32W procEntry;

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		printf("Too bad, could not create snapshot.\n");
		return 0;
	}
	procEntry.dwSize = sizeof(PROCESSENTRY32W);

	if(!Process32FirstW(hSnap, &procEntry))
	{
		printf("Too bad, Process32FirstW failed.\n");
		CloseHandle(hSnap);
		return 0;
	}

	UINT32 count = 0;
	do {
		if(wcsstr(procEntry.szExeFile, processName) != NULL)
			count++;
	} while(Process32NextW(hSnap, &procEntry));

	CloseHandle(hSnap);
	return count;
}

/*
***********************************************************************************
Purpose				:	It will restart the Machine forcefully with 
out allowing  save

Values	Populating	:	return values will be FALSE in many cases. 
1.if AdjustProcessTokenPrivilege() failed.
2.Restart failed.
*************************************************************************************
*/
BOOL SystemUtils::ForceReStart() {
	return ExitWindowsExt(EWX_REBOOT|EWX_FORCE,0);
}
/*
***********************************************************************************
Purpose				:	It will logoff current user forcefully with 
out allowing  save. this codeis common for both NT and 9X Os

Values	Populating	:	return values will be FALSE in many cases. 
1.Logoff failed.
*************************************************************************************
*/

BOOL SystemUtils::ForceLogOff() {
	return ExitWindowsEx(EWX_LOGOFF|EWX_FORCE,0);
}
BOOL SystemUtils::ForceShutDown() {	
	return ExitWindowsEx(EWX_SHUTDOWN|EWX_FORCE,0);
}

/************************************************************************************
Purpose			:Adjusting the Privilage of the Process to Shut Down the machine.

Return Values	:	 if failed	OPENING_PROCESS_TOKEN_FAILED,
ADJUST_PRIVILEGE_FAILED,
ADJUST_TOCKEN_FAILED

if Success	ADJUST_TOCKEN_SUCESS
**************************************************************************************/

BOOL SystemUtils::AdjustProcessTokenPrivilege() {
	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; 

	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
	{
		return OPENING_PROCESS_TOKEN_FAILED;
	}

	if(!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid))
	{
		return ADJUST_PRIVILEGE_FAILED;
	}

	tkp.PrivilegeCount = 1;

	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,(PTOKEN_PRIVILEGES)NULL, 0); 

	if (GetLastError() != ERROR_SUCCESS) 
	{
		return ADJUST_TOCKEN_FAILED;
	}


	return ADJUST_TOCKEN_SUCESS;
}

/************************************************************************************
Purpose				:	This function is declared to customize the purpose of the class. 
You can use this function for any kind of operation related to Exit 
Windows by passing Values to the parameter with out bothering about 
the os and Token Adjusting. The second parameter is reserved for 
future use.

Values	Populating	:	return values will be FALSE in many cases. 
1.If AdjustProcessTokenPrivilege() failed.
2.If operation failed.
**************************************************************************************/

BOOL SystemUtils::ExitWindowsExt(UINT nFlag, DWORD dwType) {
	int iRetval(0);

	switch(m_nVersionType)
	{
	case WINNT:
		{
			if((iRetval=AdjustProcessTokenPrivilege())==ADJUST_TOCKEN_SUCESS)
			{
				return ExitWindowsEx(nFlag,dwType);
			}
			else
			{
				return iRetval;
			}
			break;
		}
	case WIN9X:
		{
			return ExitWindowsEx(nFlag,dwType);
			break;
		}
	}
	return FALSE;
}

UINT SystemUtils::GetOsVersion() {
	return m_nVersionType;
}
OSVERSIONINFO SystemUtils::GetOsVersionInfo()const {
	return m_osvi;
}
/*
***********************************************************************************
Purpose				:	It will SystemUtils the Machine,, after prompting save

Values	Populating	:	return values will be FALSE in many cases. 
1.if AdjustProcessTokenPrivilege() failed.
2.SystemUtils failed.
*************************************************************************************
*/
BOOL SystemUtils::ShutDown() {
	return ExitWindowsExt(EWX_SHUTDOWN,0);
}
/*
***********************************************************************************
Purpose				:	It will restart the machine after prompting save

Return Value		:	return values will be FALSE in many cases. 
1.if AdjustProcessTokenPrivilege() failed.
2.restart failed.
*************************************************************************************
*/
BOOL SystemUtils::Restart() {
	return ExitWindowsExt(EWX_REBOOT,0);
}

/*
***********************************************************************************
Purpose				:	It will logoff the machine after prompting save

Return Value		:	return values will be FALSE if restart failed. 

*************************************************************************************
*/
BOOL SystemUtils::LogOff() {
	return ExitWindows(EWX_LOGOFF,0);
}

BOOL SystemUtils::Lock() {
	return LockWorkStation();
}
BOOL SystemUtils::IsLocked()  {
	return !OpenInputDesktop(0, FALSE, GENERIC_READ | DESKTOP_SWITCHDESKTOP);
}

/*
***********************************************************************************
Purpose				:	It using to set the Application to reserve last SystemUtils 
range. This is possible only in NT.

Return Value		:	return values will be FALSE if setting  failed.. 

*************************************************************************************
*/
BOOL SystemUtils::SetItAsLastShutDownProcess() {
	if(!SetProcessShutdownParameters(0x100,0))
		return FALSE;
	return TRUE;
}
string SystemUtils::GetProcessIdString() {
	DWORD processID = GetCurrentProcessId();
	char ts[20];
	sprintf_s(ts, 20, "%d", processID);
	return string(ts);
}
uint64 SystemUtils::GetProcessMemUsage() {
	DWORD processID = GetCurrentProcessId();
	PROCESS_MEMORY_COUNTERS pmc;

	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID );
	if(hProcess == NULL)
		throw string("Error.");

	size_t memUse = 0;
	if(GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc))) {
		memUse = pmc.PagefileUsage;
	}
	return (uint64) memUse;
}
uint64 SystemUtils::GetProcessPeakMemUsage() {
	DWORD processID = GetCurrentProcessId();
	PROCESS_MEMORY_COUNTERS pmc;

	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID );
	if(hProcess == NULL)
		throw string("Error.");

	size_t memUse = 0;
	if(GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc))) {
		memUse = pmc.PeakPagefileUsage;
	}
	return memUse;
}
uint64 SystemUtils::GetAvailableMemory() {
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	return statex.ullAvailPhys;
}
uint64 SystemUtils::GetTotalMemory() {
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	return statex.ullTotalPhys;
}
void SystemUtils::GetSystemMemoryInfo(uint64 &memAvailable, uint64 &memTotal) {
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	memAvailable = statex.ullAvailPhys;
	memTotal = statex.ullTotalPhys;
}

#endif // defined(_WINDOWS)

#if defined (__unix__)

#include <unistd.h>

void SystemUtils::TakeItEasy() {
	// TODO
}

string SystemUtils::GetProcessIdString() {
	pid_t processID = getpid();
	char ts[20];
	sprintf_s(ts, 20, "%d", processID);
	return string(ts);
}

// @see: How to determine CPU and memory consumption from inside a process?
//       http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process

uint64 parseLine(char* line){
  uint64 i = strlen(line);
  while (*line < '0' || *line > '9') line++;
  line[i-3] = '\0';
  i = atoll(line);
  return i;
}
 
uint64 SystemUtils::GetProcessMemUsage() {
	FILE* file = fopen("/proc/self/status", "r");
	uint64 result = 0;
	char line[128];

	while (fgets(line, 128, file) != NULL){
		if (strncmp(line, "VmRSS:", 6) == 0) {
			//Note: this value is in KB!
			result = parseLine(line);
			result *= 1024;
			break;
		}
	}

	fclose(file);

	return result;
}

uint64 SystemUtils::GetProcessPeakMemUsage() {
	FILE* file = fopen("/proc/self/status", "r");
	uint64 result = 0;
	char line[128];

	while (fgets(line, 128, file) != NULL){
		if (strncmp(line, "VmPeak:", 7) == 0) {
			//Note: this value is in KB!
			result = parseLine(line);
			result *= 1024;
			break;
		}
	}

	fclose(file);

	return result;
}

uint64 SystemUtils::GetAvailableMemory() {
	FILE* file = fopen("/proc/meminfo", "r");
	uint64 result = 0;
	char line[128];

	while (fgets(line, 128, file) != NULL){
		if (strncmp(line, "MemFree:", 8) == 0) {
			// Note: this value is in KB!
			result = parseLine(line);
			result *= 1024;
			break;
		}
	}

	fclose(file);

	return result;
}

uint64 SystemUtils::GetTotalMemory() {
	FILE* file = fopen("/proc/meminfo", "r");
	uint64 result = 0;
	char line[128];

	while (fgets(line, 128, file) != NULL){
		if (strncmp(line, "MemTotal:", 9) == 0) {
			// Note: this value is in KB!
			result = parseLine(line);
			result *= 1024;
			break;
		}
	}

	fclose(file);

	return result;
}

void SystemUtils::GetSystemMemoryInfo(uint64 &memAvailable, uint64 &memTotal) {
	FILE* file = fopen("/proc/meminfo", "r");
	char line[128];

	memAvailable = 0;
	memTotal = 0;
	fgets(line, 128, file);
	if (strncmp(line, "MemTotal:", 9) == 0) {
		memTotal = parseLine(line);
		memTotal *= 1024; 
	}
	fgets(line, 128, file);
	if (strncmp(line, "MemFree:", 8) == 0) {
		memAvailable = parseLine(line);
		memAvailable *= 1024;
	}
	fclose(file);
}

#endif // defined (__unix__)
