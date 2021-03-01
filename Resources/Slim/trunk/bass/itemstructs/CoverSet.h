#ifndef __COVERSET_H
#define __COVERSET_H

class Database;

#include "ItemSet.h"

class BASS_API CoverSet {
public:
	CoverSet();
	virtual ~CoverSet();
	virtual CoverSet*	Clone()=0;

	virtual	void		InitSet(ItemSet* iset)=0;
	virtual void		ResetMask()=0;

	virtual void		GetUncoveredAttributesIn(uint16 *valAr) { throw string("GetUncoveredAttributesIn :: Not Implemented for this CoverSet"); };

	virtual void		BackupMask()=0;
	virtual void		RestoreMask()=0;

	virtual uint32		LoadMask(uint32 ipoint)=0;
	virtual void		StoreMask(uint32 ipoint)=0;

	virtual void		MakeIPointUpToDate(uint32 ipoint, ItemSet* ctElem, uint32 len)=0;
	virtual void		SetIPointUpToDate(uint32 ipoint)=0;
	virtual uint32		GetMostUpToDateIPoint(uint32 ipoint)=0;

	// tricky dicky, only use when you're sure you know how to handle it.
	virtual void		Uncover(uint32 setlen, ItemSet *codeElement)=0;

	virtual bool		CanCoverIPoint(uint32 ipoint, ItemSet *ctElem)=0;

	virtual uint32		GetNumUncovered() { return mNumUncovered; }

	virtual void		SetItem(uint32 i)=0;
	virtual void		UnSetItem(uint32 i)=0;

	/* -- Cover and its alternatives -- */
	// Basic Cover, calls Cover(iset->GetLength(), iset)`
	virtual uint32		Cover(ItemSet *iset);
	// Cover, preferred over `Cover(iset)`
	virtual uint32		Cover(uint32 setlen, ItemSet *iset)=0;
	// Basic CanCover, calls CanCover(iset->GetLength(), iset)`
	virtual bool		CanCover(ItemSet *iset);
	// CanCover, preferred over `CanCover(iset)`
	virtual bool		CanCover(uint32 setlen, ItemSet *iset)=0;
	// Basic NumCover, calls NumCover(iset->GetLength(), iset)`
	virtual uint32		NumCover(ItemSet *iset);
	// NumCover, preferred over `NumCover(iset)`
	virtual uint32		NumCover(uint32 setlen, ItemSet *iset)=0;

	virtual bool		CanCoverLE(ItemSet *attr, uint32 setlen, ItemSet *iset) { throw string("nog nie"); }

	virtual uint32		CoverWithOverlap(uint32 setlen, ItemSet *codeElement)=0;
	virtual bool		CanCoverWithOverlap(uint32 setlen, ItemSet *ctElem)=0;
	virtual uint32		NumCoverWithOverlap(uint32 setlen, ItemSet *ctElem)=0;

	virtual uint32		CoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph)=0;
	virtual uint32		UnCoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph)=0;

	virtual uint32		CoverWithOverlapHalfUncovered(uint32 setlen, ItemSet *codeElement) { throw string("Not implemented. (CoverSet::CoverWithOverlapHalfUncovered)"); }
	virtual bool		CanCoverWithOverlapHalfUncovered(uint32 setlen, ItemSet *ctElem) { throw string("Not implemented. (CoverSet::CanCoverWithOverlapHalfUncovered)"); }
	virtual uint32		CoverWithOverlapNegAlphHalfUncovered(uint32 setlen, ItemSet *codeElement, uint32 *negAlph) { throw string("Not implemented. (CoverSet::CoverWithOverlapNegAlphHalfUncovered)"); }
	virtual uint32		UnCoverWithOverlapNegAlphHalfUncovered(uint32 setlen, ItemSet *codeElement, uint32 *negAlph) { throw string("Not implemented. (CoverSet::UnCoverWithOverlapNegAlphHalfUncovered)"); }

	virtual uint32		CoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *codeElement)=0;
	virtual bool		CanCoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem)=0;

	/* -- Alphabet cover -- */
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values = 0)=0;
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values = 0)=0; // { throw string("Not implemented. (CoverSet::CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt))"); }
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 *values = 0)=0;
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values = 0)=0; // { throw string("Not implemented. (CoverSet::UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt))"); }

	virtual bool		IsItemUncovered(uint32 value) =0;
	virtual string		ToString()  { throw string("Not implemented. (CoverSet::ToString())"); }

	static CoverSet*	Create(ItemSetType type, size_t abetlen, ItemSet *initwith=NULL, uint32 numipoints=0);
	static CoverSet*	Create(Database * const db, ItemSet *initwith=NULL, uint32 numipoints=0);
	static ItemSetType	StringToType(string &desc);

protected:
	uint32				mNumUncovered;		// the number of elements in the mask that are 'on'
};

#endif // __COVERSET_H
