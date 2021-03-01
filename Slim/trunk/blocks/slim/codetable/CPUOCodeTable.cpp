#include "../../../global.h"

#include <cassert>
#include <stdlib.h>
#include <algorithm>

// -- qtils
#include <RandomUtils.h>
#include <ArrayUtils.h>


// -- bass
#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>
#include <isc/ItemTranslator.h>

#include "../../krimp/codetable/CTISList.h"

#include "../../krimp/codetable/coverpartial/CPAOCodeTable.h"
#include "CPUOCodeTable.h"

CPUOCodeTable::CPUOCodeTable() : CPOCodeTable() {
}

CPUOCodeTable::CPUOCodeTable(const string &name) : CPOCodeTable(name) {
}
CPUOCodeTable::~CPUOCodeTable() {
}

string CPUOCodeTable::GetShortName() {
	string s = mOverlap ? "-ov" : "";
	if(mOrder == CTRandomOrder)
		return "cpuor" + s;
	else if (mOrder == CTLengthOrder)
		return "cpuol" + s;
	else if (mOrder == CTLengthInverseOrder)
		return "cpuoli" + s;
	else if (mOrder == CTAreaOrder)
		return "cpuoa" + s;
	else if (mOrder == CTAreaInverseOrder)
		return "cpuoai" + s;
	else if (mOrder == CTStackInverseOrder) 
		return "cpuosi" + s;
	else // if (mOrder == CTStackOrder)
		return "cpuos" + s;
}


void CPUOCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CPOCodeTable::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);


	// Prepare Alphabet Usage Arrays
	mDB->EnableFastDBOccurences();
	mAlphabetSize = mDB->GetAlphabetSize();
	mAlphabetNumRows = mDB->GetAlphabetNumRows();
	mAlphabetUsages = new uint32*[mAlphabetSize];
	mAlphabetUsageAdd = new vector<uint32>*[mAlphabetSize];
	mAlphabetUsageZap = new vector<uint32>*[mAlphabetSize];
	for (uint32 i = 0; i < mAlphabetSize; ++i) {
		mAlphabetUsages[i] = new uint32[mAlphabetNumRows[i]];
		memcpy(mAlphabetUsages[i], mDB->GetAlphabetDbOccs()[i], mAlphabetNumRows[i] * sizeof(uint32));
		mAlphabetUsageAdd[i] = new vector<uint32>();
		mAlphabetUsageZap[i] = new vector<uint32>();
	}
	mOldAlphabetUsages = new uint32*[mAlphabetSize];
	for (uint32 i = 0; i < mAlphabetSize; ++i)
		mOldAlphabetUsages[i] = new uint32[mAlphabetNumRows[i]];
	mValues = new uint32[mAlphabetSize];
}



void CPUOCodeTable::RollbackCounts() {
	// Terugrollen van de counts moet -tot/met- de mRollback rij
	// aka, hier -tot- mRollback (die op list->end() of op mChangeIter ingesteld zal staan)
	for(islist::iterator cit = mCTList->begin(); cit != mRollback; ++cit)
		((ItemSet*)(*cit))->RollbackUsage();
	if(mRollbackAlph) {
		memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
		for(uint32 i = 0; i < mAlphabetSize; ++i) {
			//copy(mOldAlphabetUsage[i], mOldAlphabetUsage[i] + mAlphabet[i], mAlphabetUsage[i]);
			mAlphabetUsageAdd[i]->clear();
			mAlphabetUsageZap[i]->clear();
			memcpy(mAlphabetUsages[i], mOldAlphabetUsages[i], mAlphabet[i] * sizeof(uint32));
		}
		mRollbackAlph = false;
	}
}



void CPUOCodeTable::CoverNoBinsDB(CoverStats &stats) {
	CoverSet *cset;
	ItemSet *iset;
	uint32 *occur, numOcc;
	islist::iterator cit,cend,naChangeIter;
	naChangeIter = mChangeIter;
	++naChangeIter;

	if(mChangeType == ChangeAdd) {
		// alle is' -tot/met- mChangeIter
		cend = naChangeIter;
		for(cit=mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();

		mChangeItem->ResetUsage();
		mChangeItem->SetPrevUsageCount(1);													// dus na ResetCount prevCount != 0
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();

		mNumUseOccs = 0;

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();

			// up till, but not including m
			cend = mChangeIter;															// up till, but not including m
			for(cit = mCTList->begin(); cit != cend; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)
					continue;
				(cset->*mCoverFunc)(iset->GetLength(),iset);
			}
			if((cset->*mCanCoverFunc)(mChangeItem->GetLength(),mChangeItem)) {			// nu m nog even doen
				mUseOccs[mNumUseOccs++] = i;
				mChangeItem->SetUsed(i);
			}
		}
		if(mChangeItem!=NULL) {
			mChangeItem->SetPrevUsageCount(0);
			if(mNumUseOccs == 0) {
				mRollback = naChangeIter;	// ??? naChangeIter ivm zap?
				mRollbackAlph = false;
				return;
			}
		} 
		stats.usgCountSum += mNumUseOccs;
		mChangeItem->AddUsageCount(mNumUseOccs);
		// M heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = mCTList->end();
		mRollbackAlph = true;
		// vanaf na mChangeIter counts up backen
		cend = mCTList->end();
		for(cit = naChangeIter; cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		swap(mAlphabetUsages, mOldAlphabetUsages);

		uint32 i;
		uint32 cnt;
		for(uint32 r=0; r<mNumUseOccs; r++) {
			i=mUseOccs[r];
			cset = mCDB[i];

			cset->BackupMask();
			//	herstelcover (--) 
			// vanaf na mChangeIter counts herstellen
			for(cit = naChangeIter; cit != cend; ++cit)
				if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					ItemSet* is = ((ItemSet*)(*cit));
					is->SupOneFromUsageCount();
					is->SetUnused(i);
					stats.usgCountSum--;
				}
			cnt = cset->UncoverAlphabet(mABLen, mAlphabet, mValues);
			for (uint32 j = 0; j < cnt; ++j) {
				mAlphabetUsageZap[mValues[j]]->push_back(i);
			}
			stats.usgCountSum -= cnt;

			//  Totale cover tot/met m
			cset->RestoreMask();
			(cset->*mCoverFunc)(mChangeItem->GetLength(),mChangeItem);

			//	Cover vanaf naChangeIter
			cend = mCTList->end();
			for(cit = naChangeIter; cit != cend; ++cit)
				if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					ItemSet* is = ((ItemSet*)(*cit));
					is->AddOneToUsageCount();
					is->SetUsed(i);
					stats.usgCountSum++;
				}
			cnt = cset->CoverAlphabet(mABLen, mAlphabet, mValues);
			for (uint32 j = 0; j < cnt; ++j) {
				mAlphabetUsageAdd[mValues[j]]->push_back(i);
			}
			stats.usgCountSum += cnt;
		}

		// Counts en CountSum zijn bijgewerkt, Usages van Alphabet worden below geupdate, dan sizes uitrekenen ...

	} else if(mChangeType == ChangeDel) {
		uint32 st = 0;
		stats = mCurStats;
		
		mRollback = mCTList->end();
		mRollbackAlph = true;
		// vanaf naChangeIter de counts up backen
		cend = mCTList->end();
		for(cit = mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		swap(mAlphabetUsages, mOldAlphabetUsages);

		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...
		mChangeItem->SetAllUnused();
		uint32 ciLen = mChangeItem->GetLength();

		uint32 r,i;
		uint32 cnt;
		for(r=0; r<numOcc; r++) {
			i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();

			// tot/met mChangeIter om cset op te bouwen
			for(cit = mCTList->begin(); cit != mChangeIter; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)
					continue;
				(cset->*mCoverFunc)(iset->GetLength(),iset);
			}
			if((cset->*mCanCoverFunc)(ciLen,mChangeItem)) {
				cset->BackupMask();
				(cset->*mCoverFunc)(ciLen,mChangeItem);

				// :: Herstelcover (--) 
				//	-: Herstel staartje mChangeRow
				cend = mCTList->end();
				for(cit = naChangeIter; cit != cend; ++cit) {
					iset = *cit;
					if((cset->*mCoverFunc)(iset->GetLength(),iset)) {
						iset->SupOneFromUsageCount();
						iset->SetUnused(i);
						stats.usgCountSum--;
					}
				}
				//	-: Herstel alphabet counts
				cnt = cset->UncoverAlphabet(mABLen, mAlphabet, mValues);
				for (uint32 j = 0; j < cnt; ++j) {
					mAlphabetUsageZap[mValues[j]]->push_back(i);
				}
				stats.usgCountSum -= cnt;

				cset->RestoreMask();
				// vanaf naChangeIter om cover zonder mChangeItem te bepalen
				for(cit = naChangeIter; cit != cend; ++cit) {
					iset = *cit;
					if((cset->*mCoverFunc)(iset->GetLength(),iset)) {
						iset->AddOneToUsageCount();
						iset->SetUsed(i);
						stats.usgCountSum++;
					}
				}

				// alphabet items
				cnt = cset->CoverAlphabet(mABLen, mAlphabet, mValues);
				for (uint32 j = 0; j < cnt; ++j) {
					mAlphabetUsageAdd[mValues[j]]->push_back(i);
				}
				stats.usgCountSum += cnt;
			}
		}

		// mAlphabetUsages are updated in below, code shared with ChangeAdd


	} else if(mChangeType == ChangeNone) {
		// init counts
		cend = mCTList->end();
		for(cit=mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->ResetUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		swap(mAlphabetUsages, mOldAlphabetUsages);
		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;

		mRollback = mCTList->end();
		mRollbackAlph = true;

		// full cover db
		islist::iterator cit,cend;
		uint32 cnt;
		for(uint32 i=0; i<mNumDBRows; i++) {
			cset = mCDB[i];
			mCDB[i]->ResetMask();

			cend = mCTList->end();
			for(cit=mCTList->begin(); cit != cend; ++cit) {
				if((mCDB[i]->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					ItemSet* is = ((ItemSet*)(*cit));
					is->AddOneToUsageCount();
					is->SetUsed(i);
					stats.usgCountSum++;

				}
			}

			// alphabet items
			cnt = cset->CoverAlphabet(mABLen, mAlphabet, mValues);
			for (uint32 j = 0; j < cnt; ++j) {
				mAlphabetUsages[mValues[j]][mAlphabet[mValues[j]]] = i;
			}
			stats.usgCountSum += cnt;
		}
	} else
		THROW("Unknown code table ChangeType");

	// Update Alphabet Usages
	if(mChangeType == ChangeAdd || mChangeType == ChangeDel) {
		for(uint32 i = 0; i < mAlphabetSize; ++i) {
			vector<uint32>::iterator addCur = mAlphabetUsageAdd[i]->begin(), addEnd = mAlphabetUsageAdd[i]->end();
			vector<uint32>::iterator zapCur = mAlphabetUsageZap[i]->begin(), zapEnd = mAlphabetUsageZap[i]->end();
			uint32 newCur = 0, oldCur = 0;
			// van voor af aan - het mix gedeelte - zolang zap niet leeg is, zitten we nog -in- de huidige usages
			for(; addCur != addEnd && zapCur != zapEnd; ) {
				if(*addCur == *zapCur) {
					// no action here
					++addCur;
					++zapCur;
				} else if(*addCur > *zapCur) {
					// zap this value
					while(mOldAlphabetUsages[i][oldCur] < *zapCur)
						mAlphabetUsages[i][newCur++] = mOldAlphabetUsages[i][oldCur++];
					oldCur++;
					++zapCur;
				} else { // (addCur->value < zapCur->value)			
					// add this value
					while(mOldAlphabetUsages[i][oldCur] < *addCur)
						mAlphabetUsages[i][newCur++] = mOldAlphabetUsages[i][oldCur++];
					mAlphabetUsages[i][newCur++] = *addCur;
					++addCur;
				}
			}
			// dan het restje zapgedeelte, omdate het om zappen gaat zitten we automatisch nog in range oude usages (aka, geen check nodig)
			for(; zapCur != zapEnd; ++zapCur) {
				// copy until that value
				while(mOldAlphabetUsages[i][oldCur] < *zapCur)
					mAlphabetUsages[i][newCur++] = mOldAlphabetUsages[i][oldCur++];
				// zap this value
				oldCur++;
			}
			// of het restje add, we hoeven niet meer in oude range te zitten
			for(; addCur != addEnd; ++addCur) {
				// copy until that value
				while(oldCur < mOldAlphabet[i] && mOldAlphabetUsages[i][oldCur] < *addCur)
					mAlphabetUsages[i][newCur++] = mOldAlphabetUsages[i][oldCur++];
				// add this value
				mAlphabetUsages[i][newCur++] = *addCur;
			}
			// en dan eventueel nog een restje old, gewoon kopieren
			memcpy(mAlphabetUsages[i] + newCur, mOldAlphabetUsages[i] + oldCur, sizeof(uint32) * (mOldAlphabet[i] - oldCur));

			mAlphabetUsageAdd[i]->clear();
			mAlphabetUsageZap[i]->clear();
		}
	}

	// Recalc sizes, counts and countSum should be up to date...
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	double codeLen = 0;
	// alle is <- ct
	cend = mCTList->end();
	for(cit = mCTList->begin(); cit != cend; ++cit) {
		uint32 curcnt = ((ItemSet*)(*cit))->GetUsageCount();
		if(curcnt == 0) continue;
		stats.numSetsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength();
		stats.encCTSize += codeLen;
	}
		
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		uint32 curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += mStdLengths[i];
		stats.encCTSize += codeLen;
	}
	stats.encSize = stats.encDbSize + stats.encCTSize;
}

void CPUOCodeTable::CoverBinnedDB(CoverStats &stats) {
	THROW_NOT_IMPLEMENTED();
}


