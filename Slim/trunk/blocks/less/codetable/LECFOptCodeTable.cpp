#ifdef BLOCK_LESS

#include "../../global.h"

#include <stdlib.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/BM128ItemSet.h>
#include <itemstructs/BM128CoverSet.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>
#include <isc/ItemTranslator.h>

#include "LECodeTable.h"
#include "LowEntropySet.h"
#include "LowEntropySetInstantiation.h"
#include "LowEntropySetCollection.h"
#include "LESList.h"

#include "LECFOptCodeTable.h"

LECFOptCodeTable::LECFOptCodeTable(string algoname) : LECodeTable() {
	mLESList = NULL;

	mNumDBRows = 0;
	mCurNumSets = 0;
	mPrevNumSets = 0;

	mABCounts = NULL;
	mAB01Counts = NULL;
	mOldABCounts = NULL;
	mOldAB01Counts = NULL;
	mZeroedABCounts = NULL;

	mABLen = 0;
	mABLH = NULL;

	mAttrValues = NULL;
	mAttributes = NULL;

	mAdded = NULL;
	mAddedCandidateId = 0;
	mAddedInsertionPos = NULL;
	mAddedInst = NULL;

	mZapped = NULL;
	mZappedInsertionPos = NULL;

	mRowInsts = NULL;
	mNumActiveRowInsts = NULL;
	mLenRowInsts = 0;
	mTmpRowInsts = NULL;

	mInstUseful = NULL;
	mLenInstUseful = 0;

	mNumBitmaps = 0;
	mNumPosBitmaps = 0;
	mBitmapCounts = NULL;
	mBitmapCountSums = NULL;

	mOptCover = NULL;
	mOptCoverLen = NULL;
	mOptCoverLH = NULL;
	mCurCover = NULL;
	mCurCoverLen = NULL;
	mCurCoverLH = NULL;
	mNewCover = NULL;
	mNewCoverLen = NULL;
	mNewCoverLH = NULL;

	if(algoname.substr(GetConfigName().length()+1).compare("llhsz-all") == 0)
		mInstOrder = LEOrderLogLHPerSizeDescForAll;
	else if(algoname.substr(GetConfigName().length()+1).compare("llhsz-sets") == 0)
		mInstOrder = LEOrderLogLHPerSizeDescForSets;
	else 
		mInstOrder = LEOrderLogLHPerSizeDescForAll;
}

LECFOptCodeTable::~LECFOptCodeTable() {
	leslist::iterator cit, cend=mLESList->GetList()->end();
	for(cit = mLESList->GetList()->begin(); cit != cend; ++cit)
		delete *cit;
	delete mLESList;

	delete[] mAttrValues;
	delete[] mAttributes;

	delete mOnesCSet;
	delete mOnesISet;
	for(uint32 i=0; i<mNumDBRows; i++)
		delete mCSets[i];
	delete[] mCSets;

	delete[] mABCounts;
	delete[] mOldABCounts;
	delete[] mZeroedABCounts;
	if(mABLH != NULL) {
		delete[] mABLH[0];
		delete[] mABLH[1];
	} delete[] mABLH;
	if(mRowInsts != NULL) {
		for(uint32 i=0; i<mNumDBRows; i++)
			delete[] mRowInsts[i];
	}
	delete[] mAB01Counts[0];
	delete[] mAB01Counts[1];
	delete[] mAB01Counts;
	delete[] mOldAB01Counts[0];
	delete[] mOldAB01Counts[1];
	delete[] mOldAB01Counts;
	for(uint32 i=0; i<mNumBitmaps; i++)
		delete[] mBitmapCounts[i];
	delete[] mBitmapCounts;
	delete[] mBitmapCountSums;

	delete[] mRowInsts;
	delete[] mNumActiveRowInsts;
	delete[] mTmpRowInsts;
	delete[] mAddedInsertionPos;
	delete[] mZappedInsertionPos;
	delete[] mAddedInst;
	delete[] mInstUseful;

	if(mOptCover != NULL) {
		for(uint32 i=0; i<mNumDBRows; i++) {
			delete[] mOptCover[i];
		}
		delete[] mOptCoverLen;
		delete[] mOptCoverLH;
	} 
	delete[] mOptCover;
	mOptCover = NULL;

	if(mCurCover != NULL) {
		delete[] mCurCoverLen;
		delete[] mCurCoverLH;
	}
	delete[] mCurCover;
	mCurCover = NULL;

	if(mNewCover != NULL) {
		for(uint32 i=0; i<mNumDBRows; i++) {
			delete[] mNewCover[i];
		}
		delete[] mNewCoverLen;
		delete[] mNewCoverLH;
	} 
	delete[] mNewCover;
	mNewCover = NULL;
}

void LECFOptCodeTable::UseThisStuff(Database *db, LEBitmapStrategy bs, uint32 maxSetLen) {
	LECodeTable::UseThisStuff(db,bs, maxSetLen);

	// Alphabet init'en
	alphabet *a = db->GetAlphabet();
	mABLen = (uint32) a->size();
	mABLH = new float*[2];
	mABLH[0] = new float[mABLen];
	mABLH[1] = new float[mABLen];

	// ab counts
	mABCounts = new uint32[mABLen];
	mAB01Counts = new uint32*[2];
	mAB01Counts[0] = new uint32[mABLen];
	mAB01Counts[1] = new uint32[mABLen];
	mOldABCounts = new uint32[mABLen];
	mOldAB01Counts = new uint32*[2];
	mOldAB01Counts[0] = new uint32[mABLen];
	mOldAB01Counts[1] = new uint32[mABLen];
	mZeroedABCounts = new uint32[mABLen];

	// dump-arrays
	mAttrValues = new uint16[mABLen];
	mAttributes = new uint16[mABLen];

	// cover stuff
	mOptCover = new LowEntropySetInstantiation**[mNumDBRows];
	mCurCover = new LowEntropySetInstantiation**[mNumDBRows];
	mNewCover = new LowEntropySetInstantiation**[mNumDBRows];
	mOptCoverLH = new double[mNumDBRows];
	mCurCoverLH = new double[mNumDBRows];
	mNewCoverLH = new double[mNumDBRows];
	mOptCoverLen = new uint32[mNumDBRows];
	mCurCoverLen = new uint32[mNumDBRows];
	mNewCoverLen = new uint32[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		// cur dont need no init'ing
		mOptCover[i] = new LowEntropySetInstantiation*[(mABLen+1)/2];	// worst set-cover consists of 2-tuples
		mNewCover[i] = new LowEntropySetInstantiation*[(mABLen+1)/2];
		mOptCoverLH[i] = 1;
		mNewCoverLH[i] = 1;
		mOptCoverLen[i] = 0;
		mNewCoverLen[i] = 0;
	}

	// stdlength 
	mABStdLen = CalcCodeLength(mNumDBRows, mNumDBRows * mABLen);

	// inst-bitmap init
	mNumBitmaps = maxSetLen + 1;
	mNumPosBitmaps = 0;
	mBitmapCounts = new uint32*[mNumBitmaps];
	mBitmapCountSums = new uint32[mNumBitmaps];
	for(uint32 i=0; i<mNumBitmaps; i++) {
		mBitmapCountSums[i] = 0;
		uint32 numInsts = i > 0 ? (1 << i) : 0;
		mNumPosBitmaps += numInsts;
		mBitmapCounts[i] = new uint32[numInsts];
		for(uint32 j=0; j<numInsts; j++)
			mBitmapCounts[i][j] = 0;
	}

	// ab lh & bitmap init 
	uint32 setCountSum = 0, bmCountSum = 0, acnt=0;
	uint32 cnt = 0;
	alphabet::iterator ait, aend=a->end();
	for(ait = a->begin(); ait != aend; ++ait) {
		if(ait->first != cnt)
			throw string("LECFCodeTable:: This CodeTable implementation requires an 0-based alphabet.");
		acnt = ait->second;

		mABLH[1][cnt] = (acnt / (float) mNumDBRows);
		mABLH[0][cnt] = (mNumDBRows - acnt) / (float) mNumDBRows;
		mAB01Counts[0][cnt] = mNumDBRows-acnt;
		mAB01Counts[1][cnt] = acnt;
		mOldAB01Counts[0][cnt] = mNumDBRows-acnt;
		mOldAB01Counts[1][cnt] = acnt;
		mABCounts[cnt] = mNumDBRows;
		mOldABCounts[cnt] = mNumDBRows;
		mZeroedABCounts[cnt] = 0;

		mBitmapCounts[1][0] += mNumDBRows - acnt;
		mBitmapCounts[1][1] += acnt;

		cnt++;
	}
	setCountSum = mABLen * mNumDBRows;
	mCurStats.setCountSum = setCountSum;
	mBitmapCountSums[1] = setCountSum;

	// encoding sets containter init
	if(mLESList == NULL) {
		mLESList = new LESList();
		mCurNumSets = 0;
		mPrevNumSets = 0;
	}

	// ones-coverset
	mOnesISet = ItemSet::CreateEmpty(mDB->GetDataType(), mABLen);
	mOnesISet->FillHerUp(mABLen);
	mOnesCSet = CoverSet::Create(mDB->GetDataType(), mABLen, mOnesISet);

	// usable le-insts per row
	mRowInsts = new LowEntropySetInstantiation**[mNumDBRows];
	mTmpRowInsts = new LowEntropySetInstantiation*[mNumDBRows];
	mAddedInst = new LowEntropySetInstantiation*[mNumDBRows];
	mNumActiveRowInsts = new uint32[mNumDBRows];
	mAddedInsertionPos = new uint32[mNumDBRows];
	mZappedInsertionPos = new uint32[mNumDBRows];
	mLenRowInsts = 50;

	mOnesCSet->GetUncoveredAttributesIn(mAttributes);
	uint32 nuc = mOnesCSet->GetNumUncovered();

	// coversets and lh-base calc
	mCSets = new CoverSet*[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCSets[i] = CoverSet::Create(mDB->GetDataType(), mABLen, mDB->GetRow(i));
		mRowInsts[i] = new LowEntropySetInstantiation*[mLenRowInsts];
		mNumActiveRowInsts[i] = 0;

		mCSets[i]->GetUncoveredAttributesIn(mAttrValues);
		uint32 nuo = mCSets[i]->GetNumUncovered();

		// first default to zeroes
		for(uint32 j=0; j<nuc; j++) {
			mOptCoverLH[i] *= mABLH[0][mAttributes[j]];
		}
		// now do the ones
		for(uint32 j=0; j<nuo; j++) {
			mOptCoverLH[i] /= mABLH[0][mAttrValues[j]];
			mOptCoverLH[i] *= mABLH[1][mAttrValues[j]];
		}
	}


	CoverDB(mCurStats);
	mPrevStats = mCurStats;
}
void LECFOptCodeTable::SetDatabase(Database *db) {
	LECodeTable::SetDatabase(db);
	mNumDBRows = mDB->GetNumRows();
}

void LECFOptCodeTable::Add(LowEntropySet *les, uint64 candidateId) {
	uint32 len = les->GetLength();
	mLESList->GetList()->push_back(les);
	++mCurNumSets;

	mAdded = les;
	mAddedCandidateId = candidateId;
}

void LECFOptCodeTable::Del(LowEntropySet *les, bool zap) {
	if(zap == true) {
		leslist *lst = mLESList->GetList();

		if(lst->back() == les) {
			lst->pop_back();
		} else {
			leslist::iterator iter, lend = lst->end();
			for(iter=lst->begin(); iter!=lend; ++iter)
				if((LowEntropySet*)(*iter) == les)
					break;
			lend = iter;
			++lend;
			lst->erase(iter);
		}

		delete les;
		mZapped = NULL;
		--mCurNumSets;
	} else
		mZapped = les;
}

void LECFOptCodeTable::CommitAdd(bool updateLog) {
	// only commits/rollbacks add
	mAdded = NULL;
	mPrevStats = mCurStats;

	if(mLenRowInsts < mCurNumSets) {
		for(uint32 i=0; i<mNumDBRows; i++) {
			LowEntropySetInstantiation** tmp = new LowEntropySetInstantiation*[mLenRowInsts * 2];
			memcpy(tmp, mRowInsts[i], mNumActiveRowInsts[i] * sizeof(LowEntropySetInstantiation*));
			delete[] mRowInsts[i];
			mRowInsts[i] = tmp;
		}
		delete[] mTmpRowInsts;
		mTmpRowInsts = new LowEntropySetInstantiation*[mLenRowInsts * 2];
		mLenRowInsts = mLenRowInsts * 2;
	}

	// mBakRowInstsDepth is OK
	for(uint32 i=0; i<mNumDBRows; i++) {
		// tot mAddedInsertionPos[i] is mBakRowInsts[i] OK

		// shift vanaf mAddedInsertionPos[i] 1 positie naar rechts
		uint32 instPos = mAddedInsertionPos[i];
		if(mAddedInsertionPos[i] != UINT32_MAX_VALUE) {
			uint32 numCopyBytes = (mNumActiveRowInsts[i] - instPos) * sizeof(LowEntropySetInstantiation*);
			memcpy(mTmpRowInsts, mRowInsts[i] + instPos, numCopyBytes);
			memcpy(mRowInsts[i] + (instPos+1), mTmpRowInsts, numCopyBytes);

			// voeg mAddedInst[i] op mAddedInsertionPos in
			mRowInsts[i][instPos] = mAddedInst[i];
			mNumActiveRowInsts[i]++;
		}

		// regel mNewCover <-> mOptCover
		if(mNewCoverLH[i] > mOptCoverLH[i]) {
			// now we gotta swap 'em
			LowEntropySetInstantiation **tmpAr = mOptCover[i];
			mOptCover[i] = mNewCover[i];
			mNewCover[i] = tmpAr;
			double tmpLH = mOptCoverLH[i];
			mOptCoverLH[i] = mNewCoverLH[i];
			mNewCoverLH[i] = tmpLH;
			uint32 tmpLen = mOptCoverLen[i];
			mOptCoverLen[i] = mNewCoverLen[i];
			mNewCoverLen[i] = tmpLen;
		}
	}
	mPrevNumSets = mCurNumSets;
}

void LECFOptCodeTable::CommitLastDel(bool zap, bool updateLog) {
	// mRowInsts opschonen
	for(uint32 i=0; i<mNumDBRows; i++) {
		uint32 instPos = mZappedInsertionPos[i];
		if(instPos != UINT32_MAX_VALUE) {
			uint32 numCopyBytes = (mNumActiveRowInsts[i] - instPos - 1) * sizeof(LowEntropySetInstantiation*);
			memcpy(mTmpRowInsts, mRowInsts[i] + (instPos + 1), numCopyBytes);
			memcpy(mRowInsts[i] + instPos, mTmpRowInsts, numCopyBytes);
			mNumActiveRowInsts[i]--;
		}
	}

	// haal mZapped uit mLEList, en zap 'm 
	Del(mZapped, zap);

	mZapped = NULL;
	mCurStats = mPrunedStats;
}

void LECFOptCodeTable::RollbackAdd() {
	// Rolls back the added LowEntropySet
	// Restores DB & CT sizes
	if(mAdded != NULL) {

		Del(mAdded, true);
		mAdded = NULL;
		mCurStats = mPrevStats;
	}
}
void LECFOptCodeTable::RollbackLastDel() {
	if(mZapped != NULL)	{
		mZapped = NULL;
	}
}
void LECFOptCodeTable::RollbackCounts() {
	mLESList->RollbackCounts();
	memcpy(mABCounts, mOldABCounts, mABLen * sizeof(uint32));
	memcpy(mAB01Counts[0], mOldAB01Counts[0], mABLen * sizeof(uint32));
	memcpy(mAB01Counts[1], mOldAB01Counts[1], mABLen * sizeof(uint32));
}

void LECFOptCodeTable::BranchOptimalCoverWithAdded(uint32 rowIdx, uint32 instOffset, uint32 &brOptCoverLen, double &brOptCoverLH, LowEntropySetInstantiation ***brOptCover) {

	LowEntropySetInstantiation **coverWithout = new LowEntropySetInstantiation*[(mABLen+1)/2];
	memcpy(coverWithout, *brOptCover, brOptCoverLen * sizeof(LowEntropySetInstantiation*));
	uint32 coverWithoutLen = brOptCoverLen;
	double coverWithoutLH = brOptCoverLH;

	uint32 i = rowIdx;
	uint32 e = instOffset;

	// first calculate the branch where we actually gonna use this set
	LowEntropySetInstantiation **coverWith = new LowEntropySetInstantiation*[(mABLen+1)/2];
	memcpy(coverWith, *brOptCover, brOptCoverLen * sizeof(LowEntropySetInstantiation*));
	mOnesCSet->Cover(mRowInsts[i][e]->GetLowEntropySet()->GetLength(), mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition());
	mCSets[i]->Cover(mRowInsts[i][e]->GetItemSet()->GetLength(), mRowInsts[i][e]->GetItemSet());
	uint32 coverWithLen = brOptCoverLen;
	double coverWithLH = brOptCoverLH * mRowInsts[i][e]->GetLikelihood();
	coverWith[coverWithLen] = mRowInsts[i][e];
	coverWithLen++;

	CalcOptimalCoverWithAdded(rowIdx, instOffset+1, coverWithLen, coverWithLH, &coverWith);
	mOnesCSet->Uncover(mRowInsts[i][e]->GetLowEntropySet()->GetLength(), mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition());
	mCSets[i]->Uncover(mRowInsts[i][e]->GetItemSet()->GetLength(), mRowInsts[i][e]->GetItemSet());

	// now calculate the branch where we dont use this set
	// . so we dont cover, adjust lh and/or len
	CalcOptimalCoverWithAdded(rowIdx, instOffset+1, coverWithoutLen, coverWithoutLH, &coverWithout);

	if(coverWithoutLH > coverWithLH) {
		brOptCoverLen = coverWithoutLen;
		brOptCoverLH = coverWithoutLH;
		memcpy(*brOptCover, coverWithout, coverWithoutLen * sizeof(LowEntropySetInstantiation*));
		if(coverWithoutLH > mBestFoundThusfarLH)
			mBestFoundThusfarLH = coverWithoutLH;
	} else {
		brOptCoverLen = coverWithLen;
		brOptCoverLH = coverWithLH;
		memcpy(*brOptCover, coverWith, coverWithLen * sizeof(LowEntropySetInstantiation*));
		if(coverWithLH > mBestFoundThusfarLH)
			mBestFoundThusfarLH = coverWithLH;
	}
	delete[] coverWithout;
	delete[] coverWith;
}

// here we continue our exhaustive search to the optimal cover, best cover found will be returned by parameters
void LECFOptCodeTable::CalcOptimalCoverWithAdded(uint32 rowIdx, uint32 instOffset, uint32 &optCoverLen, double &optCoverLH, LowEntropySetInstantiation ***optCover) {
	// Here we just check where things might get better
	uint32 i=rowIdx;
	bool didBranch = false;
	for(uint32 e=instOffset; e<mNumActiveRowInsts[rowIdx]; e++) {
		if(mOnesCSet->CanCover(mRowInsts[i][e]->GetLowEntropySet()->GetLength(), mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition())) {

			// 1. prune strat
			uint32 numInOurWay = 0;
			for(uint32 f=e+1; f<mNumActiveRowInsts[rowIdx]; f++) {
				if(mOnesCSet->CanCover(mRowInsts[i][f]->GetLowEntropySet()->GetLength(), mRowInsts[i][f]->GetLowEntropySet()->GetAttributeDefinition())) {
					if(mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition()->Intersects(mRowInsts[i][f]->GetLowEntropySet()->GetAttributeDefinition()))
						numInOurWay++;
				}
			}
			if(numInOurWay > 0) {
				double instLH = mRowInsts[i][e]->GetLikelihood();
				// !!! more branch pruning here !!!
				if(optCoverLH * instLH > mBestFoundThusfarLH) {
					BranchOptimalCoverWithAdded(rowIdx,instOffset,optCoverLen,optCoverLH,optCover);
					didBranch = true;
					break;
				}
			} else {
				// we can just simply add this one here, if it's better than singletons
				double slh = 1;
				mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition()->GetValuesIn(mAttributes);
				mRowInsts[i][e]->GetItemSet()->GetValuesIn(mAttrValues);
				uint32 nuc = mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition()->GetLength();
				uint32 nuo = mRowInsts[i][e]->GetItemSet()->GetLength();

				// first default to zeroes
				for(uint32 j=0; j<nuc; j++) {
					slh *= mABLH[0][mAttributes[j]];
				}
				// now do the ones
				for(uint32 j=0; j<nuo; j++) {
					slh /= mABLH[0][mAttrValues[j]];
					slh *= mABLH[1][mAttrValues[j]];
				}
				if(slh < mRowInsts[i][e]->GetLikelihood()) {
					// just use the candidate
					(*optCover)[optCoverLen++] = mRowInsts[i][e];
					optCoverLH *= mRowInsts[i][e]->GetLikelihood();
					mOnesCSet->Cover(mRowInsts[i][e]->GetLowEntropySet()->GetLength(), mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition());
					mCSets[i]->Cover(mRowInsts[i][e]->GetItemSet()->GetLength(), mRowInsts[i][e]->GetItemSet());
				} else {
					// dont use this bugger
					continue;
				}
			}
			//
		}
	}

	if(!didBranch) {
		// Optimally covered using sets, now fill in the blanks using alphabet elements
		mOnesCSet->GetUncoveredAttributesIn(mAttributes);
		mCSets[i]->GetUncoveredAttributesIn(mAttrValues);
		uint32 nuc = mOnesCSet->GetNumUncovered();
		uint32 nuo = mCSets[i]->GetNumUncovered();			

		// first default to zeroes
		for(uint32 j=0; j<nuc; j++) {
			optCoverLH *= mABLH[0][mAttributes[j]];
		}
		// now do the ones
		for(uint32 j=0; j<nuo; j++) {
			optCoverLH /= mABLH[0][mAttrValues[j]];
			optCoverLH *= mABLH[1][mAttrValues[j]];
		}
		// so now we're done, and optCoverLH, optCoverLen and optCover are up-to-date

		if(optCoverLH > mBestFoundThusfarLH)
			mBestFoundThusfarLH = optCoverLH;

	} // else, we've gotten the results from the branch itself
}

void LECFOptCodeTable::BranchOptimalCoverWithoutZapped(uint32 rowIdx, uint32 instOffset, uint32 &brOptCoverLen, double &brOptCoverLH, LowEntropySetInstantiation ***brOptCover) {

	LowEntropySetInstantiation **coverWithout = new LowEntropySetInstantiation*[(mABLen+1)/2];
	memcpy(coverWithout, *brOptCover, brOptCoverLen * sizeof(LowEntropySetInstantiation*));
	uint32 coverWithoutLen = brOptCoverLen;
	double coverWithoutLH = brOptCoverLH;

	uint32 i = rowIdx;
	uint32 e = instOffset;

	// first calculate the branch where we actually gonna use this set
	LowEntropySetInstantiation **coverWith = new LowEntropySetInstantiation*[(mABLen+1)/2];
	memcpy(coverWith, *brOptCover, brOptCoverLen * sizeof(LowEntropySetInstantiation*));
	mOnesCSet->Cover(mRowInsts[i][e]->GetLowEntropySet()->GetLength(), mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition());
	mCSets[i]->Cover(mRowInsts[i][e]->GetItemSet()->GetLength(), mRowInsts[i][e]->GetItemSet());
	uint32 coverWithLen = brOptCoverLen;
	double coverWithLH = brOptCoverLH * mRowInsts[i][e]->GetLikelihood();
	coverWith[coverWithLen++] = mRowInsts[i][e];
	CalcOptimalCoverWithoutZapped(rowIdx, instOffset+1, coverWithLen, coverWithLH, &coverWith);
	mCSets[i]->Uncover(mRowInsts[i][e]->GetItemSet()->GetLength(), mRowInsts[i][e]->GetItemSet());
	mOnesCSet->Uncover(mRowInsts[i][e]->GetLowEntropySet()->GetLength(), mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition());

	// now calculate the branch where we dont use this set
	// . so we dont cover, adjust lh and/or len
	CalcOptimalCoverWithoutZapped(rowIdx, instOffset+1, coverWithoutLen, coverWithoutLH, &coverWithout);

	if(coverWithoutLH > coverWithLH) {
		brOptCoverLen = coverWithoutLen;
		brOptCoverLH = coverWithoutLH;
		memcpy(*brOptCover, coverWithout, coverWithoutLen * sizeof(LowEntropySetInstantiation*));
	} else {
		brOptCoverLen = coverWithLen;
		brOptCoverLH = coverWithLH;
		memcpy(*brOptCover, coverWith, coverWithLen * sizeof(LowEntropySetInstantiation*));
	}
	delete[] coverWithout;
	delete[] coverWith;
}

void LECFOptCodeTable::CalcOptimalCoverWithoutZapped(uint32 rowIdx, uint32 instOffset, uint32 &optCoverLen, double &optCoverLH, LowEntropySetInstantiation ***optCover) {
	// Here we just check where things might get better
	uint32 i=rowIdx;
	for(uint32 e=instOffset; e<mNumActiveRowInsts[rowIdx]; e++) {
		if(e == mZappedInsertionPos[i])
			continue;
		if(mOnesCSet->CanCover(mRowInsts[i][e]->GetLowEntropySet()->GetLength(), mRowInsts[i][e]->GetLowEntropySet()->GetAttributeDefinition())) {
			double instLH = mRowInsts[i][e]->GetLikelihood();
			// !!! more branch pruning here !!!
			BranchOptimalCoverWithoutZapped(rowIdx,instOffset,optCoverLen,optCoverLH,optCover);
			break;
		}
	}
	// Optimally covered using sets, now fill in the blanks using alphabet elements

	mOnesCSet->GetUncoveredAttributesIn(mAttributes);
	mCSets[i]->GetUncoveredAttributesIn(mAttrValues);
	uint32 nuc = mOnesCSet->GetNumUncovered();
	uint32 onIdx = 0;
	throw string(" lalalal !");

	for(uint32 j=0; j<nuc; j++) {
		if(mAttributes[j] == mAttrValues[onIdx]) {
			// attribute is 'on'
			optCoverLH *= mABLH[1][mAttributes[j]];
			onIdx++;
		} else {
			// attribute is 'off'
			optCoverLH *= mABLH[0][mAttributes[j]];
		}
	}
	// so now we're done, and optCoverLH, optCoverLen and optCover are up-to-date}
}

void LECFOptCodeTable::CoverDB(LECoverStats &stats) {
	// init counts
	mLESList->ResetCounts();
	for(uint32 i=1; i<mNumBitmaps; i++) {
		uint32 numInsts = 1 << i; 
		for(uint32 j=0; j<numInsts; j++)
			mBitmapCounts[i][j] = 0;
		mBitmapCountSums[i] = 0;
	}
	leslist::iterator cit, cend = mLESList->GetList()->end();

	memcpy(mOldABCounts, mABCounts, mABLen * sizeof(uint32));
	memcpy(mABCounts, mZeroedABCounts, mABLen * sizeof(uint32));

	memcpy(mOldAB01Counts[0], mAB01Counts[0], mABLen * sizeof(uint32));
	memcpy(mOldAB01Counts[1], mAB01Counts[1], mABLen * sizeof(uint32));
	memcpy(mAB01Counts[0], mZeroedABCounts, mABLen * sizeof(uint32));
	memcpy(mAB01Counts[1], mZeroedABCounts, mABLen * sizeof(uint32));

	stats.setCountSum = 0;
	uint32 curNumAttrs = 0;

	if(mZapped == NULL) {
		// Added or stable

		if(mAdded != NULL) {
			// Added
			mAdded->GetAttributeDefinition()->GetValuesIn(mAttributes);
			curNumAttrs = mAdded->GetLength();
			uint32 numInsts = mAdded->GetNumInstantiations();

			if(numInsts > mLenInstUseful) {
				delete[] mInstUseful;
				mLenInstUseful = numInsts;
				mInstUseful = new bool[mLenInstUseful];
			}
			if(mInstOrder == LEOrderLogLHPerSizeDescForAll) {
				// welke insts zijn useful?
				LowEntropySetInstantiation *inst;
				LowEntropySetInstantiation **insts = mAdded->GetInstantiations();
				for(uint32 j=0; j<numInsts; j++) {
					inst = insts[j];
					double lh = inst->GetLikelihood();
					double thisllhsz = (lh > 0 ? log(inst->GetLikelihood()) : 0) / inst->GetLowEntropySet()->GetLength();

					inst->GetItemSet()->GetValuesIn(mAttrValues);
					uint32 curInstOne = 0;
					uint32 idx = 0;

					double ablh = 1;
					for(uint32 k=0; k<curNumAttrs; k++) {
						idx = (mAttrValues[curInstOne] != mAttributes[k]) ? 0 : 1;
						if(idx == 1)
							curInstOne++;
						ablh *= mABLH[idx][k];
					}
					if((log(ablh)/ inst->GetLowEntropySet()->GetLength()) < thisllhsz) {
						mInstUseful[j] = true;
					} else {
						mInstUseful[j] = false;
					}
				}
			} else {
				for(uint32 j=0; j<numInsts; j++) {
					mInstUseful[j] = true;
				}
			}
		} 

		// for each database row
		for(uint32 i=0; i<mNumDBRows; i++) {
			mCurCover[i] = mOptCover[i];
			mCurCoverLen[i] = mOptCoverLen[i];
			mCurCoverLH[i] = mOptCoverLH[i];

			// check whether anything is added
			if(mAdded != NULL) {
				// 1. Reset the masks
				mOnesCSet->ResetMask();
				mCSets[i]->ResetMask();

				// 2. calculate which instantiation of mAdded could be used in this transaction
				LowEntropySetInstantiation *inst;
				uint32 numInsts = mAdded->GetNumInstantiations();
				LowEntropySetInstantiation **insts = mAdded->GetInstantiations();
				uint32 instIdx=0;
				for(; instIdx<numInsts; instIdx++) {
					if(mCSets[i]->CanCoverLE(mAdded->GetAttributeDefinition(), insts[instIdx]->GetItemSet()->GetLength(), insts[instIdx]->GetItemSet())) {
						inst = insts[instIdx];
						break;
					}
				}
				mAddedInst[i] = inst;

				// 3. Calculate mAddedInsertionPos[i], the position in the ordered array of mAddedInst for transaction i
				bool useful = mInstUseful[instIdx];
				mAddedInsertionPos[i] = UINT32_MAX_VALUE;
				if(useful) {
					uint32 addInsPos = 0;
					for(; addInsPos<mNumActiveRowInsts[i]; addInsPos++) {
						if(	(log(mRowInsts[i][addInsPos]->GetLikelihood()) / mRowInsts[i][addInsPos]->GetLowEntropySet()->GetLength())
							< 
							(log(mAddedInst[i]->GetLikelihood()) / mAddedInst[i]->GetLowEntropySet()->GetLength()) ) 
							break;
					} 
					mAddedInsertionPos[i] = addInsPos;

					// 4. Calculate mNewCover, the optimal cover when using mAdded
					mOnesCSet->Cover(mAdded->GetAttributeDefinition()->GetLength(), mAdded->GetAttributeDefinition());
					mCSets[i]->Cover(mAddedInst[i]->GetItemSet()->GetLength(), mAddedInst[i]->GetItemSet());
					mNewCover[i][0] = mAddedInst[i];
					mNewCoverLen[i] = 1;
					mNewCoverLH[i] = mAddedInst[i]->GetLikelihood();
					mBestFoundThusfarLH = mOptCoverLH[i];
					CalcOptimalCoverWithAdded(i, 0, mNewCoverLen[i], mNewCoverLH[i], &(mNewCover[i]));

					// 5. Compare mNewCoverLH with mOptCoverLH
					if(mNewCoverLH[i] > mOptCoverLH[i]) {
						// deze is beter, so let's change mCurCover
						mCurCoverLH[i] = mNewCoverLH[i];
						mCurCoverLen[i] = mNewCoverLen[i];
						mCurCover[i] = mNewCover[i];
					}
				} 
			}

			mOnesCSet->ResetMask();
			mCSets[i]->ResetMask();

			mCurCoverLH[i] = 1;			
			// 4. Bereken cover, of eigenlijk de counts, ahv mCurCover (die dus of opt is, of net nieuw)
			for(uint32 j=0; j<mCurCoverLen[i]; j++) {
				LowEntropySet *les = mCurCover[i][j]->GetLowEntropySet();
				ItemSet *attrs = les->GetAttributeDefinition();
				ItemSet *inst = mCurCover[i][j]->GetItemSet();
				uint32 numAttrs = attrs->GetLength();
				mOnesCSet->Cover(numAttrs, attrs);
				mCSets[i]->Cover(inst->GetLength(), inst);
				les->AddOneToCount();
				inst->AddOneToUsageCount();
				stats.setCountSum++;
				mBitmapCounts[numAttrs][mCurCover[i][j]->GetInstIdx()]++;
				mBitmapCountSums[numAttrs]++;
				mCurCoverLH[i] *= mCurCover[i][j]->GetLikelihood();
			}

			// de rest met alphabetdingetjes doen
			mOnesCSet->GetUncoveredAttributesIn(mAttributes);
			mCSets[i]->GetUncoveredAttributesIn(mAttrValues);
			uint32 nuc = mOnesCSet->GetNumUncovered();
			uint32 nuo = mCSets[i]->GetNumUncovered();			

			stats.setCountSum += nuc;
			mBitmapCountSums[1] = nuc;

			// first default to zeroes
			for(uint32 j=0; j<nuc; j++) {
				uint16 curAttr = mAttributes[j];
				mBitmapCounts[1][0]++;
				mAB01Counts[0][curAttr] = mAB01Counts[0][curAttr] + 1;
				mCurCoverLH[i] *= mABLH[0][curAttr];
				mABCounts[curAttr]++;
			}
			// now do the ones
			for(uint32 j=0; j<nuo; j++) {
				uint16 curAttr = mAttrValues[j];
				mBitmapCounts[1][1]++;
				mBitmapCounts[1][0]--;
				mAB01Counts[0][curAttr]--;
				mAB01Counts[1][curAttr]++;
				mCurCoverLH[i] /= mABLH[0][curAttr];
				mCurCoverLH[i] *= mABLH[1][curAttr];
			}
		}

	} else {
		// for each database row
		for(uint32 i=0; i<mNumDBRows; i++) {
			mCurCover[i] = mOptCover[i];		// whatever we did before, we start optimally here
			mCurCoverLen[i] = mOptCoverLen[i];
			mCurCoverLH[i] = mOptCoverLH[i];

			mOnesCSet->ResetMask();
			mCSets[i]->ResetMask();

			mZappedInsertionPos[i] = UINT32_MAX_VALUE;

			// 1. first check whether this one was used at all, as prolly we only need to consider a few covers
			bool zapUsed = false;
			for(uint32 k=0; k<mOptCoverLen[i]; k++) {
				if(mOptCover[i][k]->GetLowEntropySet() == mZapped) {
					zapUsed = true;
					break;
				}
			}

			// 2. figure out where in the sorted CT it sits, for later zappin' 
			for(uint32 k=0; k<mNumActiveRowInsts[i]; k++) {
				if(mRowInsts[i][k]->GetLowEntropySet() == mZapped) {
					mZappedInsertionPos[i] = k;
					break;
				}
			}

			if(zapUsed) {
				// 3. Calculate mNewCover, explicitly without using mZapped
				mNewCoverLen[i] = 0;
				mNewCoverLH[i] = 1;
				CalcOptimalCoverWithoutZapped(i, 0, mNewCoverLen[i], mNewCoverLH[i], &(mNewCover[i]));

				// 5. We should use this new cover 
				mCurCoverLH[i] = mNewCoverLH[i];
				mCurCoverLen[i] = mNewCoverLen[i];
				mCurCover[i] = mNewCover[i];

				mOnesCSet->ResetMask();
				mCSets[i]->ResetMask();
			}

			// I dont fully trust this one, recalc it for the sake of mankind
			double mCCLH = 1;

			// 4. Bereken cover, of eigenlijk de counts, ahv mCurCover (die dus of opt is, of net nieuw)
			for(uint32 j=0; j<mCurCoverLen[i]; j++) {
				LowEntropySet *les = mCurCover[i][j]->GetLowEntropySet();
				ItemSet *attrs = les->GetAttributeDefinition();
				ItemSet *inst = mCurCover[i][j]->GetItemSet();
				uint32 numAttrs = attrs->GetLength();
				mOnesCSet->Cover(numAttrs, attrs);
				mCSets[i]->Cover(inst->GetLength(), inst);
				les->AddOneToCount();
				inst->AddOneToUsageCount();
				stats.setCountSum++;
				mBitmapCounts[numAttrs][mCurCover[i][j]->GetInstIdx()]++;
				mBitmapCountSums[numAttrs]++;
				mCCLH *= mCurCover[i][j]->GetLikelihood();
			}

			// de rest met alphabetdingetjes doen
			mOnesCSet->GetUncoveredAttributesIn(mAttributes);
			mCSets[i]->GetUncoveredAttributesIn(mAttrValues);
			uint32 nuc = mOnesCSet->GetNumUncovered();
			uint32 nuo = mCSets[i]->GetNumUncovered();			

			stats.setCountSum += nuc;
			mBitmapCountSums[1] = nuc;

			// first default to zeroes
			for(uint32 j=0; j<nuc; j++) {
				uint16 curAttr = mAttributes[j];
				mBitmapCounts[1][0]++;
				mAB01Counts[0][curAttr] = mAB01Counts[0][curAttr] + 1;
				mCCLH *= mABLH[0][curAttr];
				mABCounts[curAttr]++;
			}
			// now do the ones
			for(uint32 j=0; j<nuo; j++) {
				uint16 curAttr = mAttrValues[j];
				mBitmapCounts[1][1]++;
				mBitmapCounts[1][0]--;
				mAB01Counts[0][curAttr]--;
				mAB01Counts[1][curAttr]++;
				mCCLH /= mABLH[0][curAttr];
				mCCLH *= mABLH[1][curAttr];
			}

			if(mCCLH != mCurCoverLH[i])
				uint32 la = 10;
		}
	}

	CalcTotalSize(stats);
}
double LECFOptCodeTable::CalcTotalSize(LECoverStats &stats) {
	leslist::iterator cit,cend;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.dbSize = 0;
	stats.bmSize = 0;
	stats.ct1Size = 0;
	stats.ct2Size = 0;
	stats.size = 0;
	stats.likelihood = 0;
	for(uint32 i=0; i<mNumDBRows; i++)
		stats.likelihood += log(mCurCoverLH[i]);

	// lengte LE-set codes berekenen voor dbSize en ct1Size, vertaling naar stdEnc voor ct1Size
	double codeLen = 0;
	cend = mLESList->GetList()->end();
	for (cit = mLESList->GetList()->begin(); cit != cend; ++cit) {
		uint32 curcnt = ((LowEntropySet*)(*cit))->GetCount();
		if(curcnt == 0) continue;
		if(*cit == mZapped) 
			continue;
		stats.numSetsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.setCountSum);
		stats.dbSize += curcnt * codeLen;
		stats.ct1Size += ((((LowEntropySet*)(*cit))->GetLength()) * mABStdLen) + codeLen;
	}

	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		uint32 curcnt = mABCounts[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.setCountSum);
		stats.dbSize += curcnt * codeLen;
		stats.ct1Size += mABStdLen + codeLen;
	}

	if(mBitmapStrategy == LEBitmapStrategyConstantCT) {
		stats.bmSize = mNumDBRows * mABLen;
		stats.ct2Size = 0;
	} else if(mBitmapStrategy == LEBitmapStrategyOptimalCT) {
		stats.bmSize = 0;
		stats.ct2Size = 0;
		for(uint32 i=1; i<mNumBitmaps; i++) {
			if(mBitmapCountSums[i] > 0) {
				uint32 numInsts = 1 << i;
				for(uint32 j=0; j<numInsts; j++) {
					if(mBitmapCounts[i][j] > 0) {
						codeLen = CalcCodeLength(mBitmapCounts[i][j], stats.setCountSum);
						stats.bmSize += mBitmapCounts[i][j] * codeLen;
						stats.ct2Size += codeLen + i;
					}
				}
			}
		}
	} else if(mBitmapStrategy == LEBitmapStrategyFullCT) {
		stats.bmSize = 0;
		stats.ct2Size = 0;

		for(uint32 i=1; i<mNumBitmaps; i++) {
			uint32 numInsts = 1 << i;
			for(uint32 j=0; j<numInsts; j++) {
				codeLen = CalcCodeLength(mBitmapCounts[i][j] + 1, stats.setCountSum + mNumPosBitmaps);
				stats.bmSize += mBitmapCounts[i][j] * codeLen;
				stats.ct2Size += codeLen + i;
			}
		}
	}

	stats.size = stats.dbSize + stats.bmSize + stats.ct1Size + stats.ct2Size;
	return stats.size;
}

void LECFOptCodeTable::AddOneToEachCount() {
	leslist::iterator cit,cend;

	cend = mLESList->GetList()->end();
	for(cit = mLESList->GetList()->begin(); cit != cend; ++cit)
		((LowEntropySet*)(*cit))->AddOneToCount();
	mCurStats.setCountSum += mCurNumSets;

	// don't forget the alphabet!
	for(uint32 i=0; i<mABLen; i++)
		mABCounts[i]++;
	mCurStats.setCountSum += mABLen;
}
void LECFOptCodeTable::SetAlphabetCount(uint32 item, uint32 count) {
	mABCounts[item] = count;
}
uint32 LECFOptCodeTable::GetAlphabetCount(uint32 item) {
	return mABCounts[item];
}

void LECFOptCodeTable::WriteToDisk(const string &filename) {
	FILE *file = fopen(filename.c_str(), "w");
	if(!file) {
		printf("Cannot open LECodeTable file for writing!\nFilename: %s\n", filename.c_str());
		return;
	}
	alphabet *alph = mDB->GetAlphabet();
	uint32 alphLen = (uint32) alph->size();
	leslist::iterator cit,cend=mLESList->GetList()->end();
	bool hasBins = mDB->HasBinSizes();

	// Determine max set length
	uint32 maxSetLen = 1;
	for (cit = mLESList->GetList()->begin(); cit != cend; ++cit)
		if((*cit)->GetLength() > maxSetLen)
			maxSetLen = (*cit)->GetLength();

	// Header
	fprintf(file, "ficlect-1.0\nmi: nS:%d aL:%d mL:%d hB:%d\n", mCurNumSets, alphLen, maxSetLen, hasBins?1:0);

	// Sets
	for (cit = mLESList->GetList()->begin(); cit != cend; ++cit)
		fprintf(file, "%s \n", ((LowEntropySet*)(*cit))->ToCodeTableString(hasBins).c_str());

	// Alphabet
	for(uint32 i=0; i<mABLen; i++)
		fprintf(file, "%d (%d : %d %d)\n", i, mABCounts[i], mAB01Counts[0][i], mAB01Counts[1][i]);

	fclose(file);
}
void LECFOptCodeTable::WriteCoverToDisk(const string &filename) {
	FILE *file = fopen(filename.c_str(), "w");
	if(!file) {
		printf("Cannot open Cover-file for writing!\nFilename: %s\n", filename.c_str());
		return;
	}
	alphabet *alph = mDB->GetAlphabet();
	uint32 alphLen = (uint32) alph->size();
	leslist::iterator cit,cend=mLESList->GetList()->end();
	bool hasBins = mDB->HasBinSizes();

	// Determine max set length
	uint32 maxSetLen = 1;
	for (cit = mLESList->GetList()->begin(); cit != cend; ++cit)
		if((*cit)->GetLength() > maxSetLen)
			maxSetLen = (*cit)->GetLength();

	// Header
	fprintf(file, "ficlecv-1.0\nmi: nS:%d aL:%d mL:%d hB:%d\n", mCurNumSets, alphLen, maxSetLen, hasBins?1:0);

	// cover
	// for each database row
	for(uint32 i=0; i<mNumDBRows; i++) {
		mOnesCSet->ResetMask();
		mCSets[i]->ResetMask();

		// mBakRowInsts is up-to-date

		for(uint32 k=0; k<mNumActiveRowInsts[i]; k++) {
			LowEntropySet *les = mRowInsts[i][k]->GetLowEntropySet();
			ItemSet *attrs = les->GetAttributeDefinition();

			// De juiste LE-set kiezen
			if(mOnesCSet->Cover(attrs->GetLength(), attrs)) {
				fprintf(file, "{%s}", attrs->ToString(false,false).c_str());
			}
		}

		// de rest met alphabetdingetjes doen
		mOnesCSet->GetUncoveredAttributesIn(mAttrValues);
		uint32 nuc = mOnesCSet->GetNumUncovered();
		for(uint32 j=0; j<nuc; j++) {
			fprintf(file, "{%d}", mAttrValues[j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}


leslist* LECFOptCodeTable::GetPostPruneList() {
	leslist *pruneList = new leslist();
	UpdatePostPruneList(pruneList, pruneList->begin());
	return pruneList;
}

void LECFOptCodeTable::UpdatePostPruneList(leslist *pruneList, const leslist::iterator &after) {
	SortPostPruneList(pruneList, after);
	mLESList->BuildPostPruneList(pruneList, after);
}
void LECFOptCodeTable::SortPostPruneList(leslist *pruneList, const leslist::iterator &after) {
	leslist::iterator start, a, b, c, end;
	LowEntropySet *lesB, *lesC;

	start = after;
	if(after != pruneList->end())
		++start;

	a = start;
	end = pruneList->end();
	if(a == end || ++a == end) // check whether at least 2 items left, otherwise sorting is quite useless
		return;

	--end;
	bool changed = false;
	for(a=start; a!=end; ++a) {
		changed = false;
		for(c=b=a,++c, lesB=(LowEntropySet*)(*b); b!=end; b=c, lesB=lesC, ++c) {
			lesC = (LowEntropySet*)(*c);
			if(lesB->GetCount() > lesC->GetCount()) {
				changed = true;
				*b = lesC;
				*c = lesC = lesB;
			}
		}
		if(!changed)
			break;
	}
}

#endif // BLOCK_LESS
