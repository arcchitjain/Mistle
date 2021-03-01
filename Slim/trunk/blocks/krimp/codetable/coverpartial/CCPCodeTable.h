#ifndef __CCPCODETABLE_H
#define __CCPCODETABLE_H

// Voorheen het snelste cover-algoritme. Ingehaald door Sander Schuckman's Cache-Conscious implementatie (CCCPCodeTable)

class CTISList;
class CoverSet;
class ItemSet;

#include "../CodeTable.h"

class CCPCodeTable : public CodeTable {
public:
	CCPCodeTable();
	CCPCodeTable(const CCPCodeTable &ct);
	virtual ~CCPCodeTable();
	virtual CCPCodeTable* Clone() { return new CCPCodeTable(*this); }

	virtual string		GetShortName() { return "ccp"; }
	virtual string		GetConfigName() { return CCPCodeTable::GetConfigBaseName(); };
	static string		GetConfigBaseName() { return "ccoverpartial"; }

	virtual bool		NeedsDBOccurrences() { return true; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CoverDB(CoverStats &stats);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows);
	virtual void		CalcTotalSize(CoverStats &stats);
	virtual double		CalcTransactionCodeLength(ItemSet *transaction);
	virtual double		CalcTransactionCodeLength(ItemSet *row, double *stLengths);

	virtual void		Add(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);

	virtual void		CommitAdd(bool updateLog=true);
	virtual void		CommitLastDel(bool zap, bool updateLog=true);
	virtual void		CommitAllDels(bool updateLog=true) { THROW_DROP_SHIZZLE(); };
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels() { THROW_DROP_SHIZZLE(); };
	virtual void		RollbackCounts();

	virtual islist*		GetSingletonList();
	virtual islist*		GetItemSetList();
	virtual islist*		GetPrePruneList()  { THROW_DROP_SHIZZLE(); };
	virtual islist*		GetSanitizePruneList();
	virtual islist*		GetPostPruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		AddOneToEachUsageCount();
	virtual void		ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset);
	virtual void		AddSTSpecialCode();

	virtual double		CalcEncodedDbSize(Database *db);
	virtual double		CalcCodeTableSize();

	// Write state-change to log
	virtual void		UpdateCTLog();

protected:
	virtual void		CoverNoBinsDB(CoverStats &stats);
	virtual void		CoverBinnedDB(CoverStats &stats);

	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	bool				mIsBinnedDB;

	CoverSet			*mCoverSet;
	CoverSet			**mCDB;

	uint32				mCTLen;
	CTISList			**mCT;
	uint32				mMaxIdx;

	uint32				mNumDBRows;
	uint32				*mUseOccs;
	uint32				mNumUseOccs;

	uint32*				mAlphabet;
	uint32*				mOldAlphabet;
	uint32				mABLen;
	uint32				mABUsedLen;
	uint32				mABUnusedCount;

	DelInfo				mLastDel;
	dellist				mDeleted;

	uint32				mRollback;
	bool				mRollbackAlph;

	ChangeType			mChangeType;
	ItemSet*			mChangeItem;
	uint64				mChangeItemCandidateId;
	islist::iterator	mChangeIter;
	uint32				mChangeRow;
};

#endif // __CCPCODETABLE_H
