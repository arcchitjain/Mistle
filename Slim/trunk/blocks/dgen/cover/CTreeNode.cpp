#if defined(BLOCK_DATAGEN) || defined (BLOCK_CLASSIFIER)

#include "../../../global.h"

#include <RandomUtils.h>

#include <itemstructs/CoverSet.h>
#include <itemstructs/ItemSet.h>

//#include "../DGSet.h"

#include "CTreeSet.h"
#include "CTreeNode.h"

CTreeNode::CTreeNode() {
	mCTreeSet = NULL;
	mParent = NULL;
	mVirtual = false;
	mChildren = new ctnlist();
	mNumChildren = 0;
	mNumGhostChildren = 0;

	mUseCount = 0;
	mChildCntSum = 0;

	mProbability = 0;
	mStopProb = 0;
	
}

CTreeNode::CTreeNode(CTreeNode *parent, CTreeSet *ps, uint32 usecnt) {
	mParent = parent;
	mCTreeSet = ps;
	mVirtual = false;
	mChildren = new ctnlist();
	mNumChildren = 0;
	mNumGhostChildren = 0;

	mUseCount = usecnt;
	mChildCntSum = 0;

	mProbability = 0;
	mStopProb = 0;
}

CTreeNode::~CTreeNode() {
	ctnlist::iterator ci = mChildren->begin(), cend = mChildren->end();

	for(ci = mChildren->begin(); ci != cend; ++ci) {
		delete *ci;
	}
	delete mChildren;

}

CTreeNode*	CTreeNode::AddCodingSet(CTreeSet *cs, uint32 usageCnt) {
	uint32 addId = cs->GetId(), id;
	ctnlist::iterator ci, cend = mChildren->end();
	for(ci = mChildren->begin(); ci != cend; ++ci) {
		id = ((CTreeNode*)(*ci))->GetCTreeSet()->GetId();
		if(id == addId) {
			((CTreeNode*)(*ci))->AddUseCount(usageCnt);
			return *ci;
		} else if(id > addId)
			break;
	}
	// not a known child-node, lets create a new one and insert it at the right position (sorted ascending on id)
	CTreeNode *ctn = new CTreeNode(this, cs, usageCnt);
	mChildren->insert(ci, ctn);
	++mNumChildren;

	cs->AddUsage(ctn);
	return ctn;
}
CTreeNode* CTreeNode::GetCodingSet(CTreeSet *cs) {
	uint32 addId = cs->GetId(), id;
	ctnlist::iterator ci, cend = mChildren->end();
	for(ci = mChildren->begin(); ci != cend; ++ci) {
		id = ((CTreeNode*)(*ci))->GetCTreeSet()->GetId();
		if(id == addId) {
			return *ci;
		} else if(id > addId)
			return NULL;
	}
	return NULL;
}
bool CTreeNode::HasCodingSet(CTreeSet *cs) {
	uint32 addId = cs->GetId(), id;
	ctnlist::iterator ci, cend = mChildren->end();
	for(ci = mChildren->begin(); ci != cend; ++ci) {
		id = ((CTreeNode*)(*ci))->GetCTreeSet()->GetId();
		if(id == addId) {
			return true;
		} else if(id > addId)
			return false;
	}
	return false;
}

CTreeSet* CTreeNode::GetCTreeSet() {
	return mCTreeSet;
}
void CTreeNode::SetCTreeSet(CTreeSet *cs, uint32 usecnt) {
	delete mCTreeSet;
	mCTreeSet = cs;
	mUseCount = usecnt;
}
uint32	CTreeNode::GetUseCount() {
	return mUseCount;
}
void	CTreeNode::AddUseCount(uint32 useCnt) {
	mUseCount += useCnt;
}

void CTreeNode::AddOneToCounts() {
	++mUseCount;
	ctnlist::iterator pi, pend=mChildren->end();
	for(pi=mChildren->begin(); pi != pend; ++pi)
		((CTreeNode*)(*pi))->AddOneToCounts();
}
void CTreeNode::CalcProbabilities() {
	mChildCntSum = 0;
	ctnlist::iterator pi, pend=mChildren->end();
	for(pi=mChildren->begin(); pi != pend; ++pi)
		mChildCntSum += ((CTreeNode*)(*pi))->GetUseCount();
	for(pi=mChildren->begin(); pi != pend; ++pi)
		((CTreeNode*)(*pi))->CalcProbabilities();
	mProbability = mParent == NULL ? 1.0 : (float)mUseCount / mParent->GetChildCntSum();
}
double CTreeNode::CalcCoverProbability(CoverSet *cs) {
	if(cs->GetNumUncovered() == 0)
		return mProbability; // No need to go any deeper, all covered up.

	ctnlist::iterator pi, pend=mChildren->end();
	ItemSet *is;
	uint32 c;
	for(pi=mChildren->begin(); pi != pend; ++pi) {
		is = ((CTreeNode*)(*pi))->GetCTreeSet()->GetItemSet();
		c = cs->Cover(is->GetLength(), is);
		if(c > 0) {
			//printf("\t%.03f => %s\n", mProbability, is->ToString().c_str());
			return mProbability * ((CTreeNode*)(*pi))->CalcCoverProbability(cs);
		}
	}

	// No children found that cover uncovered items, but uncovered items remain
	return 0.0;
}

bool CTreeNode::GenerateItemSet(CoverSet *cset, ItemSet *gset, uint32 &fromCSet, uint32 &useCount, uint32 &numChildren) {
	cset->Cover(mCTreeSet->GetItemSet()->GetLength(),mCTreeSet->GetItemSet());
	gset->Unite(mCTreeSet->GetItemSet());

	if(mVirtual) {
		// deze node is virtueel. Vanaf hier dus verder fantasizen
		useCount = 1;
		numChildren = 1;
		fromCSet = mCTreeSet->GetId();

		return true;

	} else if(mNumChildren == 0) {
		// deze node is niet virtueel, maar wel een eindnode. dus niet verder fantasizen (toch?)
		// eigenlijk moeten we ook stopnodes introduceren
		return false;

	} else {
		// deze node is niet viruteel, en geen eindnode. dus verder traverseren.

		ctnlist::iterator pi, pend=mChildren->end();
		double draw = RandomUtils::UniformDouble();
		double p = 0.0f;
		for(pi=mChildren->begin(); pi != pend; ++pi) {
			if(draw <= p + ((CTreeNode*)(*pi))->GetProbability() ) {
				return ((CTreeNode*)(*pi))->GenerateItemSet(cset, gset, fromCSet, useCount, numChildren);
			}
			p += ((CTreeNode*)(*pi))->GetProbability();
			//printf("draw = %.03f\tp = %.03f\n", draw, p);
		}
		pi--;
		return ((CTreeNode*)(*pi))->GenerateItemSet(cset, gset, fromCSet, useCount, numChildren);
		//throw string("CTreeNode::GenerateItemSet -- Should not arrive here.");
	}
}

void CTreeNode::Print(FILE *fp, uint32 indent) {
	string pset = mCTreeSet->GetItemSet()->ToString(false, false);
	if(pset.length() > 0) {
		//fprintf(fp, "(%d:%s [%d/%d=%.3f]) ", mCTreeSet->GetId(), pset.c_str(), mUseCount, mParent==NULL?0:mParent->GetChildCntSum(), mProbability);

		fprintf(fp, "(%s) ", pset.c_str());
		indent += pset.length() + 3;
	}
	ctnlist::iterator pi, pend=mChildren->end();
	uint32 cur=0;
	for(pi=mChildren->begin(); pi != pend; ++pi) {
		if(cur++ > 0) {
			string s="";
			s.append(indent,' ');
			fprintf(fp,"\n%s",s.c_str()); // en indent!
		}
        ((CTreeNode*)(*pi))->Print(fp, indent);
	}
	//if(cur==0)
	//	fprintf(fp, "\n");
}

#endif // BLOCK_DATAGEN
