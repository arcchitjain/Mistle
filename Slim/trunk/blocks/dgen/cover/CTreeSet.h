#ifndef __CTREESET_H
#define __CTREESET_H

class ItemSet;

#include "CTreeNode.h"		// voor ctnlist
#include "../DGSet.h"

class CTreeSet : public DGSet {
public:
	CTreeSet();
	// Standard constructor. Provide an Item Set.
	CTreeSet(ItemSet *is, uint32 id);

	virtual ~CTreeSet();

	//bool			Equals(CTreeSet *pts);

	void			AddUsage(CTreeNode *ctn);
	CTreeNode**		GetUsages();

protected:

	ctnlist			*mUsages;
	uint32			mNumUsages;
};

#endif // __CTREESET_H
