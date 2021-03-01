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

