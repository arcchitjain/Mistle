#include "../../../../global.h"

#include <stdlib.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>

#include "../CodeTable.h"
#include "../CTISList.h"

#include "CPCodeTable.h"

CPCodeTable::CPCodeTable() : CodeTable() {
	mCTLen = 0;
	mCT = NULL;
	mMaxIdx = 0;

	mCoverSet = NULL;

	mNumDBRows = 0;
	mAllRows = NULL;
	mCDB = NULL;

	mABLen = 0;
	mAlphabet = NULL;
	mOldAlphabet = NULL;
	mABUsedLen = 0;
	mABUnusedCount = 0;

	mIsBinnedDB = false;

	mChangeType = ChangeNone;
}

CPCodeTable::CPCodeTable(const CPCodeTable& ct) : CodeTable(ct) {
	mCTLen = ct.mCTLen;
	mCT = new CTISList*[mCTLen];
	CTISList *next = NULL;
	for (uint32 i=0; i<mCTLen; i++) {
		if (ct.mCT[i] == NULL) {
			mCT[i] = NULL;
		} else {
			mCT[i] = new CTISList(next);
			islist::iterator cit, cend=ct.mCT[i]->GetList()->end();
			for(cit = ct.mCT[i]->GetList()->begin(); cit != cend; ++cit) {
				mCT[i]->GetList()->push_back((*cit)->Clone());
			}
			next = mCT[i];
		}
	}
	mMaxIdx = ct.mMaxIdx;

	if (ct.mCoverSet != NULL)
		mCoverSet = ct.mCoverSet->Clone();
	else
		mCoverSet = NULL;

	mNumDBRows = ct.mNumDBRows;
	mAllRows = new uint32[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		mAllRows[i] = ct.mAllRows[i];
	}
	mCDB = new CoverSet*[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCDB[i] = ct.mCDB[i]->Clone();
	}

	mABLen = ct.mABLen;
	mAlphabet = new uint32[mABLen];
	memcpy(mAlphabet, ct.mAlphabet, mABLen * sizeof(uint32));
	mOldAlphabet = new uint32[mABLen];
	memcpy(mOldAlphabet, ct.mOldAlphabet, mABLen * sizeof(uint32));
	mABUsedLen = ct.mABUsedLen;
	mABUnusedCount = ct.mABUnusedCount;

	mIsBinnedDB = ct.mIsBinnedDB;

	mChangeType = ChangeNone;
}


CPCodeTable::~CPCodeTable() {
	for(uint32 i=0; i<mNumDBRows; i++)
		delete mCDB[i];
	delete[] mCDB;	
	islist::iterator cit, cend;
	for(uint32 i=0; i<mCTLen; i++) {
		if(mCT[i] != NULL) {
			cend=mCT[i]->GetList()->end();
			for(cit = mCT[i]->GetList()->begin(); cit != cend; ++cit)
				delete *cit;
			delete mCT[i];
		}
	}
	delete[] mCT;
	delete[] mAlphabet;
	delete[] mOldAlphabet;
	delete[] mAllRows;
	delete[] mCoverSet;
}

void CPCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CodeTable::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);

	// Codetable
	if(mCT == NULL) {
		mCTLen = db->GetMaxSetLength();
		mCT = new CTISList*[mCTLen];
		for(uint32 i=0; i<mCTLen; i++)
			mCT[i] = NULL;
		mCurNumSets = 0;
		mMaxIdx = 0;
	}

	// Add Alphabet
	alphabet *a = db->GetAlphabet();
	size_t abetlen = a->size();
	mABLen = (uint32) abetlen;
	//uint32 pruneBelow = db->GetPrunedBelow();
	uint32 pruneBelow = 0;
	mAlphabet = new uint32[abetlen];
	mOldAlphabet = new uint32[abetlen];
	uint32 countSum = 0, acnt=0;
	uint32 cnt = 0;
	alphabet::iterator ait, aend=a->end();
	mABUsedLen = 0;
	mABUnusedCount = 0;
	for(ait = a->begin(); ait != aend; ++ait) {
		if(ait->first != cnt)
			THROW("ja, maar, dat kan zo maar niet!");
		acnt = ait->second;
		mAlphabet[cnt++] = acnt;
		countSum += acnt;
		if(acnt != 0 && acnt >= pruneBelow) {
			mABUsedLen++;
		} else
			mABUnusedCount += acnt;
	}

	double ctSize = 0;
	if(ctinit == InitCTAlpha) {
		for(uint32 i=0; i<cnt; i++)
			ctSize += mStdLengths[i];
		ctSize *= 2;
	}

	mABLen = cnt;
	//ECHO(2, printf(" * Alphabet length:\t%d/%d (used/total)\n", mABUsedLen, mABLen));
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

	mNumDBRows = mDB->GetNumRows();
	mAllRows = new uint32[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++)
		mAllRows[i] = i;

	mCurStats.usgCountSum = countSum;
	mCurStats.encDbSize = db->GetStdDbSize();
	mCurStats.encCTSize = ctSize;
	mCurStats.encSize = mCurStats.encDbSize + mCurStats.encCTSize;
	mCurStats.numSetsUsed = 0;
	mCurStats.alphItemsUsed = (uint32) a->size();
	mPrevStats = mCurStats;

	// en nu gaan we hier een eigen db structuur maken...
	mCDB = new CoverSet *[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++)
		mCDB[i] = CoverSet::Create(type, mABLen, mDB->GetRow(i));

	mIsBinnedDB = mDB->HasBinSizes();
}


// CP Add :: Voegt `is` wel echt toe, maar set wel de mChange-vars ..
void CPCodeTable::Add(ItemSet *is, uint64 candidateId) {
	uint32 len = is->GetLength();
	if(mCT[len-1] == NULL) {
		if(mMaxIdx < len-1) {
			mCT[len-1] = new CTISList(mCT[mMaxIdx]);
			mMaxIdx = len-1;
		} else {	// find neighbour
			for(uint32 i=len; i<mCTLen; i++)
				if(mCT[i] != NULL) {
					mCT[len-1] = new CTISList(mCT[i]->GetNext());
					mCT[i]->SetNext(mCT[len-1]);
					break;
				}
		}
	} 
	mCT[len-1]->GetList()->push_back(is);
	++mCurNumSets;

	mChangeType = ChangeAdd;
	mChangeItem = is;
	mChangeRow = len-1;
	mChangeIter = mCT[mChangeRow]->GetList()->end(); /* goed! */
	--mChangeIter;
}

void CPCodeTable::Del(ItemSet *is, bool zap, bool keepList) {
	uint32 len = is->GetLength();
	islist *lst = mCT[len-1]->GetList();

	mChangeType = ChangeDel;
	mChangeItem = is;
	mChangeRow = len-1;

	mLastDel.is = is;
	if(lst->back() == is) {
		mChangeIter = lst->end();
		--mChangeIter;
		mLastDel.before = NULL;
	} else {
		islist::iterator iter, lend = lst->end();
		for(iter=lst->begin(); iter!=lend; ++iter)
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
		lst->erase(mChangeIter);
		delete is;
		mChangeType = ChangeNone;
		mChangeItem = NULL;
	}
}

// Commits last added ItemSet
void CPCodeTable::CommitAdd(bool updateLog) {
	if(updateLog)
		UpdateCTLog();
	mPrevStats = mCurStats;
	mChangeItem->BackupUsage();
	mChangeItem = NULL;
	mChangeType = ChangeNone;
}

// Rolls back last added ItemSet, restores counts and stats
void CPCodeTable::RollbackAdd() {
	if(mChangeType == ChangeAdd && mChangeItem != NULL) {
		Del(mChangeItem, true, false);
		mLastDel.is = NULL;
		mCurStats = mPrevStats;
		RollbackCounts();
		mChangeType = ChangeNone;
		mChangeItem = NULL;
	}
}
void CPCodeTable::RollbackLastDel() {
	if(mChangeType == ChangeDel && mLastDel.is != NULL)	{
		// Er is niet echt gedelete, alleen change en delinfo moet gezapt
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeRow = -1;
		mCurStats = mPrevStats;
		RollbackCounts();

		++mCurNumSets;

		mLastDel.is = NULL;
	}
}

void CPCodeTable::CommitLastDel(bool zap, bool updateLog) {
	if(mChangeType == ChangeDel) {
		if(updateLog)
			UpdateCTLog();

		mCT[mChangeRow]->GetList()->erase(mChangeIter);
		if(zap)
			delete mChangeItem;
		mLastDel.is = NULL;
		mPrevStats = mCurStats;
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeRow = -1;
	}
}

void CPCodeTable::CommitAllDels(bool updateLog) {
	THROW_DROP_SHIZZLE();
}
void CPCodeTable::RollbackAllDels() {
	THROW_DROP_SHIZZLE();
}
void CPCodeTable::RollbackCounts() {
	for(uint32 j=mMaxIdx; j>=mRollback; --j)
		if(mCT[j] != NULL)
			mCT[j]->RollbackUsageCounts();
	if(mRollbackAlph)
		memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
}

islist* CPCodeTable::GetItemSetList() {
	islist *ilist = new islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildItemSetList(ilist);
	return ilist;
}

islist* CPCodeTable::GetPrePruneList() {
	islist *ilist = new	islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildPrePruneList(ilist);
	return ilist;
}
islist* CPCodeTable::GetSanitizePruneList() {
	islist *pruneList = new islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildSanitizePruneList(pruneList);
	return pruneList;
}
islist* CPCodeTable::GetPostPruneList() {
	islist *pruneList = new islist();
	UpdatePostPruneList(pruneList, pruneList->begin());
	return pruneList;
}

void CPCodeTable::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList, after);
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildPostPruneList(pruneList, after);
}

void CPCodeTable::CoverDB(CoverStats &stats) {
	if(mIsBinnedDB)			// switch binned/notbinned
		CoverBinnedDB(stats);
	else
		CoverNoBinsDB(stats);
}

void CPCodeTable::CoverNoBinsDB(CoverStats &stats) {
	CoverSet *cset;
	ItemSet *iset;
	uint32 *occur, numOcc;
	islist::iterator cit,cend;

	if(mChangeType == ChangeAdd) {
		uint32 mj = mChangeItem->GetLength()-1;
		for(uint32 j=mMaxIdx; j>=mj; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();

		mChangeItem->SetPrevUsageCount(1);													// dus na ResetCount prevCount != 0
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();
		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();
			for(uint32 j=mMaxIdx; j>=mj; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();									// up till, but not including m
					if(j==mj)															// !!! loop splits -> mj+1, en dan mj apart?
						--cend;
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						iset = *cit;
						if(iset->GetPrevUsageCount()==0)
							continue;
						cset->Cover(j+1,iset);
					}
				}
			}
			if(cset->CanCover(mj+1, mChangeItem)) {									// nu m nog even doen
				mChangeItem->AddOneToUsageCount();
				stats.usgCountSum++;
			}
		}
		mChangeItem->SetPrevUsageCount(0);
		if(mChangeItem->Unused()) {
			mRollback = mj;
			mRollbackAlph = false;
			return;
		}
		// M heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = 1;
		mRollbackAlph = true;
		for(uint32 j=mj-1; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		
		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];

			cset->BackupMask();
			//	herstelcover (--) 
			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->SupOneFromUsageCount();
							stats.usgCountSum--;
						}
					}
				}
			}
			stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet);
			
			//  Totale cover, ditmaal met m 
			cset->RestoreMask();
 			cset->Cover(mj+1, mChangeItem);

			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddOneToUsageCount();
							stats.usgCountSum++;
						}
					}
				}
			}
			stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...

	} else if(mChangeType == ChangeDel) {
		uint32 mj = mChangeRow;
		stats = mCurStats;

		mRollback = 1;
		mRollbackAlph = true;
		for(uint32 j=mMaxIdx; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();

			// tot mChangeRow
			for(uint32 j=mMaxIdx; j>mj; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();	// up till, but not including m
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						iset = *cit;
						if(iset->GetPrevUsageCount()==0)
							continue;
						cset->Cover(j+1,iset);
					}
				}
			}
			// tot/met mChangeIter om cset op te bouwen
			cend = mChangeIter;
			for(cit = mCT[mj]->GetList()->begin(); cit != cend; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)		// deze rij, en verder is gebackuped
					continue;
				cset->Cover(mj+1,iset);
			}

			cset->BackupMask();
			cset->Cover(mj+1,mChangeItem);

			// :: Herstelcover (--) 
			//	-: Herstel staartje mChangeRow
			cend = mCT[mj]->GetList()->end();
			cit = mChangeIter;
			++cit;
			for(; cit != cend; ++cit) {
				if(cset->Cover(mj+1,*cit)) {
					((ItemSet*)(*cit))->SupOneFromUsageCount();
					stats.usgCountSum--;
				}
			}
			//	-: Herstel na mChangeRow
			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->SupOneFromUsageCount();
							stats.usgCountSum--;
						}
					}
				}
			}
			//	-: Herstel alphabet counts
			stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet);

			cset->RestoreMask();

			// mChangeRow vanaf mChangeIter om cover zonder te bepalen
			cit = mChangeIter;
			++cit;
			cend = mCT[mj]->GetList()->end();
			for(; cit != cend; ++cit) {
				if(cset->Cover(mj+1,*cit)) {
					((ItemSet*)(*cit))->AddOneToUsageCount();
					stats.usgCountSum++;
				}
			}
			// na mChangeRow
			for(uint32 j=mj-1; j>=1; --j) {	
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddOneToUsageCount();
							stats.usgCountSum++;
						}
					}
				}
			}

			// alphabet items
			stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet);
		}
	} else if(mChangeType == ChangeNone) {
		// init counts
		for(uint32 j=mMaxIdx; j>0; --j)
			if(mCT[j] != NULL)
				mCT[j]->ResetUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;

		mRollback = 1;
		mRollbackAlph = true;

		// full cover db
		islist::iterator cit,cend;
		for(uint32 i=0; i<mNumDBRows; i++) {
			mCDB[i]->ResetMask();
			for(uint32 j=mMaxIdx; j>0; j--) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(mCDB[i]->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddOneToUsageCount();
							stats.usgCountSum++;
						}
					}
				}
			}
			// alphabet items
			stats.usgCountSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet);
		}
	} else
		THROW("Unknown code table ChangeType");

	// Recalc sizes, counts and countSum should be up to date...
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	double codeLen = 0;
	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				uint32 curcnt = ((ItemSet*)(*cit))->GetUsageCount();
				if(curcnt == 0) continue;
				stats.numSetsUsed++;
				codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
				stats.encDbSize += curcnt * codeLen;
				stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength();
				stats.encCTSize += codeLen;
			}
		}
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

void CPCodeTable::CoverBinnedDB(CoverStats &stats) {
	CoverSet *cset;
	ItemSet *iset;
	uint32 *occur, numOcc, binSize;
	islist::iterator cit,cend;

	if(mChangeType == ChangeAdd) {
		uint32 mj = mChangeItem->GetLength()-1;
		for(uint32 j=mMaxIdx; j>=mj; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();

		mChangeItem->SetPrevUsageCount(1);													// dus na ResetCount prevCount != 0
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();
		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();
			for(uint32 j=mMaxIdx; j>=mj; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();									// up till, but not including m
					if(j==mj)															// !!! loop splits -> mj+1, en dan mj apart?
						--cend;
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						iset = *cit;
						if(iset->GetPrevUsageCount()==0)
							continue;
						cset->Cover(j+1,iset);
					}
				}
			}
			if(cset->CanCover(mj+1, mChangeItem)) {									// nu m nog even doen
				binSize = mDB->GetRow(i)->GetUsageCount();
				mChangeItem->AddUsageCount(binSize);
				stats.usgCountSum+=binSize;
			}
		}
		mChangeItem->SetPrevUsageCount(0);
		if(mChangeItem->Unused()) {
			mRollback = mj;
			mRollbackAlph = false;
			return;
		}
		// M heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = 1;
		mRollbackAlph = true;
		for(uint32 j=mj-1; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];

			cset->BackupMask();
			binSize = mDB->GetRow(i)->GetUsageCount();

			//	herstelcover (--) 
			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->SupUsageCount(binSize);
							stats.usgCountSum-=binSize;
						}
					}
				}
			}
			stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet, binSize);

			//  Totale cover, ditmaal met m 
			cset->RestoreMask();
			cset->Cover(mj+1, mChangeItem);

			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddUsageCount(binSize);
							stats.usgCountSum+=binSize;
						}
					}
				}
			}
			stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...

	} else if(mChangeType == ChangeDel) {
		uint32 mj = mChangeRow;
		stats = mCurStats;

		mRollback = 1;
		mRollbackAlph = true;
		for(uint32 j=mMaxIdx; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			cset->ResetMask();

			binSize = mDB->GetRow(i)->GetUsageCount();

			// tot mChangeRow
			for(uint32 j=mMaxIdx; j>mj; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();	// up till, but not including m
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						iset = *cit;
						if(iset->GetPrevUsageCount()==0)
							continue;
						cset->Cover(j+1,iset);
					}
				}
			}
			// tot/met mChangeIter om cset op te bouwen
			cend = mChangeIter;
			for(cit = mCT[mj]->GetList()->begin(); cit != cend; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)		// deze rij, en verder is gebackuped
					continue;
				cset->Cover(mj+1,iset);
			}

			cset->BackupMask();
			cset->Cover(mj+1,mChangeItem);

			// :: Herstelcover (--) 
			//	-: Herstel staartje mChangeRow
			cend = mCT[mj]->GetList()->end();
			cit = mChangeIter;
			++cit;
			for(; cit != cend; ++cit) {
				if(cset->Cover(mj+1,*cit)) {
					((ItemSet*)(*cit))->SupUsageCount(binSize);
					stats.usgCountSum-=binSize;
				}
			}
			//	-: Herstel na mChangeRow
			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->SupUsageCount(binSize);
							stats.usgCountSum-=binSize;
						}
					}
				}
			}
			//	-: Herstel alphabet counts
			stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet, binSize);

			cset->RestoreMask();

			// mChangeRow vanaf mChangeIter om cover zonder te bepalen
			cit = mChangeIter;
			++cit;
			cend = mCT[mj]->GetList()->end();
			for(; cit != cend; ++cit) {
				if(cset->Cover(mj+1,*cit)) {
					((ItemSet*)(*cit))->AddUsageCount(binSize);
					stats.usgCountSum+=binSize;
				}
			}
			// na mChangeRow
			for(uint32 j=mj-1; j>=1; --j) {	
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddUsageCount(binSize);
							stats.usgCountSum+=binSize;
						}
					}
				}
			}

			// alphabet items
			stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
	} else if(mChangeType == ChangeNone) {
		// init counts
		for(uint32 j=mMaxIdx; j>0; --j)
			if(mCT[j] != NULL)
				mCT[j]->ResetUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;

		mRollback = 1;
		mRollbackAlph = true;

		// full cover db
		islist::iterator cit,cend;
		for(uint32 i=0; i<mNumDBRows; i++) {
			mCDB[i]->ResetMask();
			binSize = mDB->GetRow(i)->GetUsageCount();
			for(uint32 j=mMaxIdx; j>0; j--) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(mCDB[i]->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddUsageCount(binSize);
							stats.usgCountSum+=binSize;
						}
					}
				}
			}
			// alphabet items
			stats.usgCountSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
	} else
		THROW("Unknown code table ChangeType");

	// Recalc sizes, counts and countSum should be up to date...
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	double codeLen = 0;
	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				uint32 curcnt = ((ItemSet*)(*cit))->GetUsageCount();
				if(curcnt == 0) continue;
				stats.numSetsUsed++;
				codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
				stats.encDbSize += curcnt * codeLen;
				stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength();
				stats.encCTSize += codeLen;
			}
		}
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

double CPCodeTable::CalcTransactionCodeLength(ItemSet *row) {
	islist::iterator cit,cend;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, mDB->GetAlphabetSize());
	mCoverSet->InitSet(row);

	double codeLen = 0.0;
	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit)
				if(mCoverSet->Cover(j+1,*cit))
					codeLen += CalcCodeLength((*cit)->GetUsageCount(), mCurStats.usgCountSum);
		}
	}
	// alphabet items
	for(uint32 i=0; i<mABLen; i++)
		if(mCoverSet->IsItemUncovered(i))
			codeLen += CalcCodeLength(mAlphabet[i], mCurStats.usgCountSum);
	return codeLen;
}

void	CPCodeTable::CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) {
	// init counts
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->ResetUsages();
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
	islist::iterator cit,cend;
	for(uint32 i=0; i<numRows; i++) {
		mCoverSet->InitSet(mDB->GetRow(rowList[i]));

		for(uint32 j=mMaxIdx; j>=1; j--) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					if(mCoverSet->Cover(j+1,*cit)) {
						((ItemSet*)(*cit))->AddOneToUsageCount();
						stats.usgCountSum++;
					}
				}
			}
		}
		// alphabet items
		stats.usgCountSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet);
	}

	double codeLen = 0;
	uint32 curcnt;
	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				curcnt = ((ItemSet*)(*cit))->GetUsageCount();
				if(curcnt == 0) continue;
				stats.numSetsUsed++;
				codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
				stats.encDbSize += curcnt * codeLen;
				stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength() + codeLen;
			}
		}
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
void CPCodeTable::SetAlphabetCount(uint32 item, uint32 count) {
	mAlphabet[item] = count;
}
uint32 CPCodeTable::GetAlphabetCount(uint32 item) {
	return mAlphabet[item];
}
void CPCodeTable::UpdateCTLog() {
	if(mChangeType == ChangeAdd) {
		fprintf(mCTLogFile, "+ %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeDel) {
		fprintf(mCTLogFile, "- %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeNone) {
		fprintf(mCTLogFile, "0\n");
	}
}
void CPCodeTable::AddOneToEachUsageCount() {
	islist::iterator cit,cend;

	for(uint32 j=mMaxIdx; j>=1; j--)
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				((ItemSet*)(*cit))->AddOneToUsageCount();
				mCurStats.usgCountSum++;
			}
		}
	// don't forget the alphabet!
	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i]++;
	mCurStats.usgCountSum += mABLen;
}
void CPCodeTable::ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset) {
	islist::iterator cit,cend;

	for(uint32 j=mMaxIdx; j>=1; j--)
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit)
				((ItemSet*)(*cit))->SetUsageCount(lpcFactor * ((ItemSet*)(*cit))->GetUsageCount() + lpcOffset);
		}
	// don't forget the alphabet!
	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i] = lpcFactor * mAlphabet[i] + lpcOffset;

	mCurStats.usgCountSum = lpcFactor * mCurStats.usgCountSum + lpcOffset;
	++mLaplace;
}
