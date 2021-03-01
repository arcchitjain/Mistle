#ifndef __PTREESET_H
#define __PTREESET_H

#include "PTreeNode.h"
class ItemSet;

class PTreeSet {
public:
	PTreeSet();
	// Standard constructor. Provide an item
	PTreeSet(uint16 item, uint32 id);
	// Standard constructor. Provide an Item Set.
	//PTreeSet(ItemSet *is, uint32 id);

	virtual ~PTreeSet();

	uint32			GetId()			{ return mId; }
	void			SetId(uint32 id){ mId = id; }

	bool			Equals(PTreeSet *pts);

	uint16			GetItem();
	void			SetItem(uint16 item);

	uint32			GetFrequency();
	void			SetFrequency(uint32 freq);

	//ItemSet*		GetItemSet();
	//void			SetItemSet(ItemSet *is);

	void			AddUsage(PTreeNode *ptn);
	PTreeNode**		GetUsages();

protected:
	uint16			mItem;
	uint32			mFrequency;

	//ItemSet*		mItemSet;
	uint32			mId;

	ptnlist			*mUsages;
	uint32			mNumUsages;
};

#endif // __PTREESET_H
