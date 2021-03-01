#ifndef __SLIM_H
#define __SLIM_H

#include "SlimAlgo.h"

class Slim: public SlimAlgo {

// Slim (fka Mono, aka Slim_k)

	// evaluates every candidate from X \in (CT x CT)
	// SLAM?

public:

	Slim(CodeTable* ct, HashPolicyType hashPolicy, Config* config);

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

private:

	uint32 mMinSup;
};


#endif // __SLIM_H
