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

#include "LESOCodeTable.h"

LESOCodeTable::LESOCodeTable() : LECodeTable() {
	mLESList = NULL;

	mNumDBRows = 0;
	mCurNumSets = 0;
	mPrevNumSets = 0;

	mABCounts = NULL;
	mZeroedABCounts = NULL;
	mOldABCounts = NULL;

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
	//mNumActiveRowInsts = NULL;
	mLenRowInsts = 0;
	mTmpRowInsts = NULL;

	mNumBitmaps = 0;
	mNumPosBitmaps = 0;
	mBitmapCounts = NULL;
	mBitmapCountSums = NULL;
}

LESOCodeTable::~LESOCodeTable() {
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
	for(uint32 i=0; i<mNumBitmaps; i++)
		delete[] mBitmapCounts[i];
	delete[] mBitmapCounts;
	delete[] mBitmapCountSums;

	delete[] mRowInsts;
	//delete[] mNumActiveRowInsts;
	delete[] mTmpRowInsts;
	delete[] mAddedInsertionPos;
	delete[] mZappedInsertionPos;
	delete[] mAddedInst;
	//delete[] mInstUseful;
}

void LESOCodeTable::UseThisStuff(Database *db, LEBitmapStrategy bs, uint32 maxSetLen) {
	LECodeTable::UseThisStuff(db, bs, maxSetLen);

	// Alphabet init'en
	alphabet *a = db->GetAlphabet();
	mABLen = (uint32) a->size();
	mABLH = new float*[2];
	mABLH[0] = new float[mABLen];
	mABLH[1] = new float[mABLen];

	// ab counts
	mABCounts = new uint32[mABLen];
	mZeroedABCounts = new uint32[mABLen];
	mOldABCounts= new uint32[mABLen];

	// dump-arrays
	mAttrValues = new uint16[mABLen];
	mAttributes = new uint16[mABLen];

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
		mABCounts[cnt] = mNumDBRows;
		mZeroedABCounts[cnt] = 0;

		mBitmapCounts[1][0] += mNumDBRows - acnt;
		mBitmapCounts[1][1] += acnt;

		cnt++;
	}
	setCountSum = mABLen * mNumDBRows;
	mCurStats.setCountSum = setCountSum;
	mBitmapCountSums[1] = setCountSum;

	// alphabet counts backup
	memcpy(mOldABCounts, mABCounts, mABLen * sizeof(uint32));

	// encoding sets containter init
	if(mLESList == NULL) {
		mLESList = new LESList();
		mCurNumSets = 0;
		mPrevNumSets = 0;
	}

	// ones-coversets
	mRowInsts = new LowEntropySetInstantiation**[mNumDBRows];
	mTmpRowInsts = new LowEntropySetInstantiation*[mNumDBRows];
	mAddedInst = new LowEntropySetInstantiation*[mNumDBRows];
	//mNumActiveRowInsts ...
	mAddedInsertionPos = new uint32[mNumDBRows];
	mZappedInsertionPos = new uint32[mNumDBRows];
	mLenRowInsts = 50;
	mCSets = new CoverSet*[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCSets[i] = CoverSet::Create(mDB->GetDataType(), mABLen, mDB->GetRow(i));
		mRowInsts[i] = new LowEntropySetInstantiation*[mLenRowInsts];
	}

	mOnesISet = ItemSet::CreateEmpty(mDB->GetDataType(), mABLen);
	mOnesISet->FillHerUp(mABLen);
	mOnesCSet = CoverSet::Create(mDB->GetDataType(), mABLen, mOnesISet);

	CalcTotalSize(mCurStats);
	mPrevStats = mCurStats;

}
void LESOCodeTable::SetDatabase(Database *db) {
	LECodeTable::SetDatabase(db);
	mNumDBRows = mDB->GetNumRows();
}

void LESOCodeTable::Add(LowEntropySet *les, uint64 candidateId) {
	uint32 len = les->GetLength();
	mLESList->GetList()->push_back(les);
	++mCurNumSets;

	mAdded = les;
	mAddedCandidateId = candidateId;
}

void LESOCodeTable::Del(LowEntropySet *les, bool zap) {
	leslist *lst = mLESList->GetList();

	mLastDel.les = les;
	if(lst->back() == les) {
		lst->pop_back();
		mLastDel.before = NULL;
	} else {
		leslist::iterator iter, lend = lst->end();
		for(iter=lst->begin(); iter!=lend; ++iter)
			if((LowEntropySet*)(*iter) == les)
				break;
		lend = iter;
		++lend;
		mLastDel.before = (LowEntropySet*)(*lend);
		lst->erase(iter);
	}

	--mCurNumSets;
	mCurStats.setCountSum -= les->GetCount();

	if(zap)
		delete les;
}

void LESOCodeTable::CommitAdd(bool updateLog) {
	// only commits/rollbacks add
	mAdded = NULL;
	mPrevStats = mCurStats;

	if(mLenRowInsts < mCurNumSets) {
		for(uint32 i=0; i<mNumDBRows; i++) {
			LowEntropySetInstantiation** tmp = new LowEntropySetInstantiation*[mLenRowInsts * 2];
			memcpy(tmp, mRowInsts[i], mLenRowInsts * sizeof(LowEntropySetInstantiation*));
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
		uint32 numCopyBytes = (mPrevNumSets - instPos) * sizeof(LowEntropySetInstantiation*);
		memcpy(mTmpRowInsts, mRowInsts[i] + instPos, numCopyBytes);
		memcpy(mRowInsts[i] + (instPos+1), mTmpRowInsts, numCopyBytes);

		// voeg mAddedInst[i] op mAddedInsertionPos in
		mRowInsts[i][instPos] = mAddedInst[i];
	}
	mPrevNumSets = mCurNumSets;
}

/*
void LECFCodeTable::CommitLastDel(bool zap, bool updateLog) {
	if(zap)
		delete mLastDel.les;
	mLastDel.les = NULL;
	mCurStats = mPrunedStats;
}
*/
/*
void LECFCodeTable::CommitAllDels(bool updateLog) {
	ledellist::iterator iter, itend = mDeleted.end();
	for(iter=mDeleted.begin(); iter!=itend; ++iter)
		delete ((LEDelInfo)*iter).les;
	mDeleted.clear();
	mLastDel.les = NULL;
}
*/
void LESOCodeTable::RollbackAdd() {
	// Rolls back the added LowEntropySet
	// Restores DB & CT sizes
	if(mAdded != NULL) {

		Del(mAdded, true);
		mAdded = NULL;
		mLastDel.les = NULL;
		mCurStats = mPrevStats;
	}
}
void LESOCodeTable::CoverDB(LECoverStats &stats) {
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

	stats.setCountSum = 0;
	uint32 curNumAttrs = 0;

	// for each database row
	for(uint32 i=0; i<mNumDBRows; i++) {
		mOnesCSet->ResetMask();
		mCSets[i]->ResetMask();

		// check whether anything is added
		uint32 continueCoverAt = 0;
		if(mAdded != NULL) {
			// de juiste inst kiezen voor deze row

			LowEntropySetInstantiation *inst;
			uint32 numInsts = mAdded->GetNumInstantiations();
			LowEntropySetInstantiation **insts = mAdded->GetInstantiations();
			for(uint32 j=0; j<numInsts; j++) {	// skip 0-inst
				if(mCSets[i]->CanCoverLE(mAdded->GetAttributeDefinition(), insts[j]->GetItemSet()->GetLength(), insts[j]->GetItemSet())) {
					inst = insts[j];
					break;
				}
			}
			mAddedInst[i] = inst;

			// zoeken in mBakRowInsts[i] naar plek [j] van inst
			uint32 j = 0;
			for(; j<mPrevNumSets; j++) {
				// LE-set Length >, LE-inst Likelihood >
				if(mRowInsts[i][j]->GetLowEntropySet()->GetLength() < inst->GetLowEntropySet()->GetLength() ||
					(mRowInsts[i][j]->GetLowEntropySet()->GetLength() == inst->GetLowEntropySet()->GetLength() &&
					mRowInsts[i][j]->GetLikelihood() < inst->GetLikelihood())) {
						break;
				}
				/*
				// LE-inst Length >, LE-inst Likelihood >
				if(mBakRowInsts[i][j]->GetItemSet()->GetLength() < inst->GetItemSet()->GetLength() ||
					(mBakRowInsts[i][j]->GetItemSet()->GetLength() == inst->GetItemSet()->GetLength() &&
					mBakRowInsts[i][j]->GetLikelihood() < inst->GetLikelihood())) {
						break;
				} 
				*/
				/*
				// LE-inst loglh / size >
				if(	(log(mRowInsts[i][j]->GetLikelihood()) / mRowInsts[i][j]->GetLowEntropySet()->GetLength())
					< 
					(log(inst->GetLikelihood()) / inst->GetLowEntropySet()->GetLength()) ){
						break;
				} 
				*/
			} 
			mAddedInsertionPos[i] = j;
			uint32 addInsPos = j;

			// first cover till mAddedInsertionPos[i]
			for(uint32 k=0; k<addInsPos; k++) {
				LowEntropySet *les = mRowInsts[i][k]->GetLowEntropySet();
				ItemSet *attrs = les->GetAttributeDefinition();
				if(mRowInsts[i][k]->GetItemSet()->GetPrevUsageCount() == 0)
					continue;

				// De juiste LE-set kiezen
				if(mOnesCSet->Cover(attrs->GetLength(), attrs)) {
					les->AddOneToCount();
					mRowInsts[i][k]->GetItemSet()->AddOneToUsageCount();
					stats.setCountSum++;
					mBitmapCounts[les->GetLength()][mRowInsts[i][k]->GetInstIdx()]++;
					mBitmapCountSums[les->GetLength()]++;
				}
			}
			// then cover using mAddedInst
			if(mOnesCSet->Cover(mAdded->GetAttributeDefinition()->GetLength(), mAdded->GetAttributeDefinition())) {
				mAdded->AddOneToCount();
				mAddedInst[i]->GetItemSet()->AddOneToUsageCount();
				stats.setCountSum++;
				mBitmapCounts[mAdded->GetLength()][mAddedInst[i]->GetInstIdx()]++;
				mBitmapCountSums[mAdded->GetLength()]++;
			}
			// then continue from mAddedInsertionPos[i]
			continueCoverAt = mAddedInsertionPos[i];
		}
		// else { continueCoverAt = 0; } // that's the default value

		// mRowInsts[i] now has all possibly fitting insts, in order, start at continueCoverAt

		// cover met mRowInsts
		for(uint32 k=continueCoverAt; k<mPrevNumSets; k++) {
			LowEntropySet *les = mRowInsts[i][k]->GetLowEntropySet();
			ItemSet *attrs = les->GetAttributeDefinition();

			// De juiste LE-set kiezen
			if(mOnesCSet->Cover(attrs->GetLength(), attrs)) {
				les->AddOneToCount();
				mRowInsts[i][k]->GetItemSet()->AddOneToUsageCount();
				stats.setCountSum++;
				mBitmapCounts[les->GetLength()][mRowInsts[i][k]->GetInstIdx()]++;
				mBitmapCountSums[les->GetLength()]++;
			}
		}

		// de rest met alphabetdingetjes doen
		mOnesCSet->GetUncoveredAttributesIn(mAttributes);
		mCSets[i]->GetUncoveredAttributesIn(mAttrValues);
		uint32 nuc = mOnesCSet->GetNumUncovered();
		uint32 onIdx = 0;

		stats.setCountSum += nuc;
		mBitmapCountSums[1] = nuc;

		for(uint32 j=0; j<nuc; j++) {
			if(mAttributes[j] == mAttrValues[onIdx]) {
				mBitmapCounts[1][1]++;
				onIdx++;
			} else {
				mBitmapCounts[1][0]++;
			}
			mABCounts[mAttributes[j]]++;
		}
	}

	CalcTotalSize(stats);
}
double LESOCodeTable::CalcTotalSize(LECoverStats &stats) {
	leslist::iterator cit,cend;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.dbSize = 0;
	stats.bmSize = 0;
	stats.ct1Size = 0;
	stats.ct2Size = 0;
	stats.size = 0;

	// lengte LE-set codes berekenen voor dbSize en ct1Size, vertaling naar stdEnc voor ct1Size
	double codeLen = 0;
	cend = mLESList->GetList()->end();
	for (cit = mLESList->GetList()->begin(); cit != cend; ++cit) {
		uint32 curcnt = ((LowEntropySet*)(*cit))->GetCount();
		if(curcnt == 0) continue;
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

void LESOCodeTable::AddOneToEachCount() {
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
void LESOCodeTable::SetAlphabetCount(uint32 item, uint32 count) {
	mABCounts[item] = count;
}
uint32 LESOCodeTable::GetAlphabetCount(uint32 item) {
	return mABCounts[item];
}

void LESOCodeTable::WriteToDisk(const string &filename) {
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
		fprintf(file, "%d (%d)\n", i, mABCounts[i]);

	fclose(file);
}
void LESOCodeTable::WriteCoverToDisk(const string &filename) {
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

		for(uint32 k=0; k<mPrevNumSets; k++) {
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

#endif // BLOCK_LESS
