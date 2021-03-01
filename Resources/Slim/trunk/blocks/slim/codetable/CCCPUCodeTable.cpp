#include "../../../global.h"

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
//#include <omp.h>
//#include <cassert>

#include "../../krimp/codetable/CodeTable.h"
#include "../../krimp/codetable/coverpartial/CCCPCodeTable.h"
#include "../../krimp/codetable/CTISList.h"

#include "CCCPUCodeTable.h"

template <ItemSetType T>
CCCPUCodeTable<T>::CCCPUCodeTable() : CCCPCodeTable<T>() {
}

template <ItemSetType T>
CCCPUCodeTable<T>::CCCPUCodeTable(const CCCPUCodeTable &ct) : CCCPCodeTable<T>(ct) {
}

template <ItemSetType T>
CCCPUCodeTable<T>::~CCCPUCodeTable() {
	mItemSetSorted = NULL;
}

template <ItemSetType T>
void CCCPUCodeTable<T>::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CCCPCodeTable<T>::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);

	this->mDB->EnableFastDBOccurences();
	this->mAlphabetUsageAdd = new vector<uint32>*[this->mAlphabetSize];
	this->mAlphabetUsageZap = new vector<uint32>*[this->mAlphabetSize];
	for (uint32 i = 0; i < this->mAlphabetSize; ++i) {
		this->mAlphabetUsageAdd[i] = new vector<uint32>();
		this->mAlphabetUsageZap[i] = new vector<uint32>();
	}
}

template <ItemSetType T>
void CCCPUCodeTable<T>::CommitAdd(bool updateLog, bool updateInsPoints) {
	if(updateLog)
		this->UpdateCTLog();

	uint32 len = this->mChangeItem->GetLength();
	if (updateInsPoints) {
		// alle csets nu uptodate maken
		for(uint32 r=0; r< this->mUseOccs.size(); r++) {
			uint32 i = this->mUseOccs[r];
			uint32 insPoint = this->mCTLen - len + 1;
			this->mMask->Load(this->mCDB[insPoint][i]);
			this->mBackupMask->Load(this->mMask);
			this->mMask->Cover(this->mChangeItemMask);
			this->mChangeItem->PushUsed(i);
			this->mMask->Store(this->mCDB[insPoint++][i]);
			for(int32 j=len-1; j>1; --j) {
				for(uint32 idx = this->mCTIdx[j]; idx != this->mCTIdx[j-1]; ++idx) {
					if(this->mUsgCounts[idx]==0)
						continue;
					if(this->mBackupMask->Cover(this->mCTMasks[idx]))
						this->mCT[idx]->SetUnused(i);
					if(this->mMask->Cover(this->mCTMasks[idx]))
						this->mCT[idx]->SetUsed(i);
					//this->mMask->Cover(this->mCTMasks[idx]);
				}
				this->mMask->Store(this->mCDB[insPoint++][i]);
			}
		}
		// !!JV!! here be update Usages
	}

	this->mCT.insert(this->mCT.begin() + this->mChangeItemIdx, this->mChangeItem);
	this->mCTMasks.Insert(this->mChangeItemIdx, this->mChangeItemMask);
	this->mUsgCounts.insert(this->mUsgCounts.begin() + this->mChangeItemIdx, this->mChangeItemUsgCount);
	this->mBackupUsgCounts.insert(this->mBackupUsgCounts.begin() + this->mChangeItemIdx, 0);
	for(int32 i=len-1; i>=0; --i) {
		this->mCTIdx[i]++;
	}

	for(uint32 i=this->mChangeItemIdx; i<this->mCT.size(); i++) {
		this->mUsgCountForItemset[this->mCT[i]] = this->mUsgCounts[i];
	}
	
	this->mPrevStats = this->mCurStats;
	this->mChangeItem = NULL;
	this->mChangeItemUsgCount = 0;
	this->mChangeType = ChangeNone;
}

template <ItemSetType T>
void CCCPUCodeTable<T>::CommitAdd(bool updateLog) {
	if(updateLog)
		this->UpdateCTLog();

	uint32 len = this->mChangeItem->GetLength();
	this->mChangeItem->ResetUsage();
	this->mChangeItem->SetPrevUsageCount(1);
	this->mChangeItem->AddUsageCount(this->mUseOccs.size());
	//uint32 insPoint = this->mCTLen - len + 1;
	vector<uint32>		**itemsetUsageAdd = new vector<uint32>*[this->mCT.size()];
	vector<uint32>		**itemsetUsageZap = new vector<uint32>*[this->mCT.size()];
	for(uint32 i = 0; i < this->mCT.size(); i++){
		itemsetUsageAdd[i] = new vector<uint32>();
		itemsetUsageZap[i] = new vector<uint32>();
	}
	/*
	for(uint32 r=0; r<this->mUseOccs.size(); r++) {
		uint32 i = this->mUseOccs[r];
		this->mChangeItem->PushUsed(i);
		this->mMask->Load(this->mCDB[insPoint][i]);
		this->mBackupMask->Load(this->mMask);
		this->mMask->Cover(this->mChangeItemMask);

		for(uint32 idx = this->mChangeItemIdx; idx < this->mCT.size(); ++idx) {
			if(this->mBackupMask->Cover(this->mCTMasks[idx])) {
				if(!(this->mMask->Cover(this->mCTMasks[idx]))){
					itemsetUsageZap[idx]->push_back(i);
				}
			} else if(this->mMask->Cover(this->mCTMasks[idx])) {
				itemsetUsageAdd[idx]->push_back(i);
			}
		}
	}*/
	
	
	// alle csets nu uptodate maken
	for(uint32 r=0; r<this->mUseOccs.size(); r++) {
		uint32 i = this->mUseOccs[r];
		uint32 insPoint = this->mCTLen - len + 1;

		this->mChangeItem->PushUsed(i);
		this->mMask2->Load(this->mCDB[insPoint][i]);
		this->mBackupMask->Load(this->mMask2);

		this->mMask->Load(this->mMask2);
		this->mMask->Cover(this->mChangeItemMask);
		this->mMask->Store(this->mCDB[insPoint++][i]);
		this->mMask2->Cover(this->mChangeItemMask);

		for(uint32 idx = this->mChangeItemIdx; idx < this->mCT.size(); ++idx) {
			if(this->mBackupMask->Cover(this->mCTMasks[idx])) {
				if(!(this->mMask2->Cover(this->mCTMasks[idx]))){
					itemsetUsageZap[idx]->push_back(i);
				}
			} else if(this->mMask2->Cover(this->mCTMasks[idx])) {
				itemsetUsageAdd[idx]->push_back(i);
			}
		}
		
		for(int32 j=len-1; j>1; --j) {
			for(uint32 idx = this->mCTIdx[j]; idx != this->mCTIdx[j-1]; ++idx) {
				if(this->mUsgCounts[idx]==0)
					continue;
				this->mMask->Cover(this->mCTMasks[idx]);
			}
			this->mMask->Store(this->mCDB[insPoint++][i]);
		}
	}

	// !!JV!! here be update Usages
	for(uint32 i = 0; i < this->mCT.size(); i++){
		uint32 addSize = itemsetUsageAdd[i]->size();
		uint32 zapSize = itemsetUsageZap[i]->size();
		if(addSize ==0 && zapSize == 0){
			continue;
		}
		this->mCT[i]->SetUnused(itemsetUsageZap[i]);
		this->mCT[i]->SetUsed(itemsetUsageAdd[i]);
		this->mCT[i]->AddUsageCount(addSize - zapSize);
		itemsetUsageAdd[i]->clear();
		itemsetUsageZap[i]->clear();
	}

	for (uint32 i = 0; i < this->mCT.size(); i++) {
		delete itemsetUsageAdd[i];
		delete itemsetUsageZap[i];
	}
	delete itemsetUsageAdd;
	delete itemsetUsageZap;

	this->UpdateAlphabetUsages();

	this->mCT.insert(this->mCT.begin() + this->mChangeItemIdx, this->mChangeItem);
	this->mCTMasks.Insert(this->mChangeItemIdx, this->mChangeItemMask);
	this->mUsgCounts.insert(this->mUsgCounts.begin() + this->mChangeItemIdx, this->mChangeItemUsgCount);
	this->mBackupUsgCounts.insert(this->mBackupUsgCounts.begin() + this->mChangeItemIdx, 0);
	for(int32 i=len-1; i>=0; --i) {
		this->mCTIdx[i]++;
	}

	for(uint32 i=this->mChangeItemIdx; i<this->mCT.size(); i++) {
		this->mUsgCountForItemset[this->mCT[i]] = this->mUsgCounts[i];
	}

	this->mPrevStats = this->mCurStats;
	this->mChangeItem = NULL;
	this->mChangeItemUsgCount = 0;
	this->mChangeType = ChangeNone;
}

template <ItemSetType T>
void CCCPUCodeTable<T>::CommitLastDel(bool zap, bool updateLog, bool updateInsPoints) {
	if(this->mChangeType == ChangeDel) {
		if(updateLog)
			this->UpdateCTLog();

		uint32 len = this->mChangeItem->GetLength();
		this->mCT.erase(this->mCT.begin() + this->mChangeItemIdx);
		this->mCTMasks.Erase(this->mChangeItemIdx);
		this->mUsgCounts.erase(this->mUsgCounts.begin() + this->mChangeItemIdx);
		this->mBackupUsgCounts.erase(this->mBackupUsgCounts.begin() + this->mChangeItemIdx);
		for (int32 i=len-1; i>=0; i--) {
			this->mCTIdx[i]--;
		}

		this->mUsgCountForItemset.erase(this->mChangeItem);
		for(uint32 i=this->mChangeItemIdx; i<this->mCT.size(); i++) {
			this->mUsgCountForItemset[this->mCT[i]] = this->mUsgCounts[i];
		}

		this->mPrevStats = this->mCurStats;
		this->mChangeType = ChangeNone;
		if(zap)
			this->mChangeItem->UnRef();
		this->mChangeItem = NULL;
		this->mChangeItemUsgCount = 0;

		if (updateInsPoints) {
			for(uint32 r=0; r<this->mUseOccs.size(); r++) {
				uint32 insPoint = this->mCTLen - len;	// insPoint -voor- mChangeRow
				uint32 i = this->mUseOccs[r];
				this->mMask->Load(this->mCDB[insPoint++][i]);
				for (int32 j=len; j>1; --j) {
					for(uint32 idx = this->mCTIdx[j]; idx != this->mCTIdx[j-1]; ++idx) {
						if(this->mUsgCounts[idx]==0)
							continue;
						this->mMask->Cover(this->mCTMasks[idx]);
					}
					this->mMask->Store(this->mCDB[insPoint++][i]);
				}
			}
			// !!JV!! here be update Usages
		}
	}
}

template <ItemSetType T>
void CCCPUCodeTable<T>::CommitLastDel(bool zap, bool updateLog /*=true*/) {
	if(this->mChangeType == ChangeDel) {
		if(updateLog)
			this->UpdateCTLog();

		uint32 len = this->mChangeItem->GetLength();
		vector<uint32>		**itemsetUsageAdd;
		vector<uint32>		**itemsetUsageZap;
		itemsetUsageAdd = new vector<uint32> *[this->mCT.size()];
		itemsetUsageZap = new vector<uint32> *[this->mCT.size()];
		for(uint32 i = 0; i < this->mCT.size(); i++){
			itemsetUsageAdd[i] = new vector<uint32>();
			itemsetUsageZap[i] = new vector<uint32>();
		}
		uint32 insPoint = this->mCTLen - len;
		//MG - TODO - remove this loop (same as commitAdd)
		for(uint32 r=0; r<this->mUseOccs.size(); r++) {
			uint32 i = this->mUseOccs[r];
			this->mMask->Load(this->mCDB[insPoint][i]);
			
			for(uint32 idx = this->mCTIdx[len]; idx < this->mChangeItemIdx; ++idx) {
				if(this->mUsgCounts[idx]==0)
					continue;
				this->mMask->Cover(this->mCTMasks[idx]);
			}
			this->mBackupMask->Load(this->mMask);
			this->mBackupMask->Cover(this->mChangeItemMask);

			for(uint32 idx = this->mChangeItemIdx + 1; idx < this->mCT.size(); ++idx) {
				if(this->mBackupMask->Cover(this->mCTMasks[idx])) {
					if(!(this->mMask->Cover(this->mCTMasks[idx]))){
						itemsetUsageZap[idx]->push_back(i);
					}
				} else if(this->mMask->Cover(this->mCTMasks[idx])) {
					itemsetUsageAdd[idx]->push_back(i);
				}
			}
		}
		
		for(uint32 i = 0; i < this->mCT.size(); i++){
			uint32 addSize = itemsetUsageAdd[i]->size();
			uint32 zapSize = itemsetUsageZap[i]->size();
			if(addSize ==0 && zapSize == 0){
				continue;
			}
			this->mCT[i]->SetUnused(itemsetUsageZap[i]);
			this->mCT[i]->SetUsed(itemsetUsageAdd[i]);
			this->mCT[i]->AddUsageCount(addSize - zapSize);
			itemsetUsageAdd[i]->clear();
			itemsetUsageZap[i]->clear();
		}
		for (uint32 i = 0; i < this->mCT.size(); i++)
		{
			delete itemsetUsageAdd[i];
			delete itemsetUsageZap[i];
		}
		delete itemsetUsageAdd;
		delete itemsetUsageZap;

		this->mCT.erase(this->mCT.begin() + this->mChangeItemIdx);
		this->mCTMasks.Erase(this->mChangeItemIdx);
		this->mUsgCounts.erase(this->mUsgCounts.begin() + this->mChangeItemIdx);
		this->mBackupUsgCounts.erase(this->mBackupUsgCounts.begin() + this->mChangeItemIdx);
		for (int32 i=len-1; i>=0; i--) {
			this->mCTIdx[i]--;
		}

		this->mUsgCountForItemset.erase(this->mChangeItem);
		for(uint32 i=this->mChangeItemIdx; i<this->mCT.size(); i++) {
			this->mUsgCountForItemset[this->mCT[i]] = this->mUsgCounts[i];
		}

		this->mPrevStats = this->mCurStats;
		//this->mChangeType = ChangeNone;
		if(zap)
			this->mChangeItem->UnRef();
		this->mChangeItem = NULL;
		this->mChangeItemUsgCount = 0;

		for(uint32 r=0; r<this->mUseOccs.size(); r++) {
			uint32 insPoint = this->mCTLen - len;	// insPoint -voor- mChangeRow
			uint32 i = this->mUseOccs[r];
			this->mMask->Load(this->mCDB[insPoint++][i]);
			for (int32 j=len; j>1; --j) {
				for(uint32 idx = this->mCTIdx[j]; idx != this->mCTIdx[j-1]; ++idx) {
					if(this->mUsgCounts[idx]==0)
						continue;
					this->mMask->Cover(this->mCTMasks[idx]);
				}
				this->mMask->Store(this->mCDB[insPoint++][i]);
			}
		}
		// !!JV!! here be update Usages
		this->UpdateAlphabetUsages();
		this->mChangeType = ChangeNone;
	}
}

template <ItemSetType T>
void CCCPUCodeTable<T>::UpdateAlphabetUsages(){
	// Update Alphabet Usages
	for(uint32 i = 0; i < this->mAlphabetSize; ++i) {
		ItemSet *is = this->mItemSetSorted->GetItemSet(i);
		is->BackupUsage();
		uint32 *curAlphabet = is->GetUsages();
		uint32 *oldAlphabet = is->GetPrevUsages();
		uint32 prevUsageCount = is->GetPrevUsageCount();
		//curAlphabet = new uint32[is->GetPrevUsageCount() - mAlphabetUsageZap[i]->size() + mAlphabetUsageAdd[i]->size() ];

		vector<uint32>::iterator addCur = this->mAlphabetUsageAdd[i]->begin(), addEnd = this->mAlphabetUsageAdd[i]->end();
		vector<uint32>::iterator zapCur = this->mAlphabetUsageZap[i]->begin(), zapEnd = this->mAlphabetUsageZap[i]->end();
		uint32 newCur = 0, oldCur = 0;
		uint32 aa = this->mAlphabetUsageAdd[i]->size();
		uint32 bb = this->mAlphabetUsageZap[i]->size();
		if((this->mAlphabetUsageAdd[i]->size() == 0) && (this->mAlphabetUsageZap[i]->size() == 0)){
			continue;
		}

		// from scratch - the mix part - as long as it is not empty, we're still-in-the current usages
		for(; addCur != addEnd && zapCur != zapEnd; ) {
			if(*addCur == *zapCur) {
				// no action here
				++addCur;
				++zapCur;
			} else if(*addCur > *zapCur) {
				// zap this value
				while(oldAlphabet[oldCur] < *zapCur)
					curAlphabet[newCur++] = oldAlphabet[oldCur++];
				oldCur++;
				++zapCur;
			} else { // (addCur->value < zapCur->value)			
				// add this value
				while(oldAlphabet[oldCur] < *addCur)
					curAlphabet[newCur++] = oldAlphabet[oldCur++];
				curAlphabet[newCur++] = *addCur;
				++addCur;
			}
		}
		// Then zap the leftover part, because it involves zapping we are still in range automatically ancient usages (aka, no check required)
		for(; zapCur != zapEnd; ++zapCur) {
			// copy until that value
			while(oldAlphabet[oldCur] < *zapCur)
				curAlphabet[newCur++] = oldAlphabet[oldCur++];
			// zap this value
			oldCur++;
		}
		// or add leftover, we do not have to sit in old range
		for(; addCur != addEnd; ++addCur) {
			// copy until that value
			while(oldCur < prevUsageCount && oldAlphabet[oldCur] < *addCur)
				curAlphabet[newCur++] = oldAlphabet[oldCur++];
			// add this value
			curAlphabet[newCur++] = *addCur;
		}
		// and then possibly a remnant of old, just copy

		memcpy(curAlphabet + newCur, oldAlphabet + oldCur, sizeof(uint32) * (prevUsageCount - oldCur));//mg - need to verify needed or not
		is->SetUsageCount(newCur + prevUsageCount - oldCur);
		is->SetNumUsageCount(newCur + prevUsageCount - oldCur);

		this->mAlphabetUsageAdd[i]->clear();
		this->mAlphabetUsageZap[i]->clear();
	}
}

template <ItemSetType T>
void CCCPUCodeTable<T>::UpdateUsages(ItemSet *is, vector<uint32> *usageAdd, vector<uint32>	*usageZap){
		is->BackupUsage();
		uint32 *curAlphabet = is->GetUsages();
		uint32 *oldAlphabet = is->GetPrevUsages();
		uint32 prevUsageCount = is->GetPrevUsageCount();

		vector<uint32>::iterator addCur = usageAdd->begin(), addEnd = usageAdd->end();
		vector<uint32>::iterator zapCur = usageZap->begin(), zapEnd = usageZap->end();
		uint32 newCur = 0, oldCur = 0;
		uint32 aa = usageAdd->size();
		uint32 bb = usageZap->size();
		if((usageAdd->size() == 0) && (usageZap->size() == 0)){
			return;
		}

		// from scratch - the mix part - as long as it is not empty, we're still-in-the current usages
		for(; addCur != addEnd && zapCur != zapEnd; ) {
			if(*addCur == *zapCur) {
				// no action here
				++addCur;
				++zapCur;
			} else if(*addCur > *zapCur) {
				// zap this value
				while(oldAlphabet[oldCur] < *zapCur)
					curAlphabet[newCur++] = oldAlphabet[oldCur++];
				oldCur++;
				++zapCur;
			} else { // (addCur->value < zapCur->value)			
				// add this value
				while(oldAlphabet[oldCur] < *addCur)
					curAlphabet[newCur++] = oldAlphabet[oldCur++];
				curAlphabet[newCur++] = *addCur;
				++addCur;
			}
		}
		// Then zap the leftover part, because it involves zapping we are still in range automatically ancient usages (aka, no check required)
		for(; zapCur != zapEnd; ++zapCur) {
			// copy until that value
			while(oldAlphabet[oldCur] < *zapCur)
				curAlphabet[newCur++] = oldAlphabet[oldCur++];
			// zap this value
			oldCur++;
		}
		// or add leftover, we do not have to sit in old range
		for(; addCur != addEnd; ++addCur) {
			// copy until that value
			while(oldCur < prevUsageCount && oldAlphabet[oldCur] < *addCur)
				curAlphabet[newCur++] = oldAlphabet[oldCur++];
			// add this value
			curAlphabet[newCur++] = *addCur;
		}
		// and then possibly a remnant of old, just copy

		memcpy(curAlphabet + newCur, oldAlphabet + oldCur, sizeof(uint32) * (prevUsageCount - oldCur));//mg - need to verify needed or not
		is->SetUsageCount(newCur + prevUsageCount - oldCur);
		is->SetNumUsageCount(newCur + prevUsageCount - oldCur);

		usageAdd->clear();
		usageZap->clear();
}

template <ItemSetType T>
void CCCPUCodeTable<T>::RollbackCounts() {
	if (this->mRollbackAlph) {
		this->mUsgCounts.swap(this->mBackupUsgCounts);
		this->mAlphabet.swap(this->mOldAlphabet);
	}

	for(uint32 i = 0; i < this->mAlphabetSize; ++i) {
		this->mAlphabetUsageAdd[i]->clear();
		this->mAlphabetUsageZap[i]->clear();
	}
}

template <ItemSetType T>
void CCCPUCodeTable<T>::CoverNoBinsDB(CoverStats &stats) {
	if(this->mChangeType == ChangeAdd) {
		uint32 len = this->mChangeItem->GetLength();
		vector<uint32>& occur = this->mAbDbOccs[this->mChangeItem->GetLastItem()];
		//uint32 *occur = this->mChangeItem->GetUsages();
		//uint32 usageCount = this->mChangeItem->GetUsageCount();
		uint32 insPoint = this->mCTLen - len + 1;
		this->mUseOccs.clear();
		this->mRollbackAlph = false;

		for(uint32 r=0; r<occur.size(); r++) {
			uint32 i = occur[r];
			if(this->mCDB[insPoint][i]->CanCover(this->mChangeItemMask)) {
				this->mUseOccs.push_back(i);
				if (!this->mRollbackAlph) {
					this->mRollbackAlph = true;
					this->mBackupUsgCounts = this->mUsgCounts;
					this->mOldAlphabet = this->mAlphabet;
				}

				this->mMask->Load(this->mCDB[insPoint][i]);
				this->mBackupMask->Load(this->mMask);
				this->mMask->Cover(this->mChangeItemMask);
				this->mChangeItemUsgCount++;
				stats.usgCountSum++;

				for(uint32 idx = this->mChangeItemIdx; idx < this->mCT.size(); ++idx) {
					if(this->mBackupMask->Cover(this->mCTMasks[idx])) {
						if(!(this->mMask->Cover(this->mCTMasks[idx]))){
							this->mUsgCounts[idx]--;
							stats.usgCountSum--;
						}
					}
					else if(this->mMask->Cover(this->mCTMasks[idx])) {
						this->mUsgCounts[idx]++;
						stats.usgCountSum++;
					}
				}
				// for usage up to dating:
				stats.usgCountSum -= this->mBackupMask->UncoverAlphabet(this->mAlphabet, this->mAlphabetUsageZap, i);
				stats.usgCountSum += this->mMask->CoverAlphabet(this->mAlphabet, this->mAlphabetUsageAdd, i);
			}
		}
		if (this->mUseOccs.empty()) {
			return;
		}
		// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
		//this->CalcTotalSizeForCover(stats);

	} else if(this->mChangeType == ChangeDel) {
		stats = this->mCurStats;
		uint32 len = this->mChangeItem->GetLength();
		//vector<uint32>& occur = mAbDbOccs[this->mChangeItem->GetLastItem()];
		uint32 *occur = this->mChangeItem->GetUsages();
		uint32 usageCount = this->mChangeItem->GetUsageCount();
		uint32 insPoint = this->mCTLen - len;	// insPoint -voor- mChangeRow

		this->mUseOccs.clear();
		this->mRollbackAlph = true;
		this->mBackupUsgCounts = this->mUsgCounts;
		this->mOldAlphabet = this->mAlphabet;

		stats.usgCountSum -= this->mUsgCounts[this->mChangeItemIdx];
		this->mUsgCounts[this->mChangeItemIdx] = 0;
		this->mChangeItemUsgCount = 0;

		for(uint32 r=0; r<usageCount; r++) {
			uint32 i = occur[r];
			this->mMask->Load(this->mCDB[insPoint][i]);

			// begin van rij `len' tot this->mChangeItemIdx om cset op te bouwen
			// beginning of row 'and' to this->mChangeItemIdx to build set
			for(uint32 idx = this->mCTIdx[len]; idx < this->mChangeItemIdx; ++idx) {
				if(this->mUsgCounts[idx]==0)
					continue;
				this->mMask->Cover(this->mCTMasks[idx]);
			}

			// Alleen rest doen als this->mChangeItem ook echt gebruikt wordt
			// Only rest do if this->mChangeItem actually used
			if(this->mMask->CanCover(this->mChangeItemMask)) {
				this->mUseOccs.push_back(i);

				this->mBackupMask->Load(this->mMask);
				this->mBackupMask->Cover(this->mChangeItemMask);

				for(uint32 idx = this->mChangeItemIdx + 1; idx < this->mCT.size(); ++idx) {
					if(this->mBackupMask->Cover(this->mCTMasks[idx])) {
						if(!(this->mMask->Cover(this->mCTMasks[idx]))){
							this->mUsgCounts[idx]--;
							stats.usgCountSum--;
						}
					}
					else if(this->mMask->Cover(this->mCTMasks[idx])) {
						this->mUsgCounts[idx]++;
						stats.usgCountSum++;
					}
				}
				stats.usgCountSum -= this->mBackupMask->UncoverAlphabet(this->mAlphabet, this->mAlphabetUsageZap, i);
				stats.usgCountSum += this->mMask->CoverAlphabet(this->mAlphabet, this->mAlphabetUsageAdd, i);
			}
		}
	} else if(this->mChangeType == ChangeNone) {
		THROW_NOT_IMPLEMENTED();
	} else {
	    THROW("Unknown code table ChangeType");
	}
	
	// Counts en CountSum zijn bijgewerkt, nu sizes uitrekenen ...
	this->CalcTotalSizeForCover(stats);
}

template <ItemSetType T>
void CCCPUCodeTable<T>::CoverBinnedDB(CoverStats &stats) {
	THROW_DROP_SHIZZLE();
}

template <ItemSetType T>
void CCCPUCodeTable<T>::UpdatePostPruneList(islist *pruneList, const islist::iterator &after) {
	this->SortPostPruneList(pruneList,after);
	uint32 cnt;
	islist::iterator lstart, iter, lend1, lend2;
	ItemSet *is1;
	bool insert;
	for (uint32 i = this->mChangeItemIdx + 1; i < this->mCT.size(); i++) {
		if ((cnt = this->mUsgCounts[i]) < this->mBackupUsgCounts[i]) {
			is1 = this->mCT[i];
			lend2 = pruneList->end();
			//printf("Add to prunelist: %s (%d)\n", is1->ToString().c_str(), is1->GetPrevCount());
			insert = true;
			iter = after;

			if (iter != lend2){
				for (++iter; iter != lend2; ++iter){
					if (*iter == is1) {
						insert = false;
						break;
					}
				}
			}
			
			if (insert){
				iter = after;
				if (iter != lend2) {
					for (++iter; iter != lend2; ++iter)
					if (this->mUsgCountForItemset[*iter] > cnt)
						break;
				}
				pruneList->insert(iter, is1);
			}
		}
	}
}

template <ItemSetType T>
void CCCPUCodeTable<T>::UpdateEstimatedPostPruneList(islist *pruneList, const islist::iterator &after) {
	this->SortPostPruneList(pruneList, after);
	uint32 cnt;
	islist::iterator lstart, iter, lend1, lend2;
	ItemSet *is1;
	bool insert;
	vector<uint32> tempUsgCounts;
	vector<int32> tempAlphCounts;
	CoverMask<T>* prunedItemMask = CoverMask<T>::Create();
	CoverMask<T>* itemMask = CoverMask<T>::Create();
	uint64 curCountSum;
	double encCTSize, encDbSize;
	CoverStats& stats = this->GetCurStats();
	uint32 alphItemsUsed;

	for (uint32 i = this->mChangeItemIdx + 1; i < this->mCT.size(); i++) {
		if ((cnt = this->mUsgCounts[i]) < this->mBackupUsgCounts[i]) {
			is1 = this->mCT[i];
			lend2 = pruneList->end();
			//printf("Add to prunelist: %s (%d)\n", is1->ToString().c_str(), is1->GetPrevCount());
			insert = true;
			iter = after;

			if (iter != lend2){
				for (++iter; iter != lend2; ++iter){
					if (*iter == is1) {
						insert = false;
						break;
					}
				}
			}

			if (!insert) continue;

			//MG - Add to Prune list only if E(L(D ,CT')) >= L(D, CT)
			alphItemsUsed = stats.alphItemsUsed;
			encDbSize = stats.encDbSize;
			encCTSize = stats.encCTSize;

			prunedItemMask->LoadFromItemSet(is1);
			tempUsgCounts = this->mUsgCounts;
			tempAlphCounts = this->mAlphabet;
			tempUsgCounts[i] = 0;

			curCountSum = stats.usgCountSum;
			curCountSum -= this->mUsgCounts[i];
			for (uint32 inner = i + 1; inner < this->mCT.size(); inner++) {
				itemMask->LoadFromItemSet(this->mCT[inner]);
				if (prunedItemMask->Cover(itemMask)){
					tempUsgCounts[inner] += this->mUsgCounts[i];
					curCountSum += this->mUsgCounts[i];
				}
			}

			if (prunedItemMask){
				curCountSum += prunedItemMask->CoverAlphabet(tempAlphCounts, this->mUsgCounts[i]);
			}

			if (this->mUsgCounts[i] != 0) {
				double codeLen = Log2(this->mUsgCounts[i]);
				encDbSize -= this->mUsgCounts[i] * codeLen;
				encCTSize -= this->mCT[i]->GetStandardLength();
				encCTSize -= codeLen;

				for (uint32 m = 0; m < this->mCT.size(); m++) {
					if (m == i) continue;
					if (this->mUsgCounts[m] == tempUsgCounts[m]) continue;
					if (this->mUsgCounts[m] != 0) {
						double codeLen = Log2(this->mUsgCounts[m]);
						encDbSize += this->mUsgCounts[m] * codeLen;
						encCTSize += this->mCT[m]->GetStandardLength();
						encCTSize += codeLen;
					}
					if (tempUsgCounts[m] != 0) {
						double codeLen = Log2(tempUsgCounts[m]);
						encDbSize -= tempUsgCounts[m] * codeLen;
						encCTSize -= this->mCT[m]->GetStandardLength();
						encCTSize -= codeLen;
					}
				}

				// Alphabet
				for (uint32 m = 0; m < this->mAlphabetSize; m++) {
					if (this->mAlphabet[m] == tempAlphCounts[m]) continue;
					if (this->mAlphabet[m] != 0) {
						double codeLen = Log2(this->mAlphabet[m]);
						encDbSize += this->mAlphabet[m] * codeLen;
						encCTSize += this->mStdLengths[m];
						encCTSize += codeLen;
						alphItemsUsed--;
					}
					if (tempAlphCounts[m] != 0) {
						double codeLen = Log2(tempAlphCounts[m]);
						encDbSize -= tempAlphCounts[m] * codeLen;
						encCTSize -= this->mStdLengths[m];
						encCTSize -= codeLen;
						alphItemsUsed++;
					}
				}
				
				double logPrevCountSum = Log2((double)stats.usgCountSum);
				encDbSize -= stats.usgCountSum * logPrevCountSum;
				encCTSize -= (stats.numSetsUsed + stats.alphItemsUsed) * logPrevCountSum;

				double logCountSum = Log2((double)curCountSum);
				encDbSize += curCountSum * logCountSum;
				encCTSize += (stats.numSetsUsed + alphItemsUsed - 1) * logCountSum;

				double encSize = encDbSize + encCTSize;
				if (encSize > stats.encSize){
					continue;
				}
			}

			iter = after;
			if (iter != lend2) {
				for (++iter; iter != lend2; ++iter)
				if (this->mUsgCountForItemset[*iter] > cnt)
					break;
			}
			pruneList->insert(iter, is1);
		}
	}
	delete itemMask;
	delete prunedItemMask;
}

template class CCCPUCodeTable<BM128ItemSetType>;
template class CCCPUCodeTable<BAI32ItemSetType>;
template class CCCPUCodeTable<Uint16ItemSetType>;
