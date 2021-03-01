#ifdef BROKEN

#ifndef __CFOCODETABLE_H
#define __CFOCODETABLE_H

/*
	CFOCodeTable  -  CoverFull Orderly
		For testing code table orders
		Pre- and Post-Prune compatible
*/

enum CFOOrder {
	CFORandomOrder,
	CFOStackOrder,
	CFOLengthOrder,
	CFOAreaOrder
};

class CTISList;
class CoverSet;
#include "CFCodeTable.h"
#include "../../itemstructs/ItemSet.h"

class CFOCodeTable : public CFCodeTable {
public:
	CFOCodeTable();
	CFOCodeTable(const string &name);
	virtual ~CFOCodeTable();

	virtual string		GetShortName();

	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup = 0);

	virtual void		Add(ItemSet *is);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);

	virtual void		CommitAdd();
	virtual void		CommitLastDel(bool zap);
	virtual void		CommitAllDels();
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels();
	virtual void		RollbackCounts();

	virtual islist*		GetItemSetList();
	virtual islist*		GetPrePruneList();
	virtual islist*		GetPostPruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		CoverDB(CoverStats &stats);
	virtual void		CoverMaskedDB(CoverStats &stats, bool *dbMask);

	virtual double		CalcRowCodeLength(ItemSet *row);
	virtual uint32		EncodeRow(ItemSet *row, double &length, uint16 *temp);
	// Stats
	virtual void		WriteCodeLengthsToFile(const string &outFile);

	virtual void		AddOneToEachCount();
	virtual void		NormalizeCounts(uint32 total);

protected:
	void				SortPostPruneList(islist *pruneList, const islist::iterator &after);

	CTISList			*mCT;
	uint32				mMaxIdx;

	CoverSet			*mCoverSet;

	double				*mStdLengths;

	uint32				mNumDBRows;

	uint32				*mAlphabet;
	uint32				*mOldAlphabet;
	uint32				*mABUsed, *mABUnused;
	uint32				mABLen;
	uint32				mABUsedLen;
	uint32				mABUnusedCount;

	ItemSet				*mAdded;
	DelInfo				mLastDel;
	dellist				mDeleted;

	CFOOrder			mOrder;
};

#endif // __CFOCODETABLE_H

#endif // BROKEN
