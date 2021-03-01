#ifdef BLOCK_PTREE

#include "../../global.h"

#include <itemstructs/ItemSet.h>

#include "PTreeNode.h"
#include "PTreeSet.h"

PTreeSet::PTreeSet() {
	//mItemSet = NULL;
	mUsages = new ptnlist();
	mNumUsages = 0;
	mFrequency = 0;
	mId = 0;
}

PTreeSet::PTreeSet(uint16 item, uint32 id) {
	mItem = item;
	mUsages = new ptnlist();
	mNumUsages = 0;
	mFrequency = 0;
	mId = id;
}

/*PTreeSet::PTreeSet(ItemSet *is, uint32 id) {
	mItemSet = is;
	mUsages = new ptnlist();
	mNumUsages = 0;
	mId = id;
}*/

PTreeSet::~PTreeSet() {
	// ItemSet maar via orig CodeTable laten deleten?
	//delete mItemSet; // nu iig nog maar even niet

	//ptnlist::iterator ci = mUsages->begin(), cend = mUsages->end();
	//for(ci = mUsages->begin(); ci != cend; ++ci)
	//	delete *ci;
	delete mUsages;
}

void PTreeSet::AddUsage(PTreeNode *ptn) {
	mUsages->push_back(ptn);
}

bool PTreeSet::Equals(PTreeSet *pts) {
	return mId == pts->GetId();
}

uint16 PTreeSet::GetItem() {
	return mItem;
}
void PTreeSet::SetItem(uint16 item) {
	mItem = item;
}

uint32 PTreeSet::GetFrequency() {
	return mFrequency;
}
void PTreeSet::SetFrequency(uint32 freq) {
	mFrequency = freq;
}

/*
ItemSet* PTreeSet::GetItemSet() {
	return mItemSet;
}

void PTreeSet::SetItemSet(ItemSet *is) {
	delete mItemSet;
	mItemSet = is;
}
*/

#endif // BLOCK_PTREE
