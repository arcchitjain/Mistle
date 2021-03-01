#ifndef __CPOACODETABLE_H
#define __CPOACODETABLE_H

class CTISList;
class CoverSet;

#include "CPCodeTable.h"

typedef uint32 (CoverSet::*CoverFunction) (uint32, ItemSet*);
typedef bool (CoverSet::*CanCoverFunction) (uint32, ItemSet*);

class CPAOCodeTable : public CPCodeTable {
public:
	CPAOCodeTable();
	CPAOCodeTable(const string &name);
	virtual ~CPAOCodeTable();
	virtual CPAOCodeTable* Clone() { THROW_NOT_IMPLEMENTED(); }

	virtual string		GetShortName();
	virtual string		GetConfigName() { return CPAOCodeTable::GetConfigBaseName(); };
	static string		GetConfigBaseName() { return "coverpartial-alt-orderly"; }

	virtual bool		NeedsDBOccurrences() { return true; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CoverDB(CoverStats &stats);
	void				CoverNoBinsDB(CoverStats &stats);
	void				CoverBinnedDB(CoverStats &stats);

	virtual void		CalcTotalSize(CoverStats &stats);

	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows);
	virtual double		CalcTransactionCodeLength(ItemSet *transaction);
	virtual Database*	EncodeDB(const bool useSingletons, string outFile="");

	virtual void		Add(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE);
	virtual void		Del(ItemSet *is, bool zap, bool keepList);

	virtual void		CommitLastDel(bool zap,bool updateLog=true);
	virtual void		CommitAllDels(bool updateLog=true);
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackAllDels();
	virtual void		RollbackCounts();

	virtual islist*		GetSingletonList();
	virtual islist*		GetItemSetList();
	virtual islist*		GetPrePruneList();
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);

	// Write state-change to log
	virtual void		UpdateCTLog();

	virtual uint32*		GetAlphabetUsage(uint32 idx) { return mAlphabetUsages[idx]; }


protected:
	CTISList			*mCT;

	islist::iterator	mRollback;

	ChangeType			mChangeType;

	bool				mOverlap;
	uint32				mNumDBRows;
	uint32				*mUseOccs;
	uint32				mNumUseOccs;

	CTOrder				mOrder;
	islist*				mCTList;

	CoverFunction		mCoverFunc;
	CanCoverFunction	mCanCoverFunc;

	//size_t				mAlphabetSize;
	uint32				*mAlphabetNumRows;

	uint32				**mAlphabetUsages;
	vector<uint32>		**mAlphabetUsageAdd;
	vector<uint32>		**mAlphabetUsageZap;

	uint32				**mOldAlphabetUsages;
	uint32				*mValues; // temporary variable to retrieve the alphabet-cover items

	friend class SlimCS;
};

#endif // __CPOACODETABLE_H
