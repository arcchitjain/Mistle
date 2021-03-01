
#include "../clobal.h"

#include <ArrayUtils.h>
#include <logger/Log.h>
#include <itemstructs/ItemSet.h>

#include "AprioriCroupier.h"

#include "AprioriNode.h"

AprioriNode::AprioriNode(const uint32 numItems, uint16 *items, uint32 *supports) {
	mNumItems = numItems;
	mItems = items;
	mSupports = supports;

	mChildren = NULL; // #children = #items - 1 (if expanded, 0 otherwise)
	mNext = NULL;
}
AprioriNode::~AprioriNode() {
	if(mItems != NULL)
		delete[] mItems;
	if(mSupports != NULL)
		delete[] mSupports;
	if(mChildren != NULL) {
		for(uint32 i=0; i<mNumItems-1; i++)
			delete mChildren[i];
		delete[] mChildren;
	}
}
void AprioriNode::AddLevel(const uint32 minSup, AprioriNode **firstChild, AprioriNode **lastChild) {
	*firstChild = NULL, *lastChild = NULL;
	if(mNumItems <= 1)
		return;	// no children to add

	mChildren = new AprioriNode *[mNumItems-1];
	for(uint32 i=0; i<mNumItems-1; i++) {
		uint32 numItems = mNumItems - i - 1;
		uint16 *items = new uint16[numItems];
		uint32 *supports = new uint32[numItems];
		memcpy_s(items, numItems*sizeof(uint16), mItems+i+1, numItems*sizeof(uint16));
		for(uint32 j=0; j<numItems; j++)
			supports[j] = 0;
		mChildren[i] = new AprioriNode(numItems, items, supports);
		if(*lastChild != NULL)
			(*lastChild)->SetNext(mChildren[i]);
		*lastChild = mChildren[i];
		if(*firstChild == NULL)
			*firstChild = *lastChild;
	}
}
void AprioriNode::CountRow(const uint16 *set, const uint32 setIdx, const uint32 setLen, const uint32 curDepth, const uint32 countDepth) {
	if(mNumItems == 0)
		return; // nothing to do here

	if(curDepth == countDepth) {		// -- Do the counting
		for(uint32 i=setIdx; i<setLen; i++) {
			uint32 idx = ArrayUtils::BinarySearchMayNotExistAsc<uint16>(set[i], mItems, mNumItems);
			if(idx != UINT32_MAX_VALUE)
				++mSupports[idx];
		}

	} else {							// -- Go deeper
		for(uint32 i=setIdx; i<setLen; i++) {
			uint32 idx = ArrayUtils::BinarySearchMayNotExistAsc<uint16>(set[i], mItems, mNumItems);
			if(idx < mNumItems-1)
				mChildren[idx]->CountRow(set, i+1, setLen, curDepth+1, countDepth);
		}
	}
}
void AprioriNode::PruneLevel(const uint32 minSup, const uint32 curDepth, const uint32 pruneDepth) {
	if(curDepth == pruneDepth) {		// -- Do the pruning (no children yet)
		// Check frequentness
		bool *frequent = new bool[mNumItems];
		uint32 numFrequent = 0;
		for(uint32 i=0; i<mNumItems; i++) {
			if(mSupports[i] >= minSup) {
				frequent[i] = true;
				++numFrequent;
			} else{
				frequent[i] = false;
			}
		}

		// Anything frequent?
		if(numFrequent == 0) {
			mNumItems = 0;
			delete[] mItems;
			mItems = NULL;
			delete[] mSupports;
			mSupports = NULL;

		} else { // At least something frequent

			// Build new structures, keeping only frequent items
			// (mChildren == NULL)
			uint16 *items = new uint16[numFrequent];
			uint32 *supports = new uint32[numFrequent];
			numFrequent = 0;
			for(uint32 i=0; i<mNumItems; i++) {
				if(frequent[i]) {
					items[numFrequent] = mItems[i];
					supports[numFrequent++] = mSupports[i];
				}
			}
			mNumItems = numFrequent;

			// Replace current structures
			delete[] mItems;
			mItems = items;
			delete[] mSupports;
			mSupports = supports;
		}

		// Clean
		delete[] frequent;
	} else {							// -- Go deeper
		if(mNumItems > 1)
			for(uint32 i=0; i<mNumItems-1; i++)
				mChildren[i]->PruneLevel(minSup, curDepth+1, pruneDepth);
	}
}
void AprioriNode::MineItemSets(AprioriCroupier *croupier, const uint32 minSup, ItemSet *previousSet, ItemSet **buffer, const uint32 bufferSize, uint32 &numSets) {
	// Foreach item
	for(uint32 i=0; i<mNumItems; i++) {
		ItemSet *is = previousSet->Clone();
		is->AddItemToSet(mItems[i]);
		is->SetSupport(mSupports[i]);
		buffer[numSets++] = is->Clone(); // not really efficient to create another clone here and delete later, but otherwise chunking messes everything up

		if(numSets == bufferSize) {
			croupier->MinerIsErVolVanCBFunc(buffer, numSets);
			numSets = 0;
		}

		if(mChildren!=NULL && i<mNumItems-1)
			mChildren[i]->MineItemSets(croupier, minSup, is, buffer, bufferSize, numSets);

		delete is;
	}
}
