#ifdef BLOCK_SLIM

#include "../global.h"

#include <algorithm>
#include <unordered_map>
#include <itemstructs/ItemSet.h>

#include "ItemSetSorted.h"

bool wayToSort(const std::pair<uint32,UsageClass> &i, const std::pair<uint32,UsageClass> &j) { 
	return i.second.GetUsageCount() > j.second.GetUsageCount(); 
};

struct FindUsageClass {
	FindUsageClass(uint32 i) : mToFind(i) { }
	uint32 mToFind;
	bool operator() 
		( const pair<uint32, UsageClass> &p ) {
			return p.first == mToFind;
	}
};

ItemSetSorted::ItemSetSorted() {
	mCurrentCT = new vector<std::pair<uint32, UsageClass>>;
}

ItemSetSorted::~ItemSetSorted() {
	delete mCurrentCT;
}


void ItemSetSorted::SortElements(void) {
	std::sort(mCurrentCT->begin(), mCurrentCT->end(), wayToSort);
}


void ItemSetSorted::Insert(uint32 key, UsageClass &usage) {
	mCurrentCT->push_back(pair<uint32,UsageClass>(key, usage));
}

void ItemSetSorted::Remove(list<uint32> *deletedIDs) {
	list<uint32>::const_iterator dit;
	for(dit = deletedIDs->begin(); dit != deletedIDs->end(); dit++){
		std::vector<std::pair<uint32,UsageClass>>::iterator cit = find_if(mCurrentCT->begin(), mCurrentCT->end(), FindUsageClass(*dit));
		if(cit != mCurrentCT->end()){
			mCurrentCT->erase(cit);
		}
	}
}

ItemSet* ItemSetSorted::GetItemSet(uint32 id) {
	std::vector<std::pair<uint32,UsageClass>>::iterator it = find_if(mCurrentCT->begin(), mCurrentCT->end(), FindUsageClass(id));
		if(it != mCurrentCT->end()){
			return it->second.GetItemSet();
		}
	return NULL;
}

uint32 ItemSetSorted::GetUsageCount(uint32 id) {
	std::vector<std::pair<uint32,UsageClass>>::iterator it = find_if(mCurrentCT->begin(), mCurrentCT->end(), FindUsageClass(id));
	if(it != mCurrentCT->end()){
		return it->second.GetUsageCount();
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// ItemSetSortedIdx (with index, w00t w00t)
//////////////////////////////////////////////////////////////////////////
ItemSetSortedIdx::ItemSetSortedIdx() {
	mCurrentCT = new vector<std::pair<uint32, UsageClass>>;
	mCTIdxMap = new unordered_map<uint32,uint32>;
}

ItemSetSortedIdx::~ItemSetSortedIdx() {
	delete mCurrentCT;
	delete mCTIdxMap;
}


void ItemSetSortedIdx::SortElements(void) {
	std::sort(mCurrentCT->begin(), mCurrentCT->end(), wayToSort);

	vector<pair<uint32,UsageClass>>::const_iterator vit = mCurrentCT->begin(), vend = mCurrentCT->end();
	for(uint32 cid = 0; vit != vend; ++vit, ++cid) {
		uint32 uid = vit->first;
		unordered_map<uint32,uint32>::iterator itr = mCTIdxMap->find(uid);
		if(itr == mCTIdxMap->end()) {
			pair<uint32,uint32> p(uid,cid);
			mCTIdxMap->insert(p);
		} else
			itr->second = cid;
	}
}


void ItemSetSortedIdx::Insert(uint32 key, UsageClass &usage) {
	mCTIdxMap->insert(pair<uint32,uint32>(key, (uint32)mCurrentCT->size()));
	mCurrentCT->push_back(pair<uint32,UsageClass>(key, usage));
}

void ItemSetSortedIdx::Remove(list<uint32> *deletedIDs) {
	// transform uniqueID list into vector of indices of which entries in mCurrentCT are to be deleted
	list<uint32>::iterator dit = deletedIDs->begin(), dend = deletedIDs->end();
	uint32 vidx = 0, vlen = (uint32) deletedIDs->size();
	vector<uint32> vt(deletedIDs->size());
	for(; dit != dend; ++dit, ++vidx) {
		// record where in mCurrentCT this element is
		vt[vidx] = mCTIdxMap->find(*dit)->second;

		// zap from index
		mCTIdxMap->erase(*dit);
	}

	// we'll go over this from low to high
	std::sort(vt.begin(),vt.end());

	vector<pair<uint32,UsageClass>>::iterator fromIt = mCurrentCT->begin() + vt[0] + 1, fromEnd = mCurrentCT->end(), toIt = mCurrentCT->begin() + vt[0];
	uint32 curIdx = vt[0] +1, newIdx = vt[0];
	vidx = 1;

	for(; fromIt != fromEnd && vidx < vlen; ++fromIt, ++toIt, ++newIdx, ++curIdx) {
		if(curIdx == vt[vidx]) {				// skip this entry
			--newIdx; --toIt; ++vidx;;
		} else {								// copy and update index
			*toIt = *fromIt;
			mCTIdxMap->find(toIt->first)->second = newIdx;
		}
	}
	copy(fromIt, fromEnd, toIt);				// copy tail
	fromIt = fromEnd-vlen;
	mCurrentCT->erase(fromIt,fromEnd);			// zap tail
	fromEnd = mCurrentCT->end();
	for(; toIt != fromEnd; ++toIt, ++newIdx) 	// update index
		mCTIdxMap->find(toIt->first)->second = newIdx;
}

ItemSet* ItemSetSortedIdx::GetItemSet(uint32 id) {
	unordered_map<uint32,uint32>::const_iterator itr = mCTIdxMap->find(id);
	if(itr != mCTIdxMap->end())
		return mCurrentCT->at(itr->second).second.GetItemSet();
	return NULL;
}

uint32 ItemSetSortedIdx::GetUsageCount(uint32 id) {
	unordered_map<uint32,uint32>::const_iterator itr = mCTIdxMap->find(id);
	if(itr != mCTIdxMap->end())
		return mCurrentCT->at(itr->second).second.GetUsageCount();
	return 0;
}



//////////////////////////////////////////////////////////////////////////
// ItemSetSortedJV
//////////////////////////////////////////////////////////////////////////

bool wayToSortIS(const std::pair<uint32,ItemSet*> &i, const std::pair<uint32,ItemSet*> &j) { 
	return i.second->GetUsageArea() > j.second->GetUsageArea(); 
};


ItemSetSortedJV::ItemSetSortedJV() {
	currentCT = new std::vector<std::pair<uint32, ItemSet*>>;
}

ItemSetSortedJV::~ItemSetSortedJV() {
	// no usage classes anymore, no need to delete them
	/*std::vector<std::pair<uint32,ItemSet*>>::iterator it;
	for(it = currentCT->begin(); it != currentCT->end(); it++) {
		// this is plain nasty, but the easiest way to zap all UsageClass instances...
		//delete it->second; // 
	}*/
	delete currentCT;
}


void ItemSetSortedJV::SortElements(void) {
	std::sort(currentCT->begin(), currentCT->end(), wayToSortIS);
}


void ItemSetSortedJV::Insert(uint32 key, ItemSet *is) {
	std::pair<uint32,ItemSet*> pair1 = pair<uint32, ItemSet*>(key, is);
	currentCT->push_back(pair1);
}

void ItemSetSortedJV::Remove(uint32 key) {
	std::vector<std::pair<uint32,ItemSet*>>::iterator it;
	for(it = currentCT->begin(); it != currentCT->end(); it++) {
		if(it->first == key) {
			// this is kinda dirty, as SlimMG created that UsageClass*, not us -- JV
			//delete it->second;

			currentCT->erase(it);
			break;
		}
	}
}

void ItemSetSortedJV::Remove(std::list<uint32> *deletedIDs) {
	std::list<uint32>::iterator cit;
	for(cit = deletedIDs->begin(); cit != deletedIDs->end(); cit++){
		std::vector<std::pair<uint32,ItemSet*>>::iterator it;
		for(it = currentCT->begin(); it != currentCT->end(); it++) {
			if(it->first == *cit) {

				// this is kinda dirty, as SlimMG created that UsageClass*, not us -- JV
				//delete it->second; // usage class doesnt need to be deleted anymore, but who deletes the itemset?

				currentCT->erase(it);
				break;
			}
		}
	}
}

ItemSet* ItemSetSortedJV::GetItemSet(uint32 id) {
	std::vector<std::pair<uint32,ItemSet*>>::iterator it;
	for(it = currentCT->begin(); it != currentCT->end(); it++) {
		if(it->first == id) {
			return it->second;
		}
	}
	return NULL;
}


uint32 ItemSetSortedJV::GetUsageCount(uint32 id) {
	std::vector<std::pair<uint32,ItemSet*>>::iterator it;

	for(it = currentCT->begin(); it != currentCT->end(); it++) {
		if(it->first == id) {
			return it->second->GetUsageArea();
		}
	}
	return 0;
}

#endif // BLOCK_SLIM