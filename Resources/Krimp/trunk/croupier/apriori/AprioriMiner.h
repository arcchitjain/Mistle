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
#ifndef _APRIORIMINER_H
#define _APRIORIMINER_H

#if defined (_WINDOWS)
	#include <Windows.h>
#endif

class Database;
class AprioriCroupier;
class AprioriTree;

class AprioriMiner
{
public:
	AprioriMiner(Database *db, const uint32 minSup, const uint32 maxLen);
	virtual ~AprioriMiner();

	void			BuildTree();
	void			ChopDownTree();

	void			MineOnline(AprioriCroupier *croupier, ItemSet **buffer, const uint32 bufferSize);

	// -------------- Get -------------
	uint32			GetMinSup()				{ return mMinSup; }
	uint32			GetMaxLen()				{ return mMaxLen; }
	uint32			GetActualMaxLen()		{ return mActualMaxLen; }

protected:

	// Member variables
	Database		*mDatabase;
	AprioriTree		*mTree;

	// Settings
	uint32			mMinSup;
	uint32			mMaxLen;

	// Stats maintained during mining
	uint32			mActualMaxLen;
};

#endif // _APRIORIMINER_H
