#ifndef __BAI32COVERSET_H
#define __BAI32COVERSET_H

#include "CoverSet.h"
#include "BAI32ItemSet.h"

class BASS_API BAI32CoverSet : public CoverSet {
public:
	BAI32CoverSet(uint32 abetlen, ItemSet *initwith, uint32 numIPs);
	BAI32CoverSet(const BAI32CoverSet &cset);
	virtual ~BAI32CoverSet();
	virtual CoverSet*	Clone();

	virtual	void		InitSet(ItemSet* iset);
	virtual void		ResetMask();

	virtual void		BackupMask();
	virtual void		RestoreMask();

	virtual void		GetUncoveredAttributesIn(uint16 *valAr);

	virtual uint32		LoadMask(uint32 ipoint);
	virtual void		StoreMask(uint32 ipoint);

	virtual void		MakeIPointUpToDate(uint32 ipoint, ItemSet* ctElem, uint32 len);
	virtual void		SetIPointUpToDate(uint32 ipoint);
	virtual uint32		GetMostUpToDateIPoint(uint32 ipoint);

	// tricky dicky, only use when you're sure you know how to handle it.
	virtual void		Uncover(uint32 setlen, ItemSet *codeElement);

	virtual void		SetItem(uint32 i);
	virtual void		UnSetItem(uint32 i);

	virtual bool		CanCoverIPoint(uint32 ipoint, ItemSet *ctElem);

	virtual uint32		Cover(uint32 setlen, ItemSet *dbRow);
	virtual bool		CanCover(uint32 setlen, ItemSet *ctElem);
	virtual uint32		NumCover(uint32 setlen, ItemSet *ctElem) { throw string("Kan ik nog niet"); }

	virtual uint32		CoverWithOverlap(uint32 setlen, ItemSet *codeElement);
	virtual bool		CanCoverWithOverlap(uint32 setlen, ItemSet *ctElem);
	// We gaan er voor NumCoverWithOverlap(..) blind vanuit dat ie uberhaupt in deze rij past.
	// oftewel, dat CanCoverWithOverlap(setlen,ctElem)=true
	virtual uint32		NumCoverWithOverlap(uint32 setlen, ItemSet *ctElem);

	virtual uint32		UnCoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph);
	virtual uint32		CoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph);

	virtual uint32		CoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *codeElement);
	virtual bool		CanCoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem);

	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values = 0);
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values = 0);
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values = 0);
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values = 0);

	virtual bool		IsItemUncovered(uint32 value) { return mMask->IsItemInSet(value); }

	virtual ItemSet*	GetMask() { return mMask; }

protected:
	BAI32ItemSet		**mIPMasks;
	uint32				mNumIPMasks;
	uint32				mIPMasksUpToDate;

	BAI32ItemSet		*mMask;				// the masking set

	BAI32ItemSet		*mBackupMask;
	uint32				mBackupNumUncovered;
};

#endif // __BAI32COVERSET_H
