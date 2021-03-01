#ifndef _APRIORITREE_H
#define _APRIORITREE_H

#if defined (_WINDOWS)
	#include <Windows.h>
#endif

class AprioriNode;
class AprioriCroupier;
class Database;
class ItemSet;

class AprioriTree
{
public:
	AprioriTree(AprioriNode *root, Database *database, const uint32 minSup);
	virtual ~AprioriTree();

	bool			AddLevel();
	void			MineItemSets(AprioriCroupier *croupier, ItemSet **buffer, const uint32 bufferSize);

protected:
	AprioriNode		*mRoot;
	Database		*mDatabase;
	uint32			mMinSup;

	AprioriNode		**mLevels;
	uint32			mNumLevels;

	// For processing
	uint16			*mSet;
};

#endif // _APRIORITREE_H
