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
#ifndef _AFOPTCROUPIER_H
#define _AFOPTCROUPIER_H

#if defined (_WINDOWS)
	#include <Windows.h>
#endif
#include <isc/IscFile.h>

#if defined (__GNUC__)
	#include <isc/ItemSetCollection.h>
#endif

#include "../Croupier.h"

enum IscOrderType;
class Thread;
namespace Afopt {
	class AfoptMiner;
	class MemoryOut;
	class HistogramOut;
}
class CROUPIER_API AfoptCroupier : public Croupier
{
public:
	AfoptCroupier(Config *config);
	AfoptCroupier(const string &iscName);
	AfoptCroupier();
	virtual ~AfoptCroupier();

	/* --- Online Dealing (pass patterns in memory) --- */
	virtual bool	CanDealOnline()					{ return true; }	// false, as mining online currently corrupts memory

	/* --- Offline Dealing (patterns to file) --- */
	virtual string	BuildOutputFilename();

	virtual bool	MinePatternsToFile(const string &dbFilename, const string &outputFullPath, Database *db = NULL);

	virtual bool	MyMinePatternsToFile(const string &dbFilename, const string &outputFullPath);
	virtual bool	MyMinePatternsToFile(Database *db, const string &outputFullPath);

	virtual ItemSetCollection* MinePatternsToMemory(Database *db);

	virtual uint64*	MineHistogram(Database *db, const bool setLengthMode);
	virtual void	HistogramMiningIsHistoryCBFunc(uint64 * const histogram, uint32 histolen);

	virtual string	GetPatternTypeStr();

protected:
	virtual void	InitOnline(Database *db);
	virtual void	InitHistogramMining(Database *db, const bool setLengthMode);
	virtual void	SetIscProperties(ItemSetCollection *isc);

	virtual void	MinerIsErVolVanCBFunc(ItemSet **iss, uint32);
	virtual void	MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets);

	void		Configure(const string &iscName);

	string		mType;
	float		mMinSup;
	IscOrderType mOrder;
	string		mMinSupMode;

	Afopt::AfoptMiner	*mMiner;
	Afopt::MemoryOut	*mMinerOut;
	Afopt::HistogramOut *mHistoOut;
	bool		mMayHaveMore;

	friend class Afopt::MemoryOut;
//	friend class Croupier;

};

#endif // _AFOPTCROUPIER_H
