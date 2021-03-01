#ifdef BLOCK_PTREE

#include "../../global.h"

#include <RandomUtils.h>

#include <itemstructs/CoverSet.h>
#include <itemstructs/ItemSet.h>

#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <glibc_s.h>
#endif

#include "PTreeSet.h"
#include "PTreeNode.h"


PTreeNode::PTreeNode() {
	mParent = NULL;
	mChildren = new ptnlist();
	mNumChildren = 0;

	mItem = UINT16_MAX_VALUE;
	mFrequency = 0;
	mNumCovered = 0;
}

PTreeNode::PTreeNode(PTreeNode *parent, uint16 item, uint32 usecnt) {
	mParent = parent;
	mChildren = new ptnlist();
	mNumChildren = 0;

	mItem = item;
	mFrequency = usecnt;
	mNumCovered = usecnt;
}

PTreeNode::~PTreeNode() {
	ptnlist::iterator ci = mChildren->begin(), cend = mChildren->end();

	for(ci = mChildren->begin(); ci != cend; ++ci) {
		delete *ci;
	}
	delete mChildren;
}

uint16 PTreeNode::GetItem() {
	return mItem;
}

uint32 PTreeNode::GetFrequency() {
	return mFrequency;
}
void PTreeNode::IncFrequency(uint32 inc) {
	mFrequency += inc;
}
void PTreeNode::SetFrequency(uint32 freq) {
	mFrequency = freq;
}

uint32 PTreeNode::GetNumCovered() {
	return mNumCovered;
}
void PTreeNode::SetNumCovered(uint32 numCovered) {
	mNumCovered = numCovered;
}
void PTreeNode::IncNumCovered(uint32 inc) {
	mNumCovered += inc;
}

PTreeNode* PTreeNode::AddSubItem(uint16 item, uint32 count) {
	uint16 cItem;
	ptnlist::iterator ci, cend = mChildren->end();
	for(ci = mChildren->begin(); ci != cend; ++ci) {
		cItem = ((PTreeNode*)(*ci))->GetItem();
		if(cItem == item) {
			((PTreeNode*)(*ci))->IncFrequency(count);
			return *ci;
		} else if(cItem > item)
			break;
	}
	// not a known child-node, lets create a new one and insert it at the right position (sorted ascending on id)
	PTreeNode *ptn = new PTreeNode(this, item, count);
	mChildren->insert(ci, ptn);
	++mNumChildren;

	return ptn;
}

uint32 PTreeNode::GetNumChildren() {
	return mNumChildren;
}

ptnlist* PTreeNode::GetChildrenList() {
	return mChildren;
}

void PTreeNode::Print(string prefix) const {
	char tmp[10];
	sprintf_s(tmp, 10, "%d ", mItem);
	string newPrefix = prefix + tmp;
	if(mNumChildren > 0) {
		ptnlist::iterator cit, cend = mChildren->end();
		for(cit = mChildren->begin(); cit != cend; ++cit) {
			((PTreeNode*)*cit)->Print(newPrefix);
		}
	} else {
		printf("%s\n", newPrefix.c_str());
	}
}

void PTreeNode::PrintTree(string indent) const {
	if(mNumChildren > 0) {
		char tmp[10];
		uint32 num =  sprintf_s(tmp, 10, "%d ", mItem);
		string newIndent = indent + string(num, ' ');
		ptnlist::iterator cit, cend = mChildren->end();
		cit = mChildren->begin();
		printf("%s", tmp);
		((PTreeNode*)*cit)->PrintTree(newIndent);
		if(mNumChildren > 1) {
			for(++cit; cit != cend; ++cit) {
				printf("%s", newIndent.c_str());
				((PTreeNode*)*cit)->PrintTree(newIndent);
			}
		}
	} else {
		printf("%d\n", mItem);
	}
}

void PTreeNode::BuildSideWalk(ptnlist** sideWalk) {
	sideWalk[mItem]->push_back(this);
	ptnlist::iterator cit, cend = mChildren->end();
	for(cit=mChildren->begin(); cit != cend; ++cit) {
		((PTreeNode*)*cit)->BuildSideWalk(sideWalk);
	}
}

#endif // BLOCK_PTREE
