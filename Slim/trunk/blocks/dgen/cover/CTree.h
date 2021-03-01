#ifndef __CTree_H
#define __CTree_H

class CodeTable;
class Database;
class CoverSet;

class CTreeNode;
class CTreeSet;

#include "../DGen.h"

class CTree {
public:
	CTree();
	virtual ~CTree();

	void BuildCodingSets(const string &ctfname);

	void BuildCoverTree(Database *db, const string &ctfname="");
	void BuildDistributionTree(Database *db, const string &ctfile="");

	Database* BuildHMMMatrix(Database *db, const string &ctfname);

	void BDTRecurse(CoverSet *cset, CTreeNode *node, uint32 from);
	void BDTGhostChildrenRecurse(CoverSet *cset, CTreeNode *node, uint32 from);
	void BDTCTPathsRecurse(CoverSet *cset, CTreeNode *node, uint32 from);

	void CalcProbabilities();
	void AddOneToCounts();

	void WriteAnalysisFile(const string &analysisFile);

	void CoverProbabilities(Database *db, string filename);
	Database* GenerateDatabase(uint32 numRows);
	void FantasizeItemSet(CoverSet *cset, ItemSet *gset, uint32 fromCodingSet);

protected:
	CTreeNode*		mRoot;				// mTree?
	CTreeSet*		mRootSet;

	Database*		mDB;

	CTreeSet**		mCodingSets;
	uint32			mNumCodingSets;
	uint32			mCSABStart;
};

#endif // __CTree_H
