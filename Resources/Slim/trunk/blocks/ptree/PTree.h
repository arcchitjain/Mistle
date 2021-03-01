#ifndef __PTREE_H
#define __PTREE_H

class Database;

#include "PTreeNode.h"

class PTree {
public:
	PTree(Database *db);
	virtual ~PTree();
	
	void FindLargestTileFPStylee();

	void Print() const;
	void PrintTree() const;

protected:
	PTreeNode*		mRoot;				// mTree?
	ptnlist**		mSideWalk;

	uint32**		mVDB;

	uint32			mNumRows;
	ItemSet**		mRows;

	uint32**		mItemRowIdxs;
	uint32*			mItemNumRows;

	uint32			mABSize;
	uint16*			mAlphabet;
	uint32*			mABFrequency;
	uint32*			mABUncovered;

	Database*		mDB;
};

#endif // __PTREE_H
