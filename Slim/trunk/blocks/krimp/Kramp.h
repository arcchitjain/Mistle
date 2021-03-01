#ifndef __KRAMP_H
#define __KRAMP_H

#include "../slim/SlimAlgo.h"

class Kramp : public SlimAlgo {

// Krimp greedy (aka Kramp)

public:

	Kramp(CodeTable* ct, HashPolicyType hashPolicy, Config* config);

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

};


#endif // __KRAMP_H
