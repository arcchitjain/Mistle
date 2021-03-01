//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#ifndef __ALGO_H
#define __ALGO_H

class Database;
class ItemSet;
class ItemSetCollection;
class CodeTable;

enum PruneStrategy {
	NoPruneStrategy = 0,
	PreAcceptPruneStrategy,
	PostAcceptPruneStrategy,
	SanitizeOfflinePruneStrategy
};

enum ReportType {
	ReportNone = 0,
	ReportCodeTable,
	ReportStats,
	ReportAll
};

#include <itemstructs/ItemSet.h>
#include "CodeTable.h"

class Algo {
public:
	Algo();
	virtual ~Algo();

	CodeTable*		GetCodeTable() { return mCT; }

	virtual void	UseThisStuff(Database *db, ItemSetCollection *isc, ItemSetType type, CTInitType ctinit, bool fastGenerateDBOccs);
	virtual void	SetWriteFiles(const bool write);

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);
	virtual void	WriteEncodedRowLengthsToFile();

	virtual void	SetPruneStrategy(const PruneStrategy ps) { mPruneStrategy = ps; }
	virtual void	SetOutputDir(const string &s) { mOutDir = s; }
	virtual void	SetTag(const string &s) { mTag = s; }

	virtual void	SetReporting(const uint32 repSupDelta, const uint32 repCndDelta, const bool repOnAccept){ mReportSupDelta = repSupDelta; mReportCndDelta=repCndDelta; mReportOnAccept=repOnAccept;}
	virtual void	SetReportStyle(ReportType repSup, ReportType repCnd, ReportType repAcc) { mReportSupType = repSup; mReportCndType = repCnd; mReportAccType = repAcc; }
	virtual void	SetReportSupDelta(const uint32 d) { mReportSupDelta = d; }
	virtual void	SetReportCndDelta(const uint32 d) { mReportCndDelta = d; }
	virtual void	SetReportOnAccept(const bool b) { mReportOnAccept = b; }

	void			LoadCodeTable(const uint32 resumeSup, const uint64 resumeCand);

	string			GetShortName();
	string			GetShortPruneStrategyName();
	string			GetOutDir()				{ return mOutDir; }

	static PruneStrategy	StringToPruneStrategy(string pruneStrategyStr);
	static string	PruneStrategyToString(PruneStrategy pruneStrategy);
	static string	PruneStrategyToName(PruneStrategy pruneStrategy);

	static ReportType StringToReportType(string reportTypeStr);

	/* Static factory method */
	static Algo*	CreateAlgo(const string &algoname, ItemSetType type);

protected:
	/* Performs PreDecide on-the-fly pruning. */
	void			PrunePreAccept(CodeTable *ct, ItemSet *m);
	/* Performs PostDecide on-the-fly pruning. */
	void			PrunePostAccept(CodeTable *ct);
	/* Performs Sanitizing (0-count element removal) */
	void			PruneSanitize(CodeTable *ct);

	/* Output and status report methods */
	void			ProgressToScreen(const uint64 cur, const uint64 total);
	/* Write reports and codetable to disk */
	void			ProgressToDisk(CodeTable *ct, const uint32 curSup, const uint64 numCands, const bool writeCodetable, const bool writeStats);

	bool			OpenReportFile(const bool header);
	void			CloseReportFile();

	void			OpenCTLogFile();
	void			CloseCTLogFile();

	Database		*mDB;
	ItemSetCollection *mISC;
	CodeTable		*mCT;
	PruneStrategy	mPruneStrategy;
	bool			mNeedsOccs;

	int16			mProgress;
	string			mOutDir;
	string			mTag;
	uint32			mReportSupDelta;		// report codetable and stats at each n'th support level
	ReportType		mReportSupType;
	uint32			mReportCndDelta;		// report codetable at least each n candidates
	ReportType		mReportCndType;
	bool			mReportOnAccept;		// report codetable when a new candidate is accepted			
	ReportType		mReportAccType;

	uint64			mNumCandidates;

	FILE			*mReportFile;
	FILE			*mCTLogFile;

	time_t			mStartTime;
	double			mCompressionStartTime;

	time_t			mScreenReportTime;
	uint64			mScreenReportCandidateIdx;
	uint32			mScreenReportCandidateDelta;
	uint32			mScreenReportCandPerSecond;
	char*			mScreenReportTimeEstimateBuf;
	uint32			mScreenReportTimeEstimateBufLen;
	char*			mScreenReportTimeElapsedBuf;
	uint32			mScreenReportTimeElapsedBufLen;

	bool			mWriteProgressToDisk;
	bool			mWriteCTLogFile;
	bool			mWriteReportFile;
};

#endif // __ALGO_H
