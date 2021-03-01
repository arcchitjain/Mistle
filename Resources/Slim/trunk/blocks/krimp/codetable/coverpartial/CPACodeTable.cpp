#include "../../../../global.h"

#include <stdlib.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <itemstructs/CoverNode.h>
#include <isc/ItemSetCollection.h>

#include "../CodeTable.h"
#include "../CTISList.h"

#include "CPACodeTable.h"

CPACodeTable::CPACodeTable() : CodeTable() {
	mCoverSet = NULL;
	mCDB = NULL;
	mCT = NULL;
	mNumDBRows = 0;
	mCTLen = 0;
	mMaxIdx = 0;
	mAllRows = NULL;

	mAlphabet = NULL;
	mABLen = 0;
	mABUsedLen = 0;

	mChangeType = ChangeNone;

	mCurTree = NULL;
	mPrevTree = NULL;
	mCTN = NULL;
	mAuxAr = NULL;
	mABItemSets = NULL;
}

CPACodeTable::~CPACodeTable() {
	// output cover tree
	string fullpath = (Bass::GetExpDir() + "ctree.txt"); 
	FILE* fp = fopen(fullpath.c_str(), "w");
	CoverDB(mCurStats);
	mCurTree->PrintTree(fp, 0);
	fclose(fp);

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

	delete mCurTree;
	delete mPrevTree;
	delete[] mCTN;
	delete[] mAuxAr;
	for(uint32 i=0; i<mABUsedLen; i++)
		delete mABItemSets[i];
	delete[] mABItemSets;
}

void CPACodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
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
	uint32 abetlen = mABLen = (uint32) a->size();
	//uint32 pruneBelow = db->GetPrunedBelow();
	uint32 pruneBelow = 0;
	mAlphabet = new uint32[abetlen];
	mOldAlphabet = new uint32[abetlen];
	uint32 countSum = 0, acnt=0;
	uint32 cnt = 0;
	alphabet::iterator ait, aend=a->end();
	mABUsedLen = 0;
	mABUnusedCount = 0;
	mABItemSets = new ItemSet*[mABLen];
	for(ait = a->begin(); ait != aend; ++ait) {
		if(ait->first != cnt)
			THROW("Ja! Maar! Dat kan zo maar niet!");
		acnt = ait->second;
		mABItemSets[cnt] = ItemSet::CreateSingleton(type, ait->first, mABLen, acnt, acnt);
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
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

	mNumDBRows = mDB->GetNumRows();
	mAllRows = new uint32[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++)
		mAllRows[i] = i;
	mAuxAr = new uint16[mDB->GetMaxSetLength()];

	// en nu gaan we hier een eigen db structuur maken...
	mCDB = new CoverSet *[mNumDBRows];
	mCTN = new CoverNode *[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCDB[i] = CoverSet::Create(type, mABLen, mDB->GetRow(i));
		mCTN[i] = NULL;
	}

	mIsBinnedDB = mDB->HasBinSizes();
	
	// Stats need to be deduced from cover, no such thing as a free lunch anymore.
//	mCurTree = new CoverNode();
	CoverDB(mCurStats);
	mPrevStats = mCurStats;
//	mPrevTree = mCurTree->Clone();
}


// CPA Add :: Voegt `is` wel echt toe, maar set wel de mChange-vars ..
void CPACodeTable::Add(ItemSet *is, uint64 candidateId) {
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

void CPACodeTable::Del(ItemSet *is, bool zap, bool keepList) {
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
void CPACodeTable::CommitAdd(bool updateLog) {
	if(updateLog)
		UpdateCTLog();
	mPrevStats = mCurStats;
//	delete mPrevTree;
//	mPrevTree = mCurTree;
//	mCurTree = new CoverNode();
	mChangeItem = NULL;
	mChangeType = ChangeNone;
}

// Rolls back last added ItemSet, restores counts and stats
void CPACodeTable::RollbackAdd() {
	if(mChangeType == ChangeAdd && mChangeItem != NULL) {
		Del(mChangeItem, true, false);
		mLastDel.is = NULL;
		mCurStats = mPrevStats;
		if(mRollbackAlph == true) {
//			delete mCurTree;
//			mCurTree = mPrevTree;
		}
		RollbackCounts();
		mChangeType = ChangeNone;
		mChangeItem = NULL;
	}
}
void CPACodeTable::RollbackLastDel() {
	if(mChangeType == ChangeDel && mLastDel.is != NULL)	{
		// Er is niet echt gedelete, alleen change en delinfo moet gezapt
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeRow = -1;

		++mCurNumSets;

		mLastDel.is = NULL;
		//RollbackCounts(); // gebeurt via PrunePost aanroep indien
	}
}

void CPACodeTable::CommitLastDel(bool zap, bool updateLog) {
	if(mChangeType == ChangeDel) {
		if(updateLog)
			UpdateCTLog();

		mCT[mChangeRow]->GetList()->erase(mChangeIter);
		if(zap)
			delete mChangeItem;
		mLastDel.is = NULL;
		mCurStats = mPrunedStats;
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeRow = -1;
	}
}

void CPACodeTable::CommitAllDels(bool updateLog) {
	THROW_DROP_SHIZZLE();
}
void CPACodeTable::RollbackAllDels() {
	THROW_DROP_SHIZZLE();
}
void CPACodeTable::RollbackCounts() {
	for(uint32 j=mMaxIdx; j>=mRollback; --j)
		if(mCT[j] != NULL)
			mCT[j]->RollbackUsageCounts();
	if(mRollbackAlph)
		memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
}

islist* CPACodeTable::GetItemSetList() {
	islist *ilist = new islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildItemSetList(ilist);
	return ilist;
}

islist* CPACodeTable::GetPrePruneList() {
	islist *ilist = new	islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildPrePruneList(ilist);
	return ilist;
}
islist* CPACodeTable::GetSanitizePruneList() {
	islist *ilist = new	islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildSanitizePruneList(ilist);
	return ilist;
}
islist* CPACodeTable::GetPostPruneList() {
	islist *pruneList = new islist();
	UpdatePostPruneList(pruneList, pruneList->begin());
	return pruneList;
}

void CPACodeTable::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList, after);
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildPostPruneList(pruneList, after);
}

void CPACodeTable::CoverDB(CoverStats &stats) {
	if(mIsBinnedDB)			// switch binned/notbinned
		CoverBinnedDB(stats);
	else
		CoverNoBinsDB(stats);
}

void CPACodeTable::CoverNoBinsDB(CoverStats &stats) {
	CoverStats bakstats = stats;
	CoverSet *cset;
	CoverNode *cnode;
	ItemSet *iset;
	uint32 *occur, numOcc;
	islist::iterator cit,cend;

	if(false && mChangeType == ChangeAdd) {
		uint32 mj = mChangeItem->GetLength()-1;
		for(uint32 j=mMaxIdx; j>=mj; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();

		mChangeItem->SetPrevUsageCount(1);													// dus na ResetCount prevCount != 0
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();
		for(uint32 r=0; r<numOcc; r++) {
			cnode = mCurTree;
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
						if(cset->Cover(j+1,iset)) {
							cnode = cnode->GetChildNode(iset);						// keep current node up-to-date
						}
					}
				}
			}
			if(cset->CanCover(mj+1, mChangeItem)) {									// nu m nog even doen
				mChangeItem->AddOneToUsageCount();
				stats.usgCountSum++;
			}
			mCTN[i] = cnode;
		}
		mChangeItem->SetPrevUsageCount(0);
		if(mChangeItem->Unused()) {
			mRollback = mj;
			mRollbackAlph = false;
			stats = bakstats;
			return;
		}
		// mChangeItem heeft count > 0, dus we moeten de boel echt opnieuw uitrekenen...
		mRollback = 1;
		mRollbackAlph = true;
		for(uint32 j=mj-1; j>=1; --j)
			if(mCT[j] != NULL)
				mCT[j]->BackupUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

		// !!! useOcc ipv numOcc bouwen
		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			cnode = mCTN[i];

			cset->BackupMask();
			//	herstelcover (--) 
			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							if(cnode != NULL)
								cnode = cnode->SupCoverNode(*cit, stats.numNests);
							((ItemSet*)(*cit))->SupOneFromUsageCount();
							stats.usgCountSum--;
						}
					}
				}
			}
			stats.usgCountSum -= UncoverAlphabet(cset, cnode);
			//stats.countSum -= cset->UncoverAlphabet(mABLen, mAlphabet);

			//  Totale cover, ditmaal met m 
			cset->RestoreMask();
			if(cset->Cover(mj+1, mChangeItem))
				cnode = mCTN[i]->AddCoverNode(mChangeItem, stats.numNests);

			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							cnode = cnode->AddCoverNode(*cit, stats.numNests);
							((ItemSet*)(*cit))->AddOneToUsageCount();
							stats.usgCountSum++;
						}
					}
				}
			}
			stats.usgCountSum += CoverAlphabet(cset, cnode, stats.numNests);
			//stats.countSum += cset->CoverAlphabet(mABLen, mAlphabet);
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...

	} else if(false && mChangeType == ChangeDel) {
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
			// !!! stats.countSum -= cset->UncoverAlphabet(mABLen, mAlphabet);

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
			stats.usgCountSum += CoverAlphabet(cset, NULL, stats.numNests);
		}
	} else if(true || mChangeType == ChangeNone) {
		// init counts
		for(uint32 j=mMaxIdx; j>0; --j)
			if(mCT[j] != NULL)
				mCT[j]->ResetUsages();
		for(uint32 j=0; j<mABLen; j++)
			mABItemSets[j]->ResetUsage();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;
		stats.numNests = mABUnusedCount;

		mRollback = 1;
		mRollbackAlph = true;

		delete mCurTree;
		mCurTree = new CoverNode();
		
		// full cover db
		islist::iterator cit,cend;
		for(uint32 i=0; i<mNumDBRows; i++) {
			cnode = mCurTree;
			mCDB[i]->ResetMask();
			for(uint32 j=mMaxIdx; j>0; j--) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(mCDB[i]->Cover(j+1,*cit)) {
							cnode = cnode->AddCoverNode(*cit, stats.numNests);
							((ItemSet*)(*cit))->AddOneToUsageCount();
							stats.usgCountSum++;
						}
					}
				}
			}
			// alphabet items
			stats.usgCountSum += CoverAlphabet(mCDB[i], cnode, stats.numNests);
			//stats.countSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet);
			mCTN[i] = NULL;
		}
	} else {
		THROW("Unknown code table ChangeType");
	}

	CalcTotalSize(stats);
}

void CPACodeTable::CalcTotalSize(CoverStats &stats) {
	islist::iterator cit,cend;

	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	if(true) {
		// db:  code { code { code code | numRows }, code { code | numRows } }
		// ct: [(set, code)]
		// Code length dependent on # times it's actually written over # of codes written

		double codeLen = 0;
		// # rows als fixed num bits aan einde van nesting
		stats.encDbSize = 2 * CalcCodeLength((uint32) stats.numNests, stats.numNests * 3);

		for(uint32 j=mMaxIdx; j>=1; j--) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					uint32 curuse = ((ItemSet*)(*cit))->GetUsageArea();
					if(curuse == 0) continue;
					stats.numSetsUsed++;
					codeLen = CalcCodeLength(curuse, stats.numNests * 3);
					stats.encDbSize += curuse * codeLen;
					stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength();	// !!! moet ook nog op de schop
					stats.encCTSize += codeLen;
				}
			}
		}
		// Alphabet
		for(uint32 i=0; i<mABLen; i++) {
			uint32 curuse = mABItemSets[i]->GetUsageArea();
			if(curuse == 0) continue;
			stats.alphItemsUsed++;
			codeLen = CalcCodeLength(curuse, stats.numNests * 3);
			stats.encDbSize += curuse * codeLen;
			stats.encCTSize += mStdLengths[i];
			stats.encCTSize += codeLen;
		}
	} else if(false) {
		// [Set | code] table, [ code + constant ] database encoding
		// Code length dependent on # times it's actually written
		double codeLen = 0;
		stats.encDbSize = stats.numNests * (log((double)mNumDBRows) / log(2.0));
		for(uint32 j=mMaxIdx; j>=1; j--) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					uint32 curuse = ((ItemSet*)(*cit))->GetUsageArea();
					if(curuse == 0) continue;
					stats.numSetsUsed++;
					//codeLen = CalcCodeLength(curuse, stats.numNests);
					codeLen = CalcCodeLength(((ItemSet*)(*cit))->GetUsageCount(), stats.usgCountSum);
					stats.encDbSize += curuse * codeLen;
					stats.encCTSize += ((ItemSet*)(*cit))->GetStandardLength();	// !!! moet ook nog op de schop
					stats.encCTSize += codeLen;
				}
			}
		}
		// Alphabet
		for(uint32 i=0; i<mABLen; i++) {
			uint32 curuse = mABItemSets[i]->GetUsageArea();
			if(curuse == 0) continue;
			stats.alphItemsUsed++;
			//codeLen = CalcCodeLength(curuse, stats.numNests);
			codeLen = CalcCodeLength(mABItemSets[i]->GetUsageCount(), stats.usgCountSum);
			stats.encDbSize += curuse * codeLen;
			stats.encCTSize += mStdLengths[i];
			stats.encCTSize += codeLen;
		}

	} else {
		// {Set | code} table, [ code ] database encoding
		// Code length dependent on # times it's actually written
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
	}
	stats.encSize = stats.encDbSize + stats.encCTSize;

}

void CPACodeTable::CoverBinnedDB(CoverStats &stats) {
	CoverStats bakstats = stats;
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
			stats = bakstats;
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

double CPACodeTable::CalcTransactionCodeLength(ItemSet *row) {
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

void	CPACodeTable::CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) {
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
void CPACodeTable::SetAlphabetCount(uint32 item, uint32 count) {
	mAlphabet[item] = count;
}
uint32 CPACodeTable::GetAlphabetCount(uint32 item) {
	return mAlphabet[item];
}
void CPACodeTable::UpdateCTLog() {
	if(mChangeType == ChangeAdd) {
		fprintf(mCTLogFile, "+ %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeDel) {
		fprintf(mCTLogFile, "- %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeNone) {
		fprintf(mCTLogFile, "0\n");
	}
}
void CPACodeTable::AddOneToEachUsageCount() {
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

void CPACodeTable::ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset) {
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

uint32 CPACodeTable::CoverAlphabet(CoverSet *cset, CoverNode *cnode, uint64 &numNests) {
	uint32 numUncovered = cset->GetNumUncovered();
//	CoverNode *ncnode;
	if(numUncovered > 0) {
		cset->GetUncoveredAttributesIn(mAuxAr);
		for(uint32 i=0; i<numUncovered; i++) {
			cnode = cnode->AddCoverNode(mABItemSets[mAuxAr[i]], numNests);
			mABItemSets[mAuxAr[i]]->AddOneToUsageCount();
			mAlphabet[mAuxAr[i]]++;
		}
	}
	return numUncovered;
}
uint32 CPACodeTable::UncoverAlphabet(CoverSet *cset, CoverNode *cnode) {
	uint32 numUncovered = cset->GetNumUncovered();
	if(numUncovered > 0) {
		cset->GetUncoveredAttributesIn(mAuxAr);
		for(uint32 i=0; i<numUncovered; i++) {
			// nodes zijn reeds gezapped (dubbelcheck!!!)
			mABItemSets[mAuxAr[i]]->SupOneFromUsageCount();
			mAlphabet[mAuxAr[i]]--;
		}
	}
	return numUncovered;
}
