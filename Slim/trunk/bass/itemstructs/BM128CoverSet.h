#ifndef __BM128COVERSET_H
#define __BM128COVERSET_H

#include "CoverSet.h"
#include "BM128ItemSet.h"

class BASS_API BM128CoverSet : public CoverSet {
public:
	// Empty CoverSet
	BM128CoverSet(uint32 abLen, uint32 numIPs);
	// Init'd CoverSet
	BM128CoverSet(uint32 abLen, ItemSet *initwith, uint32 numIPs);
	// Copy* Constructor
	BM128CoverSet(BM128CoverSet *cset);
	virtual ~BM128CoverSet();

	virtual CoverSet*	Clone();

	virtual	void		InitSet(ItemSet* dbRow);
	virtual void		ResetMask();

	virtual void		GetUncoveredAttributesIn(uint16 *valAr);

	virtual void		BackupMask();
	virtual void		RestoreMask();

	virtual uint32		LoadMask(uint32 ipoint);
	virtual void		StoreMask(uint32 ipoint);

	virtual void		MakeIPointUpToDate(uint32 ipoint, ItemSet* ctElem, uint32 len);
	virtual void		SetIPointUpToDate(uint32 ipoint);
	virtual uint32		GetMostUpToDateIPoint(uint32 ipoint);

	virtual void		SetItem(uint32 i);
	virtual	void		UnSetItem(uint32 i);

	virtual uint32		Cover(ItemSet* codeElement);
	virtual uint32		Cover(uint32 setlen, ItemSet *codeElement);
	virtual bool		CanCover(uint32 setlen, ItemSet *ctElem);
	virtual uint32		NumCover(uint32 setlen, ItemSet *ctElem);

	virtual bool		CanCoverLE(ItemSet *attr, uint32 setlen, ItemSet *iset);

	// tricky dicky, only use when you're sure you know how to handle it.
	virtual void		Uncover(uint32 setlen, ItemSet *codeElement);

	virtual bool		CanCoverIPoint(uint32 ipoint, ItemSet *ctElem);

	virtual uint32		CoverWithOverlap(uint32 setlen, ItemSet *codeElement);
	virtual bool		CanCoverWithOverlap(uint32 setlen, ItemSet *ctElem);
	// We gaan er voor NumCoverWithOverlap(..) blind vanuit dat ie uberhaupt in deze rij past.
	// oftewel, dat CanCoverWithOverlap(setlen,ctElem)=true
	virtual uint32		NumCoverWithOverlap(uint32 setlen, ItemSet *ctElem);

	virtual uint32		CoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph);
	virtual uint32		UnCoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph);

	virtual uint32		CoverWithOverlapHalfUncovered(uint32 setlen, ItemSet *codeElement);
	virtual bool		CanCoverWithOverlapHalfUncovered(uint32 setlen, ItemSet *ctElem);
	virtual uint32		CoverWithOverlapNegAlphHalfUncovered(uint32 setlen, ItemSet *codeElement, uint32 *negAlph);
	virtual uint32		UnCoverWithOverlapNegAlphHalfUncovered(uint32 setlen, ItemSet *codeElement, uint32 *negAlph);

	virtual uint32		CoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *codeElement);
	virtual bool		CanCoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem);

	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values = 0);
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values = 0);
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values = 0);
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values = 0);

	virtual bool		IsItemUncovered(uint32 value) { return mMask->IsItemInSet(value); }
	virtual string		ToString();

	virtual ItemSet*	GetMask() { return mMask; }

protected:
	BM128ItemSet		*mMask;				// the masking set

	BM128ItemSet		*mBackupMask;
	uint32				mBackupNumUncovered;

	uint32				mIPMasksUpToDate;
	uint32				mNumIPMasks;
	BM128ItemSet		**mIPMasks;
};

#endif // __BM128COVERSET_H
