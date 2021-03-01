#ifndef __SLIMALGO_H
#define __SLIMALGO_H

#include "../krimp/KrimpAlgo.h"

#include <Config.h>

enum HashPolicyType { // specify hash policy
	hashNoCandidates=0,	// 0	n	no check
	hashAllCandidates,	// 1	a	all itemsets previously added
	hashCtCandidates,	// 2	c	only itemsets in current ct		// TODO: still need to be implemented!
};


class SlimAlgo: public KrimpAlgo {

public:

	SlimAlgo(CodeTable* ct, HashPolicyType hashPolicy, Config* config);

	virtual string	GetShortName();

	virtual void	SetReportIter(const bool b) { mReportIteration = b; }
	virtual void	SetReportIterStyle(ReportType repIter) { mReportIterType = repIter; }

	void			SetResumeCodeTable(const string& ctFile) { mCTFile = ctFile; }
	void			LoadCodeTable(const string& ctFile);

	void			OpenLogFile();
	void			CloseLogFile();

	/* Static factory method */
	static KrimpAlgo*	CreateAlgo(const string &algoname, ItemSetType type, Config* config);

	static string	HashPolicyTypeToString(HashPolicyType type);
	static string	StringToHashPolicyType(string algoname, HashPolicyType& hashPolicy);

	virtual void	ProgressToScreen(const uint64 curCandidate, CodeTable *ct);

protected:
	//static uint32		CountUsageMatches(const uint32* usage_x, const uint32 usage_cnt_x, const uint32* usage_y, const uint32 usage_cnt_y, const uint32 minUsage);

	bool			mWriteLogFile;

	FILE			*mLogFile;
	HashPolicyType 	mHashPolicy;

	bool			mReportIteration;		// report codetable each iteration
	ReportType		mReportIterType;

	string			mCTFile;

	// reportToScreen
	double mScreenReportBits;

};

#endif // __SLIMALGO_H
