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

	virtual void		GetUncoveredAttributesIn(uint16 *valAr) { throw "GetUncoveredAttributesIn :: Not Implemented for this CoverSet"; };

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

	virtual bool		CanCoverLE(ItemSet *attr, uint32 setlen, ItemSet *iset) { throw "nog nie"; }

	virtual uint32		CoverWithOverlap(uint32 setlen, ItemSet *codeElement)=0;
	virtual bool		CanCoverWithOverlap(uint32 setlen, ItemSet *ctElem)=0;
	virtual uint32		NumCoverWithOverlap(uint32 setlen, ItemSet *ctElem)=0;

	virtual uint32		CoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph)=0;
	virtual uint32		UnCoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph)=0;

	virtual uint32		CoverWithOverlapHalfUncovered(uint32 setlen, ItemSet *codeElement) { throw "Not implemented. (CoverSet::CoverWithOverlapHalfUncovered)"; }
	virtual bool		CanCoverWithOverlapHalfUncovered(uint32 setlen, ItemSet *ctElem) { throw "Not implemented. (CoverSet::CanCoverWithOverlapHalfUncovered)"; }
	virtual uint32		CoverWithOverlapNegAlphHalfUncovered(uint32 setlen, ItemSet *codeElement, uint32 *negAlph) { throw "Not implemented. (CoverSet::CoverWithOverlapNegAlphHalfUncovered)"; }
	virtual uint32		UnCoverWithOverlapNegAlphHalfUncovered(uint32 setlen, ItemSet *codeElement, uint32 *negAlph) { throw "Not implemented. (CoverSet::UnCoverWithOverlapNegAlphHalfUncovered)"; }

	virtual uint32		CoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *codeElement)=0;
	virtual bool		CanCoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem)=0;

	/* -- Alphabet cover -- */
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet)=0;
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt)=0; // { throw "Not implemented. (CoverSet::CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt))"; }
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet)=0;
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt)=0; // { throw "Not implemented. (CoverSet::UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt))"; }

	virtual bool		IsItemUncovered(uint32 value) =0;
	virtual string		ToString()  { throw "Not implemented. (CoverSet::ToString())"; }

	static CoverSet*	Create(ItemSetType type, size_t abetlen, ItemSet *initwith=NULL, uint32 numipoints=0);
	static CoverSet*	Create(Database * const db, ItemSet *initwith=NULL, uint32 numipoints=0);
	static ItemSetType	StringToType(string &desc);

protected:
	uint32				mNumUncovered;		// the number of elements in the mask that are 'on'
};

#endif // __COVERSET_H
