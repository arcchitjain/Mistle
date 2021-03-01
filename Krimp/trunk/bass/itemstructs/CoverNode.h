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
#ifndef __COVERTREE_H
#define __COVERTREE_H

class ItemSet;

class BASS_API CoverNode {
public:
	CoverNode();
	CoverNode(CoverNode *parent, ItemSet *iset);
	virtual ~CoverNode();
	virtual CoverNode*	Clone();
	
	virtual CoverNode*	AddCoverNode(ItemSet *iset, uint64 &numNests);
	virtual CoverNode*	SupCoverNode(ItemSet *iset, uint64 &numNests);

	// kills off the children of this CoverNode, keeping track of the number of nestings removed
	void				DelCoverBranch(uint64 &numNests);

	virtual	uint32		GetUsageCount() { return mUsageCount; };
	__inline void		AddOneToUsageCount() { mUsageCount++; }
	__inline void		SupOneFromUsageCount() { mUsageCount--; }
	
	virtual bool		HasChildren() { return mNumChildren > 0; }
	virtual CoverNode**	GetChildren()	{ return mChildren; }

	virtual CoverNode*	GetChildNode(ItemSet *iset);

	virtual ItemSet*	GetItemSet()	{ return mItemSet; };

	virtual void		PrintTree(FILE* fp, uint32 padlen);
	
protected:
	CoverNode	*mParentNode;
	CoverNode	**mChildren;
	ItemSet		*mItemSet;

	uint32		mNumChildren;
	uint32		mUsageCount;

};

#endif // __COVERSET_H

