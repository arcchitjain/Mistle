#ifdef BLOCK_LESS

#include "../../global.h"

#include <itemstructs/ItemSet.h>

#include "LowEntropySet.h"
#include "LowEntropySetInstantiation.h"

LowEntropySetInstantiation::LowEntropySetInstantiation() {
	mItemSet = NULL;
	mLESet = NULL;
	mLH = 0;
	mIdx = 0;
}

LowEntropySetInstantiation::LowEntropySetInstantiation(LowEntropySet *leset, ItemSet *inst, double lh, uint32 idx) {
	mItemSet = inst;
	mLESet = leset;
	mLH = lh;
	mIdx = idx;
}

LowEntropySetInstantiation::~LowEntropySetInstantiation() {
	delete mItemSet;
	// mLESet ruimt zichzelf op
}

#endif // BLOCK_LESS
