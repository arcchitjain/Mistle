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
#ifndef _CROUPIER_H
#define _CROUPIER_H

#include "Bass.h"

#include "CroupierApi.h"

#include <isc/IscFile.h>

class ItemSet;
class ItemSetCollection;
class Database;
class Config;

class Croupier;
typedef void (Croupier::*BufferFullFunc) (ItemSet **iss, uint32 numSets);

class CROUPIER_API Croupier
{
public:
	Croupier();
	Croupier(Config *config);
	virtual ~Croupier();

	// For use together with empty Constructor
	// !R! virtual void	Configure(Config *config) =0;

	// Returns the pattern type string
	virtual string	GetPatternTypeStr() =0;

	// Returns the name of the database (kill it!)
	virtual string	GetDatabaseName() { return mDbName; }

	// Make any of these
	// virtual void SetSetting(...)

	//////////////////////////////////////////////////////////////////////////
	//   --- Offline Dealing (patterns to file) --- 
	//////////////////////////////////////////////////////////////////////////
	
	// Returns suggested filename for generated pattern file. 
	// only useful after configuration
	virtual string	BuildOutputFilename();

	// Returns full path + filename of the generated pattern file.
	virtual string	MinePatternsToFile(Database *db = NULL);

	// Returns full path + filename of the generated pattern file.
	virtual string	MinePatternsToFile(const string &workingDir, Database *db = NULL);

	/*  -  Let yourself handle the filenaminggame  - */
	virtual bool	MinePatternsToFile(const string &dbFilename, const string &outputFullPath, Database *db = NULL) =0;

	virtual void	SetStoreIscFileType(IscFileType ift) { mStoreIscFileType = ift; }
	virtual void	SetChunkIscFileType(IscFileType ift) { mChunkIscFileType = ift; }


	//////////////////////////////////////////////////////////////////////////
	//   --- Online Dealing (pass patterns in memory) --- 
	//////////////////////////////////////////////////////////////////////////
	virtual bool	CanDealOnline()					{ return false; }

	// Returns an ItemSetCollection (may be entirely loaded or opened from disk)
	virtual ItemSetCollection*	MinePatternsToMemory(Database *db = NULL);


	//////////////////////////////////////////////////////////////////////////
	//   --- Static --- 
	//////////////////////////////////////////////////////////////////////////

	static Croupier *Create(Config *config);
	static Croupier *Create(const string &iscName);

protected:
	/* --- Online Dealing helper methods --- */
	virtual void	InitOnline(Database *db);
	virtual void	SetIscProperties(ItemSetCollection *isc) { THROW_NOT_IMPLEMENTED(); }

	virtual void	MinerIsErVolVanCBFunc(ItemSet **iss, uint32) { THROW_NOT_IMPLEMENTED(); }
	virtual void	MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets) { THROW_NOT_IMPLEMENTED(); }

	/* --- Histogram Mining helper method --- */
	virtual void	HistogramMiningIsHistoryCBFunc(uint64 * const histogram, uint32 histolen) { THROW_NOT_IMPLEMENTED(); }

	/* -- Members -- */
	Config		*mConfig;
	Database	*mDatabase;
	string		mDbName;

	IscFileType mStoreIscFileType;
	IscFileType mChunkIscFileType;

	/* -- Online Mining Members -- */
	uint32		mOnlineNumChunks;
	uint32		mOnlineNumTotalSets;
	ItemSetCollection *mOnlineIsc;

	/* -- Histogram Mining Members -- */
	uint64		*mHistogram;
};

#endif // _CROUPIER_H
