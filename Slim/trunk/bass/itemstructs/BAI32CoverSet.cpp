#include "../Bass.h"

#include "BAI32ItemSet.h"

#include "CoverSet.h"
#include "BAI32CoverSet.h"

BAI32CoverSet::BAI32CoverSet(uint32 abetlen, ItemSet *initwith, uint32 numIPs) {
	mNumIPMasks = numIPs+1;						// +1 want dbRow is mIPMasks[0]
	mIPMasks = new BAI32ItemSet*[mNumIPMasks];

	mIPMasks[0] = (BAI32ItemSet*)initwith;		// db rij, owned by Database
	if(initwith != NULL) {
		mMask = (BAI32ItemSet*)(initwith->Clone());
		mMask->SetDBOccurrence(NULL, 0);
		mNumUncovered = ((BAI32ItemSet*)initwith)->mBitCount;
	} else {
		mMask = new BAI32ItemSet(abetlen);
		mNumUncovered = 0;
	}

	for(uint32 i=1; i<mNumIPMasks; i++)
		mIPMasks[i] = (BAI32ItemSet*)mMask->Clone();
	mIPMasksUpToDate = mNumIPMasks-1;

	mBackupNumUncovered = mNumUncovered;
	mBackupMask = (BAI32ItemSet*)mMask->Clone();
}

BAI32CoverSet::BAI32CoverSet(const BAI32CoverSet &cset) {
	mNumIPMasks = cset.mNumIPMasks;
	mIPMasks = new BAI32ItemSet*[mNumIPMasks];

	mIPMasks[0] = cset.mIPMasks[0];			// db rij, owned by Database
	for(uint32 i=1; i<mNumIPMasks; i++) {
		mIPMasks[i] = cset.mIPMasks[i]->Clone();
	}
	mIPMasksUpToDate = cset.mIPMasksUpToDate;

	mMask = cset.mMask->Clone();
	mNumUncovered = cset.mNumUncovered;

	mBackupNumUncovered = cset.mBackupNumUncovered;
	mBackupMask = cset.mBackupMask->Clone();
}

BAI32CoverSet::~BAI32CoverSet() {
	for(uint32 i=1; i<mNumIPMasks; i++)		// skip 0, want dat is de db rij!
		delete mIPMasks[i];
	delete[] mIPMasks;

	delete mMask;
	delete mBackupMask;
}

CoverSet* BAI32CoverSet::Clone() {
	return new BAI32CoverSet(*this);
}


void BAI32CoverSet::InitSet(ItemSet* iset) {
	mMask->CopyFrom((BAI32ItemSet*) iset);
	mNumUncovered = ((BAI32ItemSet*)iset)->mBitCount;

	mIPMasks[0] = (BAI32ItemSet*)iset;
	mIPMasksUpToDate = 0;
}
void BAI32CoverSet::ResetMask() {
	mMask->CopyFrom(mIPMasks[0]);
	mNumUncovered = mMask->mBitCount;
}
void BAI32CoverSet::BackupMask() {
	mBackupMask->CopyFrom(mMask);
	mBackupNumUncovered = mNumUncovered;
}
void BAI32CoverSet::RestoreMask() {
	mMask->CopyFrom(mBackupMask);
	mNumUncovered = mBackupNumUncovered;
}

/*
InsertionPoint nummering loopt van 0 voor het initiele mask, via 1 voor het eerste insertionpoint (bovenaan CT) tot n (onderaan CT)
*/
uint32 BAI32CoverSet::LoadMask(uint32 ipoint) {
	if(ipoint > mIPMasksUpToDate)
		ipoint = mIPMasksUpToDate;
	mMask->CopyFrom(mIPMasks[ipoint]);
	mNumUncovered = mMask->GetLength();
	return ipoint;
}
void BAI32CoverSet::StoreMask(uint32 ipoint) {
	//_ASSERT(ipoint < mNumIPMasks);
	mIPMasksUpToDate = ipoint;	// we gaan er vanuit dat de ipoints voor deze ook al gestored zijn, of nog up-to-date //we assume that the ipoints for this even though gestored, or even up-to-date
	mIPMasks[ipoint]->CopyFrom(mMask);
}

void BAI32CoverSet::MakeIPointUpToDate(uint32 ipoint, ItemSet* ctElem, uint32 len) {
	if(mIPMasks[ipoint]->IsSubset(((BAI32ItemSet*)ctElem)->mMask)) {
		mIPMasks[ipoint]->RemoveSubset(((BAI32ItemSet*)ctElem)->mMask,len);
	}
	mIPMasksUpToDate = ipoint;
}
void BAI32CoverSet::SetIPointUpToDate(uint32 ipoint) {
	mIPMasksUpToDate = ipoint;
}
uint32 BAI32CoverSet::GetMostUpToDateIPoint(uint32 ipoint) {
	if(ipoint > mIPMasksUpToDate)
		ipoint = mIPMasksUpToDate;
	return ipoint;
}

/*	Tries to cover the current CoverSet with the code table element.
	Adjusts the mask if appropriate, returns the number of cover 
	matches for this element. (normally either 0 or 1, but possibly 
	higher if this coverSet has a higher occurrence)
*/
uint32 BAI32CoverSet::Cover(uint32 setlen, ItemSet *ctElem) {
	if(setlen > mNumUncovered)
		return 0;

	if(mMask->IsSubset(((BAI32ItemSet*)ctElem)->mMask)) {
		mMask->RemoveSubset(((BAI32ItemSet*)ctElem)->mMask, setlen);
		mNumUncovered -= setlen;
		return 1;
	} else return 0;
}
void BAI32CoverSet::Uncover(uint32 setlen, ItemSet *ctElem) {
	mMask->UniteNonOverlappingSet(((BAI32ItemSet*)ctElem)->mMask,setlen);
	mNumUncovered += setlen;
}

bool BAI32CoverSet::CanCover(uint32 setlen, ItemSet *ctElem) {
	if(setlen > mNumUncovered)
		return false;
	return mMask->IsSubset(((BAI32ItemSet*)ctElem)->mMask);
}
bool BAI32CoverSet::CanCoverIPoint(uint32 ipoint, ItemSet *ctElem) {
	return mIPMasks[ipoint]->IsSubset(ctElem);
}

uint32 BAI32CoverSet::CoverWithOverlap(uint32 setlen, ItemSet *ctElem) {
	if(mMask->Intersects(((BAI32ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BAI32ItemSet*)ctElem)->mMask)) {
		mMask->Remove(((BAI32ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}
bool BAI32CoverSet::CanCoverWithOverlap(uint32 setlen, ItemSet *ctElem) {
	return mMask->Intersects(((BAI32ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BAI32ItemSet*)ctElem)->mMask);
}
// we gaan er even blind vanuit dat ie uberhaupt in deze rij past.
uint32 BAI32CoverSet::NumCoverWithOverlap(uint32 setlen, ItemSet *ctElem) {
	return mMask->IntersectionLength(((BAI32ItemSet*)ctElem)->mMask);
}

uint32 BAI32CoverSet::UnCoverWithOverlapNegAlph(uint32 setlen, ItemSet *ctElem, uint32 *negAlph) {
	if(mMask->Intersects(((BAI32ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BAI32ItemSet*)ctElem)->mMask)) {
		ItemSet *overlap = ctElem->Clone();
		overlap->Remove(mMask);
		uint32 len = overlap->GetLength();
		overlap->IncrementValues(negAlph, -1);
		delete overlap;
		mMask->Remove(((BAI32ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}
uint32 BAI32CoverSet::CoverWithOverlapNegAlph(uint32 setlen, ItemSet *ctElem, uint32 *negAlph) {
	if(mMask->Intersects(((BAI32ItemSet*)ctElem)->mMask) && mIPMasks[0]->IsSubset(((BAI32ItemSet*)ctElem)->mMask)) {
		ItemSet *overlap = ctElem->Clone();
		overlap->Remove(mMask);
		uint32 len = overlap->GetLength();
		overlap->IncrementValues(negAlph, 1);
		delete overlap;
		mMask->Remove(((BAI32ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}

uint32 BAI32CoverSet::CoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem) {
	if(mMask->Intersects(((BAI32ItemSet*)ctElem)->mMask)) {
		mMask->Remove(((BAI32ItemSet*)ctElem)->mMask);
		mNumUncovered = mMask->GetLength(); // door remove toch hercount'ed
		return 1;
	} else return 0;
}
bool BAI32CoverSet::CanCoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem) {
	return mMask->Intersects(((BAI32ItemSet*)ctElem)->mMask);
}

uint32 BAI32CoverSet::CoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values) {
	if(mNumUncovered == 0)
		return 0;
	//uint32 numv = mMask->mBitCount;					// Zolang je zorgt dat mBitCount vers is, door RemoveSubset te gebruiken is geen recalc nodig
	mMask->IncrementValues(abet, 1, values);
	return mNumUncovered;
}

uint32 BAI32CoverSet::CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values) {
	if(mNumUncovered == 0)
		return 0;
	mMask->IncrementValues(abet, cnt, values);
	return mNumUncovered * cnt;
}

uint32 BAI32CoverSet::UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values) {
	if(mNumUncovered == 0)
		return 0;
	//uint32 numv = mMask->mBitCount;					// Zolang je zorgt dat mBitCount vers is, door RemoveSubset te gebruiken is geen recalc nodig
	mMask->IncrementValues(abet, -1, values);
	return mNumUncovered;
}

uint32 BAI32CoverSet::UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values) {
	if(mNumUncovered == 0)
		return 0;
	mMask->IncrementValues(abet, -(int32) cnt, values);
	return mNumUncovered * cnt;
}
void BAI32CoverSet::GetUncoveredAttributesIn(uint16 *valAr) {
	mMask->GetValuesIn(valAr);
}

void BAI32CoverSet::UnSetItem(uint32 i) {
	mMask->BAI32ItemSet::UnSet(i);
	mNumUncovered--;
}
void BAI32CoverSet::SetItem(uint32 i) {
	mMask->Set(i);
	mNumUncovered++;
}

