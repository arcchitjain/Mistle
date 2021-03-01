#ifndef __KROMP_H
#define __KROMP_H

#include "../slim/SlimAlgo.h"

class Kromp: public SlimAlgo {

// Krimp continuo (aka Kromp)
// after finishing loop over all candidates, starts from the top. hence 'o' in Kromp.

public:

	Kromp(CodeTable* ct, HashPolicyType hashPolicy, Config* config);

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

};


#endif // __KROMP_H
