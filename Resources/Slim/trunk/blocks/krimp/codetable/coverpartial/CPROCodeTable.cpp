#include "../../../../global.h"

#include <stdlib.h>

#include <RandomUtils.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>

#include "../CTISList.h"

#include "CPCodeTable.h"
#include "CPROCodeTable.h"

CPROCodeTable::CPROCodeTable() : CPCodeTable() {
	mCTList = NULL;
	mCTCodeLengths = NULL;
	mABCodeLengths = NULL;
	mCTArAr = NULL;
	mCTArNum = NULL;
	mCTArMax = NULL;
	mCTArLen = 0;
	mChangeType = ChangeNone;
	mOverlap = false;
	mUseOccs = NULL;
	mNumUseOccs = 0;
	mNumDBRows = 0;
}

CPROCodeTable::CPROCodeTable(const string &name) : CPCodeTable() {
	mCTList = NULL;
	mCTCodeLengths = NULL;
	mABCodeLengths = NULL;
	mCTArAr = NULL;
	mCTArNum = NULL;
	mCTArMax = NULL;
	mCTArLen = 0;
	mChangeType = ChangeNone;
	mUseOccs = NULL;
	mNumUseOccs = 0;
	mNumDBRows = 0;

	// coverpartial-orderly- stack random length etc
	if(name.find("-random",GetConfigBaseName().length()) != string::npos) {
		mOrder = CTRandomOrder;
		RandomUtils::Init();
	} else if(name.find("-length",GetConfigBaseName().length()) != string::npos) {
		if(name.find("-length-inv",GetConfigBaseName().length()) != string::npos)
			mOrder = CTLengthInverseOrder;
		else 
			mOrder = CTLengthOrder;
	} else if(name.find("-area",GetConfigBaseName().length()) != string::npos) {
		if(name.find("-area-inv",GetConfigBaseName().length()) != string::npos)
			mOrder = CTAreaInverseOrder;
		else 
			mOrder = CTAreaOrder;
	} else {
		if(name.find("-stack-inv",GetConfigBaseName().length()) != string::npos)
			mOrder = CTStackInverseOrder;
		else
			mOrder = CTStackOrder;
	}

	if(name.find("-ov",GetConfigBaseName().length()) != string::npos) {
		mOverlap = true;
		mCoverFunc = &CoverSet::CoverWithOverlap;
		mNumCoverFunc = &CoverSet::NumCoverWithOverlap;
		mCanCoverFunc = &CoverSet::CanCoverWithOverlap;

		//mNumCoverFunc = &CoverSet::NumCover;
		//mCoverFunc = &CoverSet::CoverWithOverlapNoSubsetCheck;
		//mCanCoverFunc = &CoverSet::CanCoverWithOverlapNoSubsetCheck;	// geen check nodig, want loop gaat toch alleen over dbOcc
	} else {
		mOverlap = false;
		mCoverFunc = &CoverSet::Cover;
		mNumCoverFunc = &CoverSet::NumCover;
		mCanCoverFunc = &CoverSet::CanCover;
	}

	if(name.find("-ic",GetConfigBaseName().length()) != string::npos) {
		mCLStylee = CLItemCounts;
	} else
		mCLStylee = CLCounts;
}

string CPROCodeTable::GetShortName() {
	string s = mOverlap ? "-ov" : "";
	if(mOrder == CTRandomOrder)
		return "cpror" + s;
	else if (mOrder == CTLengthOrder)
		return "cprol" + s;
	else if (mOrder == CTLengthInverseOrder)
		return "cproli" + s;
	else if (mOrder == CTAreaOrder)
		return "cproa" + s;
	else if (mOrder == CTAreaInverseOrder)
		return "cproai" + s;
	else if (mOrder == CTStackInverseOrder) 
		return "cprosi" + s;
	else /* if (mOrder == CTStackOrder) */
		return "cpros" + s;
}
string CPROCodeTable::GetConfigName() {
	string s;
	switch(mOrder) {
		case CTRandomOrder:			s = "-random";		break;
		case CTLengthOrder:			s = "-length";		break;
		case CTLengthInverseOrder:	s = "-length-inv";	break;
		case CTAreaOrder:			s = "-area";		break;
		case CTAreaInverseOrder:	s = "-area-inv";	break;
		case CTStackOrder:			s = "-stack";		break;
		case CTStackInverseOrder:	s = "-stack-inv";	break;
		default: break;
	}
	return GetConfigBaseName() + s + (mOverlap ? "-ov" : "") + (mCLStylee == CLItemCounts ? "-ic" : "");
}

CPROCodeTable::~CPROCodeTable() {
	islist::iterator cit, cend=mCTList->end();
	for(cit = mCTList->begin(); cit != cend; ++cit)
		delete *cit;
	delete mCT;
	delete[] mUseOccs;
	for(uint32 i=0; i<mNumDBRows; i++) {
		delete[] mCTArAr[i];
	}
	delete[] mCTArAr;
	delete[] mCTArNum;
	delete[] mCTArMax;
	mCTList = NULL;
	delete[] mCTCodeLengths;
	delete[] mABCodeLengths;
}


void CPROCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CPCodeTable::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);
	// 'CT in a List' stuffs
	mCT = new CTISList();
	mCTList = mCT->GetList();
	mCurNumSets = 0;

	mNumDBRows = mDB->GetNumRows();
	mUseOccs = new uint32[mNumDBRows];

	mCTArLen = 100;
	mCTArAr = new ItemSet**[mNumDBRows];
	mCTArMax = new uint32[mNumDBRows];
	mCTArNum = new uint32[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCTArAr[i] = new ItemSet*[mCTArLen];
		mCTArNum[i] = 0;
		mCTArMax[i] = 0;
	}
}


// CPO Add :: Voegt `is` wel echt toe, maar set wel de mChange-vars ..
void CPROCodeTable::Add(ItemSet *is, uint64 candidateId) {
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

	if(mCurNumSets > mCTArLen) {
		mCTArLen = mCTArLen * 2;
		for(uint32 i=0; i<mNumDBRows; i++) {
			delete[] mCTArAr[i];
			mCTArAr[i] = new ItemSet*[mCTArLen];
		}
	}

	mChangeType = ChangeAdd;
	mChangeItem = is;
}

void CPROCodeTable::Del(ItemSet *is, bool zap, bool keepList) {
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
void CPROCodeTable::RollbackAdd() {
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
void CPROCodeTable::RollbackLastDel() {
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

void CPROCodeTable::CommitLastDel(bool zap, bool updateLog) {
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

void CPROCodeTable::CommitAllDels(bool updateLog) {
	THROW_DROP_SHIZZLE();
}
void CPROCodeTable::RollbackAllDels() {
	THROW_DROP_SHIZZLE();
}
void CPROCodeTable::RollbackCounts() {
	// Terugrollen van de counts moet -tot/met- de mRollback rij
	// aka, hier -tot- mRollback (die op list->end() of op mChangeIter ingesteld zal staan)
	for(islist::iterator cit = mCTList->begin(); cit != mRollback; ++cit)
		((ItemSet*)(*cit))->RollbackUsage();
	if(mRollbackAlph) {
		memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
		mRollbackAlph = false;
	}
}

islist* CPROCodeTable::GetItemSetList() {
	islist *ilist = new islist();
	mCT->BuildItemSetList(ilist);
	return ilist;
}

islist* CPROCodeTable::GetPrePruneList() {
	islist *ilist = new	islist();
	mCT->BuildPrePruneList(ilist);
	return ilist;
}

void CPROCodeTable::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList, after);
	mCT->BuildPostPruneList(pruneList, after);
}

void CPROCodeTable::CoverDB(CoverStats &stats) {
	CoverSet *cset;
	//ItemSet *iset;
	ItemSet **ctArAr;
	uint32 *occur, numOcc, numItemsCovered, newItemsCovered, numUseCount, binSize;
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
			ctArAr = mCTArAr[i];
			cset->ResetMask();

			cend = mCTList->end();
			uint32 ctArIdx=0;
			for(cit = mCTList->begin(); cit != cend; ++cit) {		// DE lijst
				if(cset->CanCover((*cit)->GetLength(),*cit))
					ctArAr[ctArIdx++] = *cit;
			}
			uint32 coverMax,coverIdx,coverUse;
			uint32 ctArNum = ctArIdx;
			uint32 ctArMax = ctArIdx;
			mCTArMax[i] = ctArMax;
			// cover opbouwen tot m
			while(ctArNum > 0) {
				coverMax = 0;			// beste cover tot nu toe
				coverIdx = mCTArLen+1;	// beste coverelementindex init

				// zoek best passende element op
				for(uint32 j=0; j<ctArMax; j++) {
					if(ctArAr[j] != NULL) {
						uint32 len = ctArAr[j]->GetLength();
						if(len <= coverMax)
							continue;
						coverUse = (cset->*mNumCoverFunc)(len,ctArAr[j]);		// geeft 0 omdat ie niet helemaal past
						if(coverUse > coverMax) {
							coverMax = coverUse;
							coverIdx = j;
						} else if(coverUse == 0) {
							ctArAr[j] = NULL;
							ctArNum--;
						}
					}
				}
				if(coverMax > 0) {	
					if(ctArAr[coverIdx] == mChangeItem)	{		
						mUseOccs[mNumUseOccs++] = i;		// hij past
						numUseCount += mDB->GetRow(i)->GetUsageCount();
						break;
					} else {
						(cset->*mCoverFunc)(ctArAr[coverIdx]->GetLength(),ctArAr[coverIdx]);
						ctArNum--;
						ctArAr[coverIdx] = NULL;
					}					
				} else {
					// niets past meer.
					ctArNum = 0;
					break;
				}
			}
			mCTArNum[i] = ctArNum;
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
			ctArAr = mCTArAr[i];

			numItemsCovered = cset->GetNumUncovered();
			cset->BackupMask();
			uint32 coverMax,coverIdx,coverUse;
			uint32 ctArNum = mCTArNum[i];
			uint32 ctArMax = mCTArMax[i];
			for(uint32 j=0; j<ctArMax; j++) {
				if(ctArAr[j] == mChangeItem) {
					ctArAr[j] = NULL;
					break;
				}
			} ctArNum--;
			ItemSet **ctArBak = new ItemSet*[ctArMax];
			memcpy(ctArBak,ctArAr,sizeof(ItemSet*)*ctArMax);
			binSize = mDB->GetRow(i)->GetUsageCount();

			// herstelcover, zonder m dus
			while(ctArNum > 0) {
				coverMax = 0;			// beste cover tot nu toe
				coverIdx = mCTArLen+1;	// beste coverelementindex init

				// zoek best passende element op
				for(uint32 j=0; j<ctArMax; j++) {
					if(ctArAr[j] != NULL) {
						coverUse = (cset->*mNumCoverFunc)(ctArAr[j]->GetLength(),ctArAr[j]);
						if(coverUse > coverMax) {
							coverMax = coverUse;
							coverIdx = j;
						} else if(coverUse == 0) {
							ctArAr[j] = NULL;
							ctArNum--;
						}
					}
				}
				if(coverMax > 0) {
					(cset->*mCoverFunc)(ctArAr[coverIdx]->GetLength(),ctArAr[coverIdx]);
					newItemsCovered = cset->GetNumUncovered();
					ctArAr[coverIdx]->SupUsageArea(numItemsCovered - newItemsCovered);
					ctArAr[coverIdx]->SupUsageCount(binSize);
					numItemsCovered = newItemsCovered;
					stats.usgCountSum-=binSize;
					ctArNum--;
					ctArAr[coverIdx] = NULL;
				} else {
					// niets past meer.
					ctArNum = 0;
					break;
				}
			}
			stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet, binSize);

			// okay, nu met m;
			cset->RestoreMask();
			numItemsCovered = cset->GetNumUncovered();
			(cset->*mCoverFunc)(mChangeItem->GetLength(),mChangeItem);
			newItemsCovered = cset->GetNumUncovered();
			mChangeItem->AddUsagerea(numItemsCovered - newItemsCovered);
			numItemsCovered = newItemsCovered;
			ctArNum = mCTArNum[i]-1;
			while(ctArNum > 0) {
				coverMax = 0;			// beste cover tot nu toe
				coverIdx = mCTArLen+1;	// beste coverelementindex init

				// zoek best passende element op
				for(uint32 j=0; j<ctArMax; j++) {
					if(ctArBak[j] != NULL) {
						coverUse = (cset->*mNumCoverFunc)(ctArBak[j]->GetLength(),ctArBak[j]);
						if(coverUse > coverMax) {
							coverMax = coverUse;
							coverIdx = j;
						} else if(coverUse == 0) {
							ctArBak[j] = NULL;
							ctArNum--;
						}
					}
				}
				if(coverMax > 0) {
					(cset->*mCoverFunc)(ctArBak[coverIdx]->GetLength(),ctArBak[coverIdx]);
					ctArBak[coverIdx]->AddUsageCount(binSize);
					newItemsCovered = cset->GetNumUncovered();
					ctArBak[coverIdx]->AddUsagerea(numItemsCovered - newItemsCovered);
					numItemsCovered = newItemsCovered;
					stats.usgCountSum+=binSize;
					ctArNum--;
					ctArBak[coverIdx] = NULL;
				} else {
					// niets past meer.
					ctArNum = 0;
					break;
				}
			}
			delete[] ctArBak;
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
			cset->ResetMask();
			ctArAr = mCTArAr[i];

			// bouw cover, check of mChangeItem gebruikt zou worden
			// ifso, dan een herstelcover
			cend = mCTList->end();

			uint32 ctArIdx=0;
			for(cit = mCTList->begin(); cit != cend; ++cit) {		// DE lijst
				if(cset->CanCover((*cit)->GetLength(),*cit))
					ctArAr[ctArIdx++] = *cit;
			}
			uint32 coverMax,coverIdx,coverUse;
			uint32 ctArNum = ctArIdx;
			uint32 ctArMax = ctArIdx;
			bool recover = false;
			// cover opbouwen tot m
			while(ctArNum > 0) {
				coverMax = 0;			// beste cover tot nu toe
				coverIdx = mCTArLen+1;	// beste coverelementindex init

				// zoek best passende element op
				for(uint32 j=0; j<ctArMax; j++) {
					if(ctArAr[j] != NULL) {
						uint32 len = ctArAr[j]->GetLength();
						if(len <= coverMax)
							continue;
						coverUse = (cset->*mNumCoverFunc)(len,ctArAr[j]);
						if(coverUse > coverMax) {
							coverMax = coverUse;
							coverIdx = j;
						} else if(coverUse == 0) {
							ctArAr[j] = NULL;
							ctArNum--;
							if(ctArAr[j] == mChangeItem) {
								recover = false;
								coverMax = 0;
								break;
							}
						}
					}
				}
				if(coverMax > 0) {
					if(ctArAr[coverIdx] == mChangeItem) {
						// hij past!
						recover = true;
						ctArNum--;
						ctArAr[coverIdx] = NULL;
						break;
					} else {
						(cset->*mCoverFunc)(ctArAr[coverIdx]->GetLength(),ctArAr[coverIdx]);
						ctArNum--;
						ctArAr[coverIdx] = NULL;
					}					
				} else {
					// niets past meer.
					ctArNum = 0;
					break;
				}
			}
			if(recover) {
				ItemSet **ctArBak = new ItemSet*[ctArMax];
				memcpy(ctArBak,ctArAr,sizeof(ItemSet*)*ctArMax);
				uint32 ctArNumBak = ctArNum;

				cset->BackupMask();
				(cset->*mCoverFunc)(ciLen,mChangeItem);
				numItemsCovered = cset->GetNumUncovered();	// aantal items uncovered -nadat- m gebruikt wordt.
				binSize = mDB->GetRow(i)->GetUsageCount();

				// :: Herstelcover (--) 
				//	-: Herstel staartje mChangeRow
				while(ctArNum > 0) {
					coverMax = 0;			// beste cover tot nu toe
					coverIdx = mCTArLen+1;	// beste coverelementindex init

					// zoek best passende element op
					for(uint32 j=0; j<ctArMax; j++) {
						if(ctArAr[j] != NULL) {
							uint32 len = ctArAr[j]->GetLength();
							if(len <= coverMax)
								continue;
							coverUse = (cset->*mNumCoverFunc)(len,ctArAr[j]);
							if(coverUse > coverMax) {
								coverMax = coverUse;
								coverIdx = j;
							} else if(coverUse == 0) {
								ctArAr[j] = NULL;
								ctArNum--;
							}
						}
					}
					if(coverMax > 0) {
						(cset->*mCoverFunc)(ctArAr[coverIdx]->GetLength(),ctArAr[coverIdx]);
						newItemsCovered = cset->GetNumUncovered();
						ctArAr[coverIdx]->SupUsageCount(binSize);
						ctArAr[coverIdx]->SupUsageArea(numItemsCovered - newItemsCovered);
						numItemsCovered = newItemsCovered;
						stats.usgCountSum-=binSize;
						ctArNum--;
						ctArAr[coverIdx] = NULL;
					} else {
						// niets past meer.
						ctArNum = 0;
						break;
					}
				}
				//	-: Herstel alphabet counts
				stats.usgCountSum -= cset->UncoverAlphabet(mABLen, mAlphabet, binSize);

				cset->RestoreMask();
				numItemsCovered = cset->GetNumUncovered();		// aantal uncovered items dat -voordat- m gebruikt werd
				ctArNum = ctArNumBak;
				while(ctArNum > 0) {
					coverMax = 0;			// beste cover tot nu toe
					coverIdx = mCTArLen+1;	// beste coverelementindex init

					// zoek best passende element op
					for(uint32 j=0; j<ctArMax; j++) {
						if(ctArBak[j] != NULL) {
							uint32 len = ctArBak[j]->GetLength();
							if(len <= coverMax)
								continue;
							coverUse = (cset->*mNumCoverFunc)(len,ctArBak[j]);
							if(coverUse > coverMax) {
								coverMax = coverUse;
								coverIdx = j;
							} else if(coverUse == 0) {
								ctArBak[j] = NULL;
								ctArNum--;
							}
						}
					}
					if(coverMax > 0) {
						(cset->*mCoverFunc)(ctArBak[coverIdx]->GetLength(),ctArBak[coverIdx]);
						newItemsCovered = cset->GetNumUncovered();
						ctArBak[coverIdx]->AddUsagerea(numItemsCovered - newItemsCovered);
						numItemsCovered = newItemsCovered;
						ctArBak[coverIdx]->AddUsageCount(binSize);
						stats.usgCountSum+=binSize;
						ctArNum--;
						ctArBak[coverIdx] = NULL;
					} else {
						// niets past meer.
						ctArNum = 0;
						break;
					}
				}
				delete[] ctArBak;
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
			cset = mCDB[i];
			ctArAr = mCTArAr[i];
			binSize = mDB->GetRow(i)->GetUsageCount();

			cset->ResetMask();
			numItemsCovered = cset->GetNumUncovered();

			cend = mCTList->end();
			uint32 ctArIdx=0;
			for(cit = mCTList->begin(); cit != cend; ++cit) {		// DE lijst
				if(cset->CanCover((*cit)->GetLength(),*cit))
					ctArAr[ctArIdx++] = *cit;
			}
			uint32 coverMax,coverIdx,coverUse;
			uint32 ctArNum = ctArIdx;
			uint32 ctArMax = ctArIdx;
			// cover bouwen
			while(ctArNum > 0) {
				coverMax = 0;			// beste cover tot nu toe
				coverIdx = mCTArLen+1;	// beste coverelementindex init

				// zoek best passende element op
				for(uint32 j=0; j<ctArMax; j++) {
					if(ctArAr[j] != NULL) {
						uint32 len = ctArAr[j]->GetLength();
						if(len <= coverMax)
							continue;
						coverUse = (cset->*mNumCoverFunc)(len,ctArAr[j]);		// geeft 0 omdat ie niet helemaal past
						if(coverUse > coverMax) {
							coverMax = coverUse;
							coverIdx = j;
						} else if(coverUse == 0) {
							ctArAr[j] = NULL;
							ctArNum--;
						}
					}
				}
				if(coverMax > 0) {
					(cset->*mCoverFunc)(ctArAr[coverIdx]->GetLength(),ctArAr[coverIdx]);
					newItemsCovered = cset->GetNumUncovered();
					ctArAr[coverIdx]->AddUsagerea(numItemsCovered - newItemsCovered);
					numItemsCovered = newItemsCovered;
					ctArAr[coverIdx]->AddUsageCount(binSize);
					stats.usgCountSum += binSize;
					ctArNum--;
					ctArAr[coverIdx] = NULL;
				} else {
					// niets past meer.
					ctArNum = 0;
					break;
				}
			}
			// alphabet items
			stats.usgCountSum += mCDB[i]->CoverAlphabet(mABLen, mAlphabet, binSize);
		}
	} else
		throw string("CPROCodeTable:: Unknown code table ChangeType");

	// Recalc sizes, counts and countSum should be up to date...
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	double codeLen = 0;
	// alle is <- ct
	cend = mCTList->end();
	uint32 curcnt;
	uint64 dbarea = mDB->GetNumItems();
	for(cit = mCTList->begin(); cit != cend; ++cit) {
		curcnt = ((ItemSet*)(*cit))->GetUsageCount();
		if(curcnt == 0) continue;
		uint32 curitemcnt = ((ItemSet*)(*cit))->GetUsageArea();
		stats.numSetsUsed++;
		if(mCLStylee == CLItemCounts)
			//codeLen = CalcCodeLength(((ItemSet*)(*cit))->GetItemCount(), dbarea);
			codeLen = CalcCodeLength(curitemcnt, mDB->GetNumItems());
		else
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
		if(mCLStylee == CLItemCounts)
			//codeLen = CalcCodeLength(curcnt, dbarea);
			codeLen = CalcCodeLength(curcnt, mDB->GetNumItems());
		else 
			codeLen = CalcCodeLength(curcnt, stats.usgCountSum);
		stats.encDbSize += curcnt * codeLen;
		stats.encCTSize += mStdLengths[i];
		stats.encCTSize += codeLen;
	}
	stats.encSize = stats.encDbSize + stats.encCTSize;
}

double CPROCodeTable::CalcTransactionCodeLength(ItemSet *transaction) {
	islist::iterator cit,cend = mCTList->end();;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, mDB->GetAlphabetSize());
	mCoverSet->InitSet(transaction);

	double codeLen = 0.0;
	/*if(mCTCodeLengths == NULL) {
		mCTCodeLengths = new double[mCTLen];
		uint32 i=0;
		for(cit = mCTList->begin(); cit != cend; ++cit) {		// DE lijst
			mCTCodeLengths[i] = CalcCodeLength((*cit)->GetCount(), mCurStats.countSum);
			i++;
		}
	}
	if(mABCodeLengths == NULL) {
		mABCodeLengths = new double[mABLen];
		for(uint32 i=0; i<mABLen; i++)
			mABCodeLengths[i] = CalcCodeLength(mAlphabet[i], mCurStats.countSum);
	}*/
	ItemSet** ctArAr = mCTArAr[0];

	uint32 ctArIdx=0;
	for(cit = mCTList->begin(); cit != cend; ++cit) {		// DE lijst
		if(mCoverSet->CanCover((*cit)->GetLength(),*cit))
			ctArAr[ctArIdx++] = *cit;
	}
	uint32 coverMax,coverIdx,coverUse;
	uint32 ctArNum = ctArIdx;
	uint32 ctArMax = ctArIdx;

	// cover bouwen
	while(ctArNum > 0) {
		coverMax = 0;			// beste cover tot nu toe
		coverIdx = mCTArLen+1;	// beste coverelementindex init

		// zoek best passende element op
		for(uint32 j=0; j<ctArMax; j++) {
			if(ctArAr[j] != NULL) {
				uint32 len = ctArAr[j]->GetLength();
				if(len <= coverMax)
					continue;
				coverUse = (mCoverSet->*mNumCoverFunc)(len,ctArAr[j]);		// geeft 0 omdat ie niet helemaal past
				if(coverUse > coverMax) {
					coverMax = coverUse;
					coverIdx = j;
				} else if(coverUse == 0) {
					ctArAr[j] = NULL;
					ctArNum--;
				}
			}
		}
		if(coverMax > 0) {
			(mCoverSet->*mCoverFunc)(ctArAr[coverIdx]->GetLength(),ctArAr[coverIdx]);
			codeLen += CalcCodeLength(ctArAr[coverIdx]->GetUsageCount(), mCurStats.usgCountSum);
			ctArNum--;
			ctArAr[coverIdx] = NULL;
		} else {
			// niets past meer.
			ctArNum = 0;
			break;
		}
	}
	// alphabet items
	for(uint32 i=0; i<mABLen; i++) {
		if(mCoverSet->IsItemUncovered(i)) {
			codeLen += CalcCodeLength(mAlphabet[i], mCurStats.usgCountSum);
		}
	}
	//delete[] ctArAr;
	return codeLen;
}

void	CPROCodeTable::CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) {
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
Database *CPROCodeTable::EncodeDB(const bool useSingletons, string outFile) {
	ECHO(1,	printf(" * Encoding database with CPO...\r"));
	islist::iterator cit,cend;
	uint16 val=0;
	uint16 *emptySet = new uint16[0];
	ItemSet *set = NULL;

	FILE *fp = NULL;
	if (!outFile.empty()) {
		outFile += ".encoded";
		fp = fopen(outFile.c_str(), "w");
		fprintf(fp, "algo=%s\n", GetShortName().c_str());
	}

	ItemSet **iss = new ItemSet *[mNumDBRows];
	cend = mCTList->end();
	for(uint32 i=0; i<mNumDBRows; i++) {
		set = iss[i] = ItemSet::Create(Uint16ItemSetType, emptySet, 0, mCurNumSets);
		set->SetUsageCount(mDB->GetRow(i)->GetUsageCount());
		mCDB[i]->ResetMask();

		val = 0;
		// sets
		for(cit=mCTList->begin(); cit != cend; ++cit) {
			if((mCDB[i]->*mCoverFunc)(((ItemSet*)(*cit))->GetLength(),*cit)) {
				set->AddItemToSet(val);
				if (fp) {
					fprintf(fp, "{%s", (*cit)->ToString().c_str());
					fseek(fp, -2, SEEK_CUR);
					fprintf(fp, "/%d)} ", (*cit)->GetSupport());
				}
			}
			val++;
		}

		// singletons
		if(useSingletons)
			for(uint32 j=0; j<mABLen; j++, val++)
				if(mCDB[i]->IsItemUncovered(j)) {
					set->AddItemToSet(val);
					if (fp)
						fprintf(fp, "{%d} ", j);
				}
				if (fp)
					fprintf(fp, "\n");
	}

	delete[] emptySet;
	if (fp)
		fclose(fp);
	ECHO(1,	printf(" * Encoding:\t\tdone (used CPO)\n\n"));
	return new Database(iss, mNumDBRows, mDB->HasBinSizes());
}

void CPROCodeTable::AddOneToEachUsageCount() {
	islist::iterator cit,cend=mCTList->end();

	for(cit=mCTList->begin(); cit!=cend; ++cit) {
		((ItemSet*)(*cit))->AddOneToUsageCount();
		mCurStats.usgCountSum++;
	}
	// don't forget the alphabet!
	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i]++;
	mCurStats.usgCountSum += mABLen;

	++mLaplace;
}

void CPROCodeTable::ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset) {
	islist::iterator cit,cend=mCTList->end();

	for(cit=mCTList->begin(); cit!=cend; ++cit)
		((ItemSet*)(*cit))->SetUsageCount(lpcFactor * ((ItemSet*)(*cit))->GetUsageCount() + lpcOffset);
	// don't forget the alphabet!
	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i] = lpcFactor * mAlphabet[i] + lpcOffset;

	mCurStats.usgCountSum = lpcFactor * mCurStats.usgCountSum + lpcOffset;
	++mLaplace;
}

void CPROCodeTable::UpdateCTLog() {
	if(mChangeType == ChangeAdd) {
		fprintf(mCTLogFile, "+ %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeDel) {
		fprintf(mCTLogFile, "- %s\n", mChangeItem->ToCodeTableString().c_str());
	} else if(mChangeType == ChangeNone) {
		fprintf(mCTLogFile, "0\n");
	}
}
