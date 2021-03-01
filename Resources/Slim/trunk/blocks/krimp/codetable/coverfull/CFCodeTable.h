#ifndef __CFCODETABLE_H
#define __CFCODETABLE_H

/*
	CFCodeTable	  -  CoverFull
		The main, brute-forcish, full cover strategy
		Pre- and Post-Prune compatible
*/

class CTISList;
class CoverSet;

#include "../CodeTable.h"

// for islist, maybe in datatypes.h? pref. not in global.h
//#include <itemstructs/ItemSet.h>

class CFCodeTable : public CodeTable {
public:
	CFCodeTable();
	CFCodeTable(const CFCodeTable &ct);
	virtual ~CFCodeTable();
	virtual CFCodeTable* Clone() { return new CFCodeTable(*this); }

	virtual string		GetShortName() { return "cf"; }
	virtual string		GetConfigName() { return CFCodeTable::GetConfigBaseName(); }
	static string		GetConfigBaseName() { return "coverfull"; }

	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);
	virtual void		SetDatabase(Database *db);

	virtual void		Add(ItemSet *is, uint64 candidateId);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);

	virtual void		CommitAdd(bool updateLog=true);
	virtual void		CommitLastDel(bool zap, bool updateLog=true);
	virtual void		CommitAllDels(bool updateLog=true);
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels();
	virtual void		RollbackCounts();

	virtual islist*		GetItemSetList();
	virtual islist*		GetSingletonList();
	virtual islist*		GetPrePruneList();
	virtual islist*		GetPostPruneList();
	virtual islist*		GetSanitizePruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	virtual void		CoverDB(CoverStats &stats);
	virtual void		CalcTotalSize(CoverStats &stats);

	virtual void		AddOneToEachUsageCount();
	virtual void		ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset);
	virtual void		AddSTSpecialCode();
	virtual void		NormalizeUsageCounts(uint32 total);

	virtual double		CalcEncodedDbSize(Database *db);
	virtual double		CalcTransactionCodeLength(ItemSet *row);
	virtual double		CalcTransactionCodeLength(ItemSet *row, double *stLengths);
	virtual double		CalcCodeTableSize();

	virtual Database*	EncodeDB(const bool useSingletons, string outFile="");
	virtual uint32		EncodeRow(ItemSet *row, double &length, uint16 *temp);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows);
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows, bool addOneToCnt);

	// Stats
	virtual void		WriteCodeFile(Database *encodedDb, const string &outFile);
	virtual void		WriteCodeLengthsToFile(const string &outFile);
	virtual void		WriteCountsToFile(const string &outFile);
	virtual void		WriteCover(CoverStats &stats, const string &outFile);

protected:
	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	uint32				mCTLen;
	CTISList			**mCT;
	uint32				mMaxIdx;

	CoverSet			*mCoverSet;

	uint32				mNumDBRows;

	uint32				*mAlphabet;
	uint32				*mOldAlphabet;
	uint32				*mABUsed, *mABUnused;
	uint32				mABLen;
	uint32				mABUsedLen;
	uint32				mABUnusedCount;

	ItemSet				*mAdded;
	uint64				mAddedCandidateId;
	DelInfo				mLastDel;
	dellist				mDeleted;
};

#endif // __CFCODETABLE_H
