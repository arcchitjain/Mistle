//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

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
