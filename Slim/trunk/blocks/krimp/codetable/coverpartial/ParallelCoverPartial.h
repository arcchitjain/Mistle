#ifndef __PARARLLELCOVERPARTIAL_H
#define __PARARLLELCOVERPARTIAL_H

class CodeTable;

#include "CoverPartial.h"

class ParallelCoverPartial : public CoverPartial {
public:
	ParallelCoverPartial(CodeTable *ct);

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset, const uint32 startSup);

protected:
};

#endif // __PARARLLELCOVERPARTIAL_H
