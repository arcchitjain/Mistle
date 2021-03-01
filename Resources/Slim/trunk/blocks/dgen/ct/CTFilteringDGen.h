#ifndef __CTFILTERINGDGEN_H
#define __CTFILTERINGDGEN_H

/* CodeTable Filtering Data Generator :: CtFilteringDGen */

class CodeTable;
class Database;
class CoverSet;
class Config;

class DGSet;

#include "../DGen.h"

class CTFilteringDGen : public DGen {
public:
	CTFilteringDGen();
	virtual ~CTFilteringDGen();

	virtual string GetShortName() { return CTFilteringDGen::GetShortNameStatic(); }
	static string GetShortNameStatic() { return "ctf"; }

	virtual void BuildModel(Database *db, Config *config, const string &ctfname);

	virtual Database* GenerateDatabase(uint32 numRows);

protected:
	uint32*			mHMM;
	uint32			mSum;
};

#endif // __CTFILTERINGDGEN_H
