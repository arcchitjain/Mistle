#ifndef __CCPUCODETABLE_H
#define __CCPUCODETABLE_H

// Voorheen het snelste cover-algoritme. Ingehaald door Sander Schuckman's Cache-Conscious implementatie (CCCPCodeTable)

class CTISList;
class CoverSet;
class ItemSet;

#include "../../krimp/codetable/coverpartial/CCPCodeTable.h"

class CCPUCodeTable : public CCPCodeTable {
public:
	CCPUCodeTable();
	CCPUCodeTable(const CCPUCodeTable &ct);
	virtual ~CCPUCodeTable();
	virtual CCPUCodeTable* Clone() { return new CCPUCodeTable(*this); }

	virtual string		GetShortName() { return "ccpu"; }
	virtual string		GetConfigName() { return CCPUCodeTable::GetConfigBaseName(); };
	static string		GetConfigBaseName() { return "ccoverpartial-usg"; }

	virtual bool		NeedsDBOccurrences() { return true; }
	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);

	virtual void		CoverDB(CoverStats &stats);

protected:
	virtual void		CoverNoBinsDB(CoverStats &stats);
	virtual void		CoverBinnedDB(CoverStats &stats);

	//virtual void		SyncUsageChanges();

	virtual void		CommitAdd(bool updateLog=true);
	virtual void		CommitLastDel(bool zap, bool updateLog=true);
	virtual void		RollbackAdd();
	virtual void		RollbackLastDel();
	virtual void		RollbackCounts();

	virtual void		SetAlphabetCount(uint32 item, uint32 count);
	virtual uint32		GetAlphabetCount(uint32 item);

	friend class SlimCS;
};

#endif // __CCPUCODETABLE_H
