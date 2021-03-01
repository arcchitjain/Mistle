//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#include "../blobal.h"

#include <RandomUtils.h>

#include "../Bass.h"
#include "../db/Database.h"
#include "../isc/ItemSetCollection.h"

#include "Uint16ItemSet.h"
#include "BAI32ItemSet.h"
#include "BM128ItemSet.h"
#include "ItemSet.h"

ItemSet::ItemSet() {
	mID = UINT64_MAX_VALUE;
	mUsageCount = 0;
	mUsageArea = 0;
	mSupport = 0;
	mPrevUsageCount = 0;
	mPrevUsageArea = 0;
	mStdLength = 0.0;
	mDBOccurrence = NULL;
	mNumDBOccurs = 0;
	mRefCount = 1;

	mCompFunc = &ItemSet::CmpSupport;
}

ItemSet::ItemSet(const ItemSet& is) {
	mID = is.mID;
	mUsageCount = is.mUsageCount;
	mUsageArea = is.mUsageArea;
	mSupport = is.mSupport;
	mPrevUsageCount = is.mPrevUsageCount;
	mPrevUsageArea = is.mPrevUsageArea;
	mStdLength = is.mStdLength;

	mNumDBOccurs = is.mNumDBOccurs;
	if(is.mDBOccurrence == NULL) {
		mDBOccurrence = NULL;
	} else {
		mDBOccurrence = new uint32[mNumDBOccurs];
		memcpy(mDBOccurrence, is.mDBOccurrence, mNumDBOccurs * sizeof(uint32));
	}
	mRefCount = 1;
}

ItemSet::~ItemSet() {
	if(mRefCount > 1) {
		throw "Deleting itemset with multiple references";
	}
	delete[] mDBOccurrence;
}

// set wordt netjes gekopieerd, ownership wordt dus niet overgenomen.
ItemSet *ItemSet::Create(ItemSetType type, const uint16 *set, uint32 setlen, uint32 abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	if(type == BAI32ItemSetType)
		return new BAI32ItemSet(set, setlen, abetlen, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == Uint16ItemSetType)
		return new Uint16ItemSet(set, setlen, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == BM128ItemSetType)
		return new BM128ItemSet(set, setlen, cnt, freq, numOcc, stdLen, occurrences);
	throw "Wrong ItemSetType.";
}
ItemSet *ItemSet::CreateSingleton(ItemSetType type, uint16 elem, uint32 abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	if(type == BAI32ItemSetType)
		return new BAI32ItemSet(elem, abetlen, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == Uint16ItemSetType)
		return new Uint16ItemSet(elem, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == BM128ItemSetType)
		return new BM128ItemSet(elem, cnt, freq, numOcc, stdLen, occurrences);
	throw "Wrong ItemSetType.";
}
ItemSet *ItemSet::CreateEmpty(ItemSetType type, size_t abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	if(type == BAI32ItemSetType)
		return new BAI32ItemSet((uint32) abetlen, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == Uint16ItemSetType)
		return new Uint16ItemSet(cnt, freq, numOcc, stdLen, occurrences);
	else if(type == BM128ItemSetType)
		return new BM128ItemSet(cnt, freq, numOcc, stdLen, occurrences);
	throw "Wrong ItemSetType.";
}

uint32 ItemSet::MemUsage(const ItemSetType type, const uint32 abLen, const uint32 setLen) {
	if(type == BAI32ItemSetType)
		return BAI32ItemSet::MemUsage(abLen);
	else if(type == Uint16ItemSetType)
		return Uint16ItemSet::MemUsage(setLen);
	else if(type == BM128ItemSetType)
		return BM128ItemSet::MemUsage();
	throw "Wrong ItemSetType.";
}
uint32 ItemSet::MemUsage(Database *db) {
	ItemSetType type = db->GetDataType();
	if(type == BAI32ItemSetType)
		return BAI32ItemSet::MemUsage((uint32)db->GetAlphabetSize());
	else if(type == Uint16ItemSetType)
		return Uint16ItemSet::MemUsage(db->GetMaxSetLength());
	else if(type == BM128ItemSetType)
		return BM128ItemSet::MemUsage();
	throw "Wrong ItemSetType.";
}

ItemSetType ItemSet::StringToType(const string desc) {
	if(desc.compare("uint16") == 0)
		return Uint16ItemSetType;
	else if(desc.compare("bai32") == 0)
		return BAI32ItemSetType;
	else if(desc.compare("bm128") == 0)
		return BM128ItemSetType;
	return NoneItemSetType;
}
string ItemSet::TypeToString(const ItemSetType type) {
	if(type == BAI32ItemSetType)
		return "bai32";
	else if(type == Uint16ItemSetType)
		return "uint16";
	else if(type == BM128ItemSetType)
		return "bm128";
	return "ufo";
}

string ItemSet::TypeToFullName(const ItemSetType type) {
	if(type == BAI32ItemSetType)
		return "32bit bitmap array";
	else if(type == Uint16ItemSetType)
		return "uint16 value arrays";
	else if(type == BM128ItemSetType)
		return "128bit bitmap";
	return "aliens";
}

void ItemSet::SetDBOccurrence(uint32 *dbRows) {
	delete[] mDBOccurrence;
	mDBOccurrence = dbRows;
}

uint32* ItemSet::GetDBOccurrence() {
	return mDBOccurrence;
}

uint32 ItemSet::GetNumDBOccurs() {
	return mNumDBOccurs;
}
void ItemSet::DetermineDBOccurence(Database *db, bool calcNumOcc /* = true */) {
	db->DetermineOccurrences(this,calcNumOcc);
}

ItemSet* ItemSet::Remainder(ItemSet *is) const {
	ItemSet *retval = this->Clone();
	retval->Remove(is);
	return retval;
}

void ItemSet::SortItemSetArray(ItemSet **itemSetArray, uint32 numItemSets, IscOrderType desiredOrder) {
	if(desiredOrder == RandomIscOrder) {
		RandomUtils::PermuteArray(itemSetArray, numItemSets);
	} else if(desiredOrder != NoneIscOrder && numItemSets > 0) {
		ItemSetCollection::MergeSort(itemSetArray, numItemSets, ItemSetCollection::GetCompareFunction(desiredOrder));
	}
}

ItemSet** ItemSet::ConvertItemSetListToArray(islist *isl) {
	ItemSet **iarray = new ItemSet*[isl->size()];

	uint32 cur = 0;
	islist::iterator iend = isl->end(), icur;
	for(icur = isl->begin(); icur != iend; ++icur) {
		iarray[cur++] = (ItemSet*) (*icur);
	}
	return iarray;
}

int8 ItemSet::CmpSupport(const ItemSet *b) const {
	if(mSupport > b->mSupport)
		return 1;
	else if(mSupport < b->mSupport)
		return -1;
	else 
		return 0;
}

int8 ItemSet::CmpArea(ItemSet *b) {
	uint32 me = (mSupport * GetLength());
	uint32 he = (b->mSupport * b->GetLength());
	if(me > he)
		return 1;
	else if(me < he)
		return -1;
	else
		return 0;
}
