#include "../../../../global.h"

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <omp.h>
#include <cassert>

#include "CCCPCodeTable.h"
#include "../CTISList.h"

template <ItemSetType T>
CCCPCodeTable<T>::CCCPCodeTable() : CodeTable() {
	mNumDBRows = 0;
	mAlphabetSize = 0;
	mABUsedLen = 0;
	mChangeType = ChangeNone;

	mChangeItemMask = NULL;
	mMask = NULL;
	mBackupMask = NULL;
}

template <ItemSetType T>
CCCPCodeTable<T>::CCCPCodeTable(const CCCPCodeTable &ct) : CodeTable(ct) {
	mCT.reserve(ct.mCT.size());
	for (uint32 i=0; i<ct.mCT.size(); i++) {
		ct.mCT[i]->Ref();
		mCT.push_back(ct.mCT[i]);
	}
	mCTMasks = ct.mCTMasks;
	mCTLen = ct.mCTLen;
	mCTIdx = ct.mCTIdx;
	mUsgCounts = ct.mUsgCounts;
	mBackupUsgCounts = ct.mBackupUsgCounts;
	mUsgCountForItemset = ct.mUsgCountForItemset;

	mCDB = ct.mCDB;

	mAbDbOccs = ct.mAbDbOccs;

	mNumDBRows = ct.mNumDBRows;

	mUseOccs = ct.mUseOccs;

	mAlphabet = ct.mAlphabet;
	mOldAlphabet = ct.mOldAlphabet;
	mABUsedLen = ct.mABUsedLen;
	mABUnusedCount = ct.mABUnusedCount;

	mIsBinnedDB = ct.mIsBinnedDB;

	mChangeType = ChangeNone;

	mChangeItemMask = CoverMask<T>::Create();
	mMask = CoverMask<T>::Create();
	mBackupMask = CoverMask<T>::Create();
}

template <ItemSetType T>
CCCPCodeTable<T>::~CCCPCodeTable() {
	for (uint32 i=0; i<mCT.size(); i++) {
		mCT[i]->UnRef();
	}
	delete[] mChangeItemMask;
	delete[] mMask;
	delete[] mBackupMask;
}
	

template <ItemSetType T>
void CCCPCodeTable<T>::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CodeTable::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);

	mCTLen = maxCTElemLength==0 ? db->GetMaxSetLength() : maxCTElemLength;
	mCTIdx.resize(mCTLen + 1, 0);

	// Add Alphabet
	alphabet *a = db->GetAlphabet();
	mAlphabetSize = (uint32)a->size();
	//uint32 pruneBelow = db->GetPrunedBelow();
	uint32 pruneBelow = 0;
	mAlphabet.resize(mAlphabetSize);
	mOldAlphabet.resize(mAlphabetSize);
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

	mOldAlphabet = mAlphabet;

	mNumDBRows = mDB->GetNumRows();

	mUseOccs.reserve(mNumDBRows);

	mCurStats.usgCountSum = countSum;
	mCurStats.encDbSize = db->GetStdDbSize();
	mCurStats.encCTSize = ctSize;
	mCurStats.encSize = mCurStats.encDbSize + mCurStats.encCTSize;
	mCurStats.numSetsUsed = 0;
	mCurStats.alphItemsUsed = mABUsedLen;
	mPrevStats = mCurStats;

	CoverMask<T>::UseThisStuff(db, mAlphabetSize);
	mChangeItemMask = CoverMask<T>::Create();
	mMask = CoverMask<T>::Create();
	mBackupMask = CoverMask<T>::Create();
	mMask2 = CoverMask<T>::Create();

	// en nu gaan we hier een eigen db structuur maken...
	InitCoverDB();

	mIsBinnedDB = mDB->HasBinSizes();

	SetProvideOccs();
}

template <ItemSetType T>
void CCCPCodeTable<T>::InitCoverDB() {
	mCDB.resize(mCTLen);
	// this can be rowLen instead of mCTLen
	for(uint32 i=0; i<mCTLen; i++) {
		mCDB[i].Reserve(mNumDBRows);
	}

	for(uint32 i=0; i<mNumDBRows; i++) {
		mMask->LoadFromItemSet(mDB->GetRow(i));
		// this can be rowLen instead of mCTLen
		for(uint32 j=0; j<mCTLen; j++) {
			mCDB[j].Insert(i, mMask);
		}
	}
}

template<>
void CCCPCodeTable<Uint16ItemSetType>::InitCoverDB() {
	uint32 size = 0;
	for(uint32 i=0; i<mNumDBRows; i++) {
		size += mDB->GetRow(i)->GetLength() + 1; // +1 for length
	}
	mCDB.resize(mCTLen);
	for(uint32 i=0; i<mCTLen; i++) {
		mCDB[i].Reserve(mNumDBRows, size);
	}
	
	for(uint32 i=0; i<mNumDBRows; i++) {
		mMask->LoadFromItemSet(mDB->GetRow(i));
		for(uint32 j=0; j<mCTLen; j++) {
			mCDB[j].Insert(i, mMask);
		}
	}
}

template <ItemSetType T>
void CCCPCodeTable<T>::SetProvideOccs() {
	mAbDbOccs.resize(mAlphabetSize);

	ItemSet **rows = mDB->GetRows();
	uint32 *vals = new uint32[mDB->GetMaxSetLength()];

	for(uint32 i=0; i<mNumDBRows; i++) {
		uint32 numVals = rows[i]->GetLength();
		rows[i]->GetValuesIn(vals);
		for(uint32 j=0; j<numVals; j++) {
			mAbDbOccs[vals[j]].push_back(i);
		}
	}

	delete[] vals;
}

template <ItemSetType T>
void CCCPCodeTable<T>::Add(ItemSet *is, uint64 candidateId) {
	uint32 len = is->GetLength();
	mChangeType = ChangeAdd;
	mChangeItemCandidateId = candidateId;
	mChangeItem = is;
	mChangeItemIdx = mCTIdx[len-1]; // Insert at end of row
	mChangeItemUsgCount = 0;
	mChangeItemMask->LoadFromItemSet(is);
	mCurNumSets++;
}

template <ItemSetType T>
void CCCPCodeTable<T>::Del(ItemSet* is, bool zap, bool keepList) {
	uint32 len = is->GetLength();
	int32 i;
	for(i=mCTIdx[len-1]-1; i>=(int32)mCTIdx[len]; i--) {
		if(mCT[i] == is) {
			break;
		}
	}
	if (i<(int32)mCTIdx[len]) {
		THROW("Itemset not found!");
	}

	mChangeType = ChangeDel;
	mChangeItem = is;
	mChangeItemIdx = i;
	if (zap) {
		mCT.erase(mCT.begin() + mChangeItemIdx);
		mCTMasks.Erase(mChangeItemIdx);
		mUsgCounts.erase(mUsgCounts.begin() + mChangeItemIdx);
		mBackupUsgCounts.erase(mBackupUsgCounts.begin() + mChangeItemIdx);
		uint32 len = is->GetLength();
		for (int32 i=len-1; i>=0; i--) {
			mCTIdx[i]--;
		}
		mUsgCountForItemset.erase(mChangeItem);

		mChangeType = ChangeNone;
		mChangeItem->UnRef();
		mChangeItem = NULL;
	} else {
		mChangeItemMask->LoadFromItemSet(is);
	}

	mCurNumSets--;
}

template <ItemSetType T>
void CCCPCodeTable<T>::CommitAdd(bool updateLog, bool updateInsPoints) {
	if(updateLog)
		UpdateCTLog();

	uint32 len = mChangeItem->GetLength();
	if (updateInsPoints) {
		// alle csets nu uptodate maken
		for(uint32 r=0; r<mUseOccs.size(); r++) {
			uint32 i = mUseOccs[r];
			uint32 insPoint = mCTLen - len + 1;
			mMask->Load(mCDB[insPoint][i]);
			mMask->Cover(mChangeItemMask);
			mMask->Store(mCDB[insPoint++][i]);
			for(int32 j=len-1; j>1; --j) {
				for(uint32 idx = mCTIdx[j]; idx != mCTIdx[j-1]; ++idx) {
					if(mUsgCounts[idx]==0)
						continue;
					mMask->Cover(mCTMasks[idx]);
				}
				mMask->Store(mCDB[insPoint++][i]);
			}
		}
	}

	mCT.insert(mCT.begin() + mChangeItemIdx, mChangeItem);
	mCTMasks.Insert(mChangeItemIdx, mChangeItemMask);
	mUsgCounts.insert(mUsgCounts.begin() + mChangeItemIdx, mChangeItemUsgCount);
	mBackupUsgCounts.insert(mBackupUsgCounts.begin() + mChangeItemIdx, 0);
	for(int32 i=len-1; i>=0; --i) {
		mCTIdx[i]++;
	}

	for(uint32 i=mChangeItemIdx; i<mCT.size(); i++) {
		mUsgCountForItemset[mCT[i]] = mUsgCounts[i];
	}
	
	mPrevStats = mCurStats;
	mChangeItem = NULL;
	mChangeItemUsgCount = 0;
	mChangeType = ChangeNone;
}

template <ItemSetType T>
void CCCPCodeTable<T>::CommitLastDel(bool zap, bool updateLog, bool updateInsPoints) {
	if(mChangeType == ChangeDel) {
		if(updateLog)
			UpdateCTLog();

		uint32 len = mChangeItem->GetLength();
		mCT.erase(mCT.begin() + mChangeItemIdx);
		mCTMasks.Erase(mChangeItemIdx);
		mUsgCounts.erase(mUsgCounts.begin() + mChangeItemIdx);
		mBackupUsgCounts.erase(mBackupUsgCounts.begin() + mChangeItemIdx);
		for (int32 i=len-1; i>=0; i--) {
			mCTIdx[i]--;
		}

		mUsgCountForItemset.erase(mChangeItem);
		for(uint32 i=mChangeItemIdx; i<mCT.size(); i++) {
			mUsgCountForItemset[mCT[i]] = mUsgCounts[i];
		}

		mPrevStats = mCurStats;
		mChangeType = ChangeNone;
		if(zap)
			mChangeItem->UnRef();
		mChangeItem = NULL;
		mChangeItemUsgCount = 0;

		if (updateInsPoints) {
			for(uint32 r=0; r<mUseOccs.size(); r++) {
				uint32 insPoint = mCTLen - len;	// insPoint -voor- mChangeRow
				uint32 i = mUseOccs[r];
				mMask->Load(mCDB[insPoint++][i]);
				for (int32 j=len; j>1; --j) {
					for(uint32 idx = mCTIdx[j]; idx != mCTIdx[j-1]; ++idx) {
						if(mUsgCounts[idx]==0)
							continue;
						mMask->Cover(mCTMasks[idx]);
					}
					mMask->Store(mCDB[insPoint++][i]);
				}
			}
		}
	}
}


template <ItemSetType T>
void CCCPCodeTable<T>::RollbackAdd() {
	if(mChangeType == ChangeAdd && mChangeItem != NULL) {
		mChangeItem->UnRef();
		mCurStats = mPrevStats;
		RollbackCounts();
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeItemUsgCount = 0;
		mCurNumSets--;
	}

	// either clear usage-update lists here, or at beginning of cover
}

template <ItemSetType T>
void CCCPCodeTable<T>::RollbackLastDel() {
	if(mChangeType == ChangeDel)	{
		// Nothing has really been deleted, so we only have to clear the mChange-info and stats
		mChangeType = ChangeNone;
		mChangeItem = NULL;
		mChangeItemUsgCount = 0;
		mCurStats = mPrevStats;
		mCurNumSets++;
		RollbackCounts();
	}
	// either clear usage-update lists here, or at beginning of cover
}

template <ItemSetType T>
void CCCPCodeTable<T>::RollbackCounts() {
	if (mRollbackAlph) {
		mUsgCounts.swap(mBackupUsgCounts);
		mAlphabet.swap(mOldAlphabet);
	}
}

template <ItemSetType T>
void CCCPCodeTable<T>::AddAndCommit(ItemSet *is, uint64 candidateId) {
	Add(is, candidateId);

	CoverStats& stats = mCurStats;
	uint32 len = mChangeItem->GetLength();
	vector<uint32>& occur = mAbDbOccs[mChangeItem->GetLastItem()];
	mBackupUsgCounts = mUsgCounts;
	mOldAlphabet = mAlphabet;

	for(uint32 r=0; r<occur.size(); r++) {
		uint32 i = occur[r];
		uint32 insPoint = mCTLen - len + 1;
		if(mCDB[insPoint][i]->CanCover(mChangeItemMask)) {
			uint32 binSize = mIsBinnedDB ? mDB->GetRow(i)->GetUsageCount() : 1;
			mMask->Load(mCDB[insPoint][i]);
			mBackupMask->Load(mMask);
			mMask->Cover(mChangeItemMask);
			mMask->Store(mCDB[insPoint++][i]);

			mChangeItemUsgCount += binSize;
			stats.usgCountSum += binSize;

			for(int32 j=len-1; j>1; --j) {
				for(uint32 idx = mCTIdx[j]; idx != mCTIdx[j-1]; ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] -= binSize;
						stats.usgCountSum -= binSize;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] += binSize;
						stats.usgCountSum += binSize;
					}
				}
				mMask->Store(mCDB[insPoint++][i]);
			}
			stats.usgCountSum -= mBackupMask->UncoverAlphabet(mAlphabet, binSize);			
			stats.usgCountSum += mMask->CoverAlphabet(mAlphabet, binSize);
		}
	}
	CalcTotalSizeForCover(stats);

	CommitAdd(false, false);
}

template <ItemSetType T>
void CCCPCodeTable<T>::DelAndCommit(ItemSet *is, bool zap) {
	Del(is, false, false);

	CoverStats& stats = mCurStats;
	mBackupUsgCounts = mUsgCounts;
	mOldAlphabet = mAlphabet;
	vector<uint32>& occur = mAbDbOccs[mChangeItem->GetLastItem()];
	uint32 len = mChangeItem->GetLength();

	stats.usgCountSum -= mUsgCounts[mChangeItemIdx];
	mUsgCounts[mChangeItemIdx] = 0;
	mChangeItemUsgCount = 0;

	for(uint32 r=0; r<occur.size(); r++) {
		uint32 insPoint = mCTLen - len;	// insPoint -voor- mChangeRow
		uint32 i = occur[r];
		mMask->Load(mCDB[insPoint++][i]);
		// begin van rij 'len' tot mChangeItemIdx om cset op te bouwen
		for(uint32 idx = mCTIdx[len]; idx < mChangeItemIdx; ++idx) {
			if(mUsgCounts[idx]==0)
				continue;
			mMask->Cover(mCTMasks[idx]);
		}

		// Alleen rest doen als mChangeItem ook echt gebruikt wordt
		if(mMask->CanCover(mChangeItemMask)) {
			uint32 binSize = mIsBinnedDB ? mDB->GetRow(i)->GetUsageCount() : 1;
			mBackupMask->Load(mMask);
			mBackupMask->Cover(mChangeItemMask);
	
			// Rest van rij 'len'
			for(uint32 idx=mChangeItemIdx+1; idx < mCTIdx[len-1]; ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] -= binSize;
						stats.usgCountSum -= binSize;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] += binSize;
						stats.usgCountSum += binSize;
					}
			}
			mMask->Store(mCDB[insPoint++][i]);

			for(uint32 j=len-1; j>1; j--) {
				for(uint32 idx=mCTIdx[j]; idx != mCTIdx[j-1]; ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] -= binSize;
						stats.usgCountSum -= binSize;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] += binSize;
						stats.usgCountSum += binSize;
					}
				}
				mMask->Store(mCDB[insPoint++][i]);
			}
			stats.usgCountSum -= mBackupMask->UncoverAlphabet(mAlphabet, binSize);
			stats.usgCountSum += mMask->CoverAlphabet(mAlphabet, binSize);
		}
	}
	CalcTotalSizeForCover(stats);

	CommitLastDel(zap, false, false);
}

template <ItemSetType T>
void CCCPCodeTable<T>::UndoPrune(ItemSet *is) {
	uint32 len = is->GetLength();
	mChangeType = ChangeAdd;
	mChangeItem = is;
	uint32 i=mCTIdx[len];
	while(i < mCTIdx[len-1] && is->GetID() > mCT[i]->GetID()) {
		i++;
	}
	mChangeItemIdx = i;
	mChangeItemUsgCount = 0;
	mChangeItemMask->LoadFromItemSet(is);


	CoverStats& stats = mCurStats;
	vector<uint32>& occur = mAbDbOccs[mChangeItem->GetLastItem()];
	mBackupUsgCounts = mUsgCounts;
	mOldAlphabet = mAlphabet;

	for(uint32 r=0; r<occur.size(); r++) {
		uint32 i = occur[r];
		uint32 insPoint = mCTLen - len; // insPoint voor rij 'len'
		mMask->Load(mCDB[insPoint++][i]);
		// begin van rij 'len' tot mChangeItemIdx om cset op te bouwen
		for(uint32 idx = mCTIdx[len]; idx < mChangeItemIdx; ++idx) {
			if(mUsgCounts[idx]==0)
				continue;
			mMask->Cover(mCTMasks[idx]);
		}

		if(mMask->CanCover(mChangeItemMask)) {
			uint32 binSize = mIsBinnedDB ? mDB->GetRow(i)->GetUsageCount() : 1;
			mBackupMask->Load(mMask);
			mMask->Cover(mChangeItemMask);
			mChangeItemUsgCount += binSize;
			stats.usgCountSum += binSize;

			// Rest van rij 'len'
			for(uint32 idx=mChangeItemIdx; idx < mCTIdx[len-1]; ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] -= binSize;
						stats.usgCountSum -= binSize;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] += binSize;
						stats.usgCountSum += binSize;
					}
			}
			mMask->Store(mCDB[insPoint++][i]);

			for(int32 j=len-1; j>1; --j) {
				for(uint32 idx = mCTIdx[j]; idx != mCTIdx[j-1]; ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] -= binSize;
						stats.usgCountSum -= binSize;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] += binSize;
						stats.usgCountSum += binSize;
					}
				}
				mMask->Store(mCDB[insPoint++][i]);
			}
			stats.usgCountSum -= mBackupMask->UncoverAlphabet(mAlphabet, binSize);			
			stats.usgCountSum += mMask->CoverAlphabet(mAlphabet, binSize);
		}
	}
	CalcTotalSizeForCover(stats);

	CommitAdd(false, false);
}

template <ItemSetType T>
void CCCPCodeTable<T>::CoverDB(CoverStats &stats) {
	if(mIsBinnedDB)			// switch binned/notbinned
		CoverBinnedDB(stats);
	else
		CoverNoBinsDB(stats);
}

template <ItemSetType T>
void CCCPCodeTable<T>::CoverNoBinsDB(CoverStats &stats) {
	if(mChangeType == ChangeAdd) {
		uint32 len = mChangeItem->GetLength();
		vector<uint32>& occur = mAbDbOccs[mChangeItem->GetLastItem()];
		uint32 insPoint = mCTLen - len + 1;
		mUseOccs.clear();
		mRollbackAlph = false;

		for(uint32 r=0; r<occur.size(); r++) {
			uint32 i = occur[r];
			if(mCDB[insPoint][i]->CanCover(mChangeItemMask)) {
				mUseOccs.push_back(i);
				if (!mRollbackAlph) {
					mRollbackAlph = true;
					mBackupUsgCounts = mUsgCounts;
					mOldAlphabet = mAlphabet;
				}

				mMask->Load(mCDB[insPoint][i]);
				mBackupMask->Load(mMask);
				mMask->Cover(mChangeItemMask);
				mChangeItemUsgCount++;
				stats.usgCountSum++;

				for(uint32 idx = mChangeItemIdx; idx < mCT.size(); ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx]--;
						stats.usgCountSum--;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx]++;
						stats.usgCountSum++;
					}
				}

				stats.usgCountSum -= mBackupMask->UncoverAlphabet(mAlphabet);			
				stats.usgCountSum += mMask->CoverAlphabet(mAlphabet);
			}
		}
		if (mUseOccs.empty()) {
			return;
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
		CalcTotalSizeForCover(stats);

	} else if(mChangeType == ChangeDel) {
		stats = mCurStats;
		uint32 len = mChangeItem->GetLength();
		vector<uint32>& occur = mAbDbOccs[mChangeItem->GetLastItem()];
		uint32 insPoint = mCTLen - len;	// insPoint -voor- mChangeRow

		mUseOccs.clear();
		mRollbackAlph = true;
		mBackupUsgCounts = mUsgCounts;
		mOldAlphabet = mAlphabet;

		stats.usgCountSum -= mUsgCounts[mChangeItemIdx];
		mUsgCounts[mChangeItemIdx] = 0;
		mChangeItemUsgCount = 0;

		for(uint32 r=0; r<occur.size(); r++) {
			uint32 i = occur[r];
			mMask->Load(mCDB[insPoint][i]);

			// begin van rij `len' tot mChangeItemIdx om cset op te bouwen
			for(uint32 idx = mCTIdx[len]; idx < mChangeItemIdx; ++idx) {
				if(mUsgCounts[idx]==0)
					continue;
				mMask->Cover(mCTMasks[idx]);
			}

			// Alleen rest doen als mChangeItem ook echt gebruikt wordt
			if(mMask->CanCover(mChangeItemMask)) {
				mUseOccs.push_back(i);

				mBackupMask->Load(mMask);
				mBackupMask->Cover(mChangeItemMask);

				for(uint32 idx = mChangeItemIdx + 1; idx < mCT.size(); ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx]--;
						stats.usgCountSum--;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx]++;
						stats.usgCountSum++;
					}
				}

				stats.usgCountSum -= mBackupMask->UncoverAlphabet(mAlphabet);
				stats.usgCountSum += mMask->CoverAlphabet(mAlphabet);
			}
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
		CalcTotalSizeForCover(stats);

	} else if(mChangeType == ChangeNone) {
		THROW_NOT_IMPLEMENTED();
	} else {
	    THROW("Unknown code table ChangeType");
	}
}

template <ItemSetType T>
void CCCPCodeTable<T>::CoverBinnedDB(CoverStats &stats) {
	if(mChangeType == ChangeAdd) {
		uint32 len = mChangeItem->GetLength();
		vector<uint32>& occur = mAbDbOccs[mChangeItem->GetLastItem()];
		uint32 insPoint = mCTLen - len + 1;
		mUseOccs.clear();
		mRollbackAlph = false;

		for(uint32 r=0; r<occur.size(); r++) {
			uint32 i = occur[r];
			if(mCDB[insPoint][i]->CanCover(mChangeItemMask)) {
				uint32 binSize = mDB->GetRow(i)->GetUsageCount();
				mUseOccs.push_back(i);
				if (!mRollbackAlph) {
					mRollbackAlph = true;
					mBackupUsgCounts = mUsgCounts;
					mOldAlphabet = mAlphabet;
				}

				mMask->Load(mCDB[insPoint][i]);
				mBackupMask->Load(mMask);
				mMask->Cover(mChangeItemMask);
				mChangeItemUsgCount += binSize;
				stats.usgCountSum += binSize;

				for(uint32 idx = mChangeItemIdx; idx < mCT.size(); ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] -= binSize;
						stats.usgCountSum -= binSize;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] += binSize;
						stats.usgCountSum += binSize;
					}
				}

				stats.usgCountSum -= mBackupMask->UncoverAlphabet(mAlphabet, binSize);			
				stats.usgCountSum += mMask->CoverAlphabet(mAlphabet, binSize);
			}
		}
		if (mUseOccs.empty()) {
			return;
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
		CalcTotalSizeForCover(stats);

	} else if(mChangeType == ChangeDel) {
		stats = mCurStats;
		uint32 len = mChangeItem->GetLength();
		vector<uint32>& occur = mAbDbOccs[mChangeItem->GetLastItem()];
		uint32 insPoint = mCTLen - len;	// insPoint -voor- mChangeRow

		mUseOccs.clear();
		mRollbackAlph = true;
		mBackupUsgCounts = mUsgCounts;
		mOldAlphabet = mAlphabet;

		stats.usgCountSum -= mUsgCounts[mChangeItemIdx];
		mUsgCounts[mChangeItemIdx] = 0;
		mChangeItemUsgCount = 0;

		for(uint32 r=0; r<occur.size(); r++) {
			uint32 i = occur[r];
			mMask->Load(mCDB[insPoint][i]);

			// begin van rij `len' tot mChangeItemIdx om cset op te bouwen
			for(uint32 idx = mCTIdx[len]; idx < mChangeItemIdx; ++idx) {
				if(mUsgCounts[idx]==0)
					continue;
				mMask->Cover(mCTMasks[idx]);
			}

			// Alleen rest doen als mChangeItem ook echt gebruikt wordt
			if(mMask->CanCover(mChangeItemMask)) {
				uint32 binSize = mIsBinnedDB ? mDB->GetRow(i)->GetUsageCount() : 1;
				mUseOccs.push_back(i);

				mBackupMask->Load(mMask);
				mBackupMask->Cover(mChangeItemMask);

				for(uint32 idx = mChangeItemIdx + 1; idx < mCT.size(); ++idx) {
					if(mBackupMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] -= binSize;
						stats.usgCountSum -= binSize;
					}
					if(mMask->Cover(mCTMasks[idx])) {
						mUsgCounts[idx] += binSize;
						stats.usgCountSum += binSize;
					}
				}

				stats.usgCountSum -= mBackupMask->UncoverAlphabet(mAlphabet, binSize);
				stats.usgCountSum += mMask->CoverAlphabet(mAlphabet, binSize);
			}
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
		CalcTotalSizeForCover(stats);

	} else if(mChangeType == ChangeNone) {
		THROW_NOT_IMPLEMENTED();
	} else {
	    THROW("Unknown code table ChangeType");
	}
}

template <ItemSetType T>
islist* CCCPCodeTable<T>::GetPostPruneList() {
	islist *pruneList = new islist();
	UpdatePostPruneList(pruneList, pruneList->begin());
	return pruneList;
}

template <ItemSetType T>
islist* CCCPCodeTable<T>::GetEstimatedPostPruneList() {
	islist *pruneList = new islist();
	UpdateEstimatedPostPruneList(pruneList, pruneList->begin());
	return pruneList;
}

template <ItemSetType T>
void CCCPCodeTable<T>::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	SortPostPruneList(pruneList,after);
	uint32 cnt;
	islist::iterator lstart, iter, lend1, lend2;
	ItemSet *is1;
	bool insert;
	for(uint32 i=mChangeItemIdx+1; i<mCT.size(); i++) {
		if((cnt = mUsgCounts[i]) < mBackupUsgCounts[i]) {
			is1 = mCT[i];
			lend2 = pruneList->end();
			//printf("Add to prunelist: %s (%d)\n", is1->ToString().c_str(), is1->GetPrevCount());
			insert = true;
			iter = after;

			if(iter != lend2) {
				for(++iter; iter != lend2; ++iter) {
					if(*iter == is1) {
						insert = false;
						break;
					}
				}
			}
			////MG - Add to Prune list only if E(L(D ,CT')) > L(D, CT)
			//Del(is1, false, false);
			//CoverMask<T>* itemSetMask;
			//itemSetMask = CoverMask<T>::Create();
			//itemSetMask->LoadFromItemSet(is1);
			//RollbackLastDel();
			////MG - Add to Prune list only if E(L(D ,CT')) > L(D, CT)
			if(insert) {
				iter = after;
				if(iter != lend2) {
					for(++iter; iter != lend2; ++iter)
						if(mUsgCountForItemset[*iter] > cnt)
							break;
				}
				pruneList->insert(iter, is1);
			}
		}
	}
}
template <ItemSetType T>
islist* CCCPCodeTable<T>::GetSanitizePruneList() {
	islist *ilist = new islist();
	for(uint32 i=0; i<mCT.size(); i++) {
		if(mUsgCounts[i] == 0) {
			ilist->push_back(mCT[i]);
		}
	}
	return ilist;
}

template <ItemSetType T>
void CCCPCodeTable<T>::DebugPrint() {
	printf("(");
	for (uint32 i=0; i<mCT.size(); i++) {
#if defined (_MSC_VER) && defined (_WINDOWS)
		printf("%I64d%s", mCT[i]->GetID(), i != mCT.size() - 1 ? " " : "");
#elif defined (__GNUC__) && (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
		printf("%lu%s", mCT[i]->GetID(), i != mCT.size() - 1 ? " " : "");
#endif
	}
	printf(")\n");
}


/*__inline double Log2(double x) {
	static const double invLog2 = 1.0 / 0.69314718055994530941723212145818;
	return invLog2 * log(x);
}*/

__inline double CalcCodeLength2(uint32 count, uint64 countSum) {
	return -Log2( (double) count / countSum );
}

template <ItemSetType T>
void CCCPCodeTable<T>::CalcTotalSizeForCover(CoverStats &stats) {
	// Recalc sizes, counts and countSum should be up to date...
	if(mChangeItemUsgCount != 0) {
		double codeLen = -Log2(mChangeItemUsgCount);
		stats.encDbSize += mChangeItemUsgCount * codeLen;
		stats.encCTSize += mChangeItem->GetStandardLength();
		stats.encCTSize += codeLen;
		stats.numSetsUsed++;
	}

	for(uint32 i=mChangeItemIdx; i<mCT.size(); i++) {
		if (mBackupUsgCounts[i] == mUsgCounts[i]) continue;
		if (mBackupUsgCounts[i] != 0) {
			double codeLen = -Log2(mBackupUsgCounts[i]);
			stats.encDbSize -= mBackupUsgCounts[i] * codeLen;
			stats.encCTSize -= mCT[i]->GetStandardLength();
			stats.encCTSize -= codeLen;
			stats.numSetsUsed--;
		}
		if (mUsgCounts[i] != 0) {
			double codeLen = -Log2(mUsgCounts[i]);
			stats.encDbSize += mUsgCounts[i] * codeLen;
			stats.encCTSize += mCT[i]->GetStandardLength();
			stats.encCTSize += codeLen;
			stats.numSetsUsed++;
		}
	}

	// Alphabet
	for(uint32 i=0; i<mAlphabetSize; i++) {
		if (mOldAlphabet[i] == mAlphabet[i]) continue;
		if (mOldAlphabet[i] != 0) {
			double codeLen = -Log2(mOldAlphabet[i]);
			stats.encDbSize -= mOldAlphabet[i] * codeLen;
			stats.encCTSize -= mStdLengths[i];
			stats.encCTSize -= codeLen;
			stats.alphItemsUsed--;
		}
		if (mAlphabet[i] != 0) {
			double codeLen = -Log2(mAlphabet[i]);
			stats.encDbSize += mAlphabet[i] * codeLen;
			stats.encCTSize += mStdLengths[i];
			stats.encCTSize += codeLen;
			stats.alphItemsUsed++;
		}
	}

	double logPrevCountSum = Log2((double) mPrevStats.usgCountSum);
	stats.encDbSize -= mPrevStats.usgCountSum * logPrevCountSum;
	stats.encCTSize -= (mPrevStats.numSetsUsed + mPrevStats.alphItemsUsed) * logPrevCountSum;

	double logCountSum = Log2((double) stats.usgCountSum);
	stats.encDbSize += stats.usgCountSum * logCountSum;
	stats.encCTSize += (stats.numSetsUsed + stats.alphItemsUsed) * logCountSum;

	stats.encSize = stats.encDbSize + stats.encCTSize;
}

template <ItemSetType T>
void CCCPCodeTable<T>::CalcTotalSize(CoverStats &stats) {
	stats.encDbSize = 0;
	stats.encCTSize = 0;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;

	for(uint32 i=0; i<mCT.size(); i++) {
		if (mUsgCounts[i] != 0) {
			double codeLen = -Log2(mUsgCounts[i]);
			stats.encDbSize += (mUsgCounts[i] - mLaplace) * codeLen;
			stats.encCTSize += mCT[i]->GetStandardLength();
			stats.encCTSize += codeLen;
			stats.numSetsUsed++;
		}
	}

	// Alphabet
	for(uint32 i=0; i<mAlphabetSize; i++) {
		if (mAlphabet[i] != 0) {
			double codeLen = -Log2(mAlphabet[i]);
			stats.encDbSize += (mAlphabet[i] - mLaplace) * codeLen;
			stats.encCTSize += mStdLengths[i];
			stats.encCTSize += codeLen;
			stats.alphItemsUsed++;
		}
	}

	double logCountSum = Log2((double) stats.usgCountSum);
	uint64 correction = mLaplace * (stats.numSetsUsed + stats.alphItemsUsed);
	stats.encDbSize += (stats.usgCountSum - correction) * logCountSum;
	stats.encCTSize += (stats.numSetsUsed + stats.alphItemsUsed) * logCountSum;

	if(mSTSpecialCode) {
		stats.encCTSize += logCountSum - Log2(1);
	}
	stats.encSize = stats.encDbSize + stats.encCTSize;
}

template <ItemSetType T>
double CCCPCodeTable<T>::CalcEncodedDbSize(Database *db) {
	uint32 numRows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	double size = 0.0;

	for(uint32 i=0; i<numRows; i++)
		size += CalcTransactionCodeLength(rows[i]) * rows[i]->GetUsageCount();
	return size;
}

template <ItemSetType T>
double CCCPCodeTable<T>::CalcTransactionCodeLength(ItemSet *row) {
	double codeLen = 0.0;
	uint32 count = 0;
	mMask->LoadFromItemSet(row);

	for(uint32 idx = 0; idx < mCT.size(); ++idx)
		if(mMask->Cover(mCTMasks[idx])) {
			codeLen -= Log2(mUsgCounts[idx]);
			++count;
		}
	codeLen += mMask->AlphabetCodes(mAlphabet, count);

	double logCountSum = Log2((double) mCurStats.usgCountSum);
	codeLen += count * logCountSum;

	return codeLen;
}

template <ItemSetType T>
islist* CCCPCodeTable<T>::GetItemSetList() {
	SyncCounts2ItemSets();
	return new islist(mCT.begin(), mCT.end());
}

template <ItemSetType T>
islist* CCCPCodeTable<T>::GetSingletonList() { // TODO: Verify whether correct!
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

template <ItemSetType T>
void CCCPCodeTable<T>::SetAlphabetCount(uint32 item, uint32 count) {
	mAlphabet[item] = count;
}
template <ItemSetType T>
uint32 CCCPCodeTable<T>::GetAlphabetCount(uint32 item) {
	return mAlphabet[item];
}


template <ItemSetType T>
void CCCPCodeTable<T>::AddOneToEachUsageCount() {
	for(uint32 i=0; i<mCT.size(); i++) {
		++mUsgCounts[i];
		mCT[i]->SetUsageCount(mUsgCounts[i]);
	}
	for(uint32 i=0; i<mAlphabetSize; i++)
		++mAlphabet[i];

	mCurStats.usgCountSum += mAlphabetSize + mCT.size();
	++mLaplace;
}

template <ItemSetType T>
void CCCPCodeTable<T>::ApplyLaplaceCorrection(uint32 lpcFactor, uint32 lpcOffset) {
	for(uint32 i=0; i<mCT.size(); i++) {
		mUsgCounts[i] = lpcFactor * mUsgCounts[i] + lpcOffset;
		mCT[i]->SetUsageCount(mUsgCounts[i]);
	}
	for(uint32 i=0; i<mAlphabetSize; i++)
		mAlphabet[i] = lpcFactor * mAlphabet[i] + lpcOffset;;

	mCurStats.usgCountSum = lpcFactor * mCurStats.usgCountSum + lpcOffset;
	++mLaplace;
}

template <ItemSetType T>
void CCCPCodeTable<T>::EndOfKrimp() {
	for(uint32 i=0; i<mCT.size(); i++)
		mCT[i]->SetUsageCount(mUsgCounts[i]);
}

template <ItemSetType T>
void CCCPCodeTable<T>::SyncCounts2ItemSets() {
	for(uint32 i=0; i<mCT.size(); i++)
		mCT[i]->SetUsageCount(mUsgCounts[i]);
}

template class CCCPCodeTable<BM128ItemSetType>;
template class CCCPCodeTable<BAI32ItemSetType>;
template class CCCPCodeTable<Uint16ItemSetType>;
