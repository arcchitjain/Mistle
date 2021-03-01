#ifndef __CPROCODETABLE_H
#define __CPROCODETABLE_H

class CTISList;
class CoverSet;

#include "CPCodeTable.h"

typedef uint32 (CoverSet::*CoverFunction) (uint32, ItemSet*);
typedef bool   (CoverSet::*CanCoverFunction) (uint32, ItemSet*);
typedef uint32 (CoverSet::*NumCoverFunction) (uint32, ItemSet*);

enum CLStylee {
	CLCounts,		// count & countsum based code length calculation
	CLItemCounts	// item usage & area based code length calculation
};

class CPROCodeTable : public CPCodeTable {
public:
	CPROCodeTable();
	CPROCodeTable(const string &name);
	virtual ~CPROCodeTable();
	virtual CPROCodeTable* Clone() { throw string(__FUNCTION__) + " not implemented!"; }

	virtual string		GetShortName();
	virtual string		GetConfigName();
	static string		GetConfigBaseName() { return "coverpartial-reorderly"; }

	virtual bool		NeedsDBOccurrences() { return true; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CoverDB(CoverStats &stats);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows);
	virtual double		CalcTransactionCodeLength(ItemSet *transaction);
	virtual Database*	EncodeDB(const bool useSingletons, string outFile="");

	virtual void		AddOneToEachUsageCount();
	virtual void		ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset);

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

	ItemSet				***mCTArAr;
	uint32				*mCTArNum;
	uint32				*mCTArMax;
	uint32				mCTArLen;

	islist::iterator	mRollback;

	ChangeType			mChangeType;

	bool				mOverlap;
	uint32				mNumDBRows;
	uint32				*mUseOccs;
	uint32				mNumUseOccs;

	CTOrder				mOrder;
	islist*				mCTList;

	double				*mCTCodeLengths;
	double				*mABCodeLengths;

	CLStylee			mCLStylee;

	CoverFunction		mCoverFunc;
	NumCoverFunction	mNumCoverFunc;
	CanCoverFunction	mCanCoverFunc;
};

#endif // __CPROCODETABLE_H
