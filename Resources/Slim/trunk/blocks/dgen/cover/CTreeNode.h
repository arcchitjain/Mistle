#ifndef __CTREENODE_H
#define __CTREENODE_H

class CTreeNode;
class CTreeSet;
class CoverSet;
class DGSet;

typedef std::list<CTreeNode*> ctnlist;

class CTreeNode {
public:
	CTreeNode();
	// Standard constructor. Provide an Itemset and a usage count of how often it has covered in this branch
	CTreeNode(CTreeNode *parent, CTreeSet *cs, uint32 usecnt);

	virtual ~CTreeNode();

	bool		HasCodingSet(CTreeSet *cs);
	CTreeNode*	GetCodingSet(CTreeSet *cs);
	// Standard addition action, let the node see for itself if it knows this itemset or has to create a new node. Update it's usecount accordingly.
	CTreeNode*	AddCodingSet(CTreeSet *cs, uint32 usageCnt);

	CTreeSet*	GetCTreeSet();
	void		SetCTreeSet(CTreeSet *cs, uint32 usecnt);

	uint32		GetUseCount();
	void		AddUseCount(uint32 useCnt);

	uint32		GetNumGhostChildren();
	void		SetNumGhostChildren(uint32 numGhostChildren) { mNumGhostChildren = numGhostChildren; }

	bool		IsVirtual() { return mVirtual; }
	void		SetVirtual(bool v) { mVirtual = v; }

	void		AddOneToCounts();
	void		CalcProbabilities();
	double		CalcCoverProbability(CoverSet *cs);
	double		GetProbability()		{ return mProbability; }
	void		SetProbability(float p)	{ mProbability = p; }

	bool		GenerateItemSet(CoverSet *cset, ItemSet *gset, uint32 &fromCSet, uint32 &useCount, uint32 &numChildren);

	void		Print(FILE *fp, uint32 indent);

protected:
	uint32		GetChildCntSum() { return mChildCntSum; }

	CTreeSet*	mCTreeSet;
	uint32		mUseCount;
	uint32		mChildCntSum;
	double		mProbability;
	float		mStopProb;

	CTreeNode	*mParent;
	ctnlist		*mChildren;
	bool		mVirtual;
	uint32		mNumChildren;
	uint32		mNumGhostChildren;	// zero usecount children
};


//typedef std::map<uint16,uint32,ltalphabet> alphabet;
//typedef std::pair<uint16,uint32> alphabetEntry;


#endif // __CTREENODE_H
