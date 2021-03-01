#ifdef BROKEN

/* CoverFull Alternative... Playground for editing CoverFull */

#include "../../global.h"

#include <stdlib.h>

#include "../CodeTable.h"
#include "../../db/Database.h"
#include "../../itemstructs/Uint16ItemSet.h"
#include "../../itemstructs/CoverSet.h"
#include "../../isc/ItemSetCollection.h"

#include "../CTISList.h"

#include "CFACodeTable.h"

//#define PRUNE_DEBUG

CFACodeTable::CFACodeTable() : CFCodeTable() {
	mCT = NULL;
	mCoverSet = NULL;
	mStdLengths = NULL;
	mNumDBRows = 0;
	mCTLen = 0;
	mMaxIdx = 0;

	mAlphabet = NULL;
	mABLen = 0;
	mABUsedLen = 0;
}

CFACodeTable::~CFACodeTable() {
	delete mCT;
	delete mCoverSet;
	delete[] mAlphabet;
	delete[] mOldAlphabet;
	delete[] mABUsed;
	delete[] mABUnused;
}

void CFACodeTable::Clean() {
	CodeTable::Clean();
	delete mCoverSet;		mCoverSet = NULL;
	delete[] mAlphabet;		mAlphabet = NULL;
	delete[] mOldAlphabet;	mOldAlphabet = NULL;
	delete[] mABUsed;		mABUsed = NULL;
	delete[] mABUnused;		mABUnused = NULL;
	mNumDBRows = 0;
}

void CFACodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 toMinSup) {
	CodeTable::UseThisStuff(db, type, ctinit);

	// Codetable
	if(mCT == NULL) {
		mCT = new CTISList();
		mCurNumSets = 0;
		mMaxIdx = 0;
	}

	mStdLengths = db->GetStdLengths();

	// Add Alphabet
	alphabet *a = db->GetAlphabet();
	mABLen = a->size();
	uint32 pruneBelow = min(db->GetPrunedBelow(), toMinSup);
	mAlphabet = new uint32[mABLen];
	mOldAlphabet = new uint32[mABLen];
	uint32 countSum = 0, acnt=0;
	uint32 cnt = 0;
	alphabet::iterator ait, aend=a->end();
	mABUsedLen = 0;
	mABUnusedCount = 0;
	for(ait = a->begin(); ait != aend; ++ait) {
		if(ait->first != cnt)
			throw string("ja, maar, dat kan zo maar niet!");
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
	ECHO(2, printf(" * Alphabet length:\t%d/%d (used/total)\n", mABUsedLen, mABLen));
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

	mNumDBRows = mDB->GetNumRows();

	mCurStats.countSum = countSum;
	mCurStats.dbSize = db->GetStdDbSize();
	mCurStats.ctSize = ctSize;
	mCurStats.size = mCurStats.dbSize + mCurStats.ctSize;
	mCurStats.numSetsUsed = 0;
	mCurStats.alphItemsUsed = a->size();
	mPrevStats = mCurStats;

	// CoverSet
	mCoverSet = CoverSet::Create(mCoverSetType, a->size());
}

void CFACodeTable::Add(ItemSet *is) {
#ifdef PRUNE_DEBUG
	printf("\nAdd: %s\n", is->ToString().c_str());
#endif
	uint32 len = is->GetLength();
	if(mCT == NULL) {
		mCT = new CTISList();
	}
	islist::iterator iter, lend = mCT->GetList()->end();
	for(iter=mCT->GetList()->begin(); iter!=lend; ++iter)
		if(len > ((ItemSet*)(*iter))->GetLength())
			break;

	mCT->GetList()->insert(iter,is);
	++mCurNumSets;

	mAdded = is;
}

void CFACodeTable::Del(ItemSet *is, bool zap, bool keepList) {
#ifdef PRUNE_DEBUG
	printf("Del: %s (zap=%d, keepList=%d)\n", is->ToString().c_str(), zap, keepList);
#endif

	islist *lst = mCT->GetList();

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

	if(zap)
		delete is;
}

void CFACodeTable::CommitAdd() {
	// only commits/rollbacks add
#ifdef PRUNE_DEBUG
	if(mAdded != NULL)
		printf("Commit add: %s\n", mAdded->ToString().c_str());
	else
		printf("Commit add (NULL)\n");
#endif
	mAdded = NULL;
	mPrevStats = mCurStats;
}

void CFACodeTable::CommitLastDel(bool zap) {
#ifdef PRUNE_DEBUG
	printf("Commit del: %s (zap=%d)\n", mLastDel.is->ToString().c_str(), zap);
#endif
	if(zap)
		delete mLastDel.is;
	mLastDel.is = NULL;
	mCurStats = mPrunedStats;
}

void CFACodeTable::CommitAllDels() {
#ifdef PRUNE_DEBUG
	printf("Commit alldels (%d)\n", mDeleted.size());
#endif
	dellist::iterator iter, itend = mDeleted.end();
	for(iter=mDeleted.begin(); iter!=itend; ++iter)
		delete ((DelInfo)*iter).is;
	mDeleted.clear();
	mLastDel.is = NULL;
}

void CFACodeTable::RollbackAdd() {
	// Rolls back the added ItemSet
	// Restores DB & CT sizes
	if(mAdded != NULL) {
#ifdef PRUNE_DEBUG
		printf("Rollback add: %s\n", mAdded->ToString().c_str());
#endif
		Del(mAdded, true, false);
		mAdded = NULL;
		mLastDel.is = NULL;
		mCurStats = mPrevStats;
	}
}

void CFACodeTable::RollbackLastDel() {
	if(mLastDel.is != NULL)	{
#ifdef PRUNE_DEBUG
		printf("Rollback del: %s\n", mLastDel.is->ToString().c_str());
#endif
		uint32 len = mLastDel.is->GetLength();
		islist *lst = mCT->GetList();

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
		++mCurNumSets;

		if(!mDeleted.empty())
			mDeleted.pop_front();
	}
}

void CFACodeTable::RollbackAllDels() {
	uint32 len;
	islist *lst;
	DelInfo di;
#ifdef PRUNE_DEBUG
	printf("Rollback alldels (%d)\n", mDeleted.size());
#endif
	dellist::iterator iter, itend = mDeleted.end();
	for(iter=mDeleted.begin(); iter!=itend; ++iter) {
		di = (DelInfo)(*iter);
#ifdef PRUNE_DEBUG
		printf("\t%s\n", di.is->ToString().c_str());
#endif
		len = di.is->GetLength();
		lst = mCT->GetList();

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
	mCurNumSets += mDeleted.size();
	mDeleted.clear();
	mLastDel.is = NULL;
}

void CFACodeTable::RollbackCounts() {
	if(mCT != NULL)
		mCT->RollbackCounts();
	memcpy(mAlphabet, mOldAlphabet, mABLen * sizeof(uint32));
}

islist* CFACodeTable::GetPrePruneList() {
	islist *ilist = new	islist();
	if(mCT != NULL)
		mCT->BuildPrePruneList(ilist);
#ifdef PRUNE_DEBUG
	printf("PruneList: ");
	islist::iterator iter, iend = ilist->end();
	for(iter=ilist->begin(); iter!=iend; ++iter)
		printf("%s ", (ItemSet*)(*iter)->ToString().c_str());
	printf("\n");
#endif
	return ilist;
}
ItemSet** CFACodeTable::GetEncodingMap() {
	ItemSet **map = new ItemSet *[mCurNumSets];
	islist::iterator cit, cend;
	uint32 idx = 0;
	if(mCT != NULL) {
		cend = mCT->GetList()->end();
		for(cit = mCT->GetList()->begin(); cit != cend; ++cit) {
			map[idx++] = *cit;
		}
	}
	return map;
}
islist* CFACodeTable::GetPostPruneList() {
	islist *pruneList = new islist();
	UpdatePostPruneList(pruneList, pruneList->begin());
	return pruneList;
}

void CFACodeTable::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList, after);
	if(mCT != NULL)
		mCT->BuildPostPruneList(pruneList, after);
}
void CFACodeTable::SortPostPruneList(islist *pruneList, const islist::iterator &after) {
	islist::iterator start, a, b, c, end;
	ItemSet *isB, *isC;

	start = after;
	++start;

	a = start;
	end = pruneList->end();
	if(a == end || ++a == end) // check whether at least 2 items left, otherwise sorting is quite useless
		return;

	--end;
	bool changed = false;
	for(a=start; a!=end; ++a) {
		changed = false;
		for(c=b=a,++c, isB=(ItemSet*)(*b); b!=end; b=c, isB=isC, ++c) {
			isC = (ItemSet*)(*c);
			if(isB->GetCount() > isC->GetCount()) {
				changed = true;
				*b = isC;
				*c = isC = isB;
			}
		}
		if(!changed)
			break;
	}
}

void CFACodeTable::CoverDB(CoverStats &stats) {
	// init counts
	if(mCT != NULL)
		mCT->ResetCounts();
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
	for(uint32 i=0; i<mABUsedLen; i++)
		mAlphabet[mABUsed[i]] = 0;

	stats.countSum = mABUnusedCount;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.dbSize = 0;
	stats.ctSize = 0;

	// full cover db
	islist::iterator cit,cend;
	for(uint32 i=0; i<mNumDBRows; i++) {
		mCoverSet->InitSet(mDB->GetItemSet(i));

		if(mCT != NULL) {
			cend = mCT->GetList()->end();
			for(cit = mCT->GetList()->begin(); cit != cend; ++cit) {
				if(mCoverSet->Cover(((ItemSet*)(*cit))->GetLength(),*cit)) {
					((ItemSet*)(*cit))->AddOneToCount();
					stats.countSum++;
				}
			}
		}
		// alphabet items
		stats.countSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet);
	}

	double codeLen = 0;
	if(mCT != NULL) {
		cend = mCT->GetList()->end();
		for (cit = mCT->GetList()->begin(); cit != cend; ++cit) {
			uint32 curcnt = ((ItemSet*)(*cit))->GetCount();
			if(curcnt == 0) continue;
			stats.numSetsUsed++;
			codeLen = CalcCodeLength(curcnt, stats.countSum);
			stats.dbSize += curcnt * codeLen;
			stats.ctSize += ((ItemSet*)(*cit))->GetStandardLength() + codeLen;
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
}

void CFACodeTable::AddOneToEachCount() {
	islist::iterator cit,cend;

	if(mCT != NULL) {
		cend = mCT->GetList()->end();
		for(cit = mCT->GetList()->begin(); cit != cend; ++cit) {
			((ItemSet*)(*cit))->AddOneToCount();
			mCurStats.countSum++;
		}
	}
	// don't forget the alphabet!
	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i]++;
	mCurStats.countSum += mABLen;
}
void CFACodeTable::NormalizeCounts(uint32 total) {
	islist::iterator cit,cend;
	uint32 cnt, newCnt;
	double factor = ((double) total / (double) mCurStats.countSum );
	mCurStats.countSum = 0;
		if(mCT != NULL) {
			cend = mCT->GetList()->end();
			for(cit = mCT->GetList()->begin(); cit != cend; ++cit) {
				cnt = ((ItemSet*)(*cit))->GetCount();
				newCnt = (uint32) ceil(cnt * factor);
				if(newCnt == 0)	newCnt = 1;
				((ItemSet*)(*cit))->SetCount(newCnt);
				mCurStats.countSum += newCnt;
			}
		}
		// don't forget the alphabet!
		for(uint32 i=0; i<mABLen; i++) {
			cnt = mAlphabet[i];
			newCnt = (uint32)ceil(cnt * factor);
			if(newCnt == 0)	newCnt = 1;
			mAlphabet[i] = newCnt;
			mCurStats.countSum += newCnt;
		}
}
double	CFACodeTable::CalcRowCodeLength(ItemSet *row) {
	islist::iterator cit,cend;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, mDB->GetAlphabet()->size());
	mCoverSet->InitSet(row);

	double codeLen = 0.0;
		if(mCT != NULL) {
			cend = mCT->GetList()->end();
			for(cit = mCT->GetList()->begin(); cit != cend; ++cit)
				if(mCoverSet->Cover(((ItemSet*)(*cit))->GetLength(),*cit))
					codeLen += CalcCodeLength((*cit)->GetCount(), mCurStats.countSum);
		}
	// alphabet items
	for(uint32 i=0; i<mABLen; i++)
		if(mCoverSet->IsItemInSet(i))
			codeLen += CalcCodeLength(mAlphabet[i], mCurStats.countSum);
	return codeLen;
}
uint32 CFACodeTable::EncodeRow(ItemSet *row, double &codeLen, uint16 *temp) {
	islist::iterator cit,cend;
	if(mCoverSet == NULL)
		mCoverSet = CoverSet::Create(mCoverSetType, mDB->GetAlphabet()->size());
	mCoverSet->InitSet(row);

	codeLen = 0.0;
	uint16 id = 0;
	uint32 idx = 0;
		if(mCT != NULL) {
			cend = mCT->GetList()->end();
			for(cit = mCT->GetList()->begin(); cit != cend; ++cit) {
				if(mCoverSet->Cover(((ItemSet*)(*cit))->GetLength(),*cit)) {
					codeLen += CalcCodeLength((*cit)->GetCount(), mCurStats.countSum);
					temp[idx++] = id;
				}
				id++;
			}
		}
	// alphabet items
	for(uint32 i=0; i<mABLen; i++) {
		if(mCoverSet->IsItemInSet(i)) {
			codeLen += CalcCodeLength(mAlphabet[i], mCurStats.countSum);
			temp[idx++] = id;
		}
		id++;
	}

	return idx;
}

void	CFACodeTable::CoverMaskedDB(CoverStats &stats, bool *dbSkipMask) {
	// init counts
	if(mCT != NULL)
		mCT->ResetCounts();
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));
	for(uint32 i=0; i<mABLen; i++)
		mAlphabet[i] = 0;

	stats.countSum = mABUnusedCount;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.dbSize = 0;
	stats.ctSize = 0;
	stats.size = 0;

	// full cover db
	islist::iterator cit,cend;
	for(uint32 i=0; i<mNumDBRows; i++) {
		if(dbSkipMask[i])
			continue;

		mCoverSet->InitSet(mDB->GetItemSet(i));

		cend = mCT->GetList()->end();
		for(cit = mCT->GetList()->begin(); cit != cend; ++cit) {
			if(mCoverSet->Cover(((ItemSet*)(*cit))->GetLength(),*cit)) {
				((ItemSet*)(*cit))->AddOneToCount();
				stats.countSum++;
			}
		}
		// alphabet items
		stats.countSum += mCoverSet->CoverAlphabet(mABLen, mAlphabet);
	}

	double codeLen = 0;
	uint32 curcnt;
	cend = mCT->GetList()->end();
	for (cit = mCT->GetList()->begin(); cit != cend; ++cit) {
		curcnt = ((ItemSet*)(*cit))->GetCount();
		if(curcnt == 0) continue;
		stats.numSetsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.countSum);
		stats.dbSize += curcnt * codeLen;
		stats.ctSize += ((ItemSet*)(*cit))->GetStandardLength() + codeLen;
	}
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		curcnt = mAlphabet[i];
		if(curcnt == 0) continue;
		stats.alphItemsUsed++;
		codeLen = CalcCodeLength(curcnt, stats.countSum);
		stats.dbSize += curcnt * codeLen;
		stats.ctSize += mStdLengths[i] + codeLen;
	}

	stats.size = stats.dbSize + stats.ctSize;
}

// Stats
void CFACodeTable::WriteCodeLengthsToFile(const string &outFile) { 
	FILE *fpAll = fopen(outFile.c_str(), "w");
	if(!fpAll)
		throw string("CtAnalyser could not write file: ") + outFile;

	double codeLen;
	uint32 curcnt;
	islist::iterator cit, cend;

	cend = mCT->GetList()->end();
	for (cit = mCT->GetList()->begin(); cit != cend; ++cit) {
		curcnt = ((ItemSet*)(*cit))->GetCount();
		if(curcnt == 0)
			throw string("Writing CodeLengths To File:: count 0 shouldn't happen!");
		codeLen = CalcCodeLength(curcnt, mCurStats.countSum);
		fprintf(fpAll, "%f,", codeLen);
	}
	fseek(fpAll, -1, SEEK_CUR);	/* check return value? */
	fprintf(fpAll, "\n");
	// Alphabet
	for(uint32 i=0; i<mABLen; i++) {
		curcnt = mAlphabet[i];
		if(curcnt == 0)
			throw string("Writing CodeLengths To File:: count 0 shouldn't happen!");
		codeLen = CalcCodeLength(curcnt, mCurStats.countSum);
		fprintf(fpAll, "%f,", codeLen);
	}
	fseek(fpAll, -1, SEEK_CUR); /* check return value? */
	fprintf(fpAll, "\n");

	fclose(fpAll);
}

#endif // BROKEN
