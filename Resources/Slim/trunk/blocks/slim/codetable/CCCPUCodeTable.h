#ifndef __CCCPUCODETABLE_H
#define __CCCPUCODETABLE_H

//	This is a variant of CCCPCodeTable that keeps track of Alphabet Usages (tid lists), e.g., for Slim

#include "../../krimp/codetable/coverpartial/CCCPCodeTable.h"

#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverMask.h>
#include <itemstructs/CoverMaskArray.h>
#include <unordered_map>

#include "../structs/ItemSetSorted.h"


template <ItemSetType T>
class CCCPUCodeTable : public CCCPCodeTable<T> {
public:
	CCCPUCodeTable();
	CCCPUCodeTable(const CCCPUCodeTable &ct);
	virtual	~CCCPUCodeTable();
	virtual CCCPUCodeTable* Clone() { return new CCCPUCodeTable(*this); }

	virtual string		GetShortName() { return "cccpu"; }
	virtual string		GetConfigName() { return CCCPUCodeTable::GetConfigBaseName(); };
	static string		GetConfigBaseName() { return "cccoverpartial-usg"; }

	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CommitAdd(bool updateLog=true);
	virtual void		CommitLastDel(bool zap, bool updateLog=true);
	virtual void		RollbackCounts();

	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after);
	virtual	void		UpdateEstimatedPostPruneList(islist *pruneList, const islist::iterator &after);

	void				UpdateAlphabetUsages();//keeps the usage of alphabet up to date
	void				UpdateUsages(ItemSet *is, vector<uint32> *usageAdd, vector<uint32>	*usageZap);//update usages by iterating over all usages
	void				SetCodeTable(ItemSetSorted *itemSetSorted){ mItemSetSorted = itemSetSorted; };// set the itemSet sorted pointer in Code Table so we can access later.

	// only for parallel stuff
	virtual void		UndoPrune(ItemSet *is) { THROW_DROP_SHIZZLE(); }
	virtual void		AddAndCommit(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE)  { THROW_DROP_SHIZZLE(); }
	virtual void		DelAndCommit(ItemSet *is, bool zap) { THROW_DROP_SHIZZLE(); }

protected:
	void				CommitAdd(bool updateLog, bool updateInsPoints);
	void				CommitLastDel(bool zap, bool updateLog, bool updateInsPoints);

	virtual void		CoverNoBinsDB(CoverStats &stats);
	virtual void		CoverBinnedDB(CoverStats &stats);

	ItemSetSorted	*mItemSetSorted;
};

#endif // __CCCPUCODETABLE_H
