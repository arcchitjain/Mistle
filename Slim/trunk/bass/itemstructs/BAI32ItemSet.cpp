#include "../Bass.h"
#include "../isc/ItemTranslator.h"
#include "Uint16ItemSet.h"

#include "BAI32ItemSet.h"

#if defined (__GNUC__)
	#ifndef max
	#define max(a,b) (((a)>(b))?(a):(b))
	#endif
	#ifndef min
	#define min(a,b) (((a)<(b))?(a):(b))
	#endif
#endif

uint32* BAI32ItemSet::sValueDumper = NULL;

BAI32ItemSet::BAI32ItemSet(const BAI32ItemSet *is) : ItemSet(*is) {
	mMaskLenBits = is->mMaskLenBits;
	mMaskLenDWord = (mMaskLenBits / 32) + (mMaskLenBits % 32 > 0 ? 1 : 0);
	mMask = new uint32[mMaskLenDWord];
	memcpy(mMask, is->mMask, mMaskLenDWord * sizeof(uint32));
	mBitCount = is->mBitCount;
	mBitCountFresh = is->mBitCountFresh;
	mLastItem = is->mLastItem;

	// sValueDumper zal geinit zijn, want 'is' is tenslotte reeds gemaakt
}
BAI32ItemSet::BAI32ItemSet(uint32 abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	mMaskLenBits = abetlen;
	mMaskLenDWord = (abetlen / 32) + (abetlen % 32 > 0 ? 1 : 0);
	mMask = new uint32[mMaskLenDWord];
	memset(mMask, 0, sizeof(uint32) * mMaskLenDWord);
	mBitCount = 0;
	mBitCountFresh = true;
	mLastItem = UINT16_MAX_VALUE;

	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;

//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];

	mPrevUsageCount = mUsageCount = cnt;
	mSupport = freq;
	mStdLength = stdLen;

	if(sValueDumper == NULL)
		sValueDumper = new uint32[abetlen];
}
BAI32ItemSet::BAI32ItemSet(uint16 elem, uint32 abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	mMaskLenBits = abetlen;
	mMaskLenDWord = (abetlen / 32) + (abetlen % 32 > 0 ? 1 : 0);
	mLastItem = elem;
	if(mMaskLenBits < mLastItem)
		THROW("Encountered item does not fall within provided alphabet length");
	mMask = new uint32[mMaskLenDWord];
	memset(mMask, 0, sizeof(uint32) * mMaskLenDWord);
	Set(elem);
	mBitCount = 1;
	mBitCountFresh = true;

	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;

//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];

	mPrevUsageCount = mUsageCount = cnt;
	mSupport = freq;
	mStdLength = stdLen;

	if(sValueDumper == NULL)
		sValueDumper = new uint32[abetlen];
}
BAI32ItemSet::BAI32ItemSet(const uint16 *set, uint32 setlen, uint32 abetlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	mMaskLenBits = abetlen;
	mMaskLenDWord = (abetlen / 32) + (abetlen % 32 > 0 ? 1 : 0);
	mLastItem = setlen > 0 ? set[setlen-1] : UINT16_MAX_VALUE;
	if(mMaskLenBits < mLastItem)
		THROW("Encountered item does not fall within provided alphabet length");

	mMask = new uint32[mMaskLenDWord];
	memset(mMask, 0, sizeof(uint32) * mMaskLenDWord);
	for(uint16 i=0; i<setlen; i++) {
		if(set[i] > mLastItem) { 
			mLastItem = set[i];
			if(mMaskLenBits < mLastItem)
				THROW("Encountered item does not fall within provided alphabet length");
		}
		Set(set[i]);
	}
	mBitCount = setlen;
	mBitCountFresh = true;

	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;

//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];

	mPrevUsageCount = mUsageCount = cnt;
	mSupport = freq;
	mStdLength = stdLen;

	if(sValueDumper == NULL)
		sValueDumper = new uint32[abetlen];
}
BAI32ItemSet::~BAI32ItemSet() {
	delete[] mMask;
}

void BAI32ItemSet::CleanUpStaticMess() {
	delete[] sValueDumper;
	sValueDumper = NULL;
}

BAI32ItemSet* BAI32ItemSet::Clone() const {
	return new BAI32ItemSet((BAI32ItemSet*)this);
}

uint32 BAI32ItemSet::GetMemUsage() const {
	return sizeof(BAI32ItemSet) + 16 + mMaskLenDWord * sizeof(uint32);
}
uint32 BAI32ItemSet::MemUsage(const uint32 abLen) {
	uint32 arLenDWords = ((abLen / 32) + (abLen % 32 > 0 ? 1 : 0));
	return sizeof(BAI32ItemSet) + 16 + arLenDWords * sizeof(uint32);
}

uint32 __inline BAI32ItemSet::GetLength() {
	//ASSERT(mBitCountFresh == true);
	if(!mBitCountFresh) {

		mBitCount = 0;
		for(uint32 i=0; i<mMaskLenDWord; i++) {
			mBitCount += _count[(unsigned char)(mMask[i])] + 
				_count[(unsigned char)((mMask[i]) >> 8)] + 
				_count[(unsigned char)((mMask[i]) >> 16)] + 
				_count[(unsigned char)((mMask[i]) >> 24)];
		}
		mBitCountFresh = true;
	}
	return mBitCount;
}
void BAI32ItemSet::CalcLength() {
	if(!mBitCountFresh) {
		mBitCount = 0;
		for(uint32 i=0; i<mMaskLenDWord; i++) {
			mBitCount += _count[(unsigned char)(mMask[i])] + 
				_count[(unsigned char)((mMask[i]) >> 8)] + 
				_count[(unsigned char)((mMask[i]) >> 16)] + 
				_count[(unsigned char)((mMask[i]) >> 24)];
		}
		mBitCountFresh = true;
	}
}

void BAI32ItemSet::CopyFrom(BAI32ItemSet *is) {
	memcpy(mMask, is->mMask, mMaskLenDWord * sizeof(uint32));
	mBitCount = is->mBitCount;
	mBitCountFresh = is->mBitCountFresh;
}
const uint16 BAI32ItemSet::GetLastItem() const {
	return mLastItem;
}
void BAI32ItemSet::FillHerUp(uint16 max) {
	uint32 offset = 1;
	for(uint32 i=0; i<mMaskLenDWord; i++) {
		uint32 offset = i * 32;
		mMask[i] = (max > offset) ? 0xFFFFFFFF << (32 - (min(max-offset,32) % 32)) : 0x00000000;
	}
//	memset(mArray, 0xFFFFFFFF, sizeof(uint32) * mArLenDWord);
//	mArray[mArLenDWord-1] = 0xFFFFFFFF << (32 - (max % 32));
	mBitCount = max;
	mBitCountFresh = true;
}
void BAI32ItemSet::CleanTheSlate() {
	memset(mMask, 0, sizeof(uint32)*mMaskLenDWord);
	mBitCount = 0;
	mBitCountFresh = true;
}
/*
void __inline BAI32ItemSet::Set(uint32 i) -- now inline in header
*/
void BAI32ItemSet::UnSet(uint32 i) {
	uint32 idx = i >> 5;
	uint32 bvx = 0x01 << (i % 32);
	mMask[idx] = mMask[idx] & ~bvx;
	mBitCountFresh = false;
}
bool BAI32ItemSet::Get(uint32 i) const {
	uint32 idx = (i / 32);
	uint32 bvx = 0x01 << (i % 32);
	return (mMask[idx] & bvx) == 1;
}
void BAI32ItemSet::Not() {
	for(uint32 i=0; i<mMaskLenDWord; i++)
		mMask[i] = ~mMask[i];
	mMask[mMaskLenDWord-1] = mMask[mMaskLenDWord-1] & (0xFFFFFFFF >> (32 - (mMaskLenBits % 32)));
	mBitCount = mMaskLenBits - mBitCount;
}
// Houdt bitcountfresh stabiel
void BAI32ItemSet::AddItemToSet(uint16 val) {
	uint32 idx = val >> 5;
	uint32 bvx = 0x01 << (val % 32);
	if((mMask[idx] & bvx) == bvx)
		return;
	mMask[idx] = mMask[idx] | bvx;
	mBitCount++;
}
// Houdt bitcountfresh stabiel
void BAI32ItemSet::RemoveItemFromSet(uint16 val) {
	uint32 idx = val >> 5;
	uint32 bvx = 0x01 << (val % 32);
	if((mMask[idx] & bvx) != bvx)
		return;
	mMask[idx] = mMask[idx] & ~bvx;
	mBitCount--;
}
bool BAI32ItemSet::IsItemInSet(uint16 val) const {
	uint32 idx = (val / 32);
	uint32 bvx = 0x01 << (val % 32);
	return (mMask[idx] & bvx) == bvx;
}

int8 BAI32ItemSet::CmpLexicographically(ItemSet *is) {
	CalcLength();
	uint32 isLen = is->GetLength();
	uint32 minLen = min(mBitCount, isLen);
	this->GetValuesIn(sValueDumper);
	uint16 *set2 = ((BAI32ItemSet*) is)->GetValues();
	for(uint32 i=0; i<minLen; i++)
		if(sValueDumper[i] < set2[i]) {
			delete[] set2; return -1;
		} else if(sValueDumper[i] > set2[i]) {
			delete[] set2; return 1;
		}
	delete[] set2;
	if(mBitCount == isLen)
		return 0;
	return mBitCount < isLen ? -1 : 1;
}

bool BAI32ItemSet::Equals(ItemSet *is) const {
    return this->Equals(((BAI32ItemSet*)is)->mMask);
}
bool BAI32ItemSet::Equals(uint32 *ar) const {
	for(uint32 i=0; i < mMaskLenDWord; i++)
		if(mMask[i] != ar[i])
			return 0;
	return 1;
}

bool BAI32ItemSet::Intersects(ItemSet *is) const {
	return Intersects(((BAI32ItemSet*)is)->mMask);
}
bool BAI32ItemSet::Intersects(uint32 *ar) const {
	for(uint32 i=0; i < mMaskLenDWord; i++)
		if((mMask[i] & ar[i]) != 0)
			return 1;
	return 0;
}
uint32 BAI32ItemSet::IntersectionLength(uint32 *ar) const {
	uint32 intlen = 0;
	uint32 tmpint = 0;
	for(uint32 i=0; i < mMaskLenDWord; i++) {
		if((tmpint = (mMask[i] & ar[i])) != 0) {
			intlen += _count[(unsigned char)(tmpint)] + _count[(unsigned char)((tmpint) >> 8)] + _count[(unsigned char)((tmpint) >> 16)] + _count[(unsigned char)((tmpint) >> 24)];
		}
	}
	return intlen;
}

bool BAI32ItemSet::IsSubset(ItemSet *is) const {
	return IsSubset(((BAI32ItemSet*)is)->mMask);
}
bool BAI32ItemSet::IsSubset(uint32 *ar) const {
	for(uint32 i=0; i<mMaskLenDWord; i++) {
		if((~mMask[i]) & ar[i]) {
			return 0;
		}		
	}
	return 1;
}

// Subtracts b from a
void BAI32ItemSet::Remove(ItemSet *is) {
	BAI32ItemSet::Remove(((BAI32ItemSet*)is)->mMask);
}
void BAI32ItemSet::Remove(uint32 *ar) {
	for(uint32 i=0; i < mMaskLenDWord; i++)
		mMask[i] = mMask[i] & (mMask[i] ^ ar[i]);
	mBitCountFresh = false;
}

void BAI32ItemSet::RemoveSubset(ItemSet *is) {
	BAI32ItemSet::RemoveSubset(((BAI32ItemSet*)is)->mMask, ((BAI32ItemSet*)is)->GetLength());
}
void BAI32ItemSet::RemoveSubset(uint32 *ar, uint32 subsetlen) {
	for(uint32 i=0; i < mMaskLenDWord; i++)
		mMask[i] = mMask[i] ^ ar[i];
	mBitCount -= subsetlen;
}
void BAI32ItemSet::UniteNonOverlappingSet(uint32 *ar, uint32 noverlaplen) {
	for(uint32 i=0; i < mMaskLenDWord; i++)
		mMask[i] |= ar[i];
	mBitCount += noverlaplen;
}
void BAI32ItemSet::Unite(ItemSet *is) {
	Or((BAI32ItemSet*)is);
	if (mLastItem != UINT16_MAX_VALUE)
		mLastItem = max(mLastItem, ((BAI32ItemSet*)is)->mLastItem);
	else
		mLastItem = ((BAI32ItemSet*)is)->mLastItem;
}

// Applies Bitwise OR
void BAI32ItemSet::Or(BAI32ItemSet *ba) {
	return Or(ba->mMask);
}
void __inline BAI32ItemSet::Or(uint32 *ar) {
	for(uint32 i=0; i < mMaskLenDWord; i++)
		mMask[i] |= ar[i];
	mBitCountFresh = false;
}

// Applies Bitwise AND
void BAI32ItemSet::And(BAI32ItemSet *ba) {
	return And(ba->mMask);
}
void __inline BAI32ItemSet::And(uint32 *ar) {
	for(uint32 i=0; i<mMaskLenDWord; i++)
		mMask[i] &= ar[i];
	mBitCountFresh = false;
}


ItemSet* BAI32ItemSet::Union(ItemSet *is) const {
	BAI32ItemSet *bis = new BAI32ItemSet((BAI32ItemSet*)this);
	bis->Or(((BAI32ItemSet*)is)->mMask);
	if (bis->mLastItem != UINT16_MAX_VALUE)
		bis->mLastItem = max(bis->mLastItem, ((BAI32ItemSet*)is)->mLastItem);
	else
		bis->mLastItem = ((BAI32ItemSet*)is)->mLastItem;
	ItemSet::UpdateDBOccurrencesAfterUnion(this, is, bis);
	return bis;
}
ItemSet* BAI32ItemSet::Intersection(ItemSet *is) const {
	BAI32ItemSet *bis = new BAI32ItemSet((BAI32ItemSet*)this);
	bis->And(((BAI32ItemSet*)is)->mMask);
	return bis;
}

void BAI32ItemSet::GetValuesIn(uint32* vals) const {
	/*
		alt:
		uint32 *vals = new uint32[abetlen];		// basic values
		uint32 *cnts = new uint32[abetlen / 8];

		for each dword d
			for each byte b
			num = _count[b];
			memcpy(vals + 0 * sizeof(uint32), _idxon[b], sizeof(uint32) * num);
			cnts[idx++] = num;

		readout:
		for each vals i -> v
			realval = d[i] + v - o[i];
					

	*/
	// 8bit lookup, 110s
	const uint32 *subs;
	uint32 num;
	uint32 idx = 0;
	uint8 bte = 0;
	for(uint32 d=0; d<mMaskLenDWord; d++) {
		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] - 1);

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++) {
			vals[idx++] = d * 32 + (subs[i] + 7);
		}

		num = _count[(unsigned char)(mMask[d] >> 16)];
		subs = _idxon[(unsigned char)(mMask[d] >> 16)];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] + 15);

		num = _count[(unsigned char)(mMask[d] >> 24)];
		subs = _idxon[(unsigned char)(mMask[d] >> 24)];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] + 23);
	}
}
void BAI32ItemSet::GetValuesIn(uint16* vals) const {
	// 8bit lookup
	const uint32 *subs;
	uint32 num;
	uint32 idx = 0;
	uint8 bte = 0;
	for(uint32 d=0; d<mMaskLenDWord; d++) {
		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] - 1);

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++) {
			vals[idx++] = d * 32 + (subs[i] + 7);
		}

		num = _count[(unsigned char)(mMask[d] >> 16)];
		subs = _idxon[(unsigned char)(mMask[d] >> 16)];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] + 15);

		num = _count[(unsigned char)(mMask[d] >> 24)];
		subs = _idxon[(unsigned char)(mMask[d] >> 24)];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] + 23);
	}
}



uint16* BAI32ItemSet::GetValues() const {
	// 1bit-shifting, 118s
	/*uint16 *vals = new uint16[mCount];
	uint32 worker = 0;
	uint32 counter = 0;
	for(uint32 d=0; d<mLenDWord; d++) {
	worker = mArray[d];
	for(uint32 b=0; b<32 && worker; b++) {
	if(worker & 0x01)
	vals[counter++] = d * 32 + b;
	worker = worker >> 1;
	}
	}
	return vals;
	*/
	// 8bit lookup, 110s
	// BUG FIX: invalid write if mBitCount is updated (or 0)
	if(!mBitCountFresh) {
		mBitCount = 0;
		for(uint32 i=0; i<mMaskLenDWord; i++) {
			mBitCount += _count[(unsigned char)(mMask[i])] +
				_count[(unsigned char)((mMask[i]) >> 8)] +
				_count[(unsigned char)((mMask[i]) >> 16)] +
				_count[(unsigned char)((mMask[i]) >> 24)];
		}
		mBitCountFresh = true;
	}

	uint16 *vals = new uint16[mBitCount];
	const uint32 *subs;
	uint32 num;
	uint32 idx = 0;
	uint8 bte = 0;
	for(uint32 d=0; d<mMaskLenDWord; d++) {
		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] - 1);

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++) {
			vals[idx++] = d * 32 + (subs[i] + 7);
		}

		num = _count[(unsigned char)(mMask[d] >> 16)];
		subs = _idxon[(unsigned char)(mMask[d] >> 16)];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] + 15);

		num = _count[(unsigned char)(mMask[d] >> 24)];
		subs = _idxon[(unsigned char)(mMask[d] >> 24)];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = d * 32 + (subs[i] + 23);
	}

	return vals;
	/*
	uint32 idx = prev >> 5;
	uint32 bdx = prev % 32;
	if(!(mArray[idx] & 0xFFFFFFFF << bdx)) {
	for(++idx; idx<mLenByte; idx++)
	if(mArray[idx] != 0)
	break;
	if(idx > mLenByte)
	return 0;
	bdx = 0;
	}*/
	/*

	uint32	GetNumBitsOn();
	uint16* GetValues() : 

	// static array die hergebruikt wordt
	uint16 *subs = [mNumBytes * 8];
	for each dword
	for each byte
	memcpy(subs+c, _idxon[byte], _counts[byte]*sizeof(uint16))
	c+=_counts[byte]*sizeof(uint16);
	// wellicht gewoon terugdumpen, niet resizen

	uint16 *subs = [mNumBits];
	for each dword d
	for each byte b
	for each item i
	subs[c++] = i - 1 + nogiets;


	--
	for each dword d
	for each bit b
	if bit == true
	subs[i++] = d * 32 + b

	*/

	/*if((unsigned char)(mArray[idx] >> bdx)) 
	return _first[(unsigned char)(mArray[idx] >> bdx)];

	val = 32 -	_first[(unsigned char)(mArray[idx] >> bdx+24)] -
	_first[(unsigned char)(mArray[idx] >> bdx+16)] -
	_first[(unsigned char)(mArray[idx] >> bdx+8)] -
	_first[(unsigned char)(mArray[idx] >> bdx)];*/
}
void BAI32ItemSet::IncrementValues(uint32* items, int32 cnt, uint32* vals) const {
	// 8bit lookup, 110s
	const uint32 *subs;
	uint32 num;
	uint32 idx = 0;
	uint8 bte = 0;
	uint32 counter = 0;
	for(uint32 d=0; d<mMaskLenDWord; d++) {
		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++) {
			items[d * 32 + (subs[i] - 1)] += cnt;
			if (vals) vals[counter++] = d * 32 + (subs[i] - 1);
		}

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++) {
			items[d * 32 + (subs[i] + 7)] += cnt;
			if (vals) vals[counter++] = d * 32 + (subs[i] + 7);
		}

		num = _count[(unsigned char)(mMask[d] >> 16)];
		subs = _idxon[(unsigned char)(mMask[d] >> 16)];
		for(uint32 i=0; i<num; i++) {
			items[d * 32 + (subs[i] + 15)] += cnt;
			if (vals) vals[counter++] = d * 32 + (subs[i] + 15);
		}

		num = _count[(unsigned char)(mMask[d] >> 24)];
		subs = _idxon[(unsigned char)(mMask[d] >> 24)];
		for(uint32 i=0; i<num; i++) {
			items[d * 32 + (subs[i] + 23)] += cnt;
			if (vals) vals[counter++] = d * 32 + (subs[i] + 23);
		}
	}
}

void BAI32ItemSet::TranslateBackward(ItemTranslator *it) {
	throw string("Not implemented");
}
void BAI32ItemSet::TranslateForward(ItemTranslator *it) {
	throw string("Not implemented");
}

string BAI32ItemSet::ToString(bool printCount, bool printFreq) {
	string s = "";
	char *buffer = new char[40];

	uint32 len = GetLength();
	uint16 *set = BAI32ItemSet::GetValues(); 
	if(len > 0) {
		for(uint32 i=0; i<len-1; i++) {
			sprintf_s(buffer, 40, "%d ", set[i]);
			s.append(buffer);
		}
		sprintf_s(buffer, 40, "%d", set[len-1]);
		s.append(buffer);
	}
	if(printCount || printFreq) {
		if(printCount && !printFreq) {
			sprintf_s(buffer, 40, " (%d)", mUsageCount);
		} else if(printCount && !printFreq) {
			sprintf_s(buffer, 40, " (%d)", mSupport);
		} else
			sprintf_s(buffer, 40, " (%d,%d)", mUsageCount, mSupport);
		s.append(buffer);
	}
	delete[] set;
	delete[] buffer;
	return s;
}
string BAI32ItemSet::ToCodeTableString() {
	string s = "";
	char *buffer = new char[40];

	uint32 len = GetLength();
	uint16 *set = BAI32ItemSet::GetValues(); 
	for(uint32 i=0; i<len; i++) {
		sprintf_s(buffer, 40, "%d ", set[i]);
		s.append(buffer);
	}
	sprintf_s(buffer, 40, "(%d,%d) ", mUsageCount, mSupport);
	s.append(buffer);
	delete[] set;
	delete[] buffer;
	return s;
}

void BAI32ItemSet::CopyMaskFrom(uint32* mask, uint32 len, uint16 lastItem) {
	memcpy(mMask, mask, sizeof(uint32)*mMaskLenDWord);
	mBitCount = len;
	mBitCountFresh = true;
	mLastItem = lastItem;
}

const unsigned char BAI32ItemSet::_count[256] = {
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
		1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
		1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
		2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
		1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
		2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
		2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
		3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

const uint32 BAI32ItemSet::_idxon[256][8] = { 
	{},{1},{2},{1,2},{3},{1,3},{2,3},{1,2,3},{4},{1,4},{2,4},{1,2,4},{3,4},{1,3,4},
	{2,3,4},{1,2,3,4},{5},{1,5},{2,5},{1,2,5},{3,5},{1,3,5},{2,3,5},{1,2,3,5},{4,5},
	{1,4,5},{2,4,5},{1,2,4,5},{3,4,5},{1,3,4,5},{2,3,4,5},{1,2,3,4,5},{6},{1,6},{2,6},
	{1,2,6},{3,6},{1,3,6},{2,3,6},{1,2,3,6},{4,6},{1,4,6},{2,4,6},{1,2,4,6},{3,4,6},
	{1,3,4,6},{2,3,4,6},{1,2,3,4,6},{5,6},{1,5,6},{2,5,6},{1,2,5,6},{3,5,6},{1,3,5,6},
	{2,3,5,6},{1,2,3,5,6},{4,5,6},{1,4,5,6},{2,4,5,6},{1,2,4,5,6},{3,4,5,6},{1,3,4,5,6},
	{2,3,4,5,6},{1,2,3,4,5,6},{7},{1,7},{2,7},{1,2,7},{3,7},{1,3,7},{2,3,7},{1,2,3,7},
	{4,7},{1,4,7},{2,4,7},{1,2,4,7},{3,4,7},{1,3,4,7},{2,3,4,7},{1,2,3,4,7},{5,7},
	{1,5,7},{2,5,7},{1,2,5,7},{3,5,7},{1,3,5,7},{2,3,5,7},{1,2,3,5,7},{4,5,7},{1,4,5,7},
	{2,4,5,7},{1,2,4,5,7},{3,4,5,7},{1,3,4,5,7},{2,3,4,5,7},{1,2,3,4,5,7},{6,7},{1,6,7},
	{2,6,7},{1,2,6,7},{3,6,7},{1,3,6,7},{2,3,6,7},{1,2,3,6,7},{4,6,7},{1,4,6,7},
	{2,4,6,7},{1,2,4,6,7},{3,4,6,7},{1,3,4,6,7},{2,3,4,6,7},{1,2,3,4,6,7},{5,6,7},
	{1,5,6,7},{2,5,6,7},{1,2,5,6,7},{3,5,6,7},{1,3,5,6,7},{2,3,5,6,7},{1,2,3,5,6,7},
	{4,5,6,7},{1,4,5,6,7},{2,4,5,6,7},{1,2,4,5,6,7},{3,4,5,6,7},{1,3,4,5,6,7},
	{2,3,4,5,6,7},{1,2,3,4,5,6,7},{8},{1,8},{2,8},{1,2,8},{3,8},{1,3,8},{2,3,8},
	{1,2,3,8},{4,8},{1,4,8},{2,4,8},{1,2,4,8},{3,4,8},{1,3,4,8},{2,3,4,8},{1,2,3,4,8},
	{5,8},{1,5,8},{2,5,8},{1,2,5,8},{3,5,8},{1,3,5,8},{2,3,5,8},{1,2,3,5,8},{4,5,8},
	{1,4,5,8},{2,4,5,8},{1,2,4,5,8},{3,4,5,8},{1,3,4,5,8},{2,3,4,5,8},{1,2,3,4,5,8},
	{6,8},{1,6,8},{2,6,8},{1,2,6,8},{3,6,8},{1,3,6,8},{2,3,6,8},{1,2,3,6,8},{4,6,8},
	{1,4,6,8},{2,4,6,8},{1,2,4,6,8},{3,4,6,8},{1,3,4,6,8},{2,3,4,6,8},{1,2,3,4,6,8},
	{5,6,8},{1,5,6,8},{2,5,6,8},{1,2,5,6,8},{3,5,6,8},{1,3,5,6,8},{2,3,5,6,8},
	{1,2,3,5,6,8},{4,5,6,8},{1,4,5,6,8},{2,4,5,6,8},{1,2,4,5,6,8},{3,4,5,6,8},
	{1,3,4,5,6,8},{2,3,4,5,6,8},{1,2,3,4,5,6,8},{7,8},{1,7,8},{2,7,8},{1,2,7,8},{3,7,8},
	{1,3,7,8},{2,3,7,8},{1,2,3,7,8},{4,7,8},{1,4,7,8},{2,4,7,8},{1,2,4,7,8},{3,4,7,8},
	{1,3,4,7,8},{2,3,4,7,8},{1,2,3,4,7,8},{5,7,8},{1,5,7,8},{2,5,7,8},{1,2,5,7,8},
	{3,5,7,8},{1,3,5,7,8},{2,3,5,7,8},{1,2,3,5,7,8},{4,5,7,8},{1,4,5,7,8},{2,4,5,7,8},
	{1,2,4,5,7,8},{3,4,5,7,8},{1,3,4,5,7,8},{2,3,4,5,7,8},{1,2,3,4,5,7,8},{6,7,8},
	{1,6,7,8},{2,6,7,8},{1,2,6,7,8},{3,6,7,8},{1,3,6,7,8},{2,3,6,7,8},{1,2,3,6,7,8},
	{4,6,7,8},{1,4,6,7,8},{2,4,6,7,8},{1,2,4,6,7,8},{3,4,6,7,8},{1,3,4,6,7,8},
	{2,3,4,6,7,8},{1,2,3,4,6,7,8},{5,6,7,8},{1,5,6,7,8},{2,5,6,7,8},{1,2,5,6,7,8},
	{3,5,6,7,8},{1,3,5,6,7,8},{2,3,5,6,7,8},{1,2,3,5,6,7,8},{4,5,6,7,8},{1,4,5,6,7,8},
	{2,4,5,6,7,8},{1,2,4,5,6,7,8},{3,4,5,6,7,8},{1,3,4,5,6,7,8},{2,3,4,5,6,7,8},{1,2,3,4,5,6,7,8}
};

