#ifndef __CPACODETABLE_H
#define __CPACODETABLE_H

class CTISList;
class CoverSet;
class CoverNode;

#include "../CodeTable.h"

class CPACodeTable : public CodeTable {
public:
	CPACodeTable();
	virtual ~CPACodeTable();
	virtual CPACodeTable* Clone() { throw string(__FUNCTION__) + " not implemented!"; }

	virtual string		GetShortName() { return "cpa"; }
	virtual string		GetConfigName() { return CPACodeTable::GetConfigBaseName(); };
	static string		GetConfigBaseName() { return "coverpartial-alt"; }

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
	virtual islist*		GetSanitizePruneList();
	virtual islist*		GetPostPruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	// Write state-change to log
	virtual void		UpdateCTLog();

protected:
	void				CoverNoBinsDB(CoverStats &stats);
	void				CoverBinnedDB(CoverStats &stats);
	virtual void		CalcTotalSize(CoverStats &stats);

	uint32				CoverAlphabet(CoverSet *cset, CoverNode *cnode, uint64 &numNests);
	uint32				UncoverAlphabet(CoverSet *cset, CoverNode *cnode);

	bool				mIsBinnedDB;

	CoverSet			*mCoverSet;
	CoverSet			**mCDB;
	CoverNode			**mCTN;

	CoverNode			*mCurTree;
	CoverNode			*mPrevTree;

	uint16				*mAuxAr;

	uint32				mCTLen;
	CTISList			**mCT;
	uint32				mMaxIdx;

	uint32				mNumDBRows;

	uint32*				mAlphabet;
	ItemSet				**mABItemSets;
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

#endif // __CPACODETABLE_H
