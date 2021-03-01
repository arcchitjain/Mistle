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
#ifndef _APRIORINODE_H
#define _APRIORINODE_H

#if defined (_WINDOWS)
	#include <Windows.h>
#endif


class AprioriNode
{
public:
	AprioriNode(const uint32 numItems, uint16 *items, uint32 *supports);
	virtual ~AprioriNode();

	// ----- Tree construction & counting -----
	void			AddLevel(const uint32 minSup, AprioriNode **firstChild, AprioriNode **lastChild);
	void			CountRow(const uint16 *set, const uint32 setIdx, const uint32 setLen, const uint32 curDepth, const uint32 countDepth);
	void			PruneLevel(const uint32 minSup, const uint32 curDepth, const uint32 pruneDepth);

	// ---------- Itemset mining -----------
	void			MineItemSets(AprioriCroupier *croupier, const uint32 minSup, ItemSet *previousSet, ItemSet **buffer, const uint32 bufferSize, uint32 &numSets);

	// -------- Get & Set ---------
	const uint32	GetNumItems()			{ return mNumItems; }
	uint16*			GetItems()				{ return mItems; }
	uint16			GetItem(const uint32 i) { return mItems[i]; }
	AprioriNode**	GetChildren()			{ return mChildren; }
	AprioriNode*	GetChild(const uint32 i){ return mChildren[i]; }
	
	AprioriNode*	GetNext()				{ return mNext; } // next node on same level
	void			SetNext(AprioriNode *n) { mNext = n; }

protected:
	uint32		mNumItems;
	uint16		*mItems;
	uint32		*mSupports;
	AprioriNode **mChildren;

	AprioriNode *mNext;
};

#endif // _APRIORINODE_H
