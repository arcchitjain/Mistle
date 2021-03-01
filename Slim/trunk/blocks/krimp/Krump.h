#ifndef __KRUMP_H
#define __KRUMP_H

#include "../slim/SlimAlgo.h"

class Krump : public SlimAlgo {

// Krump, Krimp restart (aka Krump)
// -- goes back to top of candidate list after each accept into code table (hence 'u' in Krump)

public:

	Krump(CodeTable* ct, HashPolicyType hashPolicy, Config* config);

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

};


#endif // __KRUMP_H
