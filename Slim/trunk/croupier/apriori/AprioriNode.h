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
