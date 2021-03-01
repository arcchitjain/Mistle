#if defined(BLOCK_DATAGEN) || defined (BLOCK_CLASSIFIER)

#include "../../../global.h"

#include <itemstructs/ItemSet.h>

#include "CTreeNode.h"
#include "CTreeSet.h"

CTreeSet::CTreeSet() {
	mItemSet = NULL;
	mUsages = new ctnlist();
	mNumUsages = 0;
	mId = 0;
}

CTreeSet::CTreeSet(ItemSet *is, uint32 id) {
	mItemSet = is;
	mUsages = new ctnlist();
	mNumUsages = 0;
	mId = id;
}

CTreeSet::~CTreeSet() {
	// ItemSet maar via orig CodeTable laten deleten?
	delete mItemSet; // nu iig nog maar even niet

	//ctnlist::iterator ci = mUsages->begin(), cend = mUsages->end();
	//for(ci = mUsages->begin(); ci != cend; ++ci)
	//	delete *ci;
	delete mUsages;
}

void CTreeSet::AddUsage(CTreeNode *ctn) {
	mUsages->push_back(ctn);
}

#endif // BLOCK_DATAGEN
