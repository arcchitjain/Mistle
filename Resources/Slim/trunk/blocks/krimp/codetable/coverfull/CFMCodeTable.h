#define BROKEN

#ifndef __CFMCODETABLE_H
#define __CFMCODETABLE_H

/*
	CFMCodeTable  -  CoverFullMinimal
		Cover each row from DB up till the new code table element. 
		IF it has actually been used, also cover using the rest of the code table.
		Pre- and Post-Prune compatible
*/

#include "CFCodeTable.h"

class CFMCodeTable : public CFCodeTable {
public:
	CFMCodeTable();
	virtual ~CFMCodeTable();

	virtual string		GetShortName() { return "cfm"; }
	virtual string		GetConfigName() { return GetConfigBaseName(); }
	static string		GetConfigBaseName() { return "coverfull-minimal"; }

	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CoverDB(CoverStats &stats);

	virtual void		RollbackCounts();


protected:
	CoverSet**			mCDB;
	uint32				mRollback;
	bool				mRollbackAlph;
};

#endif // __CFMCODETABLE_H

#endif // BROKEN
