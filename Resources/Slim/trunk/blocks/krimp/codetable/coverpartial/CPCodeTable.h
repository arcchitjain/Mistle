#ifndef __CPCODETABLE_H
#define __CPCODETABLE_H

class CTISList;
class CoverSet;

#include "../CodeTable.h"

class CPCodeTable : public CodeTable {
public:
	CPCodeTable();
	CPCodeTable(const CPCodeTable& ct);
	virtual ~CPCodeTable();
	virtual CPCodeTable* Clone() { return new CPCodeTable(*this); }

	virtual string		GetShortName() { return "cp"; }
	virtual string		GetConfigName() { return CPCodeTable::GetConfigBaseName(); };
	static string		GetConfigBaseName() { return "coverpartial"; }

	virtual bool		NeedsDBOccurrences() { return true; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CoverDB(CoverStats &stats);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows);
	virtual double		CalcTransactionCodeLength(ItemSet *transaction);

	virtual void		Add(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);

	virtual void		CommitAdd(bool updateLog=true);
	virtual void		CommitLastDel(bool zap,bool updateLog=true);
	virtual void		CommitAllDels(bool updateLog=true);
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels();
	virtual void		RollbackCounts();

	virtual void		AddOneToEachUsageCount();
	virtual void		ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset);

	virtual islist*		GetItemSetList();
	virtual islist*		GetPrePruneList();
	virtual islist*		GetPostPruneList();
	virtual islist*		GetSanitizePruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	// Write state-change to log
	virtual void		UpdateCTLog();

	virtual uint32*		GetAlphabetCounts() { return mAlphabet; }
	virtual uint32		GetAlphabetLength() { return mABLen; }

protected:
	virtual void		CoverNoBinsDB(CoverStats &stats);
	virtual void		CoverBinnedDB(CoverStats &stats);

	bool				mIsBinnedDB;

	CoverSet			*mCoverSet;
	CoverSet			**mCDB;

	uint32				mCTLen;
	CTISList			**mCT;
	uint32				mMaxIdx;

	uint32				mNumDBRows;

	uint32*				mAlphabet;
	uint32*				mOldAlphabet;
	uint32				mABLen;
	uint32				mABUsedLen;
	uint32				mABUnusedCount;

	uint32*				mAllRows;

	DelInfo				mLastDel;
	dellist				mDeleted;

	uint32				mRollback;
	bool				mRollbackAlph;

	ChangeType			mChangeType;
	ItemSet*			mChangeItem;
	islist::iterator	mChangeIter;
	uint32				mChangeRow;

};

#endif // __CPCODETABLE_H
