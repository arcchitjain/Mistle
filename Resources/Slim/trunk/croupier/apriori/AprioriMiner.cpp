
#include "../clobal.h"

#include <Bass.h>
#include <logger/Log.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>

#include "AprioriTree.h"
#include "AprioriNode.h"

#include "AprioriMiner.h"

AprioriMiner::AprioriMiner(Database *db, const uint32 minSup, const uint32 maxLen) {
	mDatabase = db->Clone();

	mMinSup = minSup;
	mMaxLen = maxLen;

	mTree = NULL;
}

AprioriMiner::~AprioriMiner() {
	delete mDatabase; // created my own copy
}

void AprioriMiner::BuildTree() {
	ECHO(2,printf(" * Mining:\t\tin progress... (building tree)\r"));

	// Determine how many frequent items there are (from database alphabet)
	uint32 numFrequentItems = 0;
	alphabet *alph = mDatabase->GetAlphabet();
	alphabet::iterator iter;
	for(iter = alph->begin(); iter != alph->end(); iter++)
		if(iter->second >= mMinSup)
			++numFrequentItems;

	// Build root, containing frequent items and their supports
	uint16 *frequentItems = new uint16[numFrequentItems];
	uint32 *frequentItemSupports = new uint32[numFrequentItems];
	ItemSet *infrequentIS = ItemSet::CreateEmpty(mDatabase->GetDataType(), alph->size());
	uint32 idx = 0;
	for(iter = alph->begin(); iter != alph->end(); iter++) {
		if(iter->second >= mMinSup) {
			frequentItems[idx] = iter->first;
			frequentItemSupports[idx] = iter->second;
			++idx;
		} else {
			infrequentIS->AddItemToSet(iter->first);
		}
	}
	AprioriNode *root = new AprioriNode(numFrequentItems, frequentItems, frequentItemSupports);

	// Remove infrequent items from (my own copy of) the database
	mDatabase->RemoveItems(infrequentIS);
	delete infrequentIS;

	// Init tree
	mTree = new AprioriTree(root, mDatabase, mMinSup);

	// Build tree level by level
	mActualMaxLen = mMaxLen;
	for(uint32 lvl=2; lvl<=mMaxLen; lvl++) {			// stop when either maxLen reached or ...
		ECHO(2,printf(" * Mining:\t\tin progress... (level %u)\t\t\t\r", lvl));
		if(!mTree->AddLevel()) {							// ... nothing is added to tree
			mActualMaxLen = lvl - 1;
			break;
		}
	}
}
void AprioriMiner::ChopDownTree() {
	delete mTree; 
	mTree = NULL;
}
void AprioriMiner::MineOnline(AprioriCroupier *croupier, ItemSet **buffer, const uint32 bufferSize) {
	ECHO(2,printf(" * Mining:\t\tin progress... (gathering itemsets)\r"));
	mTree->MineItemSets(croupier, buffer, bufferSize);
}
