#ifdef BROKEN

#include "../../global.h"

#include <stdlib.h>

#include "../CodeTable.h"
#include "../../db/Database.h"
#include "../../itemstructs/ItemSet.h"
#include "../../itemstructs/CoverSet.h"
#include "../../isc/ItemSetCollection.h"
#include "../CTISList.h"

#include "CFCodeTable.h"
#include "CFMCodeTable.h"

//#define PRUNE_DEBUG

CFMCodeTable::CFMCodeTable() : CFCodeTable() {
	mCDB = NULL;
}

CFMCodeTable::~CFMCodeTable() {
	for(uint32 i=0; i<mNumDBRows; i++)
		delete mCDB[i];
	delete[] mCDB;	
}

void CFMCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CFCodeTable::UseThisStuff(db,type,ctinit,maxCTElemLength,toMinSup);
	delete mCoverSet;
	mCoverSet = NULL;
	
	// en nu gaan we hier een eigen db structuur maken...
	mCDB = new CoverSet *[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++)
		mCDB[i] = CoverSet::Create(type, mABLen, mDB->GetItemSet(i));
}

void CFMCodeTable::CoverDB(CoverStats &stats) {
	CoverStats bakstats = stats;
	stats.countSum = mABUnusedCount;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	// full cover db
	islist::iterator cit,cend;
	CoverSet *cset;
	uint32 mj = (mAdded == NULL) ? 1 : mAdded->GetLength()-1;

	// init counts
	for(uint32 j=mMaxIdx; j>=mj; --j)
		if(mCT[j] != NULL)
			mCT[j]->ResetCounts();

	for(uint32 i=0; i<mNumDBRows; i++) {
		cset = mCDB[i];
		cset->InitSet(mDB->GetItemSet(i));

		for(uint32 j=mMaxIdx; j>=mj; --j) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					if(cset->Cover(j+1,*cit)) {
						((ItemSet*)(*cit))->AddOneToCount();
						stats.countSum++;
					}
				}
			}
		}
	}
	if(mAdded!=NULL && mAdded->Unused()) {
		mRollback = mj;
		mRollbackAlph = false;
		stats = bakstats;
		return;
	}
	mRollback = 1;
	mRollbackAlph = true;
	for(uint32 j=mj-1; j>=1; --j)
		if(mCT[j] != NULL)
			mCT[j]->ResetCounts();
	// M heeft count > 0, dus we moeten de boel opnieuw uitrekenen...
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
	// Null Alphabet?
	for(uint32 i=0; i<mABUsedLen; i++)
		mAlphabet[mABUsed[i]] = 0;
	for(uint32 i=0; i<mNumDBRows; i++) {
		for(uint32 j=mj-1; j>=1; --j) {	// m tenslotte als laatste element in rij j toegevoegd..
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					if(mCDB[i]->Cover(j+1,*cit)) {
						((ItemSet*)(*cit))->AddOneToCount();
						stats.countSum++;
					}
				}
			}
		}
		// alphabet items
		stats.countSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet);
	}
	stats.dbSize = 0;
	stats.ctSize = 0;

	double codeLen = 0;
	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				uint32 curcnt = ((ItemSet*)(*cit))->GetCount();
				if(curcnt == 0) continue;
				stats.numSetsUsed++;
				codeLen = CalcCodeLength(curcnt, stats.countSum);
				stats.dbSize += curcnt * codeLen;
				stats.ctSize += ((ItemSet*)(*cit))->GetStandardLength();
				stats.ctSize += codeLen;
			}
		}
	}
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		uint32 curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.countSum);
		stats.dbSize += curcnt * codeLen;
		stats.ctSize += mStdLengths[i];
		stats.ctSize += codeLen;
	}

	stats.size = stats.dbSize + stats.ctSize;
}

void CFMCodeTable::RollbackCounts() {
	for(uint32 j=mMaxIdx; j>=mRollback; --j)
		if(mCT[j] != NULL)
			mCT[j]->RollbackCounts();
	if(mRollbackAlph)
		memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
}

#endif // BROKEN
