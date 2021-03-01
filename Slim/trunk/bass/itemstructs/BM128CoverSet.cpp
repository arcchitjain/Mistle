#include "../Bass.h"
#include <emmintrin.h>

#include "BM128ItemSet.h"

#include "CoverSet.h"
#include "BM128CoverSet.h"

BM128CoverSet::BM128CoverSet(uint32 abLen, uint32 numIPs) {
	mNumIPMasks = numIPs+1;						// +1 want dbRow is mIPMasks[0]
	mIPMasks = new BM128ItemSet*[mNumIPMasks];

	mMask = new BM128ItemSet();
	mNumUncovered = 0;
	mIPMasks[0] = NULL;

	for(uint32 i=1; i<mNumIPMasks; i++)
		mIPMasks[i] = (BM128ItemSet*)mMask->Clone();
	mIPMasksUpToDate = mNumIPMasks-1;

	mBackupNumUncovered = mNumUncovered;
	mBackupMask = (BM128ItemSet*)mMask->Clone();
}
BM128CoverSet::BM128CoverSet(uint32 abLen, ItemSet *initwith, uint32 numIPs) {
	//mNumIPMasks = min(((BM128ItemSet*)initwith)->mBitCount, numIPs)+1;						// +1 want dbRow is mIPMasks[0]
	mNumIPMasks = numIPs+1;						// +1 want dbRow is mIPMasks[0]
	mIPMasks = new BM128ItemSet*[mNumIPMasks];

	if(initwith != NULL) {
		mMask = (BM128ItemSet*)(initwith->CloneLight());
		mNumUncovered = ((BM128ItemSet*)initwith)->mBitCount;
		mIPMasks[0] = (BM128ItemSet*)initwith;		// db rij, owned by Database
	} else {
		mMask = new BM128ItemSet();
		mNumUncovered = 0;
		mIPMasks[0] = NULL;
	}

	// JV-idea: create IPMasks lazily, we'll likely wont need all of them anyway...
	for(uint32 i=1; i<mNumIPMasks; i++)
		mIPMasks[i] = (BM128ItemSet*)mMask->Clone();
	mIPMasksUpToDate = mNumIPMasks-1;

	mBackupNumUncovered = mNumUncovered;
	mBackupMask = (BM128ItemSet*)mMask->Clone();
}

BM128CoverSet::BM128CoverSet(BM128CoverSet *cset) {
	mNumIPMasks = cset->mNumIPMasks;
	BM128ItemSet **ipmasks = cset->mIPMasks;

	mIPMasks = new BM128ItemSet*[mNumIPMasks];	
	mIPMasks[0] = cset->mIPMasks[0];			// db rij, owned by Database
	for(uint32 i=1; i<mNumIPMasks; i++)
		mIPMasks[i] = (BM128ItemSet*)(ipmasks[i]->Clone());
	mIPMasksUpToDate = cset->mIPMasksUpToDate;

	mMask = (BM128ItemSet*)(cset->mMask->Clone());
	mNumUncovered = mMask->mBitCount;

	mBackupNumUncovered = mNumUncovered;
	mBackupMask = (BM128ItemSet*)mMask->Clone();
}

BM128CoverSet::~BM128CoverSet() {
	for(uint32 i=1; i<mNumIPMasks; i++)		// skip 0, want dat is de db rij!
		delete mIPMasks[i];
	delete[] mIPMasks;

	delete mMask;
	delete mBackupMask;
}

CoverSet* BM128CoverSet::Clone() {
	return new BM128CoverSet(this);
}

void BM128CoverSet::InitSet(ItemSet* dbRow) {
	mMask->CopyFrom((BM128ItemSet*) dbRow);
	mNumUncovered = ((BM128ItemSet*)dbRow)->mBitCount;

	mIPMasks[0] = (BM128ItemSet*)dbRow;
	mIPMasksUpToDate = 0;
}
void BM128CoverSet::ResetMask() {
	mMask->CopyFrom(mIPMasks[0]);
	mNumUncovered = mMask->GetLength();
}
void BM128CoverSet::BackupMask() {
	mBackupMask->CopyFrom(mMask);
	mBackupNumUncovered = mNumUncovered;
}
void BM128CoverSet::RestoreMask() {
	//BM128ItemSet *temp = (BM128ItemSet*) mMask->Clone();
	//uint32 tempNum = mNumUncovered;
	mMask->CopyFrom(mBackupMask);
	mNumUncovered = mBackupNumUncovered;
	//mBackupMask->CopyFrom(temp);
	//mBackupNumUncovered = tempNum;
	//delete temp;
}

/*
InsertionPoint nummering loopt van 0 voor het initiele mask, via 1 voor het eerste insertionpoint (bovenaan CT) tot n (onderaan CT)
*/
uint32 BM128CoverSet::LoadMask(uint32 ipoint) {
	if(ipoint > mIPMasksUpToDate)
		ipoint = mIPMasksUpToDate;
	mMask->CopyFrom(mIPMasks[ipoint]);
	mNumUncovered = mMask->GetLength();
	return ipoint;

	// check whether ipoint exists?
	// -> return best fit ipoint
	// :-> keep list-like format

	// always start with all masks?
	// -> mask always at the ready!
}
void BM128CoverSet::StoreMask(uint32 ipoint) {
	//_ASSERT(ipoint < mNumIPMasks);
	mIPMasksUpToDate = ipoint;	// we gaan er vanuit dat de ipoints voor deze ook al gestored zijn, of nog up-to-date
	mIPMasks[ipoint]->CopyFrom(mMask);
}

void BM128CoverSet::MakeIPointUpToDate(uint32 ipoint, ItemSet* ctElem, uint32 len) {
	if(mIPMasks[ipoint]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		mIPMasks[ipoint]->RemoveSubset(((BM128ItemSet*)ctElem)->mMask,len);
	}
	mIPMasksUpToDate = ipoint;
}
void BM128CoverSet::SetIPointUpToDate(uint32 ipoint) {
	mIPMasksUpToDate = ipoint;
}
uint32 BM128CoverSet::GetMostUpToDateIPoint(uint32 ipoint) {
	if(ipoint > mIPMasksUpToDate)
		ipoint = mIPMasksUpToDate;
	return ipoint;
}

uint32 BM128CoverSet::Cover(ItemSet *ctElem) {
	BM128ItemSet *iset = (BM128ItemSet*) ctElem;
	uint32 setlen = iset->mBitCount;
	if(setlen > mNumUncovered)
		return 0;

	if(mMask->IsSubset(iset->mMask)) {
		mMask->RemoveSubset(iset->mMask,setlen);
		mNumUncovered -= setlen;
		return 1;
	} else return 0;
}
/*	Tries to cover the current CoverSet with the code table element.
Adjusts the mask if appropriate, returns the number of cover 
matches for this element. (normally either 0 or 1, but possibly 
higher if this coverSet has a higher occurrence)
*/
uint32 BM128CoverSet::Cover(uint32 setlen, ItemSet *ctElem) {
	if(setlen > mNumUncovered)
		return 0;

	if(mMask->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		mMask->RemoveSubset(((BM128ItemSet*)ctElem)->mMask,setlen);
		mNumUncovered -= setlen;
		return 1;
	} else return 0;
}
void BM128CoverSet::Uncover(uint32 setlen, ItemSet *ctElem) {
	mMask->UniteNonOverlappingSet(((BM128ItemSet*)ctElem)->mMask,setlen);
	mNumUncovered += setlen;
}

bool BM128CoverSet::CanCover(uint32 setlen, ItemSet *ctElem) {
	if(setlen > mNumUncovered)
		return 0;
	return mMask->IsSubset(((BM128ItemSet*)ctElem)->mMask);
}
uint32 BM128CoverSet::NumCover(uint32 setlen, ItemSet *ctElem) {
	if(CanCover(setlen,ctElem))
		return setlen;
	else 
		return 0;
}

bool BM128CoverSet::CanCoverLE(ItemSet *attr, uint32 setlen, ItemSet *iset) {
	if(setlen > mNumUncovered)
		return 0;
	return mMask->IsSubSubSet(((BM128ItemSet*)attr)->mMask,((BM128ItemSet*)iset)->mMask);
}


bool BM128CoverSet::CanCoverIPoint(uint32 ipoint, ItemSet *ctElem) {
	return mIPMasks[ipoint]->IsSubset(((BM128ItemSet*)ctElem)->mMask);
}

uint32 BM128CoverSet::CoverWithOverlap(uint32 setlen, ItemSet *ctElem) {
	if(mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		mMask->Remove(((BM128ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}
bool BM128CoverSet::CanCoverWithOverlap(uint32 setlen, ItemSet *ctElem) {
	return mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask);
}
// we gaan er even blind vanuit dat ie uberhaupt in deze rij past.
uint32 BM128CoverSet::NumCoverWithOverlap(uint32 setlen, ItemSet *ctElem) {
	return mMask->IntersectionLength(((BM128ItemSet*)ctElem)->mMask);
}

uint32 BM128CoverSet::CoverWithOverlapNegAlph(uint32 setlen, ItemSet *ctElem, uint32 *negAlph) {
	if(mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		ItemSet *overlap = ctElem->Clone();
		overlap->Remove(mMask);
		uint32 len = overlap->GetLength();
		overlap->IncrementValues(negAlph, 1);
		delete overlap;
		mMask->Remove(((BM128ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}
uint32 BM128CoverSet::UnCoverWithOverlapNegAlph(uint32 setlen, ItemSet *ctElem, uint32 *negAlph) {
	if(mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		ItemSet *overlap = ctElem->Clone();
		overlap->Remove(mMask);
		uint32 len = overlap->GetLength();
		overlap->IncrementValues(negAlph, -1);
		delete overlap;
		mMask->Remove(((BM128ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}

uint32 BM128CoverSet::CoverWithOverlapHalfUncovered(uint32 setlen, ItemSet *ctElem) {
	if(mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		ItemSet *overlap = ctElem->Clone();
		overlap->Remove(mMask);
		uint32 len = overlap->GetLength();
		if(len > setlen / 2) {
			delete overlap;
			return 0;
		}
		delete overlap;
		mMask->Remove(((BM128ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}
bool BM128CoverSet::CanCoverWithOverlapHalfUncovered(uint32 setlen, ItemSet *ctElem) {
	if(mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		ItemSet *overlap = ctElem->Clone();
		overlap->Remove(mMask);
		uint32 len = overlap->GetLength();
		if(len > setlen / 2) {
			delete overlap;
			return false;
		}
		delete overlap;
		return true;
	}
	return false;
}
uint32 BM128CoverSet::CoverWithOverlapNegAlphHalfUncovered(uint32 setlen, ItemSet *ctElem, uint32 *negAlph) {
	if(mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		ItemSet *overlap = ctElem->Clone();
		overlap->Remove(mMask);
		uint32 len = overlap->GetLength();
		if(len > setlen / 2) {
			delete overlap;
			return 0;
		}
		overlap->IncrementValues(negAlph, 1);
		delete overlap;
		mMask->Remove(((BM128ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}
uint32 BM128CoverSet::UnCoverWithOverlapNegAlphHalfUncovered(uint32 setlen, ItemSet *ctElem, uint32 *negAlph) {
	if(mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		ItemSet *overlap = ctElem->Clone();
		overlap->Remove(mMask);
		uint32 len = overlap->GetLength();
		if(len > setlen / 2) {
			delete overlap;
			return 0;
		}
		overlap->IncrementValues(negAlph, -1);
		delete overlap;
		mMask->Remove(((BM128ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}

uint32 BM128CoverSet::CoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem) {
	if(mMask->Intersects(((BM128ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BM128ItemSet*)ctElem)->mMask)) {
		mMask->Remove(((BM128ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}

bool BM128CoverSet::CanCoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem) {
	return mMask->Intersects(((BM128ItemSet*)ctElem)->mMask);
}

uint32 BM128CoverSet::CoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values) {
	if(mNumUncovered == 0)
		return 0;
	mMask->IncrementValues(abet, 1, values);
	return mNumUncovered;
}

uint32 BM128CoverSet::CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values) {
	if(mNumUncovered == 0)
		return 0;
	mMask->IncrementValues(abet, cnt, values);
	return mNumUncovered * cnt;
}

uint32 BM128CoverSet::UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values) {
	if(mNumUncovered == 0)
		return 0;
	mMask->IncrementValues(abet, -1, values);
	return mNumUncovered;
}

uint32 BM128CoverSet::UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values) {
	if(mNumUncovered == 0)
		return 0;
	mMask->IncrementValues(abet, -(int32) cnt, values);
	return mNumUncovered * cnt;
}
void BM128CoverSet::GetUncoveredAttributesIn(uint16 *valAr) {
	mMask->GetValuesIn(valAr);
}

/*
int32 BM128CoverSet::AdjustAlphabet(uint32 ablen, uint32 *abet) {
	//ItemSet* temp = mMask->Intersection(mBackupMask);
	//uint32 l = temp->GetLength();
	mBackupMask->Difference(mMask); // removes overlap
	mBackupMask->CalcLength();
	mMask->CalcLength();
	uint32 lZap = mBackupMask->GetLength();
	uint32 lAdd = mMask->GetLength();
	mMask->IncrementValuesIn(abet, 1);
	mBackupMask->IncrementValuesIn(abet, -1);
	return lAdd - lZap;
}
*/
void BM128CoverSet::SetItem(uint32 i) {
	mMask->BM128ItemSet::Set(i);
	mNumUncovered--;
}
void BM128CoverSet::UnSetItem(uint32 i) {
	mMask->BM128ItemSet::UnSet(i);
	mNumUncovered++;
}
string BM128CoverSet::ToString() {
	if(mNumUncovered == 0)
		return string();
	
	uint32* values = new uint32[mNumUncovered];
	mMask->BM128ItemSet::GetValuesIn(values);
	string s = "";
	char *buffer = new char[20];

	for(uint32 i=0; i<mNumUncovered; i++) {
		sprintf_s(buffer, 20, "%d ", values[i]);
		s.append(buffer);
	}
	if(s.length() > 0)
		s.resize(s.length()-1);
	delete[] buffer;
	delete[] values;
	return s;
}
