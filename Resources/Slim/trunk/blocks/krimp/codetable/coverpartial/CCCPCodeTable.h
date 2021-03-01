#ifndef __CCCPCODETABLE_H
#define __CCCPCODETABLE_H

//	This is the cache-conscious cover partial CodeTable, the fastest cover algo we have.

#include "../CodeTable.h"

#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverMask.h>
#include <itemstructs/CoverMaskArray.h>
#include <unordered_map>

template <ItemSetType T>
class CCCPCodeTable : public CodeTable {
public:
	CCCPCodeTable();
	CCCPCodeTable(const CCCPCodeTable &ct);
	virtual	~CCCPCodeTable();
	virtual CCCPCodeTable* Clone() { return new CCCPCodeTable(*this); }

	virtual string		GetShortName() { return "cccp"; }
	virtual string		GetConfigName() { return CCCPCodeTable::GetConfigBaseName(); };
	static string		GetConfigBaseName() { return "cccoverpartial"; }

	// We generate db occurrences ourselves
	virtual bool		NeedsDBOccurrences() { return false; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);
	virtual void		InitCoverDB();
	virtual void		SetProvideOccs();

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

	virtual islist*		GetSingletonList();
	virtual islist*		GetItemSetList();
	virtual islist*		GetPrePruneList() { THROW_NOT_IMPLEMENTED(); }
	virtual islist*		GetSanitizePruneList();
	virtual islist*		GetPostPruneList();
	virtual	islist *	GetEstimatedPostPruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		AddOneToEachUsageCount();
	virtual void		ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset);

	virtual uint32		GetUsageCount(ItemSet *is) { return mUsgCountForItemset[is]; }

	virtual void		DebugPrint();

protected:
	void				CalcTotalSizeForCover(CoverStats &stats);

	void				SyncCounts2ItemSets();
	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	void				CommitAdd(bool updateLog, bool updateInsPoints);
	void				CommitLastDel(bool zap, bool updateLog, bool updateInsPoints);

	virtual void		CoverNoBinsDB(CoverStats &stats);
	virtual void		CoverBinnedDB(CoverStats &stats);

protected:
	vector<ItemSet*>	mCT;
	bool				mIsBinnedDB;

	vector<CoverMaskArray<T> > mCDB;

	CoverMaskArray<T>		mCTMasks;
	vector<uint32>		mCTIdx;
	uint32				mCTLen;
	vector<uint32>		mUsgCounts;

	unordered_map<ItemSet*, uint32> mUsgCountForItemset;

	vector<uint32>		mBackupUsgCounts;

	uint32				mNumDBRows;
	vector<uint32>		mUseOccs;

	vector<int32>		mAlphabet;
	vector<int32>		mOldAlphabet;
	uint32				mABUsedLen;
	uint32				mABUnusedCount;
	vector<vector<uint32> >	mAbDbOccs;

	bool				mRollbackAlph;

	ChangeType			mChangeType;
	ItemSet*			mChangeItem;
	uint64				mChangeItemCandidateId;
	uint32				mChangeItemIdx;
	uint32				mChangeItemUsgCount;
	CoverMask<T>*			mChangeItemMask;
	CoverMask<T>*			mMask;
	CoverMask<T>*			mBackupMask;
	CoverMask<T>*			mMask2;// mask for simultaneous covering in one loop
};

#endif // __CCCPCODETABLECC_H
