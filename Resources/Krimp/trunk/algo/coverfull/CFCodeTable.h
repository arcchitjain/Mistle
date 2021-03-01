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
#ifndef __CFCODETABLE_H
#define __CFCODETABLE_H

/*
	CFCodeTable	  -  CoverFull
		The main, brute-forcish, full cover strategy
		Pre- and Post-Prune compatible
*/

class CTISList;
class CoverSet;

#include "../CodeTable.h"

// for islist, maybe in datatypes.h? pref. not in global.h
//#include <itemstructs/ItemSet.h>

class CFCodeTable : public CodeTable {
public:
	CFCodeTable();
	CFCodeTable(const CFCodeTable &ct);
	virtual ~CFCodeTable();
	virtual CFCodeTable* Clone() { return new CFCodeTable(*this); }

	virtual string		GetShortName() { return "cf"; }
	virtual string		GetConfigName() { return GetConfigBaseName(); }
	static string		GetConfigBaseName() { return "coverfull"; }

	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);
	virtual void		SetDatabase(Database *db);

	virtual void		Add(ItemSet *is, uint64 candidateId);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);

	virtual void		CommitAdd(bool updateLog=true);
	virtual void		CommitLastDel(bool zap, bool updateLog=true);
	virtual void		CommitAllDels(bool updateLog=true);
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels();
	virtual void		RollbackCounts();

	virtual islist*		GetItemSetList();
	virtual islist*		GetSingletonList();
	virtual islist*		GetPrePruneList();
	virtual islist*		GetPostPruneList();
	virtual islist*		GetSanitizePruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		CoverDB(CoverStats &stats);
	virtual void		CalcTotalSize(CoverStats &stats);

	virtual void		AddOneToEachUsageCount();
	virtual void		AddSTSpecialCode();
	virtual void		NormalizeUsageCounts(uint32 total);

	virtual double		CalcEncodedDbSize(Database *db);
	virtual double		CalcTransactionCodeLength(ItemSet *row);
	virtual double		CalcTransactionCodeLength(ItemSet *row, double *stLengths);
	virtual double		CalcCodeTableSize();

	virtual Database*	EncodeDB(const bool useSingletons, string outFile);
	virtual uint32		EncodeRow(ItemSet *row, double &length, uint16 *temp);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows, bool addOneToCnt);

	// Stats
	virtual void		WriteCodeFile(Database *encodedDb, const string &outFile);
	virtual void		WriteCodeLengthsToFile(const string &outFile);
	virtual void		WriteCountsToFile(const string &outFile);
	virtual void		WriteCover(CoverStats &stats, const string &outFile);

protected:
	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	uint32				mCTLen;
	CTISList			**mCT;
	uint32				mMaxIdx;

	CoverSet			*mCoverSet;

	uint32				mNumDBRows;

	uint32				*mAlphabet;
	uint32				*mOldAlphabet;
	uint32				*mABUsed, *mABUnused;
	uint32				mABLen;
	uint32				mABUsedLen;
	uint32				mABUnusedCount;

	ItemSet				*mAdded;
	uint64				mAddedCandidateId;
	DelInfo				mLastDel;
	dellist				mDeleted;
};

#endif // __CFCODETABLE_H
