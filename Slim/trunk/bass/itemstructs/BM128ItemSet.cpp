#include "../Bass.h"

#ifdef SSE42
#include <nmmintrin.h>
#else
#include <xmmintrin.h>
#endif

#include "../isc/ItemTranslator.h"
#include "Uint16ItemSet.h"

#if defined (__GNUC__)
	#ifndef max
	#define max(a,b) (((a)>(b))?(a):(b))
	#endif
	#ifndef min
	#define min(a,b) (((a)<(b))?(a):(b))
	#endif
#endif

#include "BM128ItemSet.h"

uint32* BM128ItemSet::sValueDumper = NULL;

BM128ItemSet::BM128ItemSet(const BM128ItemSet *is) : ItemSet(*is) {
	mMask = (uint32*) _mm_malloc(16,16);
	memcpy(mMask, is->mMask, 16);
	mLastItem = is->mLastItem;

	mBitCount = is->mBitCount;
	mBitCountFresh = is->mBitCountFresh;

	// sValueDumper zal geinit zijn, want 'is' is tenslotte reeds gemaakt. maar voor de zekerheid..
	if(sValueDumper == NULL)
		sValueDumper = (uint32*) _mm_malloc(512,16); // allocate 512 bytes, 16byte aligned
}
BM128ItemSet::BM128ItemSet(const BM128ItemSet *is, bool copyUsages) : ItemSet(*is, copyUsages) {
	mMask = (uint32*) _mm_malloc(16,16);
	memcpy(mMask, is->mMask, 16);
	mLastItem = is->mLastItem;

	mBitCount = is->mBitCount;
	mBitCountFresh = is->mBitCountFresh;

	// sValueDumper zal geinit zijn, want 'is' is tenslotte reeds gemaakt. maar voor de zekerheid..
	if(sValueDumper == NULL)
		sValueDumper = (uint32*) _mm_malloc(512,16); // allocate 512 bytes, 16byte aligned
}
BM128ItemSet::BM128ItemSet(uint32 cnt /* =1 */, uint32 freq /* =1 */, uint32 numOcc /* =0 */, double stdLen /* = 0 */, uint32 *occurrences /* = NULL */) {
	mMask = (uint32*) _mm_malloc(16,16);	// allocate 16 bytes, aligned for 16byte blocks
	memset(mMask, 0, sizeof(uint32)*4);
	mLastItem = UINT16_MAX_VALUE;

	mBitCount = 0;
	mBitCountFresh = true;

	mPrevUsageCount = mUsageCount = cnt;
	mSupport = freq;
	mStdLength = stdLen;

	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;

//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];

	if(sValueDumper == NULL)
		sValueDumper = (uint32*) _mm_malloc(512,16); // allocate 512 bytes, 16byte aligned
}
BM128ItemSet::BM128ItemSet(uint16 elem, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	mMask = (uint32*) _mm_malloc(16,16);	// allocate 16 bytes, aligned for 16byte blocks
	memset(mMask, 0, sizeof(uint32)*4);
	Set(elem);
	mLastItem = elem;

	mBitCount = 1;
	mBitCountFresh = true;

	mPrevUsageCount = mUsageCount = cnt;
	mSupport = freq;
	mStdLength = stdLen;

	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;

//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];

	if(sValueDumper == NULL)
		sValueDumper = (uint32*) _mm_malloc(512,16); // allocate 512 bytes, 16byte aligned
}
BM128ItemSet::BM128ItemSet(const uint16 *set, uint32 setlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	mMask = (uint32*) _mm_malloc(16,16);	// allocate 16 bytes, aligned for 16byte blocks
	memset(mMask, 0, sizeof(uint32)*4);
	mLastItem = 0;
	for(uint16 i=0; i<setlen; i++) {
		Set(set[i]);
		if(set[i] > mLastItem) mLastItem = set[i];
	} if(setlen == 0) mLastItem = UINT16_MAX_VALUE;
	mBitCount = setlen;
	mBitCountFresh = true;

	mPrevUsageCount = mUsageCount = cnt;
	mSupport = freq;
	mStdLength = stdLen;

	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;

//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];

	if(sValueDumper == NULL)
		sValueDumper = (uint32*) _mm_malloc(512,16); // allocate 512 bytes, 16byte aligned
}
BM128ItemSet::~BM128ItemSet() {
	_mm_free(mMask);
}

void BM128ItemSet::CleanUpStaticMess() {
	if(sValueDumper != NULL) {
		_mm_free(sValueDumper);
		sValueDumper = NULL;
	}
}

BM128ItemSet* BM128ItemSet::Clone() const {
	return new BM128ItemSet(this);
}

BM128ItemSet* BM128ItemSet::CloneLight() const {
	return new BM128ItemSet(this, false);
}

uint32 BM128ItemSet::GetMemUsage() const {
	uint32 size = sizeof(BM128ItemSet) + 4*sizeof(uint32);
	return size + size/2;
}
uint32 BM128ItemSet::MemUsage() {
	uint32 size = sizeof(BM128ItemSet) + 4*sizeof(uint32);
	return size + size/2;
}

uint32 __inline BM128ItemSet::GetLength() {
	if(!mBitCountFresh)			// re-added this on 19-03-2007, necessary wrt up2date ToString... (maybe add a GetBitCount() for fast, direct non-checked access?)
		CalcLength();
	return mBitCount;
}

void __inline BM128ItemSet::CalcLength() {
	if(!mBitCountFresh) {
		mBitCount = 0;
		// look-up-table based -- at least as fast as the popcnt variants (2014-05-29, i5 3.3Ghz machine)
		if(true) {
			// O(1)
			mBitCount += _count[(unsigned char)(mMask[0])] + _count[(unsigned char)((mMask[0]) >> 8)] + _count[(unsigned char)((mMask[0]) >> 16)] + _count[(unsigned char)((mMask[0]) >> 24)];
			mBitCount += _count[(unsigned char)(mMask[1])] + _count[(unsigned char)((mMask[1]) >> 8)] + _count[(unsigned char)((mMask[1]) >> 16)] + _count[(unsigned char)((mMask[1]) >> 24)];
			mBitCount += _count[(unsigned char)(mMask[2])] + _count[(unsigned char)((mMask[2]) >> 8)] + _count[(unsigned char)((mMask[2]) >> 16)] + _count[(unsigned char)((mMask[2]) >> 24)];
			mBitCount += _count[(unsigned char)(mMask[3])] + _count[(unsigned char)((mMask[3]) >> 8)] + _count[(unsigned char)((mMask[3]) >> 16)] + _count[(unsigned char)((mMask[3]) >> 24)];
		} else if(false) {
			// O(1) cpu-based popcount, requires SSE4.2+ or AMD. As fast as LUT
#ifdef SSE42
			mBitCount += (uint32) (_mm_popcnt_u64(((uint64*)mMask)[0]) + _mm_popcnt_u64(((uint64*)mMask)[1])); 
#endif
			//mBitCount += (uint32) _mm_popcnt_u32(mMask[0]) + _mm_popcnt_u32(mMask[1]) + _mm_popcnt_u32(mMask[2]) + _mm_popcnt_u32(mMask[3]);
		} else if(false) {
			// O(1) quite nice parallel computation, but not faster than above
			uint64 tmp = ((uint64*)mMask)[0];
			tmp = tmp - ((tmp >> 1) & 0x5555555555555555UL);
			tmp = (tmp & 0x3333333333333333UL) + ((tmp >> 2) & 0x3333333333333333UL);
			mBitCount = (((tmp + (tmp >> 4)) & 0xF0F0F0F0F0F0F0FUL) * 0x101010101010101UL) >> 56;
			tmp = ((uint64*)mMask)[1];
			tmp = tmp - ((tmp >> 1) & 0x5555555555555555UL);
			tmp = (tmp & 0x3333333333333333UL) + ((tmp >> 2) & 0x3333333333333333UL);
			mBitCount += (((tmp + (tmp >> 4)) & 0xF0F0F0F0F0F0F0FUL) * 0x101010101010101UL) >> 56;
		} else {
			// O(n), but no lookup nor multiplication, very fast if bitcounts are low
			uint64 val = ((uint64*)mMask)[0];
			while(val) {
				++mBitCount; 
				val &= val - 1;
			}
			val = ((uint64*)mMask)[1];
			while(val) {
				++mBitCount; 
				val &= val - 1;
			}
		}

		mBitCountFresh = true;
	}
}

void BM128ItemSet::CopyFrom(BM128ItemSet *is) {	
	memcpy(mMask, is->mMask, sizeof(uint32)*4);
	mBitCount = is->mBitCount;
	mBitCountFresh = is->mBitCountFresh;
}
/*void BM128ItemSet::CopyMaskFrom(uint32* mask, uint32 len) {
	memcpy(mMask, mask, sizeof(uint32)*4);
	mBitCount = len;
	mBitCountFresh = true;
}*/
void BM128ItemSet::CopyMaskFrom(uint32* mask, uint32 len, uint16 lastItem) {
	memcpy(mMask, mask, sizeof(uint32)*4);
	mBitCount = len;
	mBitCountFresh = true;
	mLastItem = lastItem;
}
/*
void BM128ItemSet::Set(uint32 i) -- now inline in header
*/
void BM128ItemSet::UnSet(uint32 i) {
	uint32 idx = i >> 5;
	//uint32 bvx = 0x01 << (i % 32);	// orig, lsb-like
	uint32 bvx = 0x80000000 >> (i % 32);		// new, msb-like
	mMask[idx] = mMask[idx] & (~bvx);
	mBitCountFresh = false;
}
// Houdt bitcountfresh stabiel
void BM128ItemSet::AddItemToSet(uint16 val) {
	uint32 idx = (val / 32);
	//uint32 bvx = 0x01 << (val % 32);	// orig, lsb-like
	uint32 bvx = 0x80000000 >> (val % 32);		// new, msb-like
	if((mMask[idx] & bvx) == bvx)
		return;
	mMask[idx] = mMask[idx] | bvx;
	mBitCount++;
}
// Houdt bitcountfresh stabiel
void BM128ItemSet::RemoveItemFromSet(uint16 val) {
	uint32 idx = (val / 32);
	//uint32 bvx = 0x01 << (val % 32);	// orig, lsb-like
	uint32 bvx = 0x80000000 >> (val % 32);		// new, msb-like
	if((mMask[idx] & bvx) != bvx)
		return;
	mMask[idx] = mMask[idx] & (~bvx);
	mBitCount--;
}
bool BM128ItemSet::IsItemInSet(uint16 val) const {
	uint32 idx = (val / 32);
	//uint32 bvx = 0x01 << (val % 32);	// orig, lsb-like
	uint32 bvx = 0x80000000 >> (val % 32);		// new, msb-like
	return (mMask[idx] & bvx) == bvx;
}
const uint16 BM128ItemSet::GetLastItem() const {
	return mLastItem;
}
void BM128ItemSet::FillHerUp(uint16 max) {
	mMask[0] = 0xFFFFFFFF << (32 - (min(max,32) % 32));
	mMask[1] = (max > 32) ? 0xFFFFFFFF << (32 - (min(max-32,32) % 32)) : 0x00000000;
	mMask[2] = (max > 64) ? 0xFFFFFFFF << (32 - (min(max-64,32) % 32)) : 0x00000000;
	mMask[3] = (max > 96) ? 0xFFFFFFFF << (32 - (min(max-96,32) % 32)) : 0x00000000;
	mLastItem = max-1;
	mBitCount = max;
	mBitCountFresh = true;
}
void BM128ItemSet::CleanTheSlate() {
	mMask[0] = 0;
	mMask[1] = 0;
	mMask[2] = 0;
	mMask[3] = 0;
	mLastItem = UINT16_MAX_VALUE;
	mBitCount = 0;
	mBitCountFresh = true;
}
int8 BM128ItemSet::CmpLexicographically(ItemSet *is)  {
	// Hier iets veel snellers voor verzinnen.
	BM128ItemSet *bis = (BM128ItemSet*) is;

	//CalcLength();

/*	int8 o = 0;
	for(uint32 i=0; i<minLen; i++) {
		if(set1[i] < set2[i]) {
			o = -1;
			break;
		} else if(set1[i] > set2[i]) {
			o = 1;
			break;
		}
	}*/

	int8 p = 0;
	if(mMask[0] > bis->mMask[0]) {
		return -1;
	} else if(mMask[0] < bis->mMask[0])
		return 1;
	else if(mMask[1] > bis->mMask[1]) {
		return -1;
	} else if(mMask[1] < bis->mMask[1])
		return 1;
	else if(mMask[2] > bis->mMask[2]) {
		return -1;
	} else if(mMask[2] < bis->mMask[2])
		return 1;
	else if(mMask[3] > bis->mMask[3]) {
		return -1;
	} else if(mMask[3] < bis->mMask[3])
		return 1;

	uint32 isLen = is->GetLength();
	if(mBitCount == isLen)
		return 0;
	return mBitCount < isLen ? -1 : 1;
}

bool BM128ItemSet::Equals(ItemSet *is) const {
    return this->Equals(((BM128ItemSet*)is)->mMask);
}
bool BM128ItemSet::Equals(uint32 *msk) const {
	if(mMask[0] != msk[0])
		return false;
	else if(mMask[1] != msk[1])
		return false;
	else if(mMask[2] != msk[2])
		return false;
	else if(mMask[3] != msk[3])
		return false;
	return true;
}

bool BM128ItemSet::Intersects(ItemSet *is) const {
	return Intersects(((BM128ItemSet*)is)->mMask);
}
bool BM128ItemSet::Intersects(uint32 *msk) const {
	if((mMask[0] & msk[0]) != 0)
		return 1;
	else if((mMask[1] & msk[1]) != 0)
		return 1;
	else if((mMask[2] & msk[2]) != 0)
		return 1;
	else if((mMask[3] & msk[3]) != 0)
		return 1;
	else 
		return 0;
}
uint32 BM128ItemSet::IntersectionLength(uint32 *msk) const {
	uint32 intlen = 0;
	uint32 tmpint = 0;
	if((tmpint = (mMask[0] & msk[0])) != 0) {
		intlen += _count[(unsigned char)(tmpint)] + _count[(unsigned char)((tmpint) >> 8)] + _count[(unsigned char)((tmpint) >> 16)] + _count[(unsigned char)((tmpint) >> 24)];
	} if((tmpint = (mMask[1] & msk[1])) != 0) {
		intlen += _count[(unsigned char)(tmpint)] + _count[(unsigned char)((tmpint) >> 8)] + _count[(unsigned char)((tmpint) >> 16)] + _count[(unsigned char)((tmpint) >> 24)];
	} if((tmpint = (mMask[2] & msk[2])) != 0) {
		intlen += _count[(unsigned char)(tmpint)] + _count[(unsigned char)((tmpint) >> 8)] + _count[(unsigned char)((tmpint) >> 16)] + _count[(unsigned char)((tmpint) >> 24)];
	} if((tmpint = (mMask[3] & msk[3])) != 0) {
		intlen += _count[(unsigned char)(tmpint)] + _count[(unsigned char)((tmpint) >> 8)] + _count[(unsigned char)((tmpint) >> 16)] + _count[(unsigned char)((tmpint) >> 24)];
	}
	return intlen;
}


bool BM128ItemSet::IsSubset(ItemSet *is) const {
	return IsSubset(((BM128ItemSet*)is)->mMask);
}
bool BM128ItemSet::IsSubset(uint32 *msk) const {
	if((~mMask[0]) & msk[0])
		return 0;
	else if((~mMask[1]) & msk[1])
		return 0;
	else if((~mMask[2]) & msk[2])
		return 0;
	else if((~mMask[3]) & msk[3])
		return 0;
	return 1;
}

bool BM128ItemSet::IsSubSubSet(uint32 *attr, uint32 *msk) const {
	// (attr & mMask) == msk
	uint32 t = (attr[0] & mMask[0]);
	if(t != msk[0])
		return 0;
	t = (attr[1] & mMask[1]);
	if(t != msk[1])
		return 0;
	t = (attr[2] & mMask[2]);
	if(t != msk[2])
		return 0;
	t = (attr[3] & mMask[3]);
	if(t != msk[3])
		return 0;
	return 1;
}


// Subtracts b from a
void BM128ItemSet::Remove(ItemSet *is) {
	BM128ItemSet::Remove(((BM128ItemSet*)is)->mMask);
}
void BM128ItemSet::Remove(uint32 *msk) {
	//_mm_store_si128(( __m128i*)mMask,_mm_xor_si128(_mm_load_si128((const __m128i*)mMask), _mm_load_si128((const __m128i*)msk)));
	mMask[0] = mMask[0] & (mMask[0] ^ msk[0]);
	mMask[1] = mMask[1] & (mMask[1] ^ msk[1]);
	mMask[2] = mMask[2] & (mMask[2] ^ msk[2]);
	mMask[3] = mMask[3] & (mMask[3] ^ msk[3]);
	
	//_mm_store_si128((__m128i*)mMask, _mm_and_si128(_mm_load_si128((const __m128i*)mMask), _mm_xor_si128(_mm_load_si128((const __m128i*)mMask),_mm_load_si128((const __m128i*)msk))));
	mBitCountFresh = false;
	CalcLength();
}

void BM128ItemSet::RemoveSubset(ItemSet *is) {
	BM128ItemSet::RemoveSubset(((BM128ItemSet*)is)->mMask, ((BM128ItemSet*)is)->mBitCount);
}
void BM128ItemSet::RemoveSubset(uint32 *msk, uint32 subsetlen) {
	//_mm_store_si128(( __m128i*)mMask,_mm_xor_si128(_mm_load_si128((const __m128i*)mMask), _mm_load_si128((const __m128i*)msk)));
#ifdef WIN32
#ifndef WIN64	// -- WIN32 && !WIN64 = x86
	_asm {
	mov         eax,dword ptr [msk];		
	movdqa      xmm0,xmmword ptr [eax];		// load msk
	mov         ecx,dword ptr [this];
	mov         ecx,dword ptr [ecx+mMask];	
	movdqa      xmm1,xmmword ptr [ecx];		// load mMask
	pxor        xmm1,xmm0;					// keep stuff not in msk
	movdqa      xmmword ptr [ecx],xmm1;
	}
#else			// -- WIN32 && WIN64 = x64
	mMask[0] = mMask[0] & (mMask[0] ^ msk[0]);
	mMask[1] = mMask[1] & (mMask[1] ^ msk[1]);
	mMask[2] = mMask[2] & (mMask[2] ^ msk[2]);
	mMask[3] = mMask[3] & (mMask[3] ^ msk[3]);
#endif
#else			// -- !WIN
	mMask[0] = mMask[0] & (mMask[0] ^ msk[0]);
	mMask[1] = mMask[1] & (mMask[1] ^ msk[1]);
	mMask[2] = mMask[2] & (mMask[2] ^ msk[2]);
	mMask[3] = mMask[3] & (mMask[3] ^ msk[3]);
#endif
	mBitCount -= subsetlen;
}
void BM128ItemSet::UniteNonOverlappingSet(uint32 *msk, uint32 noverlaplen) {
#ifdef WIN32
#ifndef WIN64	// -- WIN32 && !WIN64 = x86
	_asm{
		mov         eax,dword ptr [msk];		
		movdqa      xmm0,xmmword ptr [eax];		// load msk
		mov         ecx,dword ptr [this];
		mov         ecx,dword ptr [ecx+mMask];	
		movdqa      xmm1,xmmword ptr [ecx];		// load mMask
		por			xmm1,xmm0;					// keep stuff in either
		movdqa      xmmword ptr [ecx],xmm1;
	}
#else			// -- WIN32 && WIN64 = x64
	mMask[0] |= msk[0];
	mMask[1] |= msk[1];
	mMask[2] |= msk[2];
	mMask[3] |= msk[3];
#endif
#else			// -- !WIN
	mMask[0] |= msk[0];
	mMask[1] |= msk[1];
	mMask[2] |= msk[2];
	mMask[3] |= msk[3];
#endif
	mBitCount += noverlaplen;
}

ItemSet* BM128ItemSet::Union(ItemSet *is) const {
	ItemSet *merged = this->Clone();
	uint32 *msk1 = ((BM128ItemSet*)(merged))->mMask;
	uint32 *msk2 = ((BM128ItemSet*)(is))->mMask;
	msk1[0] |= msk2[0];
	msk1[1] |= msk2[1];
	msk1[2] |= msk2[2];
	msk1[3] |= msk2[3];
	((BM128ItemSet*)(merged))->mBitCountFresh = false;
	((BM128ItemSet*)(merged))->CalcLength();
	if (((BM128ItemSet*)(merged))->mLastItem != UINT16_MAX_VALUE)
		((BM128ItemSet*)(merged))->mLastItem = max(((BM128ItemSet*)(merged))->mLastItem, ((BM128ItemSet*)is)->mLastItem);
	else
		((BM128ItemSet*)(merged))->mLastItem = ((BM128ItemSet*)is)->mLastItem;
	ItemSet::UpdateDBOccurrencesAfterUnion(this, is, merged);
	return merged;
}
void BM128ItemSet::Unite(ItemSet *is) {
	uint32 *msk = ((BM128ItemSet*)(is))->mMask;
	mMask[0] |= msk[0];
	mMask[1] |= msk[1];
	mMask[2] |= msk[2];
	mMask[3] |= msk[3];
	if (mLastItem != UINT16_MAX_VALUE)
		mLastItem = max(mLastItem, ((BM128ItemSet*)is)->mLastItem);
	else
		mLastItem = ((BM128ItemSet*)is)->mLastItem;
	mBitCountFresh = false;
	CalcLength();
}
ItemSet* BM128ItemSet::Intersection(ItemSet *is) const {
	uint32 *msk = ((BM128ItemSet*) is)->mMask;
	uint32 ilen = IntersectionLength(msk);

	BM128ItemSet *aux = new BM128ItemSet();
	aux->mMask[0] = mMask[0] & msk[0];
	aux->mMask[1] = mMask[1] & msk[1];
	aux->mMask[2] = mMask[2] & msk[2];
	aux->mMask[3] = mMask[3] & msk[3];
	aux->mBitCount = ilen;
	aux->mBitCountFresh = true;
	return aux;
}
void BM128ItemSet::XOR(ItemSet *is) {
	uint32 *msk = ((BM128ItemSet*)(is))->mMask;
	for(uint32 d=0; d<4; d++) {
        mMask[d] ^= msk[d];
	}
	mBitCountFresh = false;
	CalcLength();
}
void BM128ItemSet::Difference(ItemSet *is) {
	uint32 *msk = ((BM128ItemSet*)(is))->mMask;
	uint32 t;
	for(uint32 d=0; d<4; d++) {
		t = mMask[d];
		mMask[d] = mMask[d] & (mMask[d] ^ msk[d]);
		msk[d] = msk[d] & (msk[d] ^ t);
	}
	((BM128ItemSet*)(is))->mBitCountFresh = false;
	((BM128ItemSet*)(is))->CalcLength();

	mBitCountFresh = false;
	CalcLength();
}

void BM128ItemSet::GetValuesIn(uint32* vals) const {
	// 8bit lookup, 110s
	// old Stylee, lsb
	/*
	const uint32 *subs;
	uint32 num, cD, adjust;
	uint32 idx = 0;
	uint8 bte = 0;
	for(uint32 d=0; d<4; d++) {
		cD = d * 32;
		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];

		for(uint32 i=0; i<num; i++)
			vals[idx++] = cD + (subs[i] - 1);

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		adjust = cD + 7;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = cD + (subs[i] + 7);

		bte = (unsigned char)(mMask[d] >> 16);
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 8;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = adjust + subs[i];

		bte = (unsigned char)(mMask[d] >> 24);
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 8;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = adjust + (subs[i]);
	}
	*/
	// New, msb-stylee
	const uint32 *subs;
	uint32 num, offset, adjust;
	uint32 idx = 0;
	uint8 bte = 0;
	for(uint32 d=0; d<4; d++) {
		offset = d * 32;
		adjust = offset;

		bte = (unsigned char)(mMask[d] >> 24);
		num = _count[bte];		// 1 et al staan nu helemaal links
		subs = _idxon[bte];
		// adjust += 0;			// de -1 kan ik niet zomaar verwerken
		for(uint32 i=0; i<num; i++)
			vals[idx++] = offset + (subs[i] - 1);

		bte = (unsigned char)(mMask[d] >> 16);
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 7;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = adjust + subs[i];

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 8;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = adjust + subs[i];

		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 8;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = adjust + subs[i];
	}
}
void BM128ItemSet::GetValuesIn(uint16* vals) const {
	// 8bit lookup
	// New, msb-stylee
	const uint32 *subs;
	uint32 num, offset, adjust;
	uint32 idx = 0;
	uint8 bte = 0;
	for(uint32 d=0; d<4; d++) {
		offset = d * 32;
		adjust = offset;

		bte = (unsigned char)(mMask[d] >> 24);
		num = _count[bte];		// 1 et al staan nu helemaal links
		subs = _idxon[bte];
		// adjust += 0;			// de -1 kan ik niet zomaar verwerken
		for(uint32 i=0; i<num; i++)
			vals[idx++] = offset + (subs[i] - 1);

		bte = (unsigned char)(mMask[d] >> 16);
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 7;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = adjust + subs[i];

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 8;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = adjust + subs[i];

		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 8;
		for(uint32 i=0; i<num; i++)
			vals[idx++] = adjust + subs[i];
	}
}

uint16* BM128ItemSet::GetValues() const {
	/*
	// Old, lsb-stylee
	uint16 *vals = new uint16[mBitCount];
	const uint32 *subs;
	uint32 num;
	uint32 idx = 0;
	uint8 bte = 0;
	for(uint32 d=0; d<4; d++) {
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
	*/
	// New, msb-stylee
	uint16 *vals = new uint16[mBitCount];
	const uint32 *subs;
	uint32 num;
	uint32 idx = 0;
	uint8 bte = 0;
	uint32 offset = 0;
	for(uint32 d=0; d<4; d++) {
		offset = d * 32;
		bte = (unsigned char)(mMask[d] >> 24);
		num = _count[bte];		// 1 et al staan nu helemaal links
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = offset + (subs[i] - 1);

		bte = (unsigned char)(mMask[d] >> 16);
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = offset + (subs[i] + 7);

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = offset + (subs[i] + 15);

		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];
		for(uint32 i=0; i<num; i++)
			vals[idx++] = offset + (subs[i] + 23);
	}
	return vals;
}
void BM128ItemSet::IncrementValues(uint32* vals, int32 cnt, uint32* items) const {
	// New, msb-stylee
	const uint32 *subs;
	uint32 num, offset, adjust;
	uint32 idx = 0;
	uint8 bte = 0;
	uint32 counter = 0;
	for(uint32 d=0; d<4; d++) {
		offset = d * 32;
		adjust = offset;

		bte = (unsigned char)(mMask[d] >> 24);
		num = _count[bte];		// 1 et al staan nu helemaal links
		subs = _idxon[bte];
		// adjust += 0;			// de -1 kan ik niet zomaar verwerken
		for(uint32 i=0; i<num; i++) {
			vals[offset + (subs[i] - 1)] += cnt;
			if (items) items[counter++] = offset + (subs[i] - 1);
		}

		bte = (unsigned char)(mMask[d] >> 16);
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 7;
		for(uint32 i=0; i<num; i++) {
			vals[adjust + subs[i]] += cnt;
			if (items) items[counter++] = adjust + subs[i];
		}

		bte = (unsigned char)(mMask[d] >> 8);
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 8;
		for(uint32 i=0; i<num; i++) {
			vals[adjust + subs[i]] += cnt;
			if (items) items[counter++] = adjust + subs[i];
		}

		bte = (unsigned char)mMask[d];
		num = _count[bte];
		subs = _idxon[bte];
		adjust += 8;
		for(uint32 i=0; i<num; i++) {
			vals[adjust + subs[i]] += cnt;
			if (items) items[counter++] = adjust + subs[i];
		}
	}
}
void BM128ItemSet::TranslateBackward(ItemTranslator *it) {

	throw string("Not implemented");
}
void BM128ItemSet::TranslateForward(ItemTranslator *it) {
	GetValuesIn(sValueDumper);								// Get the values, int-stylee
	memset(mMask, 0, sizeof(uint32)*4);						// Erase the current mask
	for(uint32 i=0; i<mBitCount; i++)
		Set(it->ValueToBit(sValueDumper[i]));
}

string BM128ItemSet::ToString(bool printCount, bool printFreq) {
	string s = "";
	char buffer[100];

	uint32 len = GetLength();
	size_t blen = 0;
	BM128ItemSet::GetValuesIn((uint16*)sValueDumper);
	uint16 *set = (uint16*) sValueDumper;
	if(len > 0) {
		for(uint32 i=0; i<len-1; i++) {
			_itoa_s(set[i], buffer, 20, 10);
			blen = strlen(buffer);
			buffer[blen] = ' ';
			buffer[blen+1] = 0x00;
			s.append(buffer);
		} 
		_itoa_s(set[len-1], buffer, 20, 10);
		s.append(buffer);
	}
	if(printCount || printFreq) {
		buffer[0] = ' ';
		buffer[1] = '(';
		if(printCount && !printFreq) {
			_itoa_s(mUsageCount, buffer+2, 20, 10);
		} else if(!printCount && printFreq) {
			_itoa_s(mSupport, buffer+2, 20, 10);
		} else {
			_itoa_s(mUsageCount, buffer+2, 20, 10);
			blen = strlen(buffer);
			buffer[blen] = ',';
			_itoa_s(mSupport, buffer+blen+1, 20, 10);
		}
		blen = strlen(buffer);
		buffer[blen] = ')';
		buffer[blen+1] = 0x00;
		s.append(buffer);
	}
	return s;
}
string BM128ItemSet::ToCodeTableString() {
	string s = "";
	char *buffer = new char[100];

	uint32 len = GetLength();
	uint16 *set = BM128ItemSet::GetValues(); 
	for(uint32 i=0; i<len; i++) {
		sprintf_s(buffer, 100, "%d ", set[i]);
		s.append(buffer);
	}
	// 
	sprintf_s(buffer, 100, "(%d,%d) ", mUsageCount, mSupport);
	s.append(buffer);
	delete[] set;
	delete[] buffer;
	return s;
}

const unsigned char BM128ItemSet::_count[256] = {
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
		1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
		1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
		2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
		1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
		2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
		2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
		3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

// old, lsb-like 
/*
const uint32 BM128ItemSet::_idxon[256][8] = { 
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
*/

// new, msb-like 
const uint32 BM128ItemSet::_idxon[256][8] = { 
{},{8},{7},{7,8},{6},{6,8},{6,7},{6,7,8},{5},{5,8},{5,7},{5,7,8},{5,6},{5,6,8},{5,6,7},{5,6,7,8},{4},{4,8},{4,7},{4,7,8},{4,6},{4,6,8},{4,6,7},{4,6,7,8},{4,5},{4,5,8},{4,5,7},{4,5,7,8},{4,5,6},{4,5,6,8},{4,5,6,7},{4,5,6,7,8},{3},{3,8},{3,7},{3,7,8},{3,6},{3,6,8},{3,6,7},{3,6,7,8},{3,5},{3,5,8},{3,5,7},{3,5,7,8},{3,5,6},{3,5,6,8},{3,5,6,7},{3,5,6,7,8},{3,4},{3,4,8},{3,4,7},{3,4,7,8},{3,4,6},{3,4,6,8},{3,4,6,7},{3,4,6,7,8},{3,4,5},{3,4,5,8},{3,4,5,7},{3,4,5,7,8},{3,4,5,6},{3,4,5,6,8},{3,4,5,6,7},{3,4,5,6,7,8},{2},{2,8},{2,7},{2,7,8},{2,6},{2,6,8},{2,6,7},{2,6,7,8},{2,5},{2,5,8},{2,5,7},{2,5,7,8},{2,5,6},{2,5,6,8},{2,5,6,7},{2,5,6,7,8},{2,4},{2,4,8},{2,4,7},{2,4,7,8},{2,4,6},{2,4,6,8},{2,4,6,7},{2,4,6,7,8},{2,4,5},{2,4,5,8},{2,4,5,7},{2,4,5,7,8},{2,4,5,6},{2,4,5,6,8},{2,4,5,6,7},{2,4,5,6,7,8},{2,3},{2,3,8},{2,3,7},{2,3,7,8},{2,3,6},{2,3,6,8},{2,3,6,7},{2,3,6,7,8},{2,3,5},{2,3,5,8},{2,3,5,7},{2,3,5,7,8},{2,3,5,6},{2,3,5,6,8},{2,3,5,6,7},{2,3,5,6,7,8},{2,3,4},{2,3,4,8},{2,3,4,7},{2,3,4,7,8},{2,3,4,6},{2,3,4,6,8},{2,3,4,6,7},{2,3,4,6,7,8},{2,3,4,5},{2,3,4,5,8},{2,3,4,5,7},{2,3,4,5,7,8},{2,3,4,5,6},{2,3,4,5,6,8},{2,3,4,5,6,7},{2,3,4,5,6,7,8},{1},{1,8},{1,7},{1,7,8},{1,6},{1,6,8},{1,6,7},{1,6,7,8},{1,5},{1,5,8},{1,5,7},{1,5,7,8},{1,5,6},{1,5,6,8},{1,5,6,7},{1,5,6,7,8},{1,4},{1,4,8},{1,4,7},{1,4,7,8},{1,4,6},{1,4,6,8},{1,4,6,7},{1,4,6,7,8},{1,4,5},{1,4,5,8},{1,4,5,7},{1,4,5,7,8},{1,4,5,6},{1,4,5,6,8},{1,4,5,6,7},{1,4,5,6,7,8},{1,3},{1,3,8},{1,3,7},{1,3,7,8},{1,3,6},{1,3,6,8},{1,3,6,7},{1,3,6,7,8},{1,3,5},{1,3,5,8},{1,3,5,7},{1,3,5,7,8},{1,3,5,6},{1,3,5,6,8},{1,3,5,6,7},{1,3,5,6,7,8},{1,3,4},{1,3,4,8},{1,3,4,7},{1,3,4,7,8},{1,3,4,6},{1,3,4,6,8},{1,3,4,6,7},{1,3,4,6,7,8},{1,3,4,5},{1,3,4,5,8},{1,3,4,5,7},{1,3,4,5,7,8},{1,3,4,5,6},{1,3,4,5,6,8},{1,3,4,5,6,7},{1,3,4,5,6,7,8},{1,2},{1,2,8},{1,2,7},{1,2,7,8},{1,2,6},{1,2,6,8},{1,2,6,7},{1,2,6,7,8},{1,2,5},{1,2,5,8},{1,2,5,7},{1,2,5,7,8},{1,2,5,6},{1,2,5,6,8},{1,2,5,6,7},{1,2,5,6,7,8},{1,2,4},{1,2,4,8},{1,2,4,7},{1,2,4,7,8},{1,2,4,6},{1,2,4,6,8},{1,2,4,6,7},{1,2,4,6,7,8},{1,2,4,5},{1,2,4,5,8},{1,2,4,5,7},{1,2,4,5,7,8},{1,2,4,5,6},{1,2,4,5,6,8},{1,2,4,5,6,7},{1,2,4,5,6,7,8},{1,2,3},{1,2,3,8},{1,2,3,7},{1,2,3,7,8},{1,2,3,6},{1,2,3,6,8},{1,2,3,6,7},{1,2,3,6,7,8},{1,2,3,5},{1,2,3,5,8},{1,2,3,5,7},{1,2,3,5,7,8},{1,2,3,5,6},{1,2,3,5,6,8},{1,2,3,5,6,7},{1,2,3,5,6,7,8},{1,2,3,4},{1,2,3,4,8},{1,2,3,4,7},{1,2,3,4,7,8},{1,2,3,4,6},{1,2,3,4,6,8},{1,2,3,4,6,7},{1,2,3,4,6,7,8},{1,2,3,4,5},{1,2,3,4,5,8},{1,2,3,4,5,7},{1,2,3,4,5,7,8},{1,2,3,4,5,6},{1,2,3,4,5,6,8},{1,2,3,4,5,6,7},{1,2,3,4,5,6,7,8}
};

