#ifdef BLOCK_SLIM

#include "../global.h"

#include <algorithm>
#include <cassert>

#include <RandomUtils.h>
#include <ArrayUtils.h>

#include <itemstructs/ItemSet.h>

#include "UsageClass.h"

struct FindCandidate {
	FindCandidate(uint32 i) : toFind(i) { }
	uint32 toFind;
	bool operator() 
		( const std::pair<uint32, CandidateCTGain*> &p ) {
			return p.first == toFind;
	}
};

UsageClass::UsageClass() {
	mItemSet = NULL;
	mUsageCount = 0;
	mMyMap = new std::unordered_map<uint32, CandidateCTGain*>();
}

UsageClass::UsageClass(ItemSet *is) {
	mItemSet = is;
	//mItemSet->SetUsageArea(mItemSet->GetUsageCount());
	mUsageCount = mItemSet->GetUsageCount();
	mMyMap = new std::unordered_map<uint32, CandidateCTGain*>();
}

UsageClass::UsageClass(ItemSet *is, uint32 usageCount) {
	mItemSet = is;
	//mItemSet->SetUsageArea(usageCount);
	mUsageCount = usageCount;
	mMyMap = new std::unordered_map<uint32, CandidateCTGain*>();
}

UsageClass::~UsageClass(){
	//delete mItemSet; // should be deleted by its creator
}

void UsageClass::SetUsageCount(uint32 usageCount){
	mUsageCount = usageCount;
	//mItemSet->SetUsageArea(usageCount);
}

uint32 UsageClass::GetUsageCount() const {
	return mUsageCount;
	//return mItemSet->GetUsageArea();
}

void UsageClass::UpdateUsageCount(){
	//mItemSet->SetUsageArea(mItemSet->GetUsageCount());
	mUsageCount = mItemSet->GetUsageCount();
}

uint32 UsageClass::GetID(){
	return mItemSet->GetUniqueID();
}

void UsageClass::SetItemSet(ItemSet *is){
	mItemSet = is;
}

ItemSet* UsageClass::GetItemSet(){
	return mItemSet;
}

uint32*	UsageClass::GetUsages() {
	return mItemSet->GetUsages();
}

double	UsageClass::GetStandardLength() { 
	return mItemSet->GetStandardLength(); 
}

bool UsageClass::UsageChanged(){
	if(mUsageCount != mItemSet->GetUsageCount()){
	//if(mItemSet->GetUsageArea() != mItemSet->GetUsageCount()) {
		return true;
	}
	return false;
}

int8 UsageClass::UsageDropped(){
	/*if(mUsageCount == mItemSet->GetUsageCount()){
		//if(mItemSet->GetUsageArea() != mItemSet->GetUsageCount()) {
		return 0;
	} else */ if(mUsageCount > mItemSet->GetUsageCount())
		return true;
	//else
		return false;
}
bool UsageClass::HeuristicAvailable(uint32 j, uint32 i_rank, uint32 j_rank){
	std::unordered_map<uint32, CandidateCTGain*>::iterator it = std::find_if(mMyMap->begin(), mMyMap->end(), FindCandidate(j));
	if(it != mMyMap->end()){
		if(it->second){
			it->second->mRankI = i_rank;
			it->second->mRankJ = j_rank;
			return true;
		}
		mMyMap->erase(it);
	}
	return false;
}

void UsageClass::AddTopCandidateI(uint32 j, CandidateCTGain *candidate){
	mMyMap->insert(std::make_pair(j, candidate));
	candidate->AddIteratorI(mMyMap);
}

void UsageClass::AddTopCandidateJ(uint32 i, CandidateCTGain *candidate){
	mMyMap->insert(std::make_pair(i, candidate));
	candidate->AddIteratorJ(mMyMap);
}
/*
void UsageClass::RemoveTopCandidate(uint32 j){
	mymap->erase(j);
}

void UsageClass::RemoveAllTopCandidates(){
	std::unordered_map<uint32, CandidateCTGain*>::iterator cit = mymap->begin(), cend = mymap->end();
	for(; cit != cend; cit++){
		cit->second = NULL;
	}
	mymap->clear();
}
*/

#endif // BLOCK_SLIM
