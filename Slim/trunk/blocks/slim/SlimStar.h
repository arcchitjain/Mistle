#ifndef __SLIMSTAR_H
#define __SLIMSTAR_H

#include "SlimAlgo.h"

class SlimStar: public SlimAlgo {

// SlimStar (aka Slim_j or Slim_t)
	// Slim with Tiling as candidate-generator

public:

	SlimStar(CodeTable* ct, HashPolicyType hashPolicy, string typeCT, Config* config);

	virtual string	GetShortName();

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

private:

	uint32			mMaxTileLength;

	string 			mTypeCT;

	uint32 mMinSup;
};


#endif // __SLIMSTAR_H
