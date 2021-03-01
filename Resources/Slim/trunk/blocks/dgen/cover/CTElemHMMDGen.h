#ifndef __CTELEMHMMDGEN_H
#define __CTELEMHMMDGEN_H

/* CodeTable Element HMM Data Generator CTElemHMMDGen, type CTEH */

class CodeTable;
class Database;
class ItemSet;
class CoverSet;
class Config;

class DGSet;

#include "../DGen.h"

class CTElemHMMDGen : public DGen {
public:
	CTElemHMMDGen();
	virtual ~CTElemHMMDGen();

	virtual string GetShortName() { return CTElemHMMDGen::GetShortNameStatic(); }
	static string GetShortNameStatic() { return "cteh"; }

	virtual void BuildModel(Database *db, Config *config, const string &ctfname);

	virtual Database* GenerateDatabase(uint32 numRows);

protected:
	uint32 **		mHMMMatrix;
	uint32			mHMMHeight;
	uint32			mHMMWidth;

	ItemSet*		mAbetIset;
	CoverSet*		mAbetCset;
};

#endif // __CTELEMHMMDGEN_H
