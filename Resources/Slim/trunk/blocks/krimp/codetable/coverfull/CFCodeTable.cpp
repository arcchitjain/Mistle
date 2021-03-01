#include "../../../../global.h"

#include <stdlib.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>
#include <isc/ItemTranslator.h>

#include "../CodeTable.h"
#include "../CTISList.h"

#include "CFCodeTable.h"

//#define PRUNE_DEBUG

CFCodeTable::CFCodeTable() : CodeTable() {
	mCT = NULL;
	mCoverSet = NULL;
	mNumDBRows = 0;
	mCTLen = 0;
	mMaxIdx = 0;

	mAlphabet = NULL;
	mOldAlphabet = NULL;
	mABUsed = NULL;
	mABUnused = NULL;
	mABLen = 0;
	mABUsedLen = 0;

	mAdded = NULL;
	mAddedCandidateId = 0;
	mLastDel.is = NULL;
	mLastDel.before = NULL;
}

CFCodeTable::CFCodeTable(const CFCodeTable &ct) : CodeTable(ct) {
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

	mCoverSet = ct.mCoverSet->Clone();

	mNumDBRows = ct.mNumDBRows;

	mABLen = ct.mABLen;
	mAlphabet = new uint32[mABLen];
	memcpy(mAlphabet, ct.mAlphabet, mABLen * sizeof(uint32));
	mOldAlphabet = new uint32[mABLen];
	memcpy(mOldAlphabet, ct.mOldAlphabet, mABLen * sizeof(uint32));
	mABUsedLen = ct.mABUsedLen;
	mABUsed = new uint32[mABUsedLen];
	memcpy(mABUsed, ct.mABUsed, mABUsedLen * sizeof(uint32));
	mABUnused = new uint32[mABLen - mABUsedLen];
	memcpy(mABUnused, ct.mABUnused, (mABLen - mABUsedLen) * sizeof(uint32));
	mABUnusedCount = ct.mABUnusedCount;

	mAdded = NULL;
	mAddedCandidateId = 0;
}



CFCodeTable::~CFCodeTable() {
	for(uint32 i=0; i<mCTLen; i++)
		if(mCT[i] != NULL) {
			islist::iterator cit, cend=mCT[i]->GetList()->end();
			for(cit = mCT[i]->GetList()->begin(); cit != cend; ++cit)
				delete *cit;
			delete mCT[i];
		}
	delete[] mCT;
	delete mCoverSet;
	delete[] mAlphabet;
	delete[] mOldAlphabet;
	delete[] mABUsed;
	delete[] mABUnused;
}

void CFCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
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
	mABLen = (uint32)a->size();
	//uint32 pruneBelow = min(db->GetPrunedBelow(), toMinSup);
	uint32 pruneBelow = 0;
	mAlphabet = new uint32[mABLen];
	mOldAlphabet = new uint32[mABLen];
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
		if(acnt != 0 && acnt >= pruneBelow)
			mABUsedLen++;
	}
	mABUsed = new uint32[mABUsedLen];
	mABUnused = new uint32[mABLen - mABUsedLen];
	uint32 usedIdx = 0, unusedIdx = 0;
	for(uint32 i=0; i<mABLen; i++) {
		if(mAlphabet[i] == 0 || mAlphabet[i] < pruneBelow) {
			mABUnused[unusedIdx++] = i;
			mABUnusedCount += mAlphabet[i];
		} else
			mABUsed[usedIdx++] = i;
	}

	double ctSize = 0;
	if(ctinit == InitCTAlpha) {
		for(uint32 i=0; i<mABLen; i++)
			ctSize += mStdLengths[i];
		ctSize *= 2;
	}
	//ECHO(2, printf(" * Alphabet length:\t%d/%d (used/total)\n", mABUsedLen, mABLen));
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

	mNumDBRows = mDB->GetNumRows();

	mCurStats.usgCountSum = countSum;
	mCurStats.encDbSize = db->GetStdDbSize();
	mCurStats.encCTSize = ctSize;
	mCurStats.encSize = mCurStats.encDbSize + mCurStats.encCTSize;
	mCurStats.numSetsUsed = 0;
	mCurStats.alphItemsUsed = mABUsedLen;
	mPrevStats = mCurStats;

	// CoverSet
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, a->size());
}

void CFCodeTable::SetDatabase(Database *db) {
	CodeTable::SetDatabase(db);
	mNumDBRows = mDB->GetNumRows();
	mStdLengths = mDB->GetStdLengths();
}

void CFCodeTable::Add(ItemSet *is, uint64 candidateId) {
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

	mAdded = is;
	mAddedCandidateId = candidateId;
}

void CFCodeTable::Del(ItemSet *is, bool zap, bool keepList) {
	uint32 len = is->GetLength();
	islist *lst = mCT[len-1]->GetList();

	mLastDel.is = is;
	if(lst->back() == is) {
		lst->pop_back();
		mLastDel.before = NULL;
	} else {
		islist::iterator iter, lend = lst->end();
		for(iter=lst->begin(); iter!=lend; ++iter)
			if((ItemSet*)(*iter) == is)
				break;
		lend = iter;
		++lend;
		mLastDel.before = (ItemSet*)(*lend);
		lst->erase(iter);
	}

	if(keepList)
		mDeleted.push_front(DelInfo(mLastDel));

	--mCurNumSets;
	mCurStats.usgCountSum -= is->GetUsageCount();

	if(zap)
		delete is;
}

// Commits last added ItemSet
void CFCodeTable::CommitAdd(bool updateLog) {
	if(updateLog)
		UpdateCTLog();
	mPrevStats = mCurStats;
	if (mAdded)
		mAdded->BackupUsage();
	mAdded = NULL;
	mAddedCandidateId = 0;
}

// Rolls back last added ItemSet, restores counts and stats
void CFCodeTable::RollbackAdd() {
	if(mAdded != NULL) {
		Del(mAdded, true, false);
		mLastDel.is = NULL;
		mCurStats = mPrevStats;
		RollbackCounts();
		mAdded = NULL;
		mAddedCandidateId = 0;
	}
}

void CFCodeTable::RollbackLastDel() {
	if(mLastDel.is != NULL)	{
		uint32 len = mLastDel.is->GetLength();
		islist *lst = mCT[len-1]->GetList();

		if(mLastDel.before == NULL)
			lst->push_back(mLastDel.is);
		else {
			islist::iterator before;
			for(before=lst->begin(); ; ++before)
				if((ItemSet*)(*before) == mLastDel.before)
					break;
			lst->insert(before, mLastDel.is);
		}
		mLastDel.is = NULL;

		mCurStats = mPrevStats;
		RollbackCounts();

		++mCurNumSets;

		if(!mDeleted.empty())
			mDeleted.pop_front();
	}
}

void CFCodeTable::CommitLastDel(bool zap, bool updateLog) {
	if(zap)
		delete mLastDel.is;
	mLastDel.is = NULL;
	mPrevStats = mCurStats;
}


void CFCodeTable::CommitAllDels(bool updateLog) {
	dellist::iterator iter, itend = mDeleted.end();
	for(iter=mDeleted.begin(); iter!=itend; ++iter)
		delete ((DelInfo)*iter).is;
	mDeleted.clear();
	mLastDel.is = NULL;
}



void CFCodeTable::RollbackAllDels() {
	uint32 len;
	islist *lst;
	DelInfo di;
	dellist::iterator iter, itend = mDeleted.end();
	for(iter=mDeleted.begin(); iter!=itend; ++iter) {
		di = (DelInfo)(*iter);
		len = di.is->GetLength();
		lst = mCT[len-1]->GetList();

		if(di.before == NULL)
			lst->push_back(di.is);
		else {
			islist::iterator before;
			for(before=lst->begin(); ; ++before)
				if((ItemSet*)(*before) == di.before)
					break;
			lst->insert(before, di.is);
		}
	}
	mCurNumSets += (uint32)mDeleted.size();
	mDeleted.clear();
	mLastDel.is = NULL;
}

void CFCodeTable::RollbackCounts() {
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->RollbackUsageCounts();
	memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
}

islist* CFCodeTable::GetItemSetList() {
	islist *ilist = new	islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildItemSetList(ilist);
	return ilist;
}

islist* CFCodeTable::GetSingletonList() {
	islist *ilist = new	islist();
	ItemSet *is;
	uint16 *set;
	alphabet *alph = mDB->GetAlphabet();
	uint32 alphLen = (uint32)alph->size();

	for(uint32 i=0; i<mABLen; i++) {
		set = new uint16[1];
		set[0] = i;
		is = ItemSet::Create(mDB->GetDataType(), set, 1, alphLen, mAlphabet[i], alph->find(i)->second);
		delete set;
		ilist->push_back(is);
	}
	return ilist;
}

islist* CFCodeTable::GetPrePruneList() {
	islist *ilist = new	islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildPrePruneList(ilist);
	return ilist;
}

islist* CFCodeTable::GetPostPruneList() {
	islist *pruneList = new islist();
	UpdatePostPruneList(pruneList, pruneList->begin());
	return pruneList;
}
islist* CFCodeTable::GetSanitizePruneList() {
	islist *pruneList = new islist();
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildSanitizePruneList(pruneList);
	return pruneList;
}

void CFCodeTable::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList, after);
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->BuildPostPruneList(pruneList, after);
}
void CFCodeTable::CoverDB(CoverStats &stats) {
	// init counts
	for(uint32 j=mMaxIdx; j>0; --j)
		if(mCT[j] != NULL)
			mCT[j]->ResetUsages();
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
	for(uint32 i=0; i<mABUsedLen; i++) // ABUsed may change when covering with different DB!
		mAlphabet[mABUsed[i]] = 0;
	if(mLastDel.is != NULL)
		mLastDel.is->BackupUsage();

	stats.usgCountSum = mABUnusedCount;
	if(mSTSpecialCode)
		++stats.usgCountSum;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.encDbSize = 0;
	stats.encCTSize = 0;

	// full cover db
	islist::iterator cit,cend;
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCoverSet->InitSet(mDB->GetRow(i));
		uint32 binSize = mDB->GetRow(i)->GetUsageCount();
		for(uint32 j=mMaxIdx; j>=1; j--) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					if(mCoverSet->Cover(j+1,*cit)) {
						((ItemSet*)(*cit))->AddUsageCount(binSize);
						stats.usgCountSum+=binSize;
					}
				}
			}
		}
		// alphabet items
		stats.usgCountSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet, binSize);
	}

	CalcTotalSize(stats);
}
void CFCodeTable::CalcTotalSize(CoverStats &stats) {
	islist::iterator cit,cend;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.encDbSize = 0;
	stats.encCTSize = 0;

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
		stats.encDbSize += (curcnt-mLaplace) * codeLen;
		stats.encCTSize += mStdLengths[i];
		stats.encCTSize += codeLen;
	}
	if(mSTSpecialCode)
		stats.encCTSize += CalcCodeLength(1, stats.usgCountSum);

	stats.encSize = stats.encDbSize + stats.encCTSize;
}

void CFCodeTable::AddOneToEachUsageCount() {
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

	++mLaplace;
}
void CFCodeTable::ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset) {
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
void CFCodeTable::AddSTSpecialCode() {
	mCurStats.usgCountSum++;
	mSTSpecialCode = true;
}
void CFCodeTable::NormalizeUsageCounts(uint32 total) {
	islist::iterator cit,cend;
	uint32 cnt, newCnt;
	double factor = ((double) total / (double) mCurStats.usgCountSum );
	mCurStats.usgCountSum = 0;
	for(uint32 j=mMaxIdx; j>=1; j--)
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				cnt = ((ItemSet*)(*cit))->GetUsageCount();
				newCnt = (uint32) ceil(cnt * factor);
				if(newCnt == 0)	newCnt = 1;
				((ItemSet*)(*cit))->SetUsageCount(newCnt);
				mCurStats.usgCountSum += newCnt;
			}
		}
		// don't forget the alphabet!
		for(uint32 i=0; i<mABLen; i++) {
			cnt = mAlphabet[i];
			newCnt = (uint32)ceil(cnt * factor);
			if(newCnt == 0)	newCnt = 1;
			mAlphabet[i] = newCnt;
			mCurStats.usgCountSum += newCnt;
		}
}
double CFCodeTable::CalcEncodedDbSize(Database *db) {
	uint32 numRows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	double size = 0.0;

	for(uint32 i=0; i<numRows; i++)
		size += CalcTransactionCodeLength(rows[i]) * rows[i]->GetUsageCount();
	return size;
}
double CFCodeTable::CalcTransactionCodeLength(ItemSet *transaction) {
	islist::iterator cit,cend;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, mDB->GetAlphabetSize());
	mCoverSet->InitSet(transaction);

	double codeLen = 0.0;
	for(uint32 j=mMaxIdx; j>=1; j--)
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit)
				if(mCoverSet->Cover(j+1,*cit))
					codeLen += CalcCodeLength((*cit)->GetUsageCount(), mCurStats.usgCountSum);
		}
	// alphabet items
	for(uint32 i=0; i<mABLen; i++)
		if(mCoverSet->IsItemUncovered(i))
			codeLen += CalcCodeLength(mAlphabet[i], mCurStats.usgCountSum);
	return codeLen;
}
double CFCodeTable::CalcTransactionCodeLength(ItemSet *row, double *stLengths) {
	if(!mSTSpecialCode)
		return CalcTransactionCodeLength(row); // Assume Laplace

	// (Non-Laplace) ST encoding for freak items

	islist::iterator cit,cend;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType,(uint32) mDB->GetAlphabetSize());
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
	return codeLen;
}

double CFCodeTable::CalcCodeTableSize() {
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
	mCurStats.encSize = mCurStats.encDbSize + mCurStats.encCTSize;
	return mCurStats.encCTSize;
}
Database *CFCodeTable::EncodeDB(const bool useSingletons, string outFile) {
	ECHO(1,	printf(" * Encoding database...\r"));
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

	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				val = 0;
				set = (*cit);
				for(uint32 i=0; i<mNumDBRows; i++)
					if(mDB->GetRow(i)->IsSubset(set))
						val++;
				set->SetSupport(val);
			}
		}
	}

	ItemSet **iss = new ItemSet *[mNumDBRows];
	for(uint32 i=0; i<mNumDBRows; i++) {
		set = iss[i] = ItemSet::Create(Uint16ItemSetType, emptySet, 0, mCurNumSets);
		mCoverSet->InitSet(mDB->GetRow(i));
		set->SetUsageCount(mDB->GetRow(i)->GetUsageCount());

		val = 0;
		// sets
		for(uint32 j=mMaxIdx; j>=1; j--) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					if(mCoverSet->Cover(j+1,*cit)) {
						set->AddItemToSet(val);
						if (fp) {
							fprintf(fp, "{%s", (*cit)->ToString().c_str());
							fseek(fp, -1, SEEK_CUR);
							fprintf(fp, "/%d)} ", (*cit)->GetSupport());
						}
					}
					val++;
				}
			}
		}
		// singletons
		if(useSingletons)
			for(uint32 j=0; j<mABLen; j++, val++)
				if(mCoverSet->IsItemUncovered(j)) {
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
	ECHO(1,	printf(" * Encoding:\t\tdone\n\n"));

	return new Database(iss, mNumDBRows, mDB->HasBinSizes());
}
uint32 CFCodeTable::EncodeRow(ItemSet *row, double &codeLen, uint16 *temp) {
	islist::iterator cit,cend;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, (uint32) mDB->GetAlphabetSize());
	mCoverSet->InitSet(row);

	codeLen = 0.0;
	uint16 id = 0;
	uint32 idx = 0;
	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				if(mCoverSet->Cover(j+1,*cit)) {
					codeLen += CalcCodeLength((*cit)->GetUsageCount(), mCurStats.usgCountSum);
					temp[idx++] = id;
				}
				id++;
			}
		}
	}
	// alphabet items
	for(uint32 i=0; i<mABLen; i++) {
		if(mCoverSet->IsItemUncovered(i)) {
			codeLen += CalcCodeLength(mAlphabet[i], mCurStats.usgCountSum);
			temp[idx++] = id;
		}
		id++;
	}

	return idx;
}

void	CFCodeTable::CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) {
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
	uint32 binSize;
	for(uint32 i=0; i<numRows; i++) {
		mCoverSet->InitSet(mDB->GetRow(rowList[i]));
		binSize = mDB->GetRow(rowList[i])->GetUsageCount();
		for(uint32 j=mMaxIdx; j>=1; j--) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					if(mCoverSet->Cover(j+1,*cit)) {
						((ItemSet*)(*cit))->AddUsageCount(binSize);
						stats.usgCountSum+=binSize;
					}
				}
			}
		}
		// alphabet items
		stats.usgCountSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet, binSize);
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

	stats.encSize = stats.encDbSize + stats.encCTSize;
}

void	CFCodeTable::CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows, bool addOneToCnt) {
	stats.usgCountSum = 0;

	// init counts
	if(addOneToCnt) {
		islist::iterator cit,cend;

		for(uint32 j=mMaxIdx; j>=1; j--)
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					((ItemSet*)(*cit))->ResetUsage();
					((ItemSet*)(*cit))->AddOneToUsageCount();
					stats.usgCountSum++;
				}
			}
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		for(uint32 i=0; i<mABLen; i++)
			mAlphabet[i] = 1;
		stats.usgCountSum += mABLen;

	} else {
		for(uint32 j=mMaxIdx; j>0; --j)
			if(mCT[j] != NULL)
				mCT[j]->ResetUsages();
		memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
		for(uint32 i=0; i<mABLen; i++)
			mAlphabet[i] = 0;
	}

	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.encSize = 0;

	// full cover db
	islist::iterator cit,cend;
	uint32 binSize;
	for(uint32 i=0; i<numRows; i++) {
		mCoverSet->InitSet(mDB->GetRow(rowList[i]));
		binSize = mDB->GetRow(rowList[i])->GetUsageCount();
		for(uint32 j=mMaxIdx; j>=1; j--) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					if(mCoverSet->Cover(j+1,*cit)) {
						((ItemSet*)(*cit))->AddUsageCount(binSize);
						stats.usgCountSum+=binSize;
					}
				}
			}
		}
		// alphabet items
		stats.usgCountSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet, binSize);
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

	stats.encSize = stats.encDbSize + stats.encCTSize;
}

// Stats
void CFCodeTable::WriteCodeFile(Database *encodedDb, const string &outFile) {
	FILE *fp = fopen(outFile.c_str(), "w");
	ItemTranslator *it = encodedDb->GetItemTranslator();
	uint32 total = (uint32) encodedDb->GetAlphabetSize();
	uint32 val = 0;
	islist::iterator cit, cend;
	ItemSet **iss = new ItemSet *[mCurNumSets];
	for(uint32 j=mMaxIdx; j>=1; j--)
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit)
				iss[val++] = *cit;
		}
	
	for(uint32 i=0; i<total; i++) {
		val = it->BitToValue(i);
		if(val < mCurNumSets)
			fprintf(fp, "%d=>%d (%d/%d):\t\t%s\n", i,val, iss[val]->GetUsageCount(), iss[val]->GetSupport(), iss[val]->ToString().c_str());
		else
			fprintf(fp, "%d=>%d (%d):\t\t%d\n", i,val, mAlphabet[val-mCurNumSets], val-mCurNumSets);
	}
	delete[] iss;
	fclose(fp);
}
void CFCodeTable::WriteCodeLengthsToFile(const string &outFile) { 
	FILE *fpAll = fopen(outFile.c_str(), "w");
	if(!fpAll)
		THROW("CtAnalyser could not write file: " + outFile);

	double codeLen;
	uint32 curcnt;
	islist::iterator cit, cend;

	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				curcnt = ((ItemSet*)(*cit))->GetUsageCount();
				if(curcnt == 0)
					THROW("Writing CodeLengths To File:: count 0 shouldn't happen!");
				codeLen = CalcCodeLength(curcnt, mCurStats.usgCountSum);
				fprintf(fpAll, "%f,", codeLen);
			}
		}
	}
	fseek(fpAll, -1, SEEK_CUR);	/* check return value? */
	fprintf(fpAll, "\n");
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		curcnt = mAlphabet[i];
		if(curcnt == 0)
			THROW("Writing CodeLengths To File:: count 0 shouldn't happen!");
		codeLen = CalcCodeLength(curcnt, mCurStats.usgCountSum);
		fprintf(fpAll, "%f,", codeLen);
	}
	fseek(fpAll, -1, SEEK_CUR); /* check return value? */
	fprintf(fpAll, "\n");

	fclose(fpAll);
}
void CFCodeTable::WriteCountsToFile(const string &outFile) {
	FILE *fpAll = fopen(outFile.c_str(), "w");
	if(!fpAll)
		THROW("CFCodeTable::WriteCountsToFile() could not write file: " + outFile);

	uint32 curcnt;
	islist::iterator cit, cend;

	fprintf(fpAll, "%d\n%d\n", mCurNumSets, mABLen);

	// Sets
	for(uint32 j=mMaxIdx; j>=1; j--) {
		if(mCT[j] != NULL) {
			cend = mCT[j]->GetList()->end();
			for (cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
				curcnt = ((ItemSet*)(*cit))->GetUsageCount();
				fprintf(fpAll, "%d\n", curcnt);
			}
		}
	}
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		curcnt = mAlphabet[i];
		fprintf(fpAll, "%d\n", curcnt);
	}

	fclose(fpAll);
}
void CFCodeTable::WriteCover(CoverStats &stats, const string &outFile) {
	FILE *fpAll = fopen(outFile.c_str(), "w");
	if(!fpAll)
		THROW("CFCodeTable::WriteCover() could not write file: " + outFile);

	// init counts
	//for(uint32 j=mMaxIdx; j>0; --j)
	//	if(mCT[j] != NULL)
	//		mCT[j]->ResetCounts();
	//memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
	//for(uint32 i=0; i<mABUsedLen; i++) // ABUsed may change when covering with different DB!
	//	mAlphabet[mABUsed[i]] = 0;

	//stats.countSum = mABUnusedCount;
	//stats.numSetsUsed = 0;
	//stats.alphItemsUsed = 0;
	//stats.dbSize = 0;
	//stats.ctSize = 0;

	// full cover db
	islist::iterator cit,cend;
	double codeLen;
	for(uint32 i=0; i<mNumDBRows; i++) {
		codeLen = 0.0;
		mCoverSet->InitSet(mDB->GetRow(i));

		for(uint32 j=mMaxIdx; j>=1; j--) {
			if(mCT[j] != NULL) {
				cend = mCT[j]->GetList()->end();
				for(cit = mCT[j]->GetList()->begin(); cit != cend; ++cit) {
					if(mCoverSet->Cover(j+1,*cit)) {
						fprintf(fpAll, "{%s} ", ((ItemSet*)(*cit))->ToString(false, false).c_str() );
						fprintf(fpAll, "(%f) ", CalcCodeLength(((ItemSet*)(*cit))->GetUsageCount(), stats.usgCountSum) );
						codeLen += CalcCodeLength(((ItemSet*)(*cit))->GetUsageCount(), stats.usgCountSum);
	//					((ItemSet*)(*cit))->AddOneToCount();
	//					stats.countSum++;
					}
				}
			}
		}
		fprintf(fpAll, "alph: %s\n", mCoverSet->ToString().c_str() );
		for(uint32 i=0; i<mABLen; i++)
			if(mCoverSet->IsItemUncovered(i)) {
				fprintf(fpAll, "%d=(%f) ", i, CalcCodeLength(mAlphabet[i], stats.usgCountSum) );
				codeLen += CalcCodeLength(mAlphabet[i], stats.usgCountSum);
			}
		fprintf(fpAll, "all = %f\n", codeLen);
		// alphabet items
	//	stats.countSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet);
	}
/*
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
				stats.ctSize += ((ItemSet*)(*cit))->GetStandardLength() + codeLen;
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
*/
	fclose(fpAll);
}
void CFCodeTable::SetAlphabetCount(uint32 item, uint32 count) {
	mAlphabet[item] = count;
}
uint32 CFCodeTable::GetAlphabetCount(uint32 item) {
	return mAlphabet[item];
}
