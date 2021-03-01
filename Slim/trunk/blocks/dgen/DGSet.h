#ifndef __DGSET_H
#define __DGSET_H

class ItemSet;

class DGSet {
public:
	DGSet();
	// Standard constructor. Provide an Item Set.
	DGSet(ItemSet *is, uint32 id);

	virtual ~DGSet();

	uint32			GetId()			{ return mId; }
	void			SetId(uint32 id){ mId = id; }

	bool			Equals(DGSet *pts);

	ItemSet*		GetItemSet();
	void			SetItemSet(ItemSet *is);

protected:
	ItemSet*		mItemSet;
	uint32			mId;
};

#endif // __DGSET_H
