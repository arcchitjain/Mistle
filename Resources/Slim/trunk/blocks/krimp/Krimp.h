#ifndef __KRIMP_H
#define __KRIMP_H

#include "../slim/SlimAlgo.h"

class Krimp : public SlimAlgo {

// standard Krimp

public:

	Krimp(CodeTable* ct, HashPolicyType hashPolicy, Config* config);

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

};


#endif // __KRIMP_H
