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
#if defined (_WINDOWS)

//-------------------------------------------------------------------
// CVORegistry header file
//-------------------------------------------------------------------
// 
// Copyright ©2000 Virtual Office Systems Incorporated
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

#if !defined(AFX_VOREGISTRY_H__CF669509_0779_46F6_AFB4_A7E78C5219E3__INCLUDED_)
#define AFX_VOREGISTRY_H__CF669509_0779_46F6_AFB4_A7E78C5219E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "QtilsApi.h"

#define MAX_STR_VALUE_LEN 500

class QTILS_API CVORegistry  
{
public:
	DWORD GetValueSize(LPCTSTR pcszValueName);
	enum ValueType { typeError = 0, typeBinary, typeDWORD, typeString, typeStringList };
	enum ValueType GetValueType(LPCTSTR pcszValueName);
	CVORegistry(HKEY hkeyParent, LPCTSTR pcszSubkey, BOOL fCreateIfNew = TRUE);
	CVORegistry(const CVORegistry& rSrc);	// Default Copy Constructor
	virtual ~CVORegistry();
	BOOL Close();
	BOOL DeleteSubkey(LPCTSTR pcszSubkey);
	BOOL DeleteValue(LPCTSTR pcszValueName);
	DWORD GetDisposition()	{ return m_dwDisposition; }
	BOOL GetFirstSubkey(LPTSTR strSubkey);
	BOOL GetFirstValue(LPTSTR strValueName);
	BOOL GetNextSubkey(LPTSTR strSubkey);
	BOOL GetNextValue(LPTSTR strValueName);
	BOOL GetSubkey(LPTSTR strSubkey, int nOffset);
	BOOL ReadBinary(LPCTSTR pcszKey, LPBYTE pData, DWORD dwBufferSize);
	BOOL ReadBinary(LPCTSTR pcszKey, LPBYTE pData, LPDWORD lpcbData);
	DWORD ReadDWORD(LPCTSTR pcszKey, DWORD dwDefault = 0xDEF0);
	LPTSTR ReadString(LPCTSTR pcszKey, LPCTSTR pcszDefault = NULL);
	BOOL SubkeyExists(LPCTSTR pcszSubkey);
	BOOL ValueExists(LPCTSTR pcszValueName);
	BOOL WriteBinary(LPCTSTR pcszKey, LPBYTE pData, DWORD cbData);
	BOOL WriteDWORD(LPCTSTR pcszKey, DWORD dwValue);
	BOOL WriteString(LPCTSTR pcszKey, LPCTSTR pcszValue);
	operator HKEY()	{ return m_hkey; }
	operator HKEY*() { return &m_hkey; }
	operator LPCTSTR() { return m_strSubkey; }

protected:
	BOOL RecursiveDeleteSubkey(HKEY hkey, LPCTSTR pcszSubkey);

	HKEY m_hkey;
	DWORD m_dwDisposition;
	int m_nSubkeyIndex;
	LPTSTR m_strSubkey;
};

#endif // !defined(AFX_REGISTRY_H__CF669509_0779_46F6_AFB4_A7E78C5219E3__INCLUDED_)

#endif // defined(_WINDOWS)
