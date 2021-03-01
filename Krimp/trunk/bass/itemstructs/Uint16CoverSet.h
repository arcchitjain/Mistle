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
#ifndef __UINT16COVERSET_H
#define __UINT16COVERSET_H

#include "CoverSet.h"

class ItemSet;
class Uint16ItemSet;

class BASS_API Uint16CoverSet : public CoverSet {
public:
	Uint16CoverSet(ItemSet *initwith, uint32 numIPs);
	Uint16CoverSet(const Uint16CoverSet &cset);
	virtual ~Uint16CoverSet();
	virtual CoverSet*	Clone();

	virtual	void		InitSet(ItemSet* dbRow);
	virtual void		ResetMask();

	virtual void		BackupMask();
	virtual void		RestoreMask();

	virtual uint32		LoadMask(uint32 ipoint);
	virtual void		StoreMask(uint32 ipoint);

	virtual void		MakeIPointUpToDate(uint32 ipoint, ItemSet* ctElem, uint32 len);
	virtual void		SetIPointUpToDate(uint32 ipoint);
	virtual uint32		GetMostUpToDateIPoint(uint32 ipoint);

	virtual void		SetItem(uint32 i);
	virtual void		UnSetItem(uint32 i);

	// tricky dicky, only use when you're sure you know how to handle it.
	virtual void		Uncover(uint32 setlen, ItemSet *codeElement);

	virtual bool		CanCoverIPoint(uint32 ipoint, ItemSet *ctElem);

	virtual uint32		Cover(uint32 setlen, ItemSet *ctElem);
	virtual bool		CanCover(uint32 setlen, ItemSet *ctElem);
	virtual uint32		NumCover(uint32 setlen, ItemSet *ctElem) { throw "Kan ik nog niet"; }

	virtual uint32		CoverWithOverlap(uint32 setlen, ItemSet *codeElement);
	virtual bool		CanCoverWithOverlap(uint32 setlen, ItemSet *ctElem);
	// We gaan er voor NumCoverWithOverlap(..) blind vanuit dat ie uberhaupt in deze rij past.
	// NumCoverWithOverlap assumes CanCoverWithOverlap(setlen,ctElem) returns true
	virtual uint32		NumCoverWithOverlap(uint32 setlen, ItemSet *ctElem);

	virtual uint32		UnCoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph);
	virtual uint32		CoverWithOverlapNegAlph(uint32 setlen, ItemSet *codeElement, uint32 *negAlph);

	virtual uint32		CoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *codeElement);
	virtual bool		CanCoverWithOverlapNoSubsetCheck(uint32 setlen, ItemSet *ctElem);

	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet);
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt);
	virtual uint32		CoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt, uint32 *values);
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet);
	virtual uint32		UncoverAlphabet(uint32 ablen, uint32 *abet, uint32 cnt);

	virtual bool		IsItemUncovered(uint32 value);

protected:
	Uint16ItemSet		*mUint16ItemSet;

	uint16				*mSet;				// internal pointer to mIPMasks[0] set, same len as mMaskLen

	uint32				*mMask;				// the bitmask itself
	uint32				mMaskLen;			// the length of the mask
	uint32				*mOnes;				// the 1-initialized mask
	uint32				mMaxLen;			// the length of the mask array

	uint32				*mBackupMask;		// backup of the bitmask itself
	uint32				mBackupNumUncovered;

};

#endif // __UINT16COVERSET_H
