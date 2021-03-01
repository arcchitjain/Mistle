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
#ifndef __CPOCODETABLE_H
#define __CPOCODETABLE_H

class CTISList;
class CoverSet;

#include "CPCodeTable.h"

typedef uint32 (CoverSet::*CoverFunction) (uint32, ItemSet*);
typedef bool (CoverSet::*CanCoverFunction) (uint32, ItemSet*);

class CPOCodeTable : public CPCodeTable {
public:
	CPOCodeTable();
	CPOCodeTable(const string &name);
	virtual ~CPOCodeTable();
	virtual CPOCodeTable* Clone() { THROW_NOT_IMPLEMENTED(); }

	virtual string		GetShortName();
	static string		GetConfigBaseName() { return "coverpartial-orderly"; }

	virtual bool		NeedsDBOccurrences() { return true; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CoverDB(CoverStats &stats);
	void				CoverNoBinsDB(CoverStats &stats);
	void				CoverBinnedDB(CoverStats &stats);

	virtual void		CalcTotalSize(CoverStats &stats);

	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows);
	virtual double		CalcTransactionCodeLength(ItemSet *transaction);
	virtual Database*	EncodeDB(const bool useSingletons, string outFile);

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

	islist::iterator	mRollback;

	ChangeType			mChangeType;

	bool				mOverlap;
	uint32				mNumDBRows;
	uint32				*mUseOccs;
	uint32				mNumUseOccs;

	CTOrder				mOrder;
	islist*				mCTList;

	CoverFunction		mCoverFunc;
	CanCoverFunction	mCanCoverFunc;
};

#endif // __CPOCODETABLE_H
