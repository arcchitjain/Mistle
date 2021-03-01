#ifndef __PTREENODE_H
#define __PTREENODE_H

class PTreeNode;

typedef std::list<PTreeNode*> ptnlist;

class PTreeNode {
public:
	PTreeNode();
	// Standard constructor. Provide an Itemset and a usage count of how often it has covered in this branch
	PTreeNode(PTreeNode *parent, uint16 item, uint32 freq);

	virtual ~PTreeNode();

	uint16		GetItem();

	uint32		GetFrequency();
	void		SetFrequency(uint32 freq);
	void		IncFrequency(uint32 incr);

	uint32		GetNumCovered();
	void		SetNumCovered(uint32 nc);
	void		IncNumCovered(uint32 inc);

	PTreeNode*	AddSubItem(uint16 item, uint32 count);

	//PTreeNode**	GetChildren();
	ptnlist*	GetChildrenList();
	uint32		GetNumChildren();

	void		Print(string prefix = "") const;
	void		PrintTree(string prefix = "") const;

	void		BuildSideWalk(ptnlist** sideWalk);

protected:
	uint16		mItem;
	uint32		mFrequency;
	uint32		mNumCovered;

	PTreeNode	*mParent;
	ptnlist		*mChildren;
	uint32		mNumChildren;
};

#endif // __PTREENODE_H
