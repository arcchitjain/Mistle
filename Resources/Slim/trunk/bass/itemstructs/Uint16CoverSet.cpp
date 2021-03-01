
#include "../Bass.h"

#include "Uint16ItemSet.h"

#include "CoverSet.h"
#include "Uint16CoverSet.h"


Uint16CoverSet::Uint16CoverSet(ItemSet *initwith, uint32 numIPs) {
	mUint16ItemSet = (Uint16ItemSet*) initwith;
	if(initwith != NULL) {
		mMaxLen = 0;
		mBackupMask = NULL;
		mMask = NULL;
		mOnes = NULL;
		InitSet(initwith);
		//mMask->SetDBOccurrence(NULL, 0);
	} else {
		mMaxLen = 2048;
		mMaskLen = 2048;
		mNumUncovered = 2048;
		mMask = new uint32[2048];
		mBackupMask = new uint32[2048];
		mOnes = new uint32[2048];
		for(uint32 i=0; i<mMaxLen; i++)
			mOnes[i] = 1;
	}
}

Uint16CoverSet::Uint16CoverSet(const Uint16CoverSet &cset) {
	mUint16ItemSet = cset.mUint16ItemSet;
	mSet = cset.mSet;

	mMaskLen = cset.mMaskLen;
	mMaxLen = cset.mMaxLen;
	mMask = new uint32[mMaxLen];
	memcpy(mMask, cset.mMask, mMaskLen * sizeof(uint32));

	mBackupMask = new uint32[mMaxLen];
	memcpy(mBackupMask, cset.mBackupMask, mMaskLen * sizeof(uint32));

	mOnes = new uint32[mMaxLen];
	memcpy(mOnes, cset.mOnes, mMaskLen * sizeof(uint32));

	mBackupMask = new uint32[mMaxLen];
	memcpy(mBackupMask, cset.mBackupMask, mMaskLen * sizeof(uint32));

	mNumUncovered = cset.mNumUncovered;
	mBackupNumUncovered = cset.mBackupNumUncovered;
}

Uint16CoverSet::~Uint16CoverSet() {
	delete[] mMask;
	delete[] mBackupMask;
	delete[] mOnes;
	mSet = NULL;				// not ours, so dont touch
	mUint16ItemSet = NULL;		// not ours, so dont touch
}

CoverSet* Uint16CoverSet::Clone() {
	return new Uint16CoverSet(*this);
}

/* Initializes this coverset for the argument itemset */
void Uint16CoverSet::InitSet(ItemSet* iset) {
	// we gaan uit van een Uint16ItemSet, of alike
	Uint16ItemSet *uset = (Uint16ItemSet*) iset;
	mSet = uset->mSet;
	mMaskLen = uset->mLength;
	if(mMaskLen > mMaxLen) {
		delete[] mMask;
		delete[] mBackupMask;
		delete[] mOnes;
		mMaxLen = mMaskLen;
		mMask = new uint32[mMaxLen];
		mBackupMask = new uint32[mMaxLen];
		mOnes = new uint32[mMaxLen];
		for(uint32 i=0; i<mMaxLen; i++)
			mOnes[i] = 1;
	}

	mUint16ItemSet = uset;

	memcpy(mMask, mOnes, mMaskLen * sizeof(uint32));
	mNumUncovered = mMaskLen;
}
void Uint16CoverSet::ResetMask() {
	memcpy(mMask, mOnes, mMaskLen * sizeof(uint32));
	mNumUncovered = mMaskLen;
}
void Uint16CoverSet::BackupMask() {
	memcpy(mBackupMask, mMask, mMaskLen * sizeof(uint32));
	mBackupNumUncovered = mNumUncovered;
}
void Uint16CoverSet::RestoreMask() {
	mNumUncovered = mBackupNumUncovered;
	memcpy(mMask, mBackupMask, mMaskLen * sizeof(uint32));
}

uint32 Uint16CoverSet::LoadMask(uint32 ipoint) {
	memcpy(mMask, mOnes, mMaskLen * sizeof(uint32));
	mNumUncovered = mMaskLen;
	return 0;
}
void Uint16CoverSet::StoreMask(uint32 ipoint) {
}

bool Uint16CoverSet::CanCoverIPoint(uint32 ipoint, ItemSet *ctElem) {
	return false;
}
void Uint16CoverSet::MakeIPointUpToDate(uint32 ipoint, ItemSet* ctElem, uint32 len) {
}
void Uint16CoverSet::SetIPointUpToDate(uint32 ipoint) {
}
uint32 Uint16CoverSet::GetMostUpToDateIPoint(uint32 ipoint) {
	return 0;
}

/*	Tries to cover the current CoverSet with the code table element.
	Adjusts the mask if appropriate, returns the number of cover 
	matches for this element. (normally either 0 or 1, but possibly 
	higher if this coverSet has a higher occurrence)
*/
uint32 Uint16CoverSet::Cover(uint32 setlen, ItemSet *ctElem) {
	if(setlen > mNumUncovered)
		return 0;

	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;

	// Mask-based IsSubset
	uint32 c = 0, m;
	for(m=0; m<mMaskLen; m++) {
		if(mMask[m]) {
			if(set[c] == mSet[m]) {
				if(++c >= setlen)
					break;
			} else if(set[c] < mSet[m])
				return 0;
		}
	}
	if(c != setlen)	// overbodig? check bij lange run!
		return 0;

	// Got through, so it's an uncovered subset 
	c = 0;
	for(m=0; m<mMaskLen; m++) {
		if(mMask[m] && mSet[m] == set[c]) {
			mMask[m] = 0;
			if(++c==setlen)
				break;
		}
	}
	mNumUncovered -= setlen;	// keeping things tait

	return 1;
}
void Uint16CoverSet::Uncover(uint32 setlen, ItemSet *ctElem) {
	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;

	// it's a covered subset 
	uint32 c = 0;
	for(uint32 m=0; m<mMaskLen; m++) {
		if(mSet[m] == set[c]) {
			mMask[m] = 1;
			if(++c==setlen)
				break;
		}
	}
	mNumUncovered += setlen;	// keeping things tait
}

bool Uint16CoverSet::CanCover(uint32 setlen, ItemSet *ctElem) {	
	if(setlen > mNumUncovered)
		return 0;

	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;

	// Mask-based IsSubset
	uint32 c = 0, m;
	for(m=0; m<mMaskLen; m++) {
		if(mMask[m]) {
			if(set[c] == mSet[m]) {
				if(++c >= setlen)
					break;
			} else if(set[c] < mSet[m])
				return false;
		}
	}
	if(c != setlen)	// overbodig? check bij lange run!
		return false;
	return true;
}
uint32 Uint16CoverSet::CoverWithOverlap(uint32 setlen, ItemSet *ctElem) {
	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;

	// Partially Mask-based IsSubset
	uint32 c = 0, uncovered = 0, m;
	for(m=0; m<mMaskLen; m++) {
		if(set[c] == mSet[m]) {
			if(mMask[m])
				uncovered++;
			if(++c >= setlen)
				break;
		} else if(set[c] < mSet[m])
			return 0;
	}
	if(c != setlen)	// overbodig? check bij lange run!
		return 0;
	if(uncovered == 0)
		return 0;

	// Got through, so it's a subset with an uncovered part
	c = 0;
	for(m=0; m<mMaskLen; m++) {
		if (mSet[m] == set[c]) {
			if(mMask[m])
				mMask[m] = 0;
			if(++c >= setlen)
				break;
		}
	}
	mNumUncovered -= uncovered;	// keeping things tait

	return 1;
}
bool Uint16CoverSet::CanCoverWithOverlap(uint32 setlen, ItemSet *ctElem) {	
	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;
	uint32 c = 0, uncovered = 0, m;
	for(m=0; m<mMaskLen; m++) {
		if(set[c] == mSet[m]) {
			if(mMask[m])
				uncovered++;
			if(++c >= setlen)
				break;
		} else if(set[c] < mSet[m]) {
			return false;
		}
	}
	if(c != setlen)	// overbodig? check bij lange run!
		return false;
	return (uncovered > 0);
}
// Assumes CanCoverWithOverlap(setlen,ctElem) => true 
uint32 Uint16CoverSet::NumCoverWithOverlap(uint32 setlen, ItemSet *ctElem) {	
	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;
	uint32 c = 0, uncovered = 0, m;
	for(m=0; m<mMaskLen; m++) {
		if(set[c] == mSet[m]) {
			if(mMask[m])
				uncovered++;
			if(++c >= setlen)
				break;
		}
	}
	return uncovered;
}

uint32 Uint16CoverSet::UnCoverWithOverlapNegAlph(uint32 setlen, ItemSet *ctElem, uint32 *negAlph) {
	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;

	// Partially Mask-based IsSubset
	uint32 c = 0, uncovered = 0, m;
	for(m=0; m<mMaskLen; m++) {
		if(set[c] == mSet[m]) {
			if(mMask[m])
				uncovered++;
			if(++c >= setlen)
				break;
		} else if(set[c] < mSet[m])
			return 0;
	}
	if(c != setlen)	// overbodig? check bij lange run!
		return 0;
	if(uncovered == 0)
		return 0;

	// Got through, so it's a subset with an uncovered part
	c = 0;
	for(m=0; m<mMaskLen; m++) {
		if(mSet[m] == set[c]) {
			if(mMask[m])
				mMask[m] = 0;
			else
				negAlph[mSet[m]]--;
			if(++c >= setlen)
				break;
		}
	}
	mNumUncovered -= uncovered;	// keeping things tait

	return 1;
}
uint32 Uint16CoverSet::CoverWithOverlapNegAlph(uint32 setlen, ItemSet *ctElem, uint32 *negAlph) {
	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;

	// Partially Mask-based IsSubset
	uint32 c = 0, uncovered = 0, m;
	for(m=0; m<mMaskLen; m++) {
		if(set[c] == mSet[m]) {
			if(mMask[m])
				uncovered++;
			if(++c >= setlen)
				break;
		} else if(set[c] < mSet[m])
			return 0;
	}
	if(c != setlen)	// overbodig? check bij lange run!
		return 0;
	if(uncovered == 0)
		return 0;

	// Got through, so it's a subset with an uncovered part
	c = 0;
	for(m=0; m<mMaskLen; m++) {
		if(mSet[m] == set[c]) {
			if(mMask[m])
				mMask[m] = 0;
			else
				negAlph[mSet[m]]++;
			if(++c >= setlen)
				break;
		}
	}
	mNumUncovered -= uncovered;	// keeping things tait

	return 1;
}

/* 
		No Subset check - only usable for algo's with occurrence-usage 
*/

uint32 Uint16CoverSet::CoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem) {
	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;
	uint32 c = 0, uncovered = 0, m;

	for(m=0; m<mMaskLen; m++) {
		if (mSet[m] == set[c]) {
			if(mMask[m]) {
				mMask[m] = 0;
				uncovered++;
			}
			if(++c >= setlen)
				break;
		}
	}
	mNumUncovered -= uncovered;	// keeping things tait
	return uncovered > 0 ? 1 : 0;
}

// IsSubset of mMaskSet && Intersects with mMask
bool Uint16CoverSet::CanCoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem) {	
	uint16 *set = ((Uint16ItemSet*) ctElem)->mSet;
	uint32 c = 0, uncovered = 0, m;
	for(m=0; m<mMaskLen; m++) {
		if(set[c] == mSet[m]) {
			if(mMask[m])
				return true;
			if(++c >= setlen)
				break;
		} else if(set[c] < mSet[m])
			return false;
	}
	return false;
}



uint32 Uint16CoverSet::CoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values) {
	uint32 counter = 0;
	for(uint32 m=0; m<mMaskLen; ++m) {
		if(mMask[m]) {
			abet[mSet[m]]++;
			if (values) values[counter] = mSet[m];
			++counter;
			if(--mNumUncovered == 0)
				break;
		}
	}
	return counter;
}
uint32 Uint16CoverSet::CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values) {
	uint32 counter = 0;
	for(uint32 m=0; m<mMaskLen; ++m) {
		if(mMask[m]) {
			abet[mSet[m]] += cnt;
			if (values) values[counter] = mSet[m];
			++counter;
			if(--mNumUncovered == 0)
				break;
		}
	}
	return counter * cnt;
}
uint32 Uint16CoverSet::UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values) {
	uint32 counter = 0;
	for(uint32 m=0; m<mMaskLen; m++) {
		if(mMask[m]) {
			abet[mSet[m]]--;
			if (values) values[counter] = mSet[m];
			counter++;
			if(--mNumUncovered == 0)
				break;
		}
	}
	return counter;
}
uint32 Uint16CoverSet::UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values) {
	uint32 counter = 0;
	for(uint32 m=0; m<mMaskLen; m++) {
		if(mMask[m]) {
			abet[mSet[m]] -= cnt;
			if (values) values[counter] = mSet[m];
			++counter;
			if(--mNumUncovered == 0)
				break;
		}
	}
	return counter * cnt;
}


bool Uint16CoverSet::IsItemUncovered(uint32 value) {
	bool res = false;
	for(uint32 m=0; m<mMaskLen; m++)
		if(mMask[m] && mSet[m] == value) {
			res = true;
			break;
		}
	return res;
}

void Uint16CoverSet::SetItem(uint32 i) {
	for(uint32 m=0; m<mMaskLen; m++) {
		if(!mMask[m] && mSet[m] == i) {
			mMask[m] = true;
			break;
		}
	}
	mNumUncovered++;
}
void Uint16CoverSet::UnSetItem(uint32 i) {
	for(uint32 m=0; m<mMaskLen; m++) {
		if(mMask[m] && mSet[m] == i) {
			mMask[m] = false;
			break;
		}
	}
	mNumUncovered--;
}
