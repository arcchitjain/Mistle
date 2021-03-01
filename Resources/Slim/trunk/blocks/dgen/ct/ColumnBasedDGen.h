#ifndef __COLUMNBASEDDGEN_H
#define __COLUMNBASEDDGEN_H

/* Column Based Data Generator CBDGen */

class CodeTable;
class Database;
class CoverSet;
class Config;

class DGSet;

#include "../DGen.h"

class ColumnBasedDGen : public DGen {
public:
	ColumnBasedDGen();
	virtual ~ColumnBasedDGen();

	virtual string GetShortName() { return ColumnBasedDGen::GetShortNameStatic(); }
	static string GetShortNameStatic() { return "cb"; }

	virtual void BuildModel(Database *db, Config *config, const string &ctfname);
	virtual void BuildModel(Database *db, Config *config, CodeTable *ct);

	virtual Database* GenerateDatabase(uint32 numRows);

protected:
	virtual void BuildModelAf(Database *db, Config *config);

	uint32 **mHMMMatrix;
};

#endif // __COLUMNBASEDDGEN_H
