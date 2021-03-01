#include "../../../../global.h"

#include <stdlib.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>

#include "../CodeTable.h"
#include "../CTISList.h"

#include "CPCodeTable.h"
#include "CCPCodeTable.h"

CCPCodeTable::CCPCodeTable() : CodeTable() {
	mNumDBRows = 0;
	mCoverSet = NULL;
	mCDB = NULL;

	mCTLen = 0;
	mCT = NULL;
	mMaxIdx = 0;

	mUseOccs = NULL;
	mNumUseOccs = 0;

	mABLen = 0;
	mAlphabet = NULL;
	mOldAlphabet = NULL;
	mABUsedLen = 0;
	mABUnusedCount = 0;

	mIsBinnedDB = false;

	mChangeType = ChangeNone;
}

CCPCodeTable::CCPCodeTable(const CCPCodeTable &ct) : CodeTable(ct) {
	mNumDBRows = ct.mNumDBRows;
	if (ct.mCoverSet != NULL)
		mCoverSet = ct.mCoverSet->Clone();
	else
		mCoverSet = NULL;
	mCDB = new CoverSet*[mNumDBRows];
	for (uint32 i=0; i<mNumDBRows; i++) {
		mCDB[i] = ct.mCDB[i]->Clone();
	}


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


	mUseOccs = new uint32[mNumDBRows];
	for (uint32 i=0; i<mNumDBRows; i++) {
		mUseOccs[i] = ct.mUseOccs[i];
	}
	mNumUseOccs = ct.mNumUseOccs;

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


CCPCodeTable::~CCPCodeTable() {
	delete mCoverSet;
	for(uint32 i=0; i<mNumDBRows; i++)
		delete mCDB[i];
	delete[] mCDB;	
	islist::iterator cit, cend;
	CTISList *curlist;
	for(uint32 i=0; i<mCTLen; i++) {
		if(mCT[i] != NULL) {
			curlist = mCT[i];
			cend=curlist->GetList()->end();
			for(cit = curlist->GetList()->begin(); cit != cend; ++cit)
				delete *cit;
			delete curlist;
		}
	}
	if(mCT != NULL)
		delete[] mCT;
	if(mAlphabet != NULL)
		delete[] mAlphabet;
	if(mOldAlphabet != NULL)
		delete[] mOldAlphabet;
	if(mUseOccs != NULL)
		delete[] mUseOccs;
}

void CCPCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CodeTable::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);

	// Codetable
	if(mCT == NULL) {
		mCTLen = maxCTElemLength==0 ? db->GetMaxSetLength() : maxCTElemLength;
		mCT = new CTISList*[mCTLen];
		for(uint32 i=0; i<mCTLen; i++)
			mCT[i] = NULL;
		mCurNumSets = 0;
		mMaxIdx = 0;
	}

	// Add Alphabet
	alphabet *a = db->GetAlphabet();
	mABLen = (uint32) a->size();
	uint32 abetlen = mABLen;

	//uint32 pruneBelow = db->GetPrunedBelow();
	uint32 pruneBelow = 0;
	mAlphabet = new uint32[abetlen];
	mOldAlphabet = new uint32[abetlen];
	uint32 countSum=0, acnt=0, cnt=0;
	alphabet::iterator ait, aend=a->end();
	mABUsedLen = 0;
	mABUnusedCount = 0;
	for(ait = a->begin(); ait != aend; ++ait) {
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

	mUseOccs = new uint32[mNumDBRows];

	mCurStats.usgCountSum = countSum;
	mCurStats.encDbSize = db->GetStdDbSize();
	mCurStats.encCTSize = ctSize;
	mCurStats.encSize = mCurStats.encDbSize + mCurStats.encCTSize;
	mCurStats.numSetsUsed = 0;
	mCurStats.alphItemsUsed = mABUsedLen;
	mPrevStats = mCurStats;

	// en nu gaan we hier een eigen db structuur maken...
	mCDB = new CoverSet*[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCDB[i] = CoverSet::Create(type, mABLen, mDB->GetRow(i), mCTLen-1); 
	}

	mIsBinnedDB = mDB->HasBinSizes();
}


// CCP Add :: Voegt `is` wel echt toe, maar set wel de mChange-vars ..
void CCPCodeTable::Add(ItemSet *is, uint64 candidateId) {
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
	mChangeItemCandidateId = candidateId;
	mChangeItem = is;
	mChangeRow = len-1;
	mChangeIter = mCT[mChangeRow]->GetList()->end(); /* goed! */
	--mChangeIter;
}

void CCPCodeTable::Del(ItemSet *is, bool zap, bool keepList) {
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
// !?! kost relatief veel tijd, met name MakeIPointUpToDate
void CCPCodeTable::CommitAdd(bool updateLog) {
	if(updateLog)
		UpdateCTLog();

	mPrevStats = mCurStats;

	// csets nu tot insPoint uptodate maken
	//uint32 *occur = mChangeItem->GetDBOccurrence();
	//uint32 numOcc = mChangeItem->GetNumDBOccurs();
	// Doe alleen de occurrences die gebruikt worden
	uint32 *occur = mUseOccs;
	uint32 numOcc = mNumUseOccs;

	uint32 len = mChangeItem->GetLength();
	uint32 insPoint = mCTLen - len + 1;
	for(uint32 r=0; r<numOcc; r++) {
		mCDB[occur[r]]->MakeIPointUpToDate(insPoint, mChangeItem, len);
	}
	mChangeItem->BackupUsage();
	mChangeItem = NULL;
	mChangeType = ChangeNone;
}
// Rolls back last added ItemSet, restores counts and stats
void CCPCodeTable::RollbackAdd() {
	if(mChangeType == ChangeAdd && mChangeItem != NULL) {
		Del(mChangeItem, true, false);
		mLastDel.is = NULL;
		mCurStats = mPrevStats;
		RollbackCounts();
		mChangeType = ChangeNone;
		mChangeItem = NULL;
	}
}
void CCPCodeTable::RollbackLastDel() {
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

void CCPCodeTable::CommitLastDel(bool zap, bool updateLog) {
	if(mChangeType == ChangeDel) {
		if(updateLog)
			UpdateCTLog();

		// insPoint op mChangeRow up2date te maken indien mChangeIter == mChangeRow->end().
		// maar dan hebben we wel een mChangeOccs nodig!
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

islist* CCPCodeTable::GetSanitizePruneList() {
	islist *ilist = new islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildSanitizePruneList(ilist);
	return ilist;
}
void CCPCodeTable::RollbackCounts() {
	if(mRollbackAlph) {
		for(uint32 j=mRollback; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->RollbackUsageCounts();
		memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
	}
}

islist* CCPCodeTable::GetSingletonList() {
	islist *ilist = new	islist();
	ItemSet *is;
	uint16 *set;
	alphabet *alph = mDB->GetAlphabet();
	uint32 alphLen = (uint32)alph->size();

	for(uint32 i=0; i<mABLen; i++) {
		set = new uint16[1];
		set[0] = i;
		is = ItemSet::Create(mDB->GetDataType(), set, 1, alphLen, mAlphabet[i], alph->find(i)->second);
		ilist->push_back(is);
	}
	return ilist;
}

islist* CCPCodeTable::GetItemSetList() {
	islist *ilist = new islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildItemSetList(ilist);
	return ilist;
}

islist* CCPCodeTable::GetPostPruneList() {
	islist *pruneList = new islist();
	UpdatePostPruneList(pruneList, pruneList->begin());
	return pruneList;
}

void CCPCodeTable::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList, after);
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildPostPruneList(pruneList, after);
}
void CCPCodeTable::AddOneToEachUsageCount() {
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

	mLaplace++;
}
void CCPCodeTable::ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset) {
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
void CCPCodeTable::AddSTSpecialCode() {
	mCurStats.usgCountSum++;
	mSTSpecialCode = true;
}
double	CCPCodeTable::CalcEncodedDbSize(Database *db) {
	uint32 numRows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	double size = 0.0;

	for(uint32 i=0; i<numRows; i++)
		size += CalcTransactionCodeLength(rows[i]);
	return size;
}
double	CCPCodeTable::CalcCodeTableSize() {
	islist::iterator cit,cend;
	double codeLen = 0;
	mCurStats.encCTSize = 0;

	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				uint32 curcnt = ((ItemSet*)(*cit))->GetUsageCount();
				if(curcnt == 0) continue;
				codeLen = CalcCodeLength(curcnt, mCurStats.usgCountSum);
				mCurStats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength() + codeLen;
			}
		}
	}
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		uint32 curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		codeLen = CalcCodeLength(curcnt, mCurStats.usgCountSum);
		mCurStats.encCTSize += mStdLengths[i];
		mCurStats.encCTSize += codeLen;
	}
	if(mSTSpecialCode)
		mCurStats.encCTSize += CalcCodeLength(1, mCurStats.usgCountSum);
	return mCurStats.encCTSize;
}

void CCPCodeTable::CoverDB(CoverStats &stats) {
	if(mIsBinnedDB)			// switch binned/notbinned
		CoverBinnedDB(stats);
	else
		CoverNoBinsDB(stats);
}

void CCPCodeTable::CoverNoBinsDB(CoverStats &stats) {
	CoverSet *cset;
	ItemSet *iset;
	uint32 *occur, numOcc, insPoint, loadPoint;
	islist::iterator cit,cend;

	if(mChangeType == ChangeAdd) {
		uint32 mj = mChangeRow;
		uint32 len = mj+1;
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();
		insPoint = mCTLen - len + 1;
		mNumUseOccs = 0;

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			loadPoint = cset->GetMostUpToDateIPoint(insPoint);
			if(loadPoint != insPoint) {
				cset->LoadMask(loadPoint);
				// zelf tot insPoint uitrekenen, masks onderweg opslaan. 
				for(uint32 j=mCTLen-loadPoint-1; j>=mj; --j) {
					if(mCT[j] != NULL) {
						cend = mCT[j]->GetList()->end();
						if(j==mj)
							--cend;
						for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
							iset = *cit;
							if(iset->GetUsageCount()==0)
								continue;
							cset->Cover(j+1,iset);
						}
					}
					// mask opslaan
					cset->StoreMask(++loadPoint);
				}
				if(cset->CanCover(mj+1, mChangeItem)) {									// nu m nog even doen
					mUseOccs[mNumUseOccs++] = i;
				}
			} else if(cset->CanCoverIPoint(insPoint, mChangeItem)) {					// nu m ff checken
				mUseOccs[mNumUseOccs++] = i;
				cset->LoadMask(insPoint);
			}
		}

		if(mNumUseOccs == 0) {
			mRollbackAlph = false;	// geen counts rollbacken
			return;
		}
		stats.usgCountSum += mNumUseOccs;
		mChangeItem->AddUsageCount(mNumUseOccs);
		// M heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = mj-1; // counts van rijen na mj backuppen
		mRollbackAlph = true;
		for(uint32 j=mj-1; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		uint32 i;
		for(uint32 r=0; r<mNumUseOccs; r++) {
			i=mUseOccs[r];
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

			//  Totale cover met m
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
			//stats.countSum += ((BM128CoverSet*)(cset))->AdjustAlphabet(mABLen, mAlphabet);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
		CalcTotalSize(stats);
	} else if(mChangeType == ChangeDel) {
		uint32 mj = mChangeRow;
		uint32 len = mj+1;
		stats = mCurStats;

		// backup counts vanaf mChangeRow		!!! split: vanaf mChangeIter++, en verder
		mRollback = mj;
		mRollbackAlph = true;
		for(uint32 j=mj; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();
		insPoint = mCTLen - len;	// insPoint -voor- mChangeRow

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...

		uint32 r,i;
		for(r=0; r<numOcc; r++) {
			i=occur[r];
			cset = mCDB[i];
			loadPoint = cset->LoadMask(insPoint);

			if(loadPoint != insPoint) {
				// Vanaf loadPoint tot mChangeRow
				for(uint32 j=mCTLen-loadPoint-1; j>mj; --j) {
					if(mCT[j] != NULL) {
						cend = mCT[j]->GetList()->end();
						for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
							iset = *cit;
							if(iset->GetUsageCount()==0)
								continue;
							cset->Cover(j+1,iset);
						}
					}
					// Onderweg masks opslaan
					cset->StoreMask(++loadPoint);
				}
			} else
				cset->SetIPointUpToDate(insPoint);

			// mChangeRow tot mChangeIter om cset op te bouwen
			cend = mChangeIter;
			for(cit = mCT[mj]->GetList()->begin(); cit != cend; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)		// deze rij, en verder is gebackuped
					continue;
				cset->Cover(mj+1,iset);
			}
			// Alleen rest doen als mChangeItem ook echt gebruikt wordt
			if(cset->Cover(mj+1,mChangeItem)) {
				cset->BackupMask();

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
				cset->Uncover(mj+1, mChangeItem);

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
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
		CalcTotalSize(stats);
	} else if(mChangeType == ChangeNone) {
		// init counts
		for(uint32 j=mMaxIdx; j>0; --j)
			if(mCT[j] != NULL)
				mCT[j]->ResetUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;
		if(mSTSpecialCode)
			++stats.usgCountSum;

		mRollback = mMaxIdx;
		mRollbackAlph = true;

		// full cover db
		loadPoint = 0;
		for(uint32 i=0; i<mNumDBRows; i++) {
			cset = mCDB[i];
			cset->ResetMask();
			for(uint32 j=mMaxIdx; j>0; j--) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddOneToUsageCount();
							stats.usgCountSum++;
						}
					}
				}
				// Store Mask		??? wel/niet doen?
				cset->StoreMask(++loadPoint);
			}
			// alphabet items
			stats.usgCountSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
		CalcTotalSize(stats);

	} else {
		THROW("Unknown code table ChangeType");
	}
}

void CCPCodeTable::CoverBinnedDB(CoverStats &stats) {
	CoverStats bakstats = stats;
	CoverSet *cset;
	ItemSet *iset;
	uint32 *occur, numOcc, insPoint, loadPoint, binSize, numUseCount;
	islist::iterator cit,cend;

	if(mChangeType == ChangeAdd) {
		uint32 mj = mChangeRow;
		uint32 len = mj+1;
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();
		insPoint = mCTLen - len + 1;
		mNumUseOccs = 0;
		numUseCount = 0;

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			loadPoint = cset->GetMostUpToDateIPoint(insPoint);
			if(loadPoint != insPoint) {
				cset->LoadMask(loadPoint);
				// zelf tot insPoint uitrekenen, masks onderweg opslaan. 
				for(uint32 j=mCTLen-loadPoint-1; j>=mj; --j) {
					if(mCT[j] != NULL) {
						cend = mCT[j]->GetList()->end();
						if(j==mj)
							--cend;
						for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
							iset = *cit;
							if(iset->GetUsageCount()==0)
								continue;
							cset->Cover(j+1,iset);
						}
					}
					// mask opslaan
					cset->StoreMask(++loadPoint);
				}
				if(cset->CanCover(mj+1, mChangeItem)) {									// nu m nog even doen
					binSize = mDB->GetRow(i)->GetUsageCount();
					mUseOccs[mNumUseOccs++] = i;
					numUseCount += binSize;
				}
			} else if(cset->CanCoverIPoint(insPoint, mChangeItem)) {					// alleen m ff checken
				binSize = mDB->GetRow(i)->GetUsageCount();
				mUseOccs[mNumUseOccs++] = i;
				numUseCount += binSize;
				cset->LoadMask(insPoint);
			}
		}

		if(mNumUseOccs == 0) {
			mRollbackAlph = false;	// geen counts rollbacken
			stats = bakstats;
			return;
		}
		stats.usgCountSum += numUseCount;
		mChangeItem->AddUsageCount(numUseCount);
		// M heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = mj-1; // counts van rijen na mj backuppen
		mRollbackAlph = true;
		for(uint32 j=mj-1; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		uint32 i;
		for(uint32 r=0; r<mNumUseOccs; r++) {
			i=mUseOccs[r];
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
							stats.usgCountSum -= binSize;
						}
					}
				}
			}
			stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet, binSize);

			//  Totale cover met m
			cset->RestoreMask();
			cset->Cover(mj+1, mChangeItem);

			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddUsageCount(binSize);
							stats.usgCountSum += binSize;
						}
					}
				}
			}
			stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...

	} else if(mChangeType == ChangeDel) {
		uint32 mj = mChangeRow;
		uint32 len = mj+1;

		// backup counts vanaf mChangeRow		!!! split: vanaf mChangeIter++, en verder
		mRollback = mj;
		mRollbackAlph = true;
		for(uint32 j=mj; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();
		insPoint = mCTLen - len;	// insPoint -voor- mChangeRow

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...

		uint32 r,i;
		for(r=0; r<numOcc; r++) {
			i=occur[r];
			cset = mCDB[i];
			loadPoint = cset->LoadMask(insPoint);
			binSize = mDB->GetRow(i)->GetUsageCount();

			if(loadPoint != insPoint) {
				// Vanaf loadPoint tot mChangeRow
				for(uint32 j=mCTLen-loadPoint-1; j>mj; --j) {
					if(mCT[j] != NULL) {
						cend = mCT[j]->GetList()->end();
						for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
							iset = *cit;
							if(iset->GetUsageCount()==0)
								continue;
							cset->Cover(j+1,iset);
						}
					}
					// Onderweg masks opslaan
					cset->StoreMask(++loadPoint);
				}
			} else
				cset->SetIPointUpToDate(insPoint);

			// mChangeRow tot mChangeIter om cset op te bouwen
			cend = mChangeIter;
			for(cit = mCT[mj]->GetList()->begin(); cit != cend; ++cit) {
				iset = *cit;
				if(iset->GetPrevUsageCount()==0)		// deze rij, en verder is gebackuped
					continue;
				cset->Cover(mj+1,iset);
			}
			// Alleen rest doen als mChangeItem ook echt gebruikt wordt
			if(cset->Cover(mj+1,mChangeItem)) {
				cset->BackupMask();

				// :: Herstelcover (--) 
				//	-: Herstel staartje mChangeRow
				cend = mCT[mj]->GetList()->end();
				cit = mChangeIter;
				++cit;
				for(; cit != cend; ++cit) {
					if(cset->Cover(mj+1,*cit)) {
						((ItemSet*)(*cit))->SupUsageCount(binSize);
						stats.usgCountSum -= binSize;
					}
				}
				//	-: Herstel na mChangeRow
				for(uint32 j=mj-1; j>=1; --j) {
					if(mCT[j] != NULL) {
						cend = mCT[j]->GetList()->end();
						for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
							if(cset->Cover(j+1,*cit)) {
								((ItemSet*)(*cit))->SupUsageCount(binSize);
								stats.usgCountSum -= binSize;
							}
						}
					}
				}
				//	-: Herstel alphabet counts
				stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet, binSize);

				cset->RestoreMask();
				cset->Uncover(mj+1, mChangeItem);

				// mChangeRow vanaf mChangeIter om cover zonder te bepalen
				cit = mChangeIter;
				++cit;
				cend = mCT[mj]->GetList()->end();
				for(; cit != cend; ++cit) {
					if(cset->Cover(mj+1,*cit)) {
						((ItemSet*)(*cit))->AddUsageCount(binSize);
						stats.usgCountSum += binSize;
					}
				}
				// na mChangeRow
				for(uint32 j=mj-1; j>=1; --j) {	
					if(mCT[j] != NULL) {
						cend = mCT[j]->GetList()->end();
						for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
							if(cset->Cover(j+1,*cit)) {
								((ItemSet*)(*cit))->AddUsageCount(binSize);
								stats.usgCountSum += binSize;
							}
						}
					}
				}

				// alphabet items
				stats.usgCountSum += cset->CoverAlphabet(mABLen, mAlphabet, binSize);
			}
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
		if(mSTSpecialCode)
			++stats.usgCountSum;

		mRollback = mMaxIdx;
		mRollbackAlph = true;

		// full cover db
		loadPoint = 0;
		for(uint32 i=0; i<mNumDBRows; i++) {
			cset = mCDB[i];
			cset->ResetMask();
			binSize = mDB->GetRow(i)->GetUsageCount();

			for(uint32 j=mMaxIdx; j>0; j--) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							((ItemSet*)(*cit))->AddUsageCount(binSize);
							stats.usgCountSum += binSize;
						}
					}
				}
				// Store Mask		??? wel/niet doen?
				cset->StoreMask(++loadPoint);
			}
			// alphabet items
			stats.usgCountSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
	} else
		THROW("Unknown code table ChangeType");

	CalcTotalSize(stats);
}


void CCPCodeTable::CalcTotalSize(CoverStats &stats) {
	islist::iterator cit,cend;

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
				stats.encDbSize += (curcnt-mLaplace) * codeLen;
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
		stats.encDbSize += (curcnt-mLaplace) * codeLen;
		stats.encCTSize += mStdLengths[i];
		stats.encCTSize += codeLen;
	}

	if(mSTSpecialCode)
		stats.encCTSize += CalcCodeLength(1, stats.usgCountSum);
	stats.encSize = stats.encDbSize + stats.encCTSize;
}

// Assume Laplace solution (when required)
double CCPCodeTable::CalcTransactionCodeLength(ItemSet *row) {
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
double CCPCodeTable::CalcTransactionCodeLength(ItemSet *row, double *stLengths) {
	if(!mSTSpecialCode)
		return CalcTransactionCodeLength(row); // Assume Laplace

	// (Non-Laplace) ST encoding for freak items

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

	bool useST = false;
	// alphabet items
	for(uint32 i=0; i<mABLen; i++)
		if(mCoverSet->IsItemUncovered(i))
			if(mAlphabet[i] == 0) {				
				useST = true;
				codeLen += stLengths[i];
			} else
				codeLen += CalcCodeLength(mAlphabet[i], mCurStats.usgCountSum);
	if(useST)
		codeLen += CalcCodeLength(1, mCurStats.usgCountSum);
	if(codeLen == 0.0)
		bool strange = true;
	return codeLen;
}

void CCPCodeTable::CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) {
	// init counts
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->ResetUsages();
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i] = 0;

	stats.usgCountSum = 0;
	if(mSTSpecialCode)
		++stats.usgCountSum;
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
		curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += mStdLengths[i] + codeLen;
	}
	if(mSTSpecialCode)
		stats.encCTSize += CalcCodeLength(1, stats.usgCountSum);

	stats.encSize = stats.encDbSize + stats.encCTSize;
}
void CCPCodeTable::SetAlphabetCount(uint32 item, uint32 count) {
	mAlphabet[item] = count;
}
uint32 CCPCodeTable::GetAlphabetCount(uint32 item) {
	return mAlphabet[item];
}
void CCPCodeTable::UpdateCTLog() {
	if(mCTLogFile != NULL) {
		/*
		+ candnr is.len:is.set (cnt,sup)
		3: el+cnt el-cnt
		*/
		if(mChangeType == ChangeAdd) {
			fprintf(mCTLogFile, "+ %d:%s%" I64d "\n", mChangeItem->GetLength(), mChangeItem->ToCodeTableString().c_str(), mChangeItemCandidateId);
		} else if(mChangeType == ChangeDel) {
			fprintf(mCTLogFile, "- %d:%s\n", mChangeItem->GetLength(), mChangeItem->ToCodeTableString().c_str());
		} else if(mChangeType == ChangeNone) {
			fprintf(mCTLogFile, "0\n");
		}

		if(mChangeType == ChangeAdd || mChangeType == ChangeDel) {
			islist::iterator cit,cend;
			uint32 elId = 0;
			fprintf(mCTLogFile, "  ");
			for(uint32 j=mMaxIdx; j>mChangeRow; j--)
				if(mCT[j] != NULL)
					elId += (uint32) mCT[j]->GetList()->size();
			for(uint32 j=mChangeRow; j>=1; j--) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						uint32 prev = (*cit)->GetPrevUsageCount();
						uint32 curr = (*cit)->GetUsageCount();
						if(prev != curr)
							fprintf(mCTLogFile, "%d%c%d ", elId, prev<curr?'+':'-', prev<curr?curr-prev:prev-curr);
						elId++;
					}
				}
			}
			fprintf(mCTLogFile, "\n");
			fflush(mCTLogFile);
			// fprintf de wijzigingen.
		}
	}
}
