#ifndef __LOWENTROPYSETINSTANTIATION_H
#define __LOWENTROPYSETINSTANTIATION_H

#include <Primitives.h>

class ItemSet;
class LowEntropySet;

class LowEntropySetInstantiation {
public:
	LowEntropySetInstantiation();
	LowEntropySetInstantiation(LowEntropySet *leset, ItemSet *inst, double lh, uint32 idx=0);
	virtual ~LowEntropySetInstantiation();

	ItemSet*			GetItemSet() { return mItemSet; }
	LowEntropySet*		GetLowEntropySet() { return mLESet; }
	double				GetLikelihood() { return mLH; }
	uint32				GetInstIdx() { return mIdx; }

protected:
	ItemSet				*mItemSet;
	LowEntropySet		*mLESet;
	double				mLH;
	uint32				mIdx;
};

#endif // __LOWENTROPYSET_H
