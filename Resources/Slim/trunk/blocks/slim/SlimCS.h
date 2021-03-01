#ifndef __SLIMCS_H
#define __SLIMCS_H

#include "SlimAlgo.h"

class Codetable;
class CCPUCodeTable;
class CPUOCodeTable;
class CPAOCodeTable;

struct Candidate {
	uint32 i;
	uint32 j;
	double heuristic;
	double tmp; // we need a temporary value to store the not yet complete gain_ct_ij;
	Candidate(uint32 i, uint32 j, double heuristic, double tmp = 0.0) : i(i), j(j), heuristic(heuristic), tmp(tmp) {};
	friend bool operator< (const Candidate &x, const Candidate &y);
	friend bool operator> (const Candidate &x, const Candidate &y);
	friend bool operator== (const Candidate &x, const Candidate &y);
};

bool operator< (const Candidate &x, const Candidate &y);
bool operator> (const Candidate &x, const Candidate &y);
bool operator== (const Candidate &x, const Candidate &y);

enum slim_est_strategy { SLIM_USAGE, SLIM_GAIN, SLIM_NORMALIZED_GAIN, SLIM_NORMALIZED_GAIN_CT};

class SlimCS: public SlimAlgo {

// Slim in cover space

public:

	SlimCS(CodeTable* ct, HashPolicyType hashPolicy, Config* config);
	~SlimCS();

	virtual string	GetShortName();

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

protected:
	void				EstimateQualityOnUsage();	// simple
	void				EstimateQualityOnLocalGain();	// probably the real thing

	void				EstimateQualityOnGlobalGainDB();		// testing ground
	void				EstimateQualityOnGlobalGain();	


	void				UpdateLocalCTStuff();
	ItemSet*			NextCandidate(double& heuristic);
	//static uint32		CountUsageMatches(uint32* usage_x, uint32 usage_cnt_x, uint32* usage_y, uint32 usage_cnt_y, uint32 minUsage);
	void				ProgressToScreen(const uint64 curCandidate, CodeTable *ct);


	uint32 mMinSup;

	// Local CT stuff
	CPUOCodeTable* mFriendlyCPUO;
	CPAOCodeTable* mFriendlyCPAO;
	CCPUCodeTable* mFriendlyCCPU;

	uint32 mCurNumCTElems;
	uint32* mUsageCounts; // Usage count of both code table elements and alphabet
	uint32* mOrigIdx; //
	uint32* mAlphabetIdx; // AlphabetIdx or AlphabetSize, which reflects precense of itemset
	ItemSet** mItemSetIdx; // Pointer to itemset or null
	double* mStdLengths;

	uint32 mCandidate_i;
	uint32 mCandidate_j;
	list<uint32>* mRejectedCandidates; // maybe use array
	vector<Candidate> mTopCandidates;
	bool mMoreCandidates;

	double mScreenReportBits;

	slim_est_strategy mEstStrategy;
	uint32 mNumTopCandidates;

	double mMaxTime;
};

#endif //__SLIMCS_H