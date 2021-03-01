#ifndef __CPUACODETABLE_H
#define __CPUACODETABLE_H

class CTISList;
class CoverSet;

#include "../../krimp/codetable/coverpartial/CPOCodeTable.h"

typedef uint32 (CoverSet::*CoverFunction) (uint32, ItemSet*);
typedef bool (CoverSet::*CanCoverFunction) (uint32, ItemSet*);

class CPUOCodeTable : public CPOCodeTable {
public:
	CPUOCodeTable();
	CPUOCodeTable(const string &name);
	virtual ~CPUOCodeTable();
	virtual CPUOCodeTable* Clone() { THROW_NOT_IMPLEMENTED(); }

	virtual string		GetShortName();
	virtual string		GetConfigName() { return CPUOCodeTable::GetConfigBaseName(); };
	static string		GetConfigBaseName() { return "coverpartial-usg-orderly"; }

	virtual bool		NeedsDBOccurrences() { return true; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);


	virtual void		RollbackCounts();


protected:
	virtual void		CoverNoBinsDB(CoverStats &stats);
	virtual void		CoverBinnedDB(CoverStats &stats);

	virtual uint32*		GetAlphabetCounts() { return mAlphabet; }
	virtual uint32*		GetAlphabetUsage(uint32 idx) { return mAlphabetUsages[idx]; }

	friend class SlimCS;
	friend class SlimMG;
};

#endif // __CPUOCODETABLE_H
