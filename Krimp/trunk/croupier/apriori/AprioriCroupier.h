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
#ifndef _APRIORICROUPIER_H
#define _APRIORICROUPIER_H

#if defined (_WINDOWS)
	#include <Windows.h>
#endif
#include <isc/IscFile.h>

#if defined (__GNUC__)
	#include <isc/ItemSetCollection.h>
#endif

#include "../Croupier.h"

class AprioriMiner;

enum IscOrderType;
class CROUPIER_API AprioriCroupier : public Croupier
{
public:
	AprioriCroupier(Config *config);
	AprioriCroupier(const string &iscName);
	AprioriCroupier();
	virtual ~AprioriCroupier();

	/* --- Online Dealing (pass patterns in memory when possible) --- */
	virtual bool	CanDealOnline()					{ return true; }

	virtual ItemSetCollection* MinePatternsToMemory(Database *db);

	/* --- Offline Dealing (patterns to file) --- */
	virtual string	BuildOutputFilename();
	virtual bool	MinePatternsToFile(const string &dbFilename, const string &outputFullPath, Database *db = NULL);

	virtual string	GetPatternTypeStr();

protected:
	void			Configure(const string &iscName);
	virtual void	SetIscProperties(ItemSetCollection *isc);

	// Online
	virtual void	InitOnline(Database *db);
	virtual void	MinerIsErVolVanCBFunc(ItemSet **iss, uint32 numSets);
	virtual void	MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets);

	// To file
	bool			MyMinePatternsToFile(Database *db, const string &outputFile);

	string		mType;
	IscOrderType mOrder;
	float		mMinSup;
	string		mMinSupMode;
	uint32		mMaxLen;

	AprioriMiner *mMiner;

	friend class AprioriNode;
	friend class AprioriTree;
};

#endif // _APRIORICROUPIER_H
