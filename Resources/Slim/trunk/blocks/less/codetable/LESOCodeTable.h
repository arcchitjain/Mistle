#ifndef __LESOCODETABLE_H
#define __LESOCODETABLE_H

class LESList;
class CoverSet;

#include "LECodeTable.h"

class LESOCodeTable : public LECodeTable {
public:
	LESOCodeTable();
	virtual ~LESOCodeTable();

	virtual string		GetShortName() { return "leso"; }
	virtual string		GetConfigName() { return GetConfigBaseName(); }
	static string		GetConfigBaseName() { return "setorder"; }

	virtual void		UseThisStuff(Database *db, LEBitmapStrategy bs, uint32 maxSetLen);
	virtual void		SetDatabase(Database *db);

	virtual void		Add(LowEntropySet *les, uint64 candidateId);
	virtual void		Del(LowEntropySet *les, bool zap);

	virtual void		CommitAdd(bool updateLog=true);
	virtual void		CommitLastDel(bool zap, bool updateLog=true) { throw string("kan ik nie"); }
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel() { throw string("kan ik nie"); }
	virtual void		RollbackCounts() { throw string("kan ik nie"); }

	// Provides an item set list of code table elements to be considered for post pruning
	virtual leslist*	GetPostPruneList() { throw string("kan ik nie"); }
	// During post pruning counts can change once more, here we find such elements
	virtual void		UpdatePostPruneList(leslist *pruneList, const leslist::iterator &after) { throw string("kan ik nie"); }
	virtual	void		SortPostPruneList(leslist *pruneList, const leslist::iterator &after) { throw string("kan ik nie"); }

	virtual void		CoverDB(LECoverStats &stats);
	virtual double		CalcTotalSize(LECoverStats &stats);

	virtual void		AddOneToEachCount();

	virtual void		WriteToDisk(const string &filename);
	virtual void		WriteCoverToDisk(const string &filename);

protected:
	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	CoverSet			**mCSets;
	CoverSet			*mOnesCSet;
	ItemSet				*mOnesISet;

	uint32				mABLen;
	double				mABStdLen;
	float				**mABLH;
	uint32				*mABCounts;
	uint32				*mZeroedABCounts;
	uint32				*mOldABCounts;

	uint16				*mAttrValues;
	uint16				*mAttributes;

	uint64				mAddedCandidateId;
	LEDelInfo			mLastDel;
	ledellist			mDeleted;

	LowEntropySet		*mAdded;
	LowEntropySetInstantiation **mAddedInst;
	uint32				*mAddedInsertionPos;

	LowEntropySet		*mZapped;
	uint32				*mZappedInsertionPos;

	LESList				*mLESList;

	LowEntropySetInstantiation ***mRowInsts;
	LowEntropySetInstantiation **mTmpRowInsts;
	uint32				mLenRowInsts;

	uint32				mNumBitmaps;		// number of bitmap arrays of different lenghts
	uint32				mNumPosBitmaps;		// total number of possible bitmaps
	uint32				*mBitmapCountSums;
	uint32				**mBitmapCounts;
};

#endif // __LESOCODETABLE_H
