#if defined(_WINDOWS)
//-------------------------------------------------------------------
// CVORegistry implementation file
//-------------------------------------------------------------------
// 
// Copyright ©2000-2003 Virtual Office Systems Incorporated
// All Rights Reserved                      
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included.
//
// This code can be compiled, modified and distributed freely, providing
// that this copyright information remains intact in the distribution.
//
// This code may be compiled in original or modified form in any private 
// or commercial application.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage, in any form, caused
// by this code. Use it at your own risk.
//-------------------------------------------------------------------

#include "qlobal.h"
#include "logger/Log.h"

#include <windows.h>
#include <tchar.h>

#include "VORegistry.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVORegistry::CVORegistry(const CVORegistry& rSrc) 
{
	if(rSrc.m_hkey)
	{
		if(RegCreateKeyEx(rSrc.m_hkey, rSrc.m_strSubkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &m_hkey, &m_dwDisposition) != ERROR_SUCCESS)
		{
			LOG_DEBUG(string("CVORegistry() Copy Constructor: Unable to create subkey: ") + rSrc.m_strSubkey);
		}
	}
	else
		m_hkey = 0;

	m_strSubkey = new char[MAX_PATH + 1];
	strcpy_s(m_strSubkey, MAX_PATH, rSrc.m_strSubkey);
	m_nSubkeyIndex = 0;
}

CVORegistry::CVORegistry(HKEY hkeyParent, LPCTSTR pcszSubkey, BOOL fCreateIfNew) : m_nSubkeyIndex(0)
{
	if(fCreateIfNew)
	{
		if(RegCreateKeyEx(hkeyParent, pcszSubkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &m_hkey, &m_dwDisposition) != ERROR_SUCCESS)
		{
			LOG_WARNING(string("CVORegistry() Constructor: Unable to create subkey: ") + pcszSubkey);
		}
	}
	else
	{
		if(RegOpenKeyEx(hkeyParent, pcszSubkey, 0, KEY_ALL_ACCESS, &m_hkey) != ERROR_SUCCESS)
		{
			m_hkey = 0;
			LOG_WARNING(string("CVORegistry() Constructor: Unable to open subkey: ") + pcszSubkey);
		}
	}
	m_strSubkey = new char[MAX_PATH + 1];
}

CVORegistry::~CVORegistry()
{
	delete[] m_strSubkey;
	RegCloseKey(m_hkey);
}

BOOL CVORegistry::Close()
{
	if(m_hkey)
	{
		RegCloseKey(m_hkey);
		m_hkey = 0;
		return TRUE;
	}

	return FALSE;
}

BOOL CVORegistry::ReadBinary(LPCTSTR pcszKey, LPBYTE pData, DWORD dwBufferSize)
{
	return ReadBinary(pcszKey, pData, &dwBufferSize);	// Note - buffer size not passed to caller
}

BOOL CVORegistry::ReadBinary(LPCTSTR pcszKey, LPBYTE pData, LPDWORD pcbData)
{
	/*if(pData && pcbData && *pcbData && (!AfxIsValidAddress(pData, *pcbData, TRUE)) )
	{
		//TRACE(TEXT("CVORegistry::ReadBinary() Invalid buffer specified\n"));
		return FALSE;
	}*/

	DWORD dwType = REG_BINARY;

	return (RegQueryValueEx(m_hkey, pcszKey, 0, &dwType, (PBYTE)pData, pcbData) == ERROR_SUCCESS);
}

BOOL CVORegistry::WriteBinary(LPCTSTR pcszKey, LPBYTE pData, DWORD cbData)
{
	/*if(!AfxIsValidAddress(pData, cbData, FALSE))
	{
		//TRACE(TEXT("CVORegistry::WriteBinary() Invalid buffer specified\n"));
		return FALSE;
	}*/

	return (RegSetValueEx(m_hkey, pcszKey, 0, REG_BINARY, (PBYTE)pData, cbData) == ERROR_SUCCESS);
}

DWORD CVORegistry::ReadDWORD(LPCTSTR pcszKey, DWORD dwDefault)
{
	DWORD dwValue;
	DWORD dwValueSize = sizeof(DWORD);
	DWORD dwType = REG_DWORD;

	if(RegQueryValueEx(m_hkey, pcszKey, 0, &dwType, (PBYTE)&dwValue, &dwValueSize) == ERROR_SUCCESS)
		return dwValue;

	if(dwDefault != 0xDEF0)	// Default specified
		RegSetValueEx(m_hkey, pcszKey, 0, REG_DWORD, (PBYTE)&dwDefault, sizeof(DWORD));

	return dwDefault;
}

BOOL CVORegistry::WriteDWORD(LPCTSTR pcszKey, DWORD dwValue)
{
	return (RegSetValueEx(m_hkey, pcszKey, 0, REG_DWORD, (PBYTE)&dwValue, sizeof(DWORD)) == ERROR_SUCCESS);
}

LPTSTR CVORegistry::ReadString(LPCTSTR pcszKey, LPCTSTR pcszDefault)
{
	DWORD	dwDataSize = 0;
	DWORD	dwType = REG_SZ;

	LPTSTR strValue = new char[MAX_STR_VALUE_LEN];

	if(RegQueryValueEx(m_hkey, pcszKey, 0, &dwType, (PBYTE)NULL, &dwDataSize) == ERROR_SUCCESS)
	{
		if(dwType != REG_SZ)
		{
			if(pcszDefault)
				strcpy_s(strValue, MAX_STR_VALUE_LEN, pcszDefault);

			return strValue;
		}

		if(RegQueryValueEx(m_hkey, pcszKey, 0, &dwType, (PBYTE)strValue, &dwDataSize) == ERROR_SUCCESS)
		{
			return strValue;
		}
		else
		{
			LOG_WARNING(string("CVORegistry::ReadString() Unable to read variable ") + pcszKey);
		}
	}

	if(pcszDefault) {
		strcpy_s(strValue, MAX_STR_VALUE_LEN, pcszDefault);
		if(!WriteString(pcszKey, strValue))
			LOG_WARNING(string("CVORegistry::ReadString() Unable to write default value to variable ") + pcszKey);
		return strValue;
	}
	delete[] strValue;
	return NULL;
}

BOOL CVORegistry::WriteString(LPCTSTR pcszKey, LPCTSTR pcszValue)
{
	if(!pcszValue)
	{
		LOG_DEBUG(string("CVORegistry::WriteString() Invalid Value specified"));
		return FALSE;
	}

	return (RegSetValueEx(m_hkey, pcszKey, 0, REG_SZ, (PBYTE)pcszValue, (DWORD)(_tcslen(pcszValue) + 1)) == ERROR_SUCCESS);
}

BOOL CVORegistry::DeleteSubkey(LPCTSTR pcszSubkey)
{
	return RecursiveDeleteSubkey(m_hkey, pcszSubkey);
}

BOOL CVORegistry::GetFirstSubkey(LPTSTR strSubkey)
{
	DWORD	dwNameSize = MAX_PATH;

	m_nSubkeyIndex = 0;

	if(RegEnumKeyEx(m_hkey, m_nSubkeyIndex, strSubkey, &dwNameSize, 0, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		strSubkey = "";
		return FALSE;	// No subkeys
	}
	return TRUE;
}

BOOL CVORegistry::GetNextSubkey(LPTSTR strSubkey)
{
	DWORD	dwNameSize = MAX_PATH;

	m_nSubkeyIndex++;

	if(RegEnumKeyEx(m_hkey, m_nSubkeyIndex, strSubkey, &dwNameSize, 0, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		strSubkey = "";
		return FALSE;	// No more subkeys
	}
	return TRUE;
}

BOOL CVORegistry::GetSubkey(LPTSTR strSubkey, int nOffset)
{
	DWORD	dwNameSize = MAX_PATH;
	LONG	rc;

	m_nSubkeyIndex = nOffset;

	rc = RegEnumKeyEx(m_hkey, m_nSubkeyIndex, strSubkey, &dwNameSize, 0, NULL, NULL, NULL);

	if(rc != ERROR_SUCCESS)
	{
		if(rc != ERROR_NO_MORE_ITEMS)
		{
			LOG_WARNING("CVORegistry::GetSubkey() : RegEnumKeyEx() Error!\n");
		}

		strSubkey = "";
		return FALSE;	// No subkeys
	}

	return TRUE;
}


BOOL CVORegistry::SubkeyExists(LPCTSTR pcszSubkey)
{
	HKEY hkeySubkey;
	DWORD dwRC = RegOpenKeyEx(m_hkey, pcszSubkey, 0, KEY_ALL_ACCESS, &hkeySubkey);

	if(dwRC != ERROR_SUCCESS)
		return FALSE;

	RegCloseKey(hkeySubkey);
	return TRUE;
}

BOOL CVORegistry::DeleteValue(LPCTSTR pcszValueName)
{
	return(RegDeleteValue(m_hkey, pcszValueName) == ERROR_SUCCESS);
}

BOOL CVORegistry::GetFirstValue(LPTSTR strValueName)
{
	DWORD	dwNameSize = MAX_PATH;

	m_nSubkeyIndex = 0;

	if(RegEnumValue(m_hkey, m_nSubkeyIndex, strValueName, &dwNameSize, 0, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		return FALSE;	// No subkeys
	}

	return TRUE;
}

BOOL CVORegistry::GetNextValue(LPTSTR strValueName)
{
	DWORD	dwNameSize = MAX_PATH;

	m_nSubkeyIndex++;

	if(RegEnumValue(m_hkey, m_nSubkeyIndex, strValueName, &dwNameSize, 0, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		return FALSE;	// No subkeys
	}

	return TRUE;
}

BOOL CVORegistry::ValueExists(LPCTSTR pcszValueName)
{
	DWORD	dwDataSize = 0;
	DWORD	dwType = REG_SZ;

	return(RegQueryValueEx(m_hkey, pcszValueName, 0, &dwType, (PBYTE)NULL, &dwDataSize) == ERROR_SUCCESS);
}

// Recursively delete a key in the registry
BOOL CVORegistry::RecursiveDeleteSubkey(HKEY hkey, LPCTSTR pcszSubkey)
{
	if(!pcszSubkey)
		return FALSE;
	else
	{
		// Recursively delete any subkeys for the target subkey
		HKEY	hkeySubkey;
		LPTSTR	strSubkey;

		DWORD dwRC = RegOpenKeyEx(hkey, pcszSubkey, 0, KEY_ALL_ACCESS, &hkeySubkey);

		if(dwRC != ERROR_SUCCESS)
			return FALSE;

		DWORD	dwNameSize = MAX_PATH;
		strSubkey = new char[MAX_PATH+1];

		while(RegEnumKeyEx(hkeySubkey, 0, strSubkey, &dwNameSize, 0, NULL, NULL, NULL) == ERROR_SUCCESS)
		{

			if(! RecursiveDeleteSubkey(hkeySubkey, strSubkey))
			{
				RegCloseKey(hkeySubkey);
				delete strSubkey;
				return FALSE;
			}

			dwNameSize = MAX_PATH;
		}

		delete strSubkey;
		RegCloseKey(hkeySubkey);
	}
	
	return (RegDeleteKey(hkey, pcszSubkey) == ERROR_SUCCESS);
}


enum CVORegistry::ValueType CVORegistry::GetValueType(LPCTSTR pcszValueName)
{
	DWORD dwType = 0;

	if(RegQueryValueEx(*this, pcszValueName, 0, &dwType, NULL, NULL) != ERROR_SUCCESS)
		return typeError;

	switch(dwType)
	{
	case REG_BINARY:
		return typeBinary;

	case REG_DWORD:
	case REG_DWORD_BIG_ENDIAN:
		return typeDWORD;

	case REG_EXPAND_SZ:
	case REG_SZ:
		return typeString;

	case REG_MULTI_SZ:
		return typeStringList;

	default:
		return typeError;	// There are other types, but not supported by CVORegistry
	}
}

DWORD CVORegistry::GetValueSize(LPCTSTR pcszValueName)
{
	DWORD dwSize = 0;

	RegQueryValueEx(*this, pcszValueName, 0, NULL, NULL, &dwSize);

	return dwSize;
}

#endif // defined(_WINDOWS)
