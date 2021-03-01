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
#ifndef __CCCPCODETABLE_H
#define __CCCPCODETABLE_H

#include "../CodeTable.h"

#include <itemstructs/ItemSet.h>
#include <itemstructs/Mask.h>
#include <itemstructs/MaskArray.h>

template <ItemSetType T>
class CCCPCodeTable : public CodeTable {
public:
	CCCPCodeTable();
	CCCPCodeTable(const CCCPCodeTable &ct);
	virtual	~CCCPCodeTable();
	virtual CCCPCodeTable* Clone() { return new CCCPCodeTable(*this); }

	virtual string		GetShortName() { return "cccp"; }
	static string		GetConfigBaseName() { return "cccoverpartial"; }

	// We genereren de db occurrences zelf
	virtual bool		NeedsDBOccurrences() { return false; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);
	virtual void		InitCoverDB();
	virtual void		SetProvideDBOccurrences();

	virtual void		CoverDB(CoverStats &stats);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) { THROW_NOT_IMPLEMENTED(); }
	virtual void		EndOfKrimp();
	virtual void		CalcTotalSize(CoverStats &stats);
	virtual double		CalcEncodedDbSize(Database *db);
	virtual double		CalcTransactionCodeLength(ItemSet *row);
	virtual void		Add(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);
	virtual void		AddAndCommit(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE);
	virtual void		DelAndCommit(ItemSet *is, bool zap);
	virtual void		UndoPrune(ItemSet *is);

	virtual void		CommitAdd(bool updateLog=true) { CommitAdd(updateLog, true); }
	virtual void		CommitLastDel(bool zap, bool updateLog=true) { CommitLastDel(zap, updateLog, true); }
	virtual void		CommitAllDels(bool updateLog=true) { THROW_NOT_IMPLEMENTED(); }
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels() { THROW_NOT_IMPLEMENTED(); }
	virtual void		RollbackCounts();

	virtual islist*		GetSingletonList() { THROW_NOT_IMPLEMENTED(); }
	virtual islist*		GetItemSetList();
	virtual islist*		GetPrePruneList() { THROW_NOT_IMPLEMENTED(); }
	virtual islist*		GetSanitizePruneList();
	virtual islist*		GetPostPruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		AddOneToEachUsageCount();

	virtual uint32		GetUsageCount(ItemSet *is) { return mUsgCountForItemset[is]; }

	virtual void		DebugPrint();

protected:
	void				CalcTotalSizeForCover(CoverStats &stats);

	void				SyncCounts2ItemSets();
	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	void				CommitAdd(bool updateLog, bool updateInsPoints);
	void				CommitLastDel(bool zap, bool updateLog, bool updateInsPoints);

	void				CoverNoBinsDB(CoverStats &stats);
	void				CoverBinnedDB(CoverStats &stats);

protected:
	vector<ItemSet*>	mCT;
	bool				mIsBinnedDB;

	vector<MaskArray<T> > mCDB;

	MaskArray<T>		mCTMasks;
	vector<uint32>		mCTIdx;
	uint32				mCTLen;
	vector<uint32>		mUsgCounts;
#if defined (_WINDOWS)
	hash_map<ItemSet*, uint32> mUsgCountForItemset;
#elif defined (__GNUC__) && defined (__unix__)
	unordered_map<ItemSet*, uint32> mUsgCountForItemset;
#endif
	vector<uint32>		mBackupUsgCounts;

	uint32				mNumDBRows;
	vector<uint32>		mUseOccs;

	vector<int32>		mAlphabet;
	vector<int32>		mOldAlphabet;
	uint32				mABLen;
	uint32				mABUsedLen;
	uint32				mABUnusedCount;
	vector<vector<uint32> >	mAbDbOccs;

	bool				mRollbackAlph;

	ChangeType			mChangeType;
	ItemSet*			mChangeItem;
	uint64				mChangeItemCandidateId;
	uint32				mChangeItemIdx;
	uint32				mChangeItemUsgCount;
	Mask<T>*			mChangeItemMask;
	Mask<T>*			mMask;
	Mask<T>*			mBackupMask;
};

#endif // __CCCPCODETABLECC_H
