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

#include "LECFCodeTable.h"

LECFCodeTable::LECFCodeTable(string algoname) : LECodeTable() {
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

	if(algoname.substr(GetConfigName().length()+1).compare("llhsz-all") == 0)
		mInstOrder = LEOrderLogLHPerSizeDescForAll;
	else if(algoname.substr(GetConfigName().length()+1).compare("llhsz-sets") == 0)
		mInstOrder = LEOrderLogLHPerSizeDescForSets;
	else if(algoname.substr(GetConfigName().length()+1).compare("lenlh") == 0)
		mInstOrder = LEOrderLengthDescLHDesc;
	else 
		mInstOrder = LEOrderLogLHPerSizeDescForAll;
}

LECFCodeTable::~LECFCodeTable() {
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
}

void LECFCodeTable::UseThisStuff(Database *db, LEBitmapStrategy bs, uint32 maxSetLen) {
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

	// ones-coversets
	mRowInsts = new LowEntropySetInstantiation**[mNumDBRows];
	mTmpRowInsts = new LowEntropySetInstantiation*[mNumDBRows];
	mAddedInst = new LowEntropySetInstantiation*[mNumDBRows];
	mNumActiveRowInsts = new uint32[mNumDBRows];
	mAddedInsertionPos = new uint32[mNumDBRows];
	mZappedInsertionPos = new uint32[mNumDBRows];
	mLenRowInsts = 50;
	mCSets = new CoverSet*[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCSets[i] = CoverSet::Create(mDB->GetDataType(), mABLen, mDB->GetRow(i));
		mRowInsts[i] = new LowEntropySetInstantiation*[mLenRowInsts];
		mNumActiveRowInsts[i] = 0;
	}

	mOnesISet = ItemSet::CreateEmpty(mDB->GetDataType(), mABLen);
	mOnesISet->FillHerUp(mABLen);
	mOnesCSet = CoverSet::Create(mDB->GetDataType(), mABLen, mOnesISet);

	//CalcTotalSize(mCurStats);
	mPrevStats = mCurStats;

	if(mInstOrder == LEOrderLogLHPerSizeDescForAll) {
		mInstCompFunc = CompareInstsLogLHPerSizeDesc;
	} else if(mInstOrder == LEOrderLogLHPerSizeDescForSets) {
		mInstCompFunc = CompareInstsLogLHPerSizeDesc;
	} else 
		mInstCompFunc = CompareInstsLengthDescLHDesc;

	CoverDB(mCurStats);
	mPrevStats = mCurStats;
}
bool LECFCodeTable::CompareInstsLogLHPerSizeDesc(LowEntropySetInstantiation *a, LowEntropySetInstantiation *b) {
	// LE-inst loglh / size >
	/*
	if(	(log(mRowInsts[i][j]->GetLikelihood()) / mRowInsts[i][j]->GetLowEntropySet()->GetLength())
		< 
		(log(inst->GetLikelihood()) / inst->GetLowEntropySet()->GetLength()) ) {
		
		break;
	} */
	if(	(log(a->GetLikelihood()) / a->GetLowEntropySet()->GetLength())
		< 
		(log(b->GetLikelihood()) / b->GetLowEntropySet()->GetLength()) ) 
			return true;
	return false;
}
bool LECFCodeTable::CompareInstsLengthDescLHDesc(LowEntropySetInstantiation *a, LowEntropySetInstantiation *b) {
	// LE-set Length >, LE-inst Likelihood >
	if(a->GetLowEntropySet()->GetLength() < b->GetLowEntropySet()->GetLength() ||
		(a->GetLowEntropySet()->GetLength() == b->GetLowEntropySet()->GetLength() &&
		a->GetLikelihood() < b->GetLikelihood())) {
			return true;
	}
	/*
	// LE-inst Length >, LE-inst Likelihood >
	if(mBakRowInsts[i][j]->GetItemSet()->GetLength() < inst->GetItemSet()->GetLength() ||
	(mBakRowInsts[i][j]->GetItemSet()->GetLength() == inst->GetItemSet()->GetLength() &&
	mBakRowInsts[i][j]->GetLikelihood() < inst->GetLikelihood())) {
	break;
	} 
	*/
	return false;
}
void LECFCodeTable::SetDatabase(Database *db) {
	LECodeTable::SetDatabase(db);
	mNumDBRows = mDB->GetNumRows();
}

void LECFCodeTable::Add(LowEntropySet *les, uint64 candidateId) {
	uint32 len = les->GetLength();
	mLESList->GetList()->push_back(les);
	++mCurNumSets;

	mAdded = les;
	mAddedCandidateId = candidateId;
}

void LECFCodeTable::Del(LowEntropySet *les, bool zap) {
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

void LECFCodeTable::CommitAdd(bool updateLog) {
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
	}
	mPrevNumSets = mCurNumSets;
}

void LECFCodeTable::CommitLastDel(bool zap, bool updateLog) {
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

void LECFCodeTable::RollbackAdd() {
	// Rolls back the added LowEntropySet
	// Restores DB & CT sizes
	if(mAdded != NULL) {

		Del(mAdded, true);
		mAdded = NULL;
		mCurStats = mPrevStats;
	}
}
void LECFCodeTable::RollbackLastDel() {
	if(mZapped != NULL)	{
		mZapped = NULL;
	}
}
void LECFCodeTable::RollbackCounts() {
	mLESList->RollbackCounts();
	memcpy(mABCounts, mOldABCounts, mABLen * sizeof(uint32));
	memcpy(mAB01Counts[0], mOldAB01Counts[0], mABLen * sizeof(uint32));
	memcpy(mAB01Counts[1], mOldAB01Counts[1], mABLen * sizeof(uint32));
}

void LECFCodeTable::CoverDB(LECoverStats &stats) {
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
	stats.likelihood = 0;
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
			mOnesCSet->ResetMask();
			mCSets[i]->ResetMask();

			double lh = 1;

			// check whether anything is added
			uint32 continueCoverAt = 0;
			if(mAdded != NULL) {
				// de juiste inst kiezen voor deze row

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

				// checken of deze inst wel nuttig is, tov alphabet codering
				bool useful = mInstUseful[instIdx];

				mAddedInsertionPos[i] = UINT32_MAX_VALUE;

				if(useful) {
					// zoeken in mBakRowInsts[i] naar plek [j] van inst
					uint32 addInsPos = 0;
					for(; addInsPos<mNumActiveRowInsts[i]; addInsPos++) {
						if(mInstCompFunc(mRowInsts[i][addInsPos], inst))
							break;
					} 
					mAddedInsertionPos[i] = addInsPos;

					// first cover till mAddedInsertionPos[i]
					for(uint32 k=0; k<addInsPos; k++) {
						LowEntropySet *les = mRowInsts[i][k]->GetLowEntropySet();
						ItemSet *attrs = les->GetAttributeDefinition();

						// De juiste LE-set kiezen
						if(mOnesCSet->Cover(attrs->GetLength(), attrs)) {
							lh *= mRowInsts[i][k]->GetLikelihood();
							mCSets[i]->Cover(mRowInsts[i][k]->GetItemSet()->GetLength(), mRowInsts[i][k]->GetItemSet());
							les->AddOneToCount();
							mRowInsts[i][k]->GetItemSet()->AddOneToUsageCount();
							stats.setCountSum++;
							mBitmapCounts[les->GetLength()][mRowInsts[i][k]->GetInstIdx()]++;
							mBitmapCountSums[les->GetLength()]++;
						}
					}
					// then cover using mAddedInst
					if(mOnesCSet->Cover(mAdded->GetAttributeDefinition()->GetLength(), mAdded->GetAttributeDefinition())) {
						mCSets[i]->Cover(mAddedInst[i]->GetItemSet()->GetLength(), mAddedInst[i]->GetItemSet());
						lh *= mAddedInst[i]->GetLikelihood();
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
			}
			// mRowInsts[i] now has all possibly fitting insts, in order, start at continueCoverAt

			// cover met mRowInsts
			for(uint32 k=continueCoverAt; k<mNumActiveRowInsts[i]; k++) {
				LowEntropySet *les = mRowInsts[i][k]->GetLowEntropySet();
				ItemSet *attrs = les->GetAttributeDefinition();

				// De juiste LE-set kiezen
				if(mOnesCSet->Cover(attrs->GetLength(), attrs)) {
					mCSets[i]->Cover(mRowInsts[i][k]->GetItemSet()->GetLength(), mRowInsts[i][k]->GetItemSet());
					lh *= mRowInsts[i][k]->GetLikelihood();
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
			uint32 nuo = mCSets[i]->GetNumUncovered();
			uint32 onIdx = 0;

			stats.setCountSum += nuc;
			mBitmapCountSums[1] += nuc;

			// first default to zeroes
			for(uint32 j=0; j<nuc; j++) {
				uint16 curAttr = mAttributes[j];
				mBitmapCounts[1][0]++;
				mAB01Counts[0][curAttr]++;
				if(mABLH[0][curAttr] > 0)
					lh *= mABLH[0][curAttr];
				mABCounts[curAttr]++;
			}
			// now do the ones
			for(uint32 j=0; j<nuo; j++) {
				uint16 curAttr = mAttrValues[j];
				mBitmapCounts[1][1]++;
				mBitmapCounts[1][0]--;
				mAB01Counts[0][curAttr]--;
				mAB01Counts[1][curAttr]++;
				if(mABLH[0][curAttr] > 0)
					lh /= mABLH[0][curAttr];
				lh *= mABLH[1][curAttr];
			}

			stats.likelihood += log(lh);
		}

	} else {
		// for each database row
		for(uint32 i=0; i<mNumDBRows; i++) {
			mOnesCSet->ResetMask();
			mCSets[i]->ResetMask();
			double lh = 1;

			mZappedInsertionPos[i] = UINT32_MAX_VALUE;

			// cover met mRowInsts
			uint32 curInst = 0;
			for(uint32 curInst=0; curInst<mNumActiveRowInsts[i]; curInst++) {
				LowEntropySet *les = mRowInsts[i][curInst]->GetLowEntropySet();
				if(les == mZapped) {
					mZappedInsertionPos[i] = curInst;
					continue;
				}
				ItemSet *attrs = les->GetAttributeDefinition();

				// De juiste LE-set kiezen
				if(mOnesCSet->Cover(attrs->GetLength(), attrs)) {
					mCSets[i]->Cover(mRowInsts[i][curInst]->GetItemSet()->GetLength(), mRowInsts[i][curInst]->GetItemSet());
					lh *= mRowInsts[i][curInst]->GetLikelihood();
					les->AddOneToCount();
					mRowInsts[i][curInst]->GetItemSet()->AddOneToUsageCount();
					stats.setCountSum++;
					mBitmapCounts[les->GetLength()][mRowInsts[i][curInst]->GetInstIdx()]++;
					mBitmapCountSums[les->GetLength()]++;
				}
			}

			// de rest met alphabetdingetjes doen
			mOnesCSet->GetUncoveredAttributesIn(mAttributes);
			mCSets[i]->GetUncoveredAttributesIn(mAttrValues);
			uint32 nuc = mOnesCSet->GetNumUncovered();
			uint32 nuo = mCSets[i]->GetNumUncovered();

			stats.setCountSum += nuc;
			mBitmapCountSums[1] += nuc;

			// first default to zeroes
			for(uint32 j=0; j<nuc; j++) {
				uint16 curAttr = mAttributes[j];
				mBitmapCounts[1][0]++;
				mAB01Counts[0][curAttr]++;
				if(mABLH[0][curAttr] > 0)
					lh *= mABLH[0][curAttr];
				mABCounts[curAttr]++;
			}
			// now do the ones
			for(uint32 j=0; j<nuo; j++) {
				uint16 curAttr = mAttrValues[j];
				mBitmapCounts[1][1]++;
				mBitmapCounts[1][0]--;
				mAB01Counts[0][curAttr]--;
				mAB01Counts[1][curAttr]++;
				if(mABLH[0][curAttr] > 0)
					lh /= mABLH[0][curAttr];
				lh *= mABLH[1][curAttr];
			}
			stats.likelihood += log(lh);
		}
	}

	CalcTotalSize(stats);
}
double LECFCodeTable::CalcTotalSize(LECoverStats &stats) {
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
	} else if(mBitmapStrategy == LEBitmapStrategyFullCTPerLength) {
		stats.bmSize = 0;
		stats.ct2Size = 0;

		for(uint32 i=1; i<mNumBitmaps; i++) {
			uint32 numInsts = 1 << i;
			for(uint32 j=0; j<numInsts; j++) {
				codeLen = CalcCodeLength(mBitmapCounts[i][j] + 1, mBitmapCountSums[i] + numInsts);
				stats.bmSize += mBitmapCounts[i][j] * codeLen;
				stats.ct2Size += codeLen + i;
			}
		}
	} else if(mBitmapStrategy == LEBitmapStrategyIndivInstEncoding) {
		stats.bmSize = 0;
		stats.ct2Size = 0;

		cend = mLESList->GetList()->end();
		for (cit = mLESList->GetList()->begin(); cit != cend; ++cit) {
			uint32 lecnt = ((LowEntropySet*)(*cit))->GetCount();
			if(lecnt == 0) continue;
			if(*cit == mZapped) 
				continue;
			uint32 numInsts = ((LowEntropySet*)(*cit))->GetNumInstantiations();
			LowEntropySetInstantiation **insts = ((LowEntropySet*)(*cit))->GetInstantiations();
			for(uint32 j=0; j<numInsts; j++) {
				codeLen = CalcCodeLength(insts[j]->GetItemSet()->GetUsageCount(), lecnt);
				stats.bmSize += insts[j]->GetItemSet()->GetUsageCount() * codeLen;
				stats.ct2Size += ((LowEntropySet*)(*cit))->GetLength() + codeLen;

			}
		}

		// Alphabet
		for(uint32 i=0; i<mABLen; i++) {
			uint32 curcnt = mAB01Counts[0][i];
			if(curcnt > 0) {
				codeLen = CalcCodeLength(curcnt, mABCounts[i]);
				stats.bmSize += curcnt * codeLen;
				stats.ct2Size += 1 + codeLen;
			}
			curcnt = mAB01Counts[1][i];
			if(curcnt > 0) {
				codeLen = CalcCodeLength(curcnt, mABCounts[i]);
				stats.bmSize += curcnt * codeLen;
				stats.ct2Size += 1 + codeLen;
			}
		}
	}

	stats.size = stats.dbSize + stats.bmSize + stats.ct1Size + stats.ct2Size;
	return stats.size;
}

void LECFCodeTable::AddOneToEachCount() {
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
void LECFCodeTable::SetAlphabetCount(uint32 item, uint32 count) {
	mABCounts[item] = count;
}
uint32 LECFCodeTable::GetAlphabetCount(uint32 item) {
	return mABCounts[item];
}

void LECFCodeTable::WriteToDisk(const string &filename) {
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
void LECFCodeTable::WriteCoverToDisk(const string &filename) {
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


leslist* LECFCodeTable::GetPostPruneList() {
	leslist *pruneList = new leslist();
	UpdatePostPruneList(pruneList, pruneList->begin());
	return pruneList;
}

void LECFCodeTable::UpdatePostPruneList(leslist *pruneList, const leslist::iterator &after) {
	SortPostPruneList(pruneList, after);
	mLESList->BuildPostPruneList(pruneList, after);
}
void LECFCodeTable::SortPostPruneList(leslist *pruneList, const leslist::iterator &after) {
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
