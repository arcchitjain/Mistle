#include "../../../../global.h"

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

#include "../CTISList.h"

#include "CPCodeTable.h"
#include "CPAOCodeTable.h"

CPAOCodeTable::CPAOCodeTable() : CPCodeTable() {
	mCTList = NULL;
	mCT = NULL;
	mChangeType = ChangeNone;
	mOverlap = false;
	mUseOccs = NULL;
	mNumUseOccs = 0;
	mNumDBRows = 0;
	mAlphabetSize = 0;
	mAlphabetUsages = NULL;
	mAlphabetUsageAdd = NULL;
	mAlphabetUsageZap = NULL;

	mAlphabetNumRows = NULL;
	mOldAlphabetUsages = NULL;
	mValues = NULL;
}

//CPOACodeTable::CPOACodeTable(const string &name) : CPCodeTable() {
CPAOCodeTable::CPAOCodeTable(const string &name) {
	mCTList = NULL;
	mCT = NULL;
	mChangeType = ChangeNone;
	mUseOccs = NULL;
	mNumUseOccs = 0;
	mNumDBRows = 0;
	mAlphabetSize = 0;
	mAlphabetUsages = NULL;
	mAlphabetUsageAdd = NULL;
	mAlphabetUsageZap = NULL;


	mAlphabetNumRows = NULL;
	mOldAlphabetUsages = NULL;
	mValues = NULL;

	// coverpartial-alt-orderly- stack random length etc
	if(name.compare(25,6,"random") == 0) {
		mOrder = CTRandomOrder;
		RandomUtils::Init();
	} else if(name.compare(25,6,"length") == 0) {
		if(name.compare(25,10,"length-inv") == 0)
			mOrder = CTLengthInverseOrder;
		else 
			mOrder = CTLengthOrder;
	} else if(name.compare(25,4,"area") == 0) {
		if(name.compare(25,8,"area-inv") == 0)
			mOrder = CTAreaInverseOrder;
		else 
			mOrder = CTAreaOrder;
	} else {
		if(name.compare(25,9,"stack-inv") == 0)
			mOrder = CTStackInverseOrder;
		else
			mOrder = CTStackOrder;
	}

	if(name.substr(name.length()-3,3).compare("-ov") == 0) {
		mOverlap = true;
		mCoverFunc = &CoverSet::CoverWithOverlap;
		mCanCoverFunc = &CoverSet::CanCoverWithOverlap;

		//mCoverFunc = &CoverSet::CoverWithOverlapNoSubsetCheck;
		//mCanCoverFunc = &CoverSet::CanCoverWithOverlapNoSubsetCheck;	// geen check nodig, want loop gaat toch alleen over dbOcc
	} else {
		mOverlap = false;
		mCoverFunc = &CoverSet::Cover;
		mCanCoverFunc = &CoverSet::CanCover;
	}
}

string CPAOCodeTable::GetShortName() {
	string s = mOverlap ? "-ov" : "";
	if(mOrder == CTRandomOrder)
		return "cpoar" + s;
	else if (mOrder == CTLengthOrder)
		return "cpoal" + s;
	else if (mOrder == CTLengthInverseOrder)
		return "cpoali" + s;
	else if (mOrder == CTAreaOrder)
		return "cpoaa" + s;
	else if (mOrder == CTAreaInverseOrder)
		return "cpoaai" + s;
	else if (mOrder == CTStackInverseOrder) 
		return "cpoasi" + s;
	else /* if (mOrder == CTStackOrder) */
		return "cpoas" + s;
}

CPAOCodeTable::~CPAOCodeTable() {
	if(mCTList != NULL) {
		islist::iterator cit, cend=mCTList->end();
		for(cit = mCTList->begin(); cit != cend; ++cit)
			delete *cit;
	}
	delete mCT;
	delete[] mUseOccs;
	mCTList = NULL;

	for (uint32 i = 0; i < mAlphabetSize; ++i) {
		delete[] mAlphabetUsages[i];
		delete mAlphabetUsageAdd[i];
		delete mAlphabetUsageZap[i];
	} 
	delete[] mAlphabetUsages;
	delete[] mAlphabetUsageAdd;
	delete[] mAlphabetUsageZap;


	for (uint32 i = 0; i < mAlphabetSize; ++i)
		delete[] mOldAlphabetUsages[i];
	delete[] mOldAlphabetUsages;
	delete[] mValues;

/*
#if(defined(__unix__) && !defined(NDEBUG))
	delete[] mAlphabetUsageDebug;
	delete[] mOldAlphabetUsageDebug;
#endif
*/
}


void CPAOCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CPCodeTable::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);
	// 'CT in a List' stuffs
	mCT = new CTISList();
	mCTList = mCT->GetList();
	mCurNumSets = 0;

	mNumDBRows = mDB->GetNumRows();
	mUseOccs = new uint32[mNumDBRows];
	mIsBinnedDB = mDB->HasBinSizes();

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


// CPO Add :: Voegt `is` wel echt toe, maar set wel de mChange-vars ..
void CPAOCodeTable::Add(ItemSet *is, uint64 candidateId) {
	if(mOrder == CTRandomOrder) {
		uint32 pos = RandomUtils::UniformUint32(mCurNumSets+1); // uint32 from [0,mCurNumSets]
		islist::iterator iter=mCTList->begin(), lend = mCTList->end();
		for(pos; pos>0 && iter!=lend; --pos)
			++iter;
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTLengthOrder) {
		islist::iterator iter, lend = mCTList->end();
		uint32 isl = is->GetLength();
		for(iter=mCTList->begin(); iter!=lend; ++iter) {
			if(isl > ((ItemSet*)(*iter))->GetLength())
				break;
		}
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTLengthInverseOrder) {
		islist::iterator iter, lend = mCTList->end();
		uint32 isl = is->GetLength();
		for(iter=mCTList->begin(); iter!=lend; ++iter) {
			if(isl < ((ItemSet*)(*iter))->GetLength())
				break;
		}
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTAreaOrder) {
		islist::iterator iter, lend = mCTList->end();
		uint32 area = is->GetLength() * is->GetSupport();
		for(iter=mCTList->begin(); iter!=lend; ++iter) {
			if(area > (((ItemSet*)(*iter))->GetLength() * ((ItemSet*)(*iter))->GetSupport()))
				break;
		}
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTAreaInverseOrder) {
		islist::iterator iter, lend = mCTList->end();
		uint32 area = is->GetLength() * is->GetSupport();
		for(iter=mCTList->begin(); iter!=lend; ++iter) {
			if(area < (((ItemSet*)(*iter))->GetLength() * ((ItemSet*)(*iter))->GetSupport()))
				break;
		}
		mChangeIter = mCTList->insert(iter,is);
	} else if(mOrder == CTStackInverseOrder) {
		mCTList->push_front(is);
		mChangeIter = mCTList->begin();
	} else if(mOrder == CTStackOrder) {
		mCTList->push_back(is);
		mChangeIter = mCTList->end();
		--mChangeIter;
		mCTList->begin();
	}

	++mCurNumSets;

	mChangeType = ChangeAdd;
	mChangeItem = is;
}

void CPAOCodeTable::Del(ItemSet *is, bool zap, bool keepList) {
	mChangeType = ChangeDel;
	mChangeItem = is;

	mLastDel.is = is;
	if(mCTList->back() == is) {
		mChangeIter = mCTList->end();
		--mChangeIter;
		mLastDel.before = NULL;
	} else {
		islist::iterator iter, lend = mCTList->end();
		for(iter=mCTList->begin(); iter!=lend; ++iter)
			if((ItemSet*)(*iter) == is)
				break;
		mChangeIter = iter;
		lend = iter;
		++lend;
		mLastDel.before = (ItemSet*)(*lend);
	}

	if(keepList)
		mDeleted.push_front(DelInfo(mLastDel));

	--mCurNumSets;
	if(zap) {
		mCTList->erase(mChangeIter);
		delete is;
		mChangeType = ChangeNone;
		mChangeItem = NULL;
	}
}

// Rolls back last added ItemSet, restores counts and stats
void CPAOCodeTable::RollbackAdd() {
	if(mChangeType == ChangeAdd && mChangeItem != NULL) {
		// Dan het ding van de aardbodem doen verdwijnen
		Del(mChangeItem, true, false);
		mLastDel.is = NULL;
		mCurStats = mPrevStats;
		// Eerst counts terugrollen, want dan is mRollback = mChangeIter nog geldig!
		RollbackCounts();
		mChangeType = ChangeNone;
		mChangeItem = NULL;
	}
}
void CPAOCodeTable::RollbackLastDel() {
	if(mChangeType == ChangeDel && mLastDel.is != NULL)	{
		// Omdat er niet echt gedelete is, moeten alleen change en delinfo gezapt
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeRow = -1;
		mCurStats = mPrevStats;
		RollbackCounts();

		++mCurNumSets;

		mLastDel.is = NULL;
	}
}

void CPAOCodeTable::CommitLastDel(bool zap, bool updateLog) {
	if(mChangeType == ChangeDel) {
		if(updateLog)
			UpdateCTLog();

		mCTList->erase(mChangeIter);
		if(zap)
			delete mChangeItem;
		mLastDel.is = NULL;
		mPrevStats = mCurStats;
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeRow = -1;
	}
}

void CPAOCodeTable::CommitAllDels(bool updateLog) {
	THROW_DROP_SHIZZLE();
}
void CPAOCodeTable::RollbackAllDels() {
	THROW_DROP_SHIZZLE();
}
void CPAOCodeTable::RollbackCounts() {
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

islist* CPAOCodeTable::GetItemSetList() {
	islist *ilist = new islist();
	mCT->BuildItemSetList(ilist);
	return ilist;
}

islist* CPAOCodeTable::GetSingletonList() { // TODO: Verify whether correct!
	islist *ilist = new	islist();
	ItemSet *is;
	uint16 *set;
	alphabet *alph = mDB->GetAlphabet();
	uint32 alphLen = (uint32)alph->size();

	for(uint32 i=0; i<alphLen; i++) {
		set = new uint16[1];
		set[0] = i;
		is = ItemSet::Create(mDB->GetDataType(), set, 1, alphLen, mAlphabet[i], alph->find(i)->second);
		ilist->push_back(is);
		delete[] set;
	}
	return ilist;
}

islist* CPAOCodeTable::GetPrePruneList() {
	islist *ilist = new	islist();
	mCT->BuildPrePruneList(ilist);
	return ilist;
}

void CPAOCodeTable::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList, after);
	mCT->BuildPostPruneList(pruneList, after);
}

void CPAOCodeTable::CoverDB(CoverStats &stats) {
	if(mIsBinnedDB)			// switch binned/notbinned
		CoverBinnedDB(stats);
	else
		CoverNoBinsDB(stats);
}

void CPAOCodeTable::CoverNoBinsDB(CoverStats &stats) {
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

void CPAOCodeTable::CoverBinnedDB(CoverStats &stats) {
	CoverSet *cset;
	ItemSet *iset;
	uint32 *occur, numOcc, numUseCount;
	islist::iterator cit,cend,naChangeIter;
	naChangeIter = mChangeIter;
	++naChangeIter;

	if(mChangeType == ChangeAdd) {
		// alle is' -tot/met- mChangeIter
		cend = naChangeIter;
		for(cit=mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();

		mChangeItem->SetPrevUsageCount(1);													// dus na ResetCount prevCount != 0
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();

		mNumUseOccs = 0;
		numUseCount = 0;

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
				uint32 binSize = mDB->GetRow(i)->GetUsageCount();
				mUseOccs[mNumUseOccs++] = i;
				numUseCount += binSize;
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
		stats.usgCountSum += numUseCount;
		mChangeItem->AddUsageCount(numUseCount);
		// M heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = mCTList->end();
		mRollbackAlph = true;
		// vanaf na mChangeIter counts up backen
		cend = mCTList->end();
		for(cit = naChangeIter; cit != cend; ++cit)
			((ItemSet*)(*cit))->BackupUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		uint32 i;
		for(uint32 r=0; r<mNumUseOccs; r++) {
			i=mUseOccs[r];
			cset = mCDB[i];

			uint32 binSize = mDB->GetRow(i)->GetUsageCount();

			cset->BackupMask();
			//	herstelcover (--) 
			// vanaf na mChangeIter counts herstellen
			for(cit = naChangeIter; cit != cend; ++cit)
				
				if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					((ItemSet*)(*cit))->SupUsageCount(binSize);
					stats.usgCountSum -= binSize;
				}
				stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet, binSize);

				//  Totale cover tot/met m
				cset->RestoreMask();
				(cset->*mCoverFunc)(mChangeItem->GetLength(),mChangeItem);

				//	Cover vanaf naChangeIter
				cend = mCTList->end();
				for(cit = naChangeIter; cit != cend; ++cit)
					//if(cset->Cover(((ItemSet*)(*cit))->GetLength(),*cit)) {
					if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
						((ItemSet*)(*cit))->AddUsageCount(binSize);
						stats.usgCountSum += binSize;
					}
					stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...

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
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...
		uint32 ciLen = mChangeItem->GetLength();

		uint32 r,i;
		for(r=0; r<numOcc; r++) {
			i=occur[r];
			cset = mCDB[i];
			uint32 binSize = mDB->GetRow(i)->GetUsageCount();
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
						iset->SupUsageCount(binSize);
						stats.usgCountSum-=binSize;
					}
				}
				//	-: Herstel alphabet counts
				stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet,binSize);

				cset->RestoreMask();
				// vanaf naChangeIter om cover zonder mChangeItem te bepalen
				for(cit = naChangeIter; cit != cend; ++cit) {
					if((cset->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
						((ItemSet*)(*cit))->AddUsageCount(binSize);
						stats.usgCountSum+=binSize;
					}
				}

				// alphabet items
				stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet, binSize);
			}
		}
	} else if(mChangeType == ChangeNone) {
		// init counts
		cend = mCTList->end();
		for(cit=mCTList->begin(); cit != cend; ++cit)
			((ItemSet*)(*cit))->ResetUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;

		mRollback = mCTList->end();
		mRollbackAlph = true;

		// full cover db
		islist::iterator cit,cend;
		for(uint32 i=0; i<mNumDBRows; i++) {
			//mCoverSet->InitSet(mDB->GetRow(i));
			//mCDB[i]->InitSet(mDB->GetRow(i));
			mCDB[i]->ResetMask();
			uint32 binSize = mDB->GetRow(i)->GetUsageCount();

			cend = mCTList->end();
			for(cit=mCTList->begin(); cit != cend; ++cit) {
				if((mCDB[i]->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
					((ItemSet*)(*cit))->AddUsageCount(binSize);
					stats.usgCountSum+=binSize;
				}
			}

			// alphabet items
			stats.usgCountSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
	} else
		THROW("Unknown code table ChangeType");

	CalcTotalSize(stats);
}

void CPAOCodeTable::CalcTotalSize(CoverStats &stats) {
	islist::iterator cit,cend;

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


double	CPAOCodeTable::CalcTransactionCodeLength(ItemSet *transaction) {
	islist::iterator cit,cend = mCTList->end();;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, mDB->GetAlphabetSize());
	mCoverSet->InitSet(transaction);

	double codeLen = 0.0;
	for(cit = mCTList->begin(); cit != cend; ++cit) {
		if(mCoverSet->Cover(((ItemSet*)(*cit))->GetLength(),*cit))
			codeLen += CalcCodeLength((*cit)->GetUsageCount(), mCurStats.usgCountSum);
	}
	// alphabet items
	for(uint32 i=0; i<mABLen; i++)
		if(mCoverSet->IsItemUncovered(i))
			codeLen += CalcCodeLength(mAlphabet[i], mCurStats.usgCountSum);
	return codeLen;
}

void	CPAOCodeTable::CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) {
	// init counts
	mCT->ResetUsages();
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i] = 0;

	stats.usgCountSum = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.encSize = 0;

	// full cover db
	islist::iterator cit,cend = mCTList->end();
	for(uint32 i=0; i<numRows; i++) {
		mCoverSet->InitSet(mDB->GetRow(rowList[i]));

		for(cit = mCTList->begin(); cit != cend; ++cit) {
			if(mCoverSet->Cover(((ItemSet*)(*cit))->GetLength(),*cit)) {
				((ItemSet*)(*cit))->AddOneToUsageCount();
				stats.usgCountSum++;
			}
		}
		// alphabet items
		stats.usgCountSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet);
	}

	double codeLen = 0;
	uint32 curcnt;
	for (cit = mCTList->begin(); cit != cend; ++cit) {
		curcnt = ((ItemSet*)(*cit))->GetUsageCount();
		if(curcnt == 0) continue;
		stats.numSetsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength() + codeLen;
	}
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		uint32 curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += mStdLengths[i] + codeLen;
	}

	stats.encSize = stats.encDbSize + stats.encCTSize;
}
Database *CPAOCodeTable::EncodeDB(const bool useSingletons, string outFile) {
	ECHO(1,	printf(" * Encoding database with CPO...\r"));
	islist::iterator cit,cend=mCTList->end();
	uint16 val=0;
	uint16 *emptySet = new uint16[0];
	ItemSet *is = NULL;
	uint16 *set;

	FILE *fp = NULL;
	if (!outFile.empty()) {
		outFile += ".encoded";
		fp = fopen(outFile.c_str(), "w");
		fprintf(fp, "algo=%s\n", GetShortName().c_str());
	}

	double* stdLengths = mDB->GetStdLengths();

	alphabet* encodedAlphabet = new alphabet();
	size_t encodedAlphabetSize = mDB->GetAlphabetSize() + mCTList->size();
	uint32* encodedAlphabetNumRows = new uint32[encodedAlphabetSize];
//	double* encodedStdLengths  = new double[encodedAlphabetSize];
	ItemSet** encodedAlphabetDefinition = new ItemSet *[encodedAlphabetSize];

	for(cit=mCTList->begin(); cit != cend; ++cit, ++val) {
		(*encodedAlphabet)[val] = 0; //(uintptr_t)(*cit); // Store pointer to itemset
		encodedAlphabetNumRows[val] = 0;
//		encodedStdLengths[val] = (*cit)->GetStandardLength();
		encodedAlphabetDefinition[val] = (*cit)->Clone();
	}
	for(uint32 j=0; j<mABLen; j++, val++) {
		(*encodedAlphabet)[val] = 0;//(*mDB->GetAlphabet())[j];
		encodedAlphabetNumRows[val] = 0;
//		encodedStdLengths[val] = stdLengths[j];
		set = new uint16[1];
		set[0] = j;
		is = ItemSet::Create(mDB->GetDataType(), set, 1, (uint32) mDB->GetAlphabetSize(), mAlphabet[j], mDB->GetAlphabet()->find(j)->second);
		encodedAlphabetDefinition[val] = is;
		delete[] set;
	}

	ItemSet **iss = new ItemSet *[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		is = iss[i] = ItemSet::CreateEmpty(mDB->GetDataType(), encodedAlphabetSize);
		is->SetUsageCount(mDB->GetRow(i)->GetUsageCount());
		mCDB[i]->ResetMask();

		val = 0;
		// sets
		for(cit=mCTList->begin(); cit != cend; ++cit) {
			if((mCDB[i]->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
				is->AddItemToSet(val);
				if (fp) {
					fprintf(fp, "{%s", (*cit)->ToString().c_str());
					fseek(fp, -2, SEEK_CUR);
					fprintf(fp, "/%d)} ", (*cit)->GetSupport());
				}
				++encodedAlphabetNumRows[val];
				++(*encodedAlphabet)[val];
			}
			val++;
		}

		// singletons
		if(useSingletons)
			for(uint32 j=0; j<mABLen; ++j, ++val) {
				if(mCDB[i]->IsItemUncovered(j)) {
					is->AddItemToSet(val);
					if (fp)
						fprintf(fp, "{%d} ", j);
					++encodedAlphabetNumRows[val];
					++(*encodedAlphabet)[val];
				}
			}

		if (fp)
			fprintf(fp, "\n");
	}

	delete[] emptySet;
	if (fp)
		fclose(fp);
	ECHO(1,	printf(" * Encoding:\t\tdone (used CPO)\n\n"));

	Database* encodedDB = new Database(iss, mNumDBRows, mDB->HasBinSizes());
	encodedDB->SetAlphabet(encodedAlphabet);
	encodedDB->ComputeEssentials();
	encodedDB->SetColumnDefinition(encodedAlphabetDefinition, (uint32) encodedAlphabetSize);
	encodedDB->SetAlphabetNumRows(encodedAlphabetNumRows);
//	encodedDB->SetStdLengths(encodedStdLengths);

/*
	encodedDB->TranslateForward();
	ItemTranslator* itranslator = encodedDB->GetItemTranslator();

	alphabet* ab = encodedDB->GetAlphabet();
	uint32* idxs = new uint32[encodedDB->GetAlphabetSize()];
	for (alphabet::iterator it = ab->begin(); it != ab->end(); ++it) {
		printf("%d -> %d\n", it->first, itranslator->BitToValue(it->first));
		idxs[it->first] = itranslator->BitToValue(it->first);
	}

	ArrayUtils::Permute(encodedAlphabetDefinition, idxs, encodedDB->GetAlphabetSize());
	encodedDB->SetColumnDefinition(encodedAlphabetDefinition, encodedAlphabetSize);
	ArrayUtils::Permute(encodedAlphabetNumRows, idxs, encodedDB->GetAlphabetSize());
	encodedDB->SetAlphabetNumRows(encodedAlphabetNumRows);

	encodedDB->ComputeEssentials();
*/


/*
	encodedDB->SetAlphabetNumRows(encodedAlphabetNumRows);
	encodedDB->ComputeEssentials();
	encodedDB->EnableFastDBOccurences();
*/
	return encodedDB;
}
void CPAOCodeTable::UpdateCTLog() {
	if(mChangeType == ChangeAdd) {
		fprintf(mCTLogFile, "+ %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeDel) {
		fprintf(mCTLogFile, "- %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeNone) {
		fprintf(mCTLogFile, "0\n");
	}
}
