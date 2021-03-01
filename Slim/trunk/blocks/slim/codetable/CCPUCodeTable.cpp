#include "../../../global.h"

#include <stdlib.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>

#include "../../krimp/codetable/CodeTable.h"
#include "../../krimp/codetable/CTISList.h"

#include "../../krimp/codetable/coverpartial/CCPCodeTable.h"
#include "CCPUCodeTable.h"

CCPUCodeTable::CCPUCodeTable() : CCPCodeTable() {
}


CCPUCodeTable::CCPUCodeTable(const CCPUCodeTable &ct) : CCPCodeTable(ct) {
}


CCPUCodeTable::~CCPUCodeTable() {

}

void CCPUCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
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

	// and now we make our own DB data structure here...
	mCDB = new CoverSet*[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		// new, saving memory by only creating as many masks as the row is long
		mCDB[i] = CoverSet::Create(type, mABLen, mDB->GetRow(i), mDB->GetRow(i)->GetLength()-1); 
		// old, wasting memory by always creating as many masks as the CT has insertion points
		//mCDB[i] = CoverSet::Create(type, mABLen, mDB->GetRow(i), mCTLen-1); 
	}

	mIsBinnedDB = mDB->HasBinSizes();

	//////////////////////////////////////////////////////////////////////////
	// CCPU specific
	//////////////////////////////////////////////////////////////////////////
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


void CCPUCodeTable::CoverDB(CoverStats &stats) {
	if(mIsBinnedDB)			// switch binned/notbinned
		CoverBinnedDB(stats);
	else
		CoverNoBinsDB(stats);
}

void CCPUCodeTable::CoverNoBinsDB(CoverStats &stats) {
	CoverSet *cset;//, *bset = mCDB[i] = CoverSet::Create(type, mABLen, mDB->GetRow(i), 0);
	ItemSet *iset; 
	uint32 *occur, numOcc, insPoint, loadPoint;
	islist::iterator cit,cend;

	if(mChangeType == ChangeAdd) {
		uint32 mj = mChangeRow;
		uint32 len = mj+1;
		uint32 rlen = 0;
		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();
		mNumUseOccs = 0;

		mChangeItem->ResetUsage();
		mChangeItem->SetPrevUsageCount(1);	// !=0, to make sure codelength is properly calculated

		for(uint32 r=0; r<numOcc; r++) {
			uint32 i=occur[r];
			cset = mCDB[i];
			rlen = mDB->GetRow(i)->GetLength();

			insPoint = rlen - len + 1;

			loadPoint = cset->GetMostUpToDateIPoint(insPoint);
			
			if(loadPoint != insPoint) {
				cset->LoadMask(loadPoint);
				// zelf tot insPoint uitrekenen, masks onderweg opslaan. 
				for(uint32 j=rlen-loadPoint-1; j>=mj; --j) {
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
					cset->StoreMask(++loadPoint);	// storing masks now is a bad idea, accepts are rare, better only do after accept!
				}
			} 
			if(cset->CanCoverIPoint(insPoint, mChangeItem)) {					// nu m ff checken
				mUseOccs[mNumUseOccs++] = i;
				mChangeItem->PushUsed(i);
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
		swap(mAlphabetUsages, mOldAlphabetUsages);

		uint32 i;
		uint32 cnt;
		for(uint32 r=0; r<mNumUseOccs; r++) {
			i=mUseOccs[r];
			cset = mCDB[i];
			CoverSet *bset = mCDB[i];

			bset->BackupMask();
			//	herstelcover (--) 
			for(uint32 j=mj-1; j>=1; --j) {
				if	(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(bset->Cover(j+1,*cit)) {
							ItemSet* is = ((ItemSet*)(*cit));
							is->SupOneFromUsageCount();
							//is->ZapUsage(i);
							is->SetUnused(i);
							stats.usgCountSum--;
						}
					}
				}
			}
			cnt = bset->UncoverAlphabet(mABLen, mAlphabet, mValues);
			for (uint32 j = 0; j < cnt; ++j) {
				mAlphabetUsageZap[mValues[j]]->push_back(i);
			}
			stats.usgCountSum -= cnt;

			//  Totale cover met m
			cset->RestoreMask();
			cset->Cover(mj+1, mChangeItem);

			for(uint32 j=mj-1; j>=1; --j) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							ItemSet* is = ((ItemSet*)(*cit));
							is->AddOneToUsageCount();
							//is->AddUsage(i);
							is->SetUsed(i);
							stats.usgCountSum++;
						}
					}
				}
			}
			cnt = cset->CoverAlphabet(mABLen, mAlphabet, mValues);
			for (uint32 j = 0; j < cnt; ++j) {
				mAlphabetUsageAdd[mValues[j]]->push_back(i);
			}
			stats.usgCountSum += cnt;
		}
		// Counts en CountSum zijn bijgewerkt, Usages van Alphabet worden below geupdate, dan sizes uitrekenen ...

		//SyncUsageChanges();

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
		swap(mAlphabetUsages, mOldAlphabetUsages);

		occur = mChangeItem->GetOccs();
		numOcc = mChangeItem->GetNumOccs();

		stats.usgCountSum -= mChangeItem->GetPrevUsageCount();
		mChangeItem->SetUsageCount(0);	// want gedelete...
		mChangeItem->SetAllUnused();

		uint32 r,i;
		uint32 cnt;
		uint32 rlen;
		for(r=0; r<numOcc; r++) {
			i=occur[r];
			cset = mCDB[i];
			rlen = mDB->GetRow(i)->GetLength();

			insPoint = rlen - len;
			loadPoint = cset->LoadMask(insPoint);

			if(loadPoint != insPoint) {
				// Vanaf loadPoint tot mChangeRow
				for(uint32 j=rlen-loadPoint-1; j>mj; --j) {
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
						iset = ((ItemSet*)(*cit));
						iset->SupOneFromUsageCount();
						//iset->ZapUsage(i);
						iset->SetUnused(i);
						stats.usgCountSum--;
					}
				}
				//	-: Herstel na mChangeRow
				for(uint32 j=mj-1; j>=1; --j) {
					if(mCT[j] != NULL) {
						cend = mCT[j]->GetList()->end();
						for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
							if(cset->Cover(j+1,*cit)) {
								iset = ((ItemSet*)(*cit));
								iset->SupOneFromUsageCount();
								//iset->ZapUsage(i);
								iset->SetUnused(i);
								stats.usgCountSum--;
							}
						}
					}
				}
				//	-: Herstel alphabet counts
				cnt = cset->UncoverAlphabet(mABLen, mAlphabet, mValues);
				for (uint32 j = 0; j < cnt; ++j) {
					mAlphabetUsageZap[mValues[j]]->push_back(i);
				}
				stats.usgCountSum -= cnt;

				cset->RestoreMask();
				cset->Uncover(mj+1, mChangeItem);

				// mChangeRow vanaf mChangeIter om cover zonder te bepalen
				cit = mChangeIter;
				++cit;
				cend = mCT[mj]->GetList()->end();
				for(; cit != cend; ++cit) {
					if(cset->Cover(mj+1,*cit)) {
						iset = *cit;
						iset->AddOneToUsageCount();
						//iset->AddUsage(i);
						iset->SetUsed(i);
						stats.usgCountSum++;
					}
				}
				// na mChangeRow
				for(uint32 j=mj-1; j>=1; --j) {	
					if(mCT[j] != NULL) {
						cend = mCT[j]->GetList()->end();
						for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
							if(cset->Cover(j+1,*cit)) {
								iset = *cit;
								iset->AddOneToUsageCount();
								//iset->AddUsage(i);
								iset->SetUsed(i);
								stats.usgCountSum++;
							}
						}
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
		for(uint32 j=mMaxIdx; j>0; --j)
			if(mCT[j] != NULL)
				mCT[j]->ResetUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		swap(mAlphabetUsages, mOldAlphabetUsages);
		for(uint32 i=0; i<mABUsedLen; i++)
			mAlphabet[i] = 0;

		stats.usgCountSum = mABUnusedCount;

		mRollback = mMaxIdx;
		mRollbackAlph = true;

		// full cover db
		loadPoint = 0;
		uint32 cnt;
		for(uint32 i=0; i<mNumDBRows; i++) {
			cset = mCDB[i];
			cset->ResetMask();
			for(uint32 j=mMaxIdx; j>0; j--) {
				if(mCT[j] != NULL) {
					cend = mCT[j]->GetList()->end();
					for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
						if(cset->Cover(j+1,*cit)) {
							iset = ((ItemSet*)(*cit));
							iset->AddOneToUsageCount();
							iset->SetUsed(i);
							stats.usgCountSum++;
						}
					}
				}
				// Store Mask		??? wel/niet doen?
				cset->StoreMask(++loadPoint);
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
			// from scratch - the mix part - as long as it is not empty, we're still-in-the current usages
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

	// Recalc sizes; counts and countSum should be up to date
	CalcTotalSize(stats);
}

void CCPUCodeTable::CoverBinnedDB(CoverStats &stats) {
	THROW_NOT_IMPLEMENTED();
}

/*
void CCPUCodeTable::SyncUsageChanges() {
	uint32 insPoint = mCTLen; // - len + 1;

	islist::iterator cit,cend;
	for(uint32 j=insPoint; j>1; --j) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				((ItemSet*)*cit)->SyncUsed();
			}
		}
	}
}
*/

// Commits last added ItemSet
// !?! kost relatief veel tijd, met name MakeIPointUpToDate
void CCPUCodeTable::CommitAdd(bool updateLog) {
	if(updateLog)
		UpdateCTLog();

	mPrevStats = mCurStats;

	uint32 len = mChangeItem->GetLength();
	uint32 insPoint = mCTLen - len + 1;

	//mChangeItem->SyncUsed();

	// for every X \in CT for which r(X) <= mChangeItem
	/*islist::iterator cit,cend;
	for(uint32 j=insPoint; j>1; --j) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				((ItemSet*)*cit)->SyncUsed();
			}
		}
	}*/

	// csets nu tot insPoint uptodate maken
	//uint32 *occur = mChangeItem->GetDBOccurrence();
	//uint32 numOcc = mChangeItem->GetNumDBOccurs();
	// Doe alleen de occurrences die gebruikt worden
	uint32 *occur = mUseOccs;
	uint32 numOcc = mNumUseOccs;

	for(uint32 r=0; r<numOcc; r++) {
		//uint32 insPoint = mCTLen - len + 1;
		insPoint = mDB->GetRow(occur[r])->GetLength() - len + 1;

		mCDB[occur[r]]->MakeIPointUpToDate(insPoint, mChangeItem, len);
	}
	mChangeItem->BackupUsage();
	mChangeItem = NULL;
	mChangeType = ChangeNone;
}

void CCPUCodeTable::CommitLastDel(bool zap, bool updateLog /* =true */) {
	//SyncUsageChanges();

	/*
	uint32 len = mChangeItem->GetLength();
	uint32 insPoint = mCTLen - len;
	islist::iterator cit,cend;
	for(uint32 j=insPoint; j>1; --j) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				((ItemSet*)*cit)->SyncUsed();
			}
		}
	}*/
	CCPCodeTable::CommitLastDel(zap, updateLog);
}

void CCPUCodeTable::RollbackCounts() {
	if(mRollbackAlph) {
		islist::iterator cit,cend;
		for(uint32 j=mRollback; j>=1; --j) {
			if(mCT[j] != NULL) {
				// rolls back counts AND usages!
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					((ItemSet*)*cit)->RollbackUsage();
					//((ItemSet*)*cit)->ClearUsed();	// this can be pushed into RollBackCounts
				}
			}
		}
		memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
		for(uint32 i = 0; i < mAlphabetSize; ++i) {
			mAlphabetUsageAdd[i]->clear();
			mAlphabetUsageZap[i]->clear();
			memcpy(mAlphabetUsages[i], mOldAlphabetUsages[i], mAlphabet[i] * sizeof(uint32));
		}
		// this wasnt here in orig CCP, should it be?
		mRollbackAlph = false; // ??? 
	}
}

void CCPUCodeTable::RollbackAdd() {
	CCPCodeTable::RollbackAdd();
}

void CCPUCodeTable::RollbackLastDel() {
	CCPCodeTable::RollbackLastDel();
}

void CCPUCodeTable::SetAlphabetCount(uint32 item, uint32 count) {
	mAlphabet[item] = count;
}
uint32 CCPUCodeTable::GetAlphabetCount(uint32 item) {
	return mAlphabet[item];
}
