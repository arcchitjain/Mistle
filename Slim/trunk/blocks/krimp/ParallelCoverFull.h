#ifndef __PARALLELCOVERFULL_H
#define __PARALLELCOVERFULL_H

class CodeTable;

#include <itemstructs/ItemSet.h>

#include "CoverFullAlgo.h"

class ParallelCoverFull : public CoverFullAlgo {
public:
	ParallelCoverFull(CodeTable *ct);

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset, const uint32 startSup);

protected:
	uint32 mNumRechecked;
};

#endif // __PARALLELCOVERFULL_H
