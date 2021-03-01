#ifndef __CTREEDGEN_H
#define __CTREEDGEN_H

/* CoverTree DataGenerator CTreeDGen */

class CodeTable;
class Database;
class CoverSet;
class Config;

class CTreeNode;
class CTreeSet;

class DGSet;

#include "../DGen.h"

class CTreeDGen : public DGen {

public:
	CTreeDGen();
	virtual ~CTreeDGen();

	virtual string GetShortName() { return CTreeDGen::GetShortNameStatic(); }
	static string GetShortNameStatic() { return "ctree"; }

	virtual void BuildModel(Database *db, Config *config, const string &ctfname);

	virtual Database* GenerateDatabase(uint32 numRows);

protected:
	virtual void BuildCodingSets(const string &ctfname);

	CTreeNode*		mRoot;				// mTree?
	CTreeSet*		mRootSet;
};

#endif // __CTREEDGEN_H
