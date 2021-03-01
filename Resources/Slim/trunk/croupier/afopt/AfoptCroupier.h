#ifndef _AFOPTCROUPIER_H
#define _AFOPTCROUPIER_H

#if defined (_WINDOWS)
	#include <Windows.h>
#endif
#include <isc/IscFile.h>

#if defined (__GNUC__)
	#include <isc/ItemSetCollection.h>
#endif

#include "../Croupier.h"

enum IscOrderType;
class Thread;
namespace Afopt {
	class AfoptMiner;
	class MemoryOut;
	class HistogramOut;
}
class CROUPIER_API AfoptCroupier : public Croupier
{
public:
	AfoptCroupier(Config *config);
	AfoptCroupier(const string &iscName);
	AfoptCroupier();
	virtual ~AfoptCroupier();

	/* --- Online Dealing (pass patterns in memory) --- */
	virtual bool	CanDealOnline()					{ return true; }	// false, as mining online currently corrupts memory

	/* --- Offline Dealing (patterns to file) --- */
	virtual string	BuildOutputFilename();

	virtual bool	MinePatternsToFile(const string &dbFilename, const string &outputFullPath, Database *db = NULL);

	virtual bool	MyMinePatternsToFile(const string &dbFilename, const string &outputFullPath);
	virtual bool	MyMinePatternsToFile(Database *db, const string &outputFullPath);

	virtual ItemSetCollection* MinePatternsToMemory(Database *db);

	virtual uint64*	MineHistogram(Database *db, const bool setLengthMode);
	virtual void	HistogramMiningIsHistoryCBFunc(uint64 * const histogram, uint32 histolen);

	virtual string	GetPatternTypeStr();

protected:
	virtual void	InitOnline(Database *db);
	virtual void	InitHistogramMining(Database *db, const bool setLengthMode);
	virtual void	SetIscProperties(ItemSetCollection *isc);

	virtual void	MinerIsErVolVanCBFunc(ItemSet **iss, uint32);
	virtual void	MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets);

	void		Configure(const string &iscName);

	string		mType;
	float		mMinSup;
	IscOrderType mOrder;
	string		mMinSupMode;

	Afopt::AfoptMiner	*mMiner;
	Afopt::MemoryOut	*mMinerOut;
	Afopt::HistogramOut *mHistoOut;
	bool		mMayHaveMore;

	friend class Afopt::MemoryOut;
//	friend class Croupier;

};

#endif // _AFOPTCROUPIER_H
