#include "../blobal.h"

#include <RandomUtils.h>
#include <ArrayUtils.h>

#include "../Bass.h"
#include "../db/Database.h"
#include "../isc/ItemSetCollection.h"

#include "Uint16ItemSet.h"
#include "BAI32ItemSet.h"
#include "BM128ItemSet.h"
#include "ItemSet.h"

#include <algorithm>
#include <cassert>

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
	mUsage = NULL;
	mNumUsages = 0;
	mPrevUsage = NULL;
	mNumPrevUsages = 0;
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
	mNumUsages = is.mNumUsages;
	mNumPrevUsages = is.mNumPrevUsages;
	if(is.mDBOccurrence == NULL) {
		mDBOccurrence = NULL;
	} else {
		mDBOccurrence = new uint32[mNumDBOccurs];
		memcpy(mDBOccurrence, is.mDBOccurrence, mNumDBOccurs * sizeof(uint32));
	}
	if(is.mUsage == NULL) {
		mUsage = NULL;
	} else {
		mUsage = new uint32[mNumDBOccurs];
		memcpy(mUsage, is.mUsage, mNumUsages * sizeof(uint32));
	}
	if(is.mPrevUsage == NULL) {
		mPrevUsage = NULL;
	} else {
		mPrevUsage = new uint32[mNumDBOccurs];
		memcpy(mPrevUsage, is.mPrevUsage, mNumPrevUsages * sizeof(uint32));
	}
	mRefCount = 1;
}

ItemSet::ItemSet(const ItemSet& is, bool copyUsages) {
	mID = is.mID;
	mUsageCount = is.mUsageCount;
	mUsageArea = is.mUsageArea;
	mSupport = is.mSupport;
	mPrevUsageCount = is.mPrevUsageCount;
	mPrevUsageArea = is.mPrevUsageArea;
	mStdLength = is.mStdLength;

	mNumDBOccurs = is.mNumDBOccurs;
	mNumUsages = 0;
	mNumPrevUsages = 0;
	if(is.mDBOccurrence == NULL) {
		mDBOccurrence = NULL;
	} else {
		mDBOccurrence = new uint32[mNumDBOccurs];
		memcpy(mDBOccurrence, is.mDBOccurrence, mNumDBOccurs * sizeof(uint32));
	}
	if(is.mUsage == NULL) {
		mUsage = NULL;
	} else {
		mUsage = new uint32[mNumDBOccurs];
		//memcpy(mUsage, is.mUsage, mNumUsages * sizeof(uint32));
	}
	if(is.mPrevUsage == NULL) {
		mPrevUsage = NULL;
	} else {
		mPrevUsage = new uint32[mNumDBOccurs];
		//memcpy(mPrevUsage, is.mPrevUsage, mNumPrevUsages * sizeof(uint32));
	}
	mRefCount = 1;
}

ItemSet::~ItemSet() {
	if(mRefCount > 1) {
		throw string("Deleting itemset with multiple references");
	}
	delete[] mDBOccurrence;
	delete[] mUsage;
	delete[] mPrevUsage;
}

// set wordt netjes gekopieerd, ownership wordt dus niet overgenomen.
ItemSet *ItemSet::Create(ItemSetType type, const uint16 *set, uint32 setlen, uint32 abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	if(type == BAI32ItemSetType)
		return new BAI32ItemSet(set, setlen, abetlen, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == Uint16ItemSetType)
		return new Uint16ItemSet(set, setlen, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == BM128ItemSetType)
		return new BM128ItemSet(set, setlen, cnt, freq, numOcc, stdLen, occurrences);
	throw string("Wrong ItemSetType.");
}
ItemSet *ItemSet::CreateSingleton(ItemSetType type, uint16 elem, uint32 abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	if(type == BAI32ItemSetType)
		return new BAI32ItemSet(elem, abetlen, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == Uint16ItemSetType)
		return new Uint16ItemSet(elem, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == BM128ItemSetType)
		return new BM128ItemSet(elem, cnt, freq, numOcc, stdLen, occurrences);
	throw string("Wrong ItemSetType.");
}
ItemSet *ItemSet::CreateEmpty(ItemSetType type, size_t abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	if(type == BAI32ItemSetType)
		return new BAI32ItemSet((uint32) abetlen, cnt, freq, numOcc, stdLen, occurrences);
	else if(type == Uint16ItemSetType)
		return new Uint16ItemSet(cnt, freq, numOcc, stdLen, occurrences);
	else if(type == BM128ItemSetType)
		return new BM128ItemSet(cnt, freq, numOcc, stdLen, occurrences);
	throw string("Wrong ItemSetType.");
}

uint32 ItemSet::MemUsage(const ItemSetType type, const uint32 abLen, const uint32 setLen) {
	if(type == BAI32ItemSetType)
		return BAI32ItemSet::MemUsage(abLen);
	else if(type == Uint16ItemSetType)
		return Uint16ItemSet::MemUsage(setLen);
	else if(type == BM128ItemSetType)
		return BM128ItemSet::MemUsage();
	throw string("Wrong ItemSetType.");
}
uint32 ItemSet::MemUsage(Database *db) {
	ItemSetType type = db->GetDataType();
	if(type == BAI32ItemSetType)
		return BAI32ItemSet::MemUsage((uint32)db->GetAlphabetSize());
	else if(type == Uint16ItemSetType)
		return Uint16ItemSet::MemUsage(db->GetMaxSetLength());
	else if(type == BM128ItemSetType)
		return BM128ItemSet::MemUsage();
	throw string("Wrong ItemSetType.");
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

void ItemSet::SetDBOccurrence(uint32 *dbRows, uint32 num) {
	delete[] mDBOccurrence;
	mDBOccurrence = dbRows;
	mNumDBOccurs = num;
	delete[] mUsage;
	mUsage = new uint32[mNumDBOccurs];
	mNumUsages = 0;
	delete[] mPrevUsage;
	mPrevUsage = new uint32[mNumDBOccurs];
	mNumPrevUsages = 0;
}

void ItemSet::SetUsage(uint32 *dbRows, uint32 num) {
	//copy(dbRows, dbRows + num, mUsage);
	memcpy(mUsage, dbRows, num * sizeof(uint32));
	mNumUsages = num;
}

uint32* ItemSet::GetOccs() {
	return mDBOccurrence;
}

uint32 ItemSet::GetNumOccs() {
	return mNumDBOccurs;
}
void ItemSet::DetermineDBOccurence(Database *db, bool calcNumOcc /* = true */) {
	db->DetermineOccurrences(this,calcNumOcc);
}


uint32*	ItemSet::GetUsages()	const {
	return mUsage;
}
uint32*	ItemSet::GetPrevUsages()	const {
	return mPrevUsage;
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

void ItemSet::UpdateDBOccurrencesAfterUnion(const ItemSet* i, const ItemSet* j, ItemSet* result) {
	// old, koen-ware
	
		vector<uint32> tmpDBOccurrence(min(i->mNumDBOccurs, j->mNumDBOccurs));
		vector<uint32>::iterator it;

		it = set_intersection(i->mDBOccurrence, i->mDBOccurrence + i->mNumDBOccurs, j->mDBOccurrence, j->mDBOccurrence + j->mNumDBOccurs, tmpDBOccurrence.begin());

		if (result->mDBOccurrence) {
			delete[] result->mDBOccurrence;
			result->mDBOccurrence = NULL;
		}
		if (result->mUsage) {
			delete[] result->mUsage;
			result->mUsage = NULL;
		}
		if (result->mPrevUsage) {
			delete[] result->mPrevUsage;
			result->mPrevUsage = NULL;
		}

		result->mNumDBOccurs = uint32(it - tmpDBOccurrence.begin());
		result->mDBOccurrence = new uint32[result->mNumDBOccurs];
		memcpy(result->mDBOccurrence, &tmpDBOccurrence[0], result->mNumDBOccurs * sizeof(uint32));

		result->mUsage = new uint32[result->mNumDBOccurs];
		result->mPrevUsage = new uint32[result->mNumDBOccurs];
		result->mNumUsages = 0;
		result->mNumPrevUsages = 0;
	
	// new jv-ware
	/*
		vector<uint32> tmpDBOccurrence(min(i->mNumDBOccurs, j->mNumDBOccurs));
		vector<uint32>::iterator it;

		it = set_intersection(i->mDBOccurrence, i->mDBOccurrence + i->mNumDBOccurs, j->mDBOccurrence, j->mDBOccurrence + j->mNumDBOccurs, tmpDBOccurrence.begin());

		// Save into result
		// re-use arrays (blindly assumes that result->mDBOccurrence, result->mUsage and result->mPrevUsage exist and are large enough -- which they should be)

		result->mNumDBOccurs = uint32(it - tmpDBOccurrence.begin());;
		//copy(tmpDBOccurrence.begin(), it, result->mDBOccurrence);
		memcpy(result->mDBOccurrence, &tmpDBOccurrence[0], result->mNumDBOccurs * sizeof(uint32));
		result->mNumUsages = 0;
		result->mNumPrevUsages = 0;
	*/
}

void ItemSet::SetAllUsed() {
	mNumUsages = mNumDBOccurs;
	memcpy(mUsage, mDBOccurrence, mNumUsages * sizeof(uint32));
}

void ItemSet::SetAllUnused() {
	mNumUsages = 0;
}

void ItemSet::SetUsed(uint32 tid) { //, bool keepSorted ) {
	uint32* p = mUsage + ArrayUtils::BinarySearchInsertionLocationAsc(tid, mUsage, mNumUsages);
	memmove(p + 1, p, (mNumUsages - (p - mUsage)) * sizeof(uint32));
	*p = tid;
	++mNumUsages;
}

void ItemSet::SetUnused(uint32 tid) { //, bool keepSorted /* = true */) {
	if (mNumUsages > 1 && mUsage[mNumUsages - 1] != tid) {
		uint32* p = mUsage + 1 + ArrayUtils::BinarySearchAsc(tid, mUsage, mNumUsages);
		memmove(p - 1, p, (mNumUsages - (p - mUsage)) * sizeof(uint32));
	}
	--mNumUsages;
}

void ItemSet::SetUnused(vector<uint32> *usageZap) {
	vector<uint32> ::iterator zapCur = usageZap->begin(), zapEnd = usageZap->end();
	uint32 actualIndex = 0, oldNumUsages = mNumUsages, oldActualIndex = 0;
	for(uint32 i = 0; i < usageZap->size(); i++){
		uint32 index = ArrayUtils::BinarySearchAsc(usageZap->at(i), mUsage + actualIndex , oldNumUsages - actualIndex);
		memmove(mUsage + actualIndex - i, mUsage + actualIndex, (index) * sizeof(uint32));
		mNumUsages--;
		actualIndex += index + 1;
	}
	memmove(mUsage + actualIndex - usageZap->size(), mUsage + actualIndex, (oldNumUsages - actualIndex) * sizeof(uint32));
}

void ItemSet::SetUsed(vector<uint32> *usageAdd) {
	vector<uint32> ::iterator zapCur = usageAdd->begin(), zapEnd = usageAdd->end();
	uint32 oldP = 0;
	uint32 curNumUsages = mNumUsages, step = usageAdd->size(), oldIndex = 0;
	for(int i = usageAdd->size() - 1; i >= 0; i--){
 		uint32 index = ArrayUtils::BinarySearchInsertionLocationAsc(usageAdd->at(i), mUsage, curNumUsages);
		memmove(mUsage + index + step, mUsage + index, (curNumUsages - index) * sizeof(uint32));
		mUsage[index + step - 1] = usageAdd->at(i);
		mNumUsages++;
		step--;
		curNumUsages = index;
	}
}

/*
void ItemSet::AddUsage(uint32 tid) {
	//SetUsed(tid);
	mUsageAdds.push_back(tid);
}

void ItemSet::ZapUsage(uint32 tid) {
	//SetUnused(tid);
	mUsageZaps.push_back(tid);
}
*/
// use with extreme care.
void ItemSet::PushUsed(uint32 tid) {
	mUsage[mNumUsages++] = tid;
}
/*
void ItemSet::SyncUsed() {
	list<uint32>::iterator it = mUsageZaps.begin(), end = mUsageZaps.end();
	for(;it != end; ++it)
		SetUnused(*it);
	it = mUsageAdds.begin(), end = mUsageAdds.end();
	for(;it != end; ++it)
		SetUsed(*it);
	ClearUsed();
}

void ItemSet::ClearUsed() {
	mUsageZaps.clear();
	mUsageAdds.clear();
}
*/