#ifdef BLOCK_PTREE

#include "../../global.h"

#include <RandomUtils.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>

#include "../../algo/CTFile.h"
#include "PTree.h"
#include "PTreeNode.h"

PTree::PTree(Database *db) {
	mDB = db;
	mRoot = new PTreeNode();

	mABSize = (uint32) db->GetAlphabetSize();
	mAlphabet = new uint16[mABSize];
	mABFrequency = new uint32[mABSize];
	mABUncovered = new uint32[mABSize];

	mVDB = new uint32*[mABSize];
	uint32 *vDBColLen = new uint32[mABSize];
	mSideWalk = new ptnlist*[mABSize];

	alphabet *ab = db->GetAlphabet();
	mItemRowIdxs = new uint32*[mABSize];
	mItemNumRows = new uint32[mABSize];
	uint32 i=0;
	for(alphabet::iterator ait = ab->begin(); ait != ab->end(); ++ait) {
		mAlphabet[i] = ait->first;
		// !!! toch maar even lui uitgaan van 0-base item-nummering
		mABFrequency[i] = ait->second;
		mABUncovered[i] = ait->second;

		mVDB[i] = new uint32[mABFrequency[i]];
		vDBColLen[i] = 0;

		mSideWalk[i] = new ptnlist;

		mItemRowIdxs[i] = new uint32[ait->second];
		mItemNumRows[i] = 0;

		i++;
	}

	uint16 *vals = new uint16[mABSize];
	mNumRows = db->GetNumRows();
	mRows = db->GetRows();
	for(uint32 r=0; r<mNumRows; r++) {
		mRows[r]->GetValuesIn(vals);
		uint32 numVals = mRows[r]->GetLength();

		PTreeNode *curNode = mRoot;
		for(uint32 v=0; v<numVals; v++) {
			curNode = curNode->AddSubItem(vals[v], mRows[r]->GetUsageCount());
			mItemRowIdxs[vals[v]][mItemNumRows[vals[v]]] = r;
			mItemNumRows[vals[v]]++;

			mVDB[vals[v]][vDBColLen[vals[v]]++] = r;
		}
	}
	delete[] vals;
	delete[] vDBColLen;

	
	ptnlist *rc = mRoot->GetChildrenList();
	ptnlist::iterator cit, cend = rc->end();
	for(cit = rc->begin(); cit != cend; ++cit) {
		((PTreeNode*)*cit)->BuildSideWalk(mSideWalk);
	}
}

PTree::~PTree() {
	delete mRoot;

	for(uint32 a=0; a<mABSize; a++) {
		delete[] mItemRowIdxs[a];
		delete[] mVDB[a];
		delete mSideWalk[a];
	}
	delete[] mItemRowIdxs;
	delete[] mItemNumRows;
	delete[] mVDB;
	delete[] mSideWalk;

	delete[] mAlphabet;
	delete[] mABFrequency;
	delete[] mABUncovered;
}

void PTree::FindLargestTileFPStylee() {
	// calc areaLB 
	uint32 areaUB1 = mABUncovered[0];
	uint32 areaLB1 = mABUncovered[0];
	uint32 areaLB1ItemIdx = 0;
	for(uint32 i=1; i<mABSize; i++) {
		if(mABUncovered[i] > areaLB1) {
			areaLB1 = mABUncovered[i];
			areaLB1ItemIdx = i;
		}
		areaUB1 += mABUncovered[i];
	}
	printf("areaLB1: %d\tMax uncovered column\n", areaLB1);
	uint32 areaLB2 = 0;
	for(uint32 r=0; r<mNumRows; r++) {
		if(mRows[r]->GetLength() > areaLB2)
			areaLB2 = mRows[r]->GetLength();
	}
	printf("areaLB2: %d\tMax uncovered row\n", areaLB2);
	uint32 areaLB = max(areaLB1, areaLB2);

	printf("areaUB1: %d\tTotal number of uncovered items\n", areaUB1);

	// assume full-dependency, ab ordered on numUncovered
	// then simply find largest square in histogram
	uint32 areaUB2 = 0;
	uint32 itemUB2 = 0;
	for(uint32 i=0; i<mABSize; i++) {
		uint32 abItemMax = (i+1) * mABUncovered[i];
		if(abItemMax > areaUB2) {
			areaUB2 = abItemMax;
			itemUB2 = i;
		}
	}
	printf("areaUB2: %d\tMax square uncovered histogram\n", areaUB2);

	uint32 *areaUBItem = new uint32[mABSize];
	uint32 areaUB3 = areaUB1;
	uint32 areaUB4 = areaUB1;
	uint32 *tmpArray = new uint32[mABSize];
	uint16 *removableItemArr = new uint16[mABSize];
	uint32 prunedABSize = mABSize;
	uint32 *rowLengthHistogram = new uint32[mABSize];
	memset(rowLengthHistogram, 0, mABSize);
	// build rowlength histogram for db
	for(uint32 i=0; i<mNumRows; i++) {
		rowLengthHistogram[mRows[i]->GetLength()]++;		
	}

	bool converged = false;
	while(!converged) {
		areaUB3 = areaUB1;

		uint32 numRemovableItems = 0;

		// UB4
		// -> ordered mItemNumRows
		areaUB4 = 0;
		uint32 maxRowLen = mDB->GetMaxSetLength();
		for(uint32 i=0; i<maxRowLen; i++) {
			areaUB4 += mItemNumRows[i];
		}

		for(uint32 i=0; i<prunedABSize; i++) {
			areaUBItem[i] = 0;
			memset(tmpArray, 0, mABSize);

			// UB3
			uint32 tmpMaxIdx = 0;
			uint32 tmpMinIdx = mABSize-1;
			// en dus niet vanaf item, maar tot/met
			for(uint32 j=0; j<mItemNumRows[i]; j++) {
				uint32 rowLen = mRows[mItemRowIdxs[i][j]]->GetLength();
				tmpArray[rowLen]++;
				if(tmpMaxIdx < rowLen)
					tmpMaxIdx = rowLen;
				if(tmpMinIdx > rowLen)
					tmpMinIdx = rowLen;
			}
			uint32 sum = mItemNumRows[i];
			for(uint32 l=tmpMinIdx; l<=tmpMaxIdx; l++) {
				if(areaUBItem[i] < l *  sum)
					areaUBItem[i] = l * sum;
				sum -= tmpArray[l];
			}
			if(areaUBItem[i] > areaUB3)
				areaUB3 = areaUBItem[i];
			if(areaUBItem[i] < areaLB1) {
				// ??? if items ordered right, can we stop and zap all from i onward, including i?
				removableItemArr[numRemovableItems++] = mAlphabet[i];
			}
		}
		if(numRemovableItems == 0)
			converged = true;
		else {
			for(uint32 p=0; p<numRemovableItems; p++) {
				for(uint32 j=0; j<mItemNumRows[removableItemArr[p]]; j++) {
					mRows[mItemRowIdxs[removableItemArr[p]][j]]->RemoveItemFromSet(removableItemArr[p]);
				}
			}
			prunedABSize -= numRemovableItems;
		}
	}
	printf("areaUB3: %d\tMax sup(i) * smallest row that includes i\n", areaUB3);
	printf("areaUB4: %d\tMax sum num uncovered (wrt row length and stuff)\n", areaUB4);
	delete[] tmpArray;
	delete[] areaUBItem;

	// recurse into tree, use dwars-links-table
}

void PTree::Print() const {
	ptnlist *rc = mRoot->GetChildrenList();
	ptnlist::iterator cit, cend = rc->end();
	for(cit = rc->begin(); cit != cend; ++cit) {
		((PTreeNode*)*cit)->Print();
	}
}
void PTree::PrintTree() const {
	ptnlist *rc = mRoot->GetChildrenList();
	ptnlist::iterator cit, cend = rc->end();
	for(cit = rc->begin(); cit != cend; ++cit) {
		((PTreeNode*)*cit)->PrintTree();
	}
}

#endif // BLOCK_PTREE