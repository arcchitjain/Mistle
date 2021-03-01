#ifdef BROKEN

#ifndef __PARALLELCFCODETABLE_H
#define __PARALLELCFCODETABLE_H

// Parallel DB-Covering CFCodeTable

#include "CFCodeTable.h"

class ParallelCFCodeTable : public CFCodeTable {
public:
	ParallelCFCodeTable();
	~ParallelCFCodeTable();
	virtual ParallelCFCodeTable* Clone() { throw string(__FUNCTION__) + ": Not implemented."; }

	virtual string		GetShortName() { return "pcf"; }
	static string		GetConfigBaseName() { return "parallel-coverfull"; }

	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);
	virtual void		CoverDB(CoverStats &stats);

	virtual void		CommitAdd(bool updateLog=true);

private:
	uint32 mNumThreads;
	std::vector<std::vector<uint32> > mLocalCounts;
	std::vector<std::vector<uint32> > mLocalAlphabet;
	std::vector<CoverSet*> mCoverSets;
};

#endif // __PARALLELCFCODETABLE_H

#endif // BROKEN
