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
#ifndef __CPROCODETABLE_H
#define __CPROCODETABLE_H

class CTISList;
class CoverSet;

#include "CPCodeTable.h"

typedef uint32 (CoverSet::*CoverFunction) (uint32, ItemSet*);
typedef bool   (CoverSet::*CanCoverFunction) (uint32, ItemSet*);
typedef uint32 (CoverSet::*NumCoverFunction) (uint32, ItemSet*);

enum CLStylee {
	CLCounts,		// count & countsum based code length calculation
	CLItemCounts	// item usage & area based code length calculation
};

class CPROCodeTable : public CPCodeTable {
public:
	CPROCodeTable();
	CPROCodeTable(const string &name);
	virtual ~CPROCodeTable();
	virtual CPROCodeTable* Clone() { throw string(__FUNCTION__) + " not implemented!"; }

	virtual string		GetShortName();
	virtual string		GetConfigName();
	static string		GetConfigBaseName() { return "coverpartial-reorderly"; }

	virtual bool		NeedsDBOccurrences() { return true; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CoverDB(CoverStats &stats);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows);
	virtual double		CalcTransactionCodeLength(ItemSet *transaction);
	virtual Database*	EncodeDB(const bool useSingletons, string outFile);

	virtual void		AddOneToEachUsageCount();

	virtual void		Add(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);

	virtual void		CommitLastDel(bool zap,bool updateLog=true);
	virtual void		CommitAllDels(bool updateLog=true);
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels();
	virtual void		RollbackCounts();

	virtual islist*		GetItemSetList();
	virtual islist*		GetPrePruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	// Write state-change to log
	virtual void		UpdateCTLog();

protected:
	CTISList			*mCT;

	ItemSet				***mCTArAr;
	uint32				*mCTArNum;
	uint32				*mCTArMax;
	uint32				mCTArLen;

	islist::iterator	mRollback;

	ChangeType			mChangeType;

	bool				mOverlap;
	uint32				mNumDBRows;
	uint32				*mUseOccs;
	uint32				mNumUseOccs;

	CTOrder				mOrder;
	islist*				mCTList;

	double				*mCTCodeLengths;
	double				*mABCodeLengths;

	CLStylee			mCLStylee;

	CoverFunction		mCoverFunc;
	NumCoverFunction	mNumCoverFunc;
	CanCoverFunction	mCanCoverFunc;
};

#endif // __CPROCODETABLE_H
