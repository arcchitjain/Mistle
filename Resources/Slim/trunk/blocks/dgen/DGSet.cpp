#if defined(BLOCK_DATAGEN) || defined (BLOCK_CLASSIFIER)

#include "../global.h"

#include <itemstructs/ItemSet.h>

#include "DGSet.h"

DGSet::DGSet() {
	mItemSet = NULL;
	mId = 0;
}

DGSet::DGSet(ItemSet *is, uint32 id) {
	mItemSet = is;
	mId = id;
}

DGSet::~DGSet() {
	// ItemSet maar via orig CodeTable laten deleten?
//	delete mItemSet; // nu iig nog maar even niet
}

bool DGSet::Equals(DGSet *pts) {
	return mId == pts->GetId();
}

ItemSet* DGSet::GetItemSet() {
	return mItemSet;
}

void DGSet::SetItemSet(ItemSet *is) {
	delete mItemSet;
	mItemSet = is;
}

#endif // BLOCK_DATAGEN
