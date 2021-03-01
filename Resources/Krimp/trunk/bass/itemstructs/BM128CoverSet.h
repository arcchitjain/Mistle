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

	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet);
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt);
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet);
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt);

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
