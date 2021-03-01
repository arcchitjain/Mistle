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
