#ifndef __LEALGO_H
#define __LEALGO_H

class Database;
class ItemSet;
class LowEntropySet;
class LowEntropySetCollection;
class LECodeTable;

enum LEPruneStrategy {
	LENoPruneStrategy = 0,
	LEPreAcceptPruneStrategy,
	LEPostAcceptPruneStrategy
};
enum LEBitmapStrategy {
	LEBitmapStrategyConstantCT = 0,
	LEBitmapStrategyFullCT,
	LEBitmapStrategyOptimalCT,
	LEBitmapStrategyFullCTPerLength,
	LEBitmapStrategyIndivInstEncoding,
};



#include "LowEntropySet.h"
#include "LECodeTable.h"

class LEAlgo {
public:
	LEAlgo ();
	virtual ~LEAlgo ();

	LECodeTable*	GetCodeTable() { return mCT; }

	virtual void	UseThisStuff(Database *db, LowEntropySetCollection *lesc, LECodeTable *ct, bool fastGenerateDBOccs, LEBitmapStrategy bs);
	virtual void	SetWriteFiles(const bool write);

	virtual LECodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);
	virtual void	WriteEncodedRowLengthsToFile();

	virtual void	SetPruneStrategy(const LEPruneStrategy ps) { mPruneStrategy = ps; }
	//virtual void	SetBitmapStrategy(const LEBitmapStrategy bs) { mBitmapStrategy = bs; }
	void			SetOutputDir(const string &s) { mOutDir = s; }
	void			SetTag(const string &s) { mTag = s; }
	void			SetReportSupDelta(const uint32 d) { mReportSupDelta = d; }
	void			SetReportCndDelta(const uint32 d) { mReportCndDelta = d; }

	//void			LoadCodeTable(const uint32 resumeSup, const uint64 resumeCand);

	string			GetShortName();
	string			GetShortPruneName();
	string			GetOutDir()				{ return mOutDir; }

	/* Static factory method */
	static LEAlgo*	Create(const string &algoname);

protected:
	/* Performs PostDecide on-the-fly pruning. */
	void			PrunePost();

	/* Output and status report methods */
	void			ProgressToScreen(const uint64 cur, const uint64 total);
	/* Write reports and codetable to disk */
	void			ProgressToDisk(const float curSup, const uint64 numCands, const bool writeCodetable, const bool writeStats, const bool writeCover, const string postfix="");

	bool			OpenReportFile(const bool header);
	void			CloseReportFile();

	Database		*mDB;
	LowEntropySetCollection *mLESC;
	LECodeTable		*mCT;
	LEPruneStrategy	mPruneStrategy;
	LEBitmapStrategy mBitmapStrategy;

	int16			mProgress;
	string			mOutDir;
	string			mTag;
	uint32			mReportSupDelta;		// report codetable and stats at each n'th support level
	uint32			mReportCndDelta;		// report codetable at least each n candidates

	FILE			*mReportFile;
	FILE			*mCTLogFile;

	time_t			mStartTime;

	time_t			mScreenReportTime;
	uint64			mScreenReportCandidateIdx;
	uint32			mScreenReportCandidateDelta;
	uint32			mScreenReportCandPerSecond;

	bool			mWriteProgressToDisk;
	bool			mWriteReportFile;
};

#endif // __LEALGO_H
