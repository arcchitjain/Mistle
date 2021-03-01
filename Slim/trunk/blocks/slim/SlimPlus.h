#ifndef __SLIMPLUS_H
#define __SLIMPLUS_H

#include <queue>
#include <unordered_map>

#include "SlimAlgo.h"

class SlimPlus: public SlimAlgo {

// SlimPlus
	// Slim with additional scoring heuristics

public:

	SlimPlus(CodeTable* ct, HashPolicyType hashPolicy, Config* config);
	~SlimPlus();

	virtual string	GetShortName();

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

	void UpdateCandidates();

private:

	static bool compareCandidates(const ItemSet* a, const ItemSet* b);

	enum ScoreItemSetType { EXACT, SUPPORT_DB, SUPPORT_CS};
	ScoreItemSetType mScoreItemSetType;
	double ScoreItemSet(const ItemSet* is);

	typedef unordered_map<const ItemSet*, double> CandidatesScores;
	static CandidatesScores mCandidatesScores;

	typedef priority_queue<const ItemSet*, vector<const ItemSet*>, bool (*)(const ItemSet*, const ItemSet*)> Candidates;
	Candidates mCandidates;

	ItemSet* mAcceptedCandidate;

	islist* mSingletonList;
	islist* mItemSetList;

	uint32 mMinSup;
};


#endif // __SLIMPLUS_H
