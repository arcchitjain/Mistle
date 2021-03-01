
#include "../clobal.h"

#include <Bass.h>
#include <db/Database.h>
#include <logger/Log.h>

#include "AprioriCroupier.h"
#include "AprioriNode.h"

#include "AprioriTree.h"

AprioriTree::AprioriTree(AprioriNode *root, Database *database, const uint32 minSup) {
	mRoot = root;
	mDatabase = database;
	mMinSup = minSup;

	mNumLevels = 1;
	mLevels = new AprioriNode *[mDatabase->GetMaxSetLength()];
	mLevels[0] = mRoot;

	mSet = new uint16[mDatabase->GetMaxSetLength()];
}

AprioriTree::~AprioriTree() {
	delete mRoot; // should recursively delete everything
	delete[] mLevels;
	delete mSet;
}

bool AprioriTree::AddLevel() {
	// Add another level to the tree
	AprioriNode *firstOfNewLevel = NULL, *previousLastChild = NULL, *firstChild, *lastChild;
	for(AprioriNode *node = mLevels[mNumLevels-1]; node != NULL; node = node->GetNext()) {
		// Add level to current node
		node->AddLevel(mMinSup, &firstChild, &lastChild);

		// If anything added
		if(firstChild != NULL) {
			// Store first of new level
			if(firstOfNewLevel == NULL)
				firstOfNewLevel = firstChild;
			// Establish link if there is a previous lastChild
			if(previousLastChild != NULL)
				previousLastChild->SetNext(firstChild);	// attach firstchild to previous lastchild
			// Save last child for future linkage
			previousLastChild = lastChild;
		}
	}

	// Break if nothing added
	if(firstOfNewLevel == NULL)
		return false;

	// New level added
	mLevels[mNumLevels++] = firstOfNewLevel;

	// Throw database in the tree, to do the necessary counting on the latest level				
	// -- Possible optimisation: store uint16 sets locally in lists in leaves, only need to throw them down from there! (More bookkeeping to do though.)
	uint32 numRows = mDatabase->GetNumRows();
	ItemSet **rows = mDatabase->GetRows();
	for(uint32 i=0; i<numRows; i++)
		if(rows[i]->GetLength() >= mNumLevels) {
			rows[i]->GetValuesIn(mSet);
			mRoot->CountRow(mSet, 0, rows[i]->GetLength(), 1, mNumLevels);
		}

	// Prune the latest level (remove all items with sup<minSup)
	mRoot->PruneLevel(mMinSup, 1, mNumLevels);

	return true;
}
void AprioriTree::MineItemSets(AprioriCroupier *croupier, ItemSet **buffer, const uint32 bufferSize) {
	uint32 numSets = 0;
	ItemSet *emptySet = ItemSet::CreateEmpty(mDatabase->GetDataType(), mDatabase->GetAlphabetSize());
	mRoot->MineItemSets(croupier, mMinSup, emptySet, buffer, bufferSize, numSets);
	delete emptySet;
	croupier->MinerIsErMooiKlaarMeeCBFunc(buffer, numSets);
}
