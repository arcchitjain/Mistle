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
#include "../Bass.h"

#include <stdlib.h>
#include "ItemSet.h"

#include "CoverNode.h"

CoverNode::CoverNode() {
	mNumChildren = 0;
	mChildren = NULL;
	mUsageCount = 0;
	mParentNode = NULL;
	mItemSet = NULL;
}
CoverNode::CoverNode(CoverNode *parent, ItemSet *iset) {
	mNumChildren = 0;
	mChildren = NULL;
	mUsageCount = 0;
	mParentNode = parent;
	mItemSet = iset;
}
CoverNode::~CoverNode() {
	for(uint32 i=0; i<mNumChildren; i++) {
		delete mChildren[i];
	}
	delete[] mChildren;
	// mItemSet aint ours
}
CoverNode* CoverNode::Clone() {
	CoverNode *cn = new CoverNode();
	cn->mNumChildren = mNumChildren;
	cn->mItemSet = mItemSet != NULL ? mItemSet->Clone() : NULL;
	cn->mUsageCount = mUsageCount;
	//cn->mParentNode moet via loop hieronder

	CoverNode **children = new CoverNode*[mNumChildren];
	for(uint32 i=0; i<mNumChildren; i++) {
		children[i] = mChildren[i]->Clone();
		children[i]->mParentNode = cn;
	}
	
	return cn;
}
CoverNode* CoverNode::AddCoverNode(ItemSet *iset, uint64 &numNests) {
	// First see if it already exists
	for(uint32 i=0; i<mNumChildren; i++) {
		if(mChildren[i]->GetItemSet() == iset) {	// pointer-equals is valid, as its drawn from same CodeTable
			mChildren[i]->AddOneToUsageCount();
			return mChildren[i];
		}
	}
	// Nope. Add-time.
	CoverNode *newNode = new CoverNode(this, iset);
	CoverNode **newChildrenNodes = new CoverNode*[mNumChildren+1];
	memcpy_s(newChildrenNodes, (mNumChildren+1) * sizeof(CoverNode*), mChildren, mNumChildren * sizeof(CoverNode*));
	newChildrenNodes[mNumChildren] = newNode;
	delete[] mChildren;
	mChildren = newChildrenNodes;
	mNumChildren++;
	numNests++;
	iset->AddUsagerea(1);
	AddOneToUsageCount();
	return newNode;
}

// Mighty Magic Shizzle happens here!
CoverNode* CoverNode::SupCoverNode(ItemSet *iset, uint64 &numNests) {
	for(uint32 i=0; i<mNumChildren; i++) {
		if(mChildren[i]->GetItemSet() == iset) {
			mChildren[i]->SupOneFromUsageCount();
			if(mChildren[i]->GetUsageCount() == 0) {
				// aaawh, it's gotta be euthanised!
				mChildren[i]->DelCoverBranch(numNests);	// puts the branch down
				delete mChildren[i];	// die hard.
				numNests--;
				// do mind the children
				memcpy(mChildren + i, mChildren + 1 + i, mNumChildren - i - 1);
				mNumChildren--;
				return NULL;
			} else {
				// we need a bigger boat!
				return mChildren[i];
			}
		}
	}
	return NULL;
}

void CoverNode::PrintTree(FILE* fp, uint32 padlen) {
	string isetString = "";
	if(mItemSet != NULL)
		isetString = mItemSet->ToString(false,false);
	if(mNumChildren > 0) {
		uint32 usageCount = (mItemSet == 0 ? 0 : mItemSet->GetUsageArea());
		fprintf(fp, "[%s x%d] ", isetString.c_str(), usageCount);
		uint32 addLen = (uint32) isetString.length() + 3 + 3 + (uint32)ceil(log((double)usageCount)/log((double)10));
		char pad[10240];
		memset(pad, ' ', padlen + addLen);
		pad[padlen + addLen] = 0;
		mChildren[0]->PrintTree(fp, padlen + addLen);
		for(uint32 i=1; i<mNumChildren; i++) {
			fprintf(fp, "%s", pad);
			mChildren[i]->PrintTree(fp, padlen + addLen);
		}
	} else { 
		fprintf(fp, "[%s x%d]\n", isetString.c_str(), (mItemSet == 0 ? 0 : mItemSet->GetUsageArea()));
	}
}
CoverNode* CoverNode::GetChildNode(ItemSet *iset) {
	for(uint32 i=0; i<mNumChildren; i++) {
		if(mChildren[i]->GetItemSet() == iset)
			return mChildren[i];
	}
	return NULL;
}

void CoverNode::DelCoverBranch(uint64 &numNests) {
	for(uint32 i=0; i<mNumChildren; i++) {
		mChildren[i]->DelCoverBranch(numNests);
		delete mChildren[i];
	}
	numNests -= mNumChildren;
	mNumChildren = 0;
}
