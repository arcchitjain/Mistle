#ifndef __LECFCODETABLE_H
#define __LECFCODETABLE_H

class LESList;
class CoverSet;

#include "LECodeTable.h"

typedef bool (*LEInstCompFunc)(LowEntropySetInstantiation* a,LowEntropySetInstantiation *b);

enum LEInstOrder {
	LEOrderLogLHPerSizeDescForAll = 1,
	LEOrderLogLHPerSizeDescForSets,
	LEOrderLengthDescLHDesc,
	LEOrderOptimalLHForSets,
};

class LECFCodeTable : public LECodeTable {
public:
	LECFCodeTable(string algoname = "");
	virtual ~LECFCodeTable();

	virtual string		GetShortName() { return "lecf"; }
	virtual string		GetConfigName() { return GetConfigBaseName(); }
	static string		GetConfigBaseName() { return "lecoverfull"; }

	virtual void		UseThisStuff(Database *db, LEBitmapStrategy bs, uint32 maxSetLen);
	virtual void		SetDatabase(Database *db);

	virtual void		Add(LowEntropySet *les, uint64 candidateId);
	virtual void		Del(LowEntropySet *les, bool zap);

	virtual void		CommitAdd(bool updateLog=true);
	virtual void		CommitLastDel(bool zap, bool updateLog=true);
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackCounts();

	// Provides an item set list of code table elements to be considered for post pruning
	virtual leslist*	GetPostPruneList();
	// During post pruning counts can change once more, here we find such elements
	virtual void		UpdatePostPruneList(leslist *pruneList, const leslist::iterator &after);
	virtual	void		SortPostPruneList(leslist *pruneList, const leslist::iterator &after);

	virtual void		CoverDB(LECoverStats &stats);
	virtual double		CalcTotalSize(LECoverStats &stats);

	virtual void		AddOneToEachCount();

	virtual void		WriteToDisk(const string &filename);
	virtual void		WriteCoverToDisk(const string &filename);

protected:
	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	static bool			CompareInstsLogLHPerSizeDesc(LowEntropySetInstantiation *a, LowEntropySetInstantiation *b);
	static bool			CompareInstsLengthDescLHDesc(LowEntropySetInstantiation *a, LowEntropySetInstantiation *b);

	CoverSet			**mCSets;
	CoverSet			*mOnesCSet;
	ItemSet				*mOnesISet;

	uint32				mABLen;
	double				mABStdLen;
	float				**mABLH;
	uint32				*mABCounts;
	uint32				**mAB01Counts;
	uint32				**mOldAB01Counts;
	uint32				*mOldABCounts;
	uint32				*mZeroedABCounts;

	uint16				*mAttrValues;
	uint16				*mAttributes;

	uint64				mAddedCandidateId;

	LowEntropySet		*mAdded;
	LowEntropySetInstantiation **mAddedInst;
	uint32				*mAddedInsertionPos;

	LowEntropySet		*mZapped;
	uint32				*mZappedInsertionPos;

	LESList				*mLESList;

	LowEntropySetInstantiation ***mRowInsts;
	LowEntropySetInstantiation **mTmpRowInsts;
	uint32				mLenRowInsts;
	uint32				*mNumActiveRowInsts;

	uint32				mPrevNumSets;

	uint32				mLenInstUseful;
	bool				*mInstUseful;

	uint32				mNumBitmaps;		// number of bitmap arrays of different lenghts
	uint32				mNumPosBitmaps;		// total number of possible bitmaps
	uint32				*mBitmapCountSums;
	uint32				**mBitmapCounts;

	LEInstCompFunc		mInstCompFunc;
	LEInstOrder			mInstOrder;
};

#endif // __LECFCODETABLE_H
