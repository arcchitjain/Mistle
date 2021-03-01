#ifdef BROKEN

#ifndef __CFACODETABLE_H
#define __CFACODETABLE_H

/*
	CFACodeTable  -  CoverFull Alternative
		Playground for editing the CF algorithm
*/

class CTISList;
class CoverSet;
#include "CFCodeTable.h"
#if defined (_WINDOWS)
	#include "../../itemstructs/ItemSet.h"
#elif defined (__GNUC__) && (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <itemstructs/ItemSet.h>
#endif


class CFACodeTable : public CFCodeTable {
public:
	CFACodeTable();
	virtual ~CFACodeTable();
	virtual void		Clean();

	virtual string		GetShortName() { return "cfa"; }

	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 toMinSup = 0);

	virtual void		Add(ItemSet *is);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);

	virtual void		CommitAdd();
	virtual void		CommitLastDel(bool zap);
	virtual void		CommitAllDels();
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels();
	virtual void		RollbackCounts();

	virtual ItemSet**	GetEncodingMap();
	virtual islist*		GetPrePruneList();
	virtual islist*		GetPostPruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		CoverDB(CoverStats &stats);

	virtual void		AddOneToEachCount();
	virtual void		NormalizeCounts(uint32 total);

	virtual double		CalcRowCodeLength(ItemSet *row);
	virtual uint32		EncodeRow(ItemSet *row, double &length, uint16 *temp);
	virtual void		CoverMaskedDB(CoverStats &stats, bool *dbMask);

	// Stats
	virtual void		WriteCodeLengthsToFile(const string &outFile);
	
protected:
	void				SortPostPruneList(islist *pruneList, const islist::iterator &after);

	uint32				mCTLen;
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
};

#endif // __CFACODETABLE_H

#endif // BROKEN
