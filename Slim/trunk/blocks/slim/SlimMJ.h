#ifndef __SlimMJ_H
#define __SlimMJ_H

#include <unordered_set>

#include "SlimAlgo.h"
#include "structs/ItemSetSorted.h"
#include "codetable/CCCPUCodeTable.h"
// Slim in Cover Space, with candidate caching
//  estimates Candidate Gain using usage counts, not just frequencies, not just by blindly testing all pairs

class Codetable;

enum SlimMJEstStrategy { SlimMJ_USAGE, SlimMJ_GAIN };

class SlimMJ : public SlimAlgo {

// Slim, Manan and Jilles-style

public:

	SlimMJ(CodeTable* ct, HashPolicyType hashPolicy, Config* config);
	~SlimMJ();

	virtual CodeTable*	DoeJeDing(const uint64 candidateOffset=0, const uint32 startSup=0);

protected:
	//void				GenerateCandidates(bool cache /* =1 */);  //calculates Heuristic and reuse all the Heuristic information it has from previous iteration, if bool = true, otherwise completely re-calculates Heuristic for next iteration
	//virtual void		EstimateQualityUsage(const bool cache = 1);
	//virtual void		EstimateQualityGain(const bool cache = 1);


	void				UpdateLocalCTStuff(std::list<uint32> deletedIDs);
	ItemSet*			NextCandidate(double& heuristic, uint32 &candidateCount);

	virtual list<uint32> PrunePostAccept(CodeTable *ct);

	void				FillCodeTable();//MG
	void				GenerateCandidates();//calculates Heuristic and reuse all the Heuristic information it has from previous iteration.

	uint32*			mUsgChangedIds;	// auxiliary array to keep track of which entries were changed
	uint32			mUsgChangedIdsLen;
	bool*			mUsgChangedLU;	// auxiliary array to keep track of which entries were changed
	uint32			mUsgChangedLULen;

	//////////////////////////////////////////////////////////////////////////
	// Optimization counters
	//////////////////////////////////////////////////////////////////////////
	/*uint32			mOMGCounter;
	uint32			mLOLCounter;
	uint32			mCopterCounter;
	uint32			mROFLCounter;*/

	uint32			mMinSup;
	double			mMaxTime;

	double			mMinGain;			// lower bound for actual gain
	double			mMinGainEst;		// lower bound for estimated gain
	double			mCurMinGainEst;		

	uint32			mCurNumCTElems;

	vector<CandidateCTGain*> mTopCandidates;
	unordered_map<uint32,unordered_set<uint32>> *mNoCa;
	unordered_map<uint32,unordered_set<uint32>> *mBaCa;

	//counter for giving unique ID to each itemSet.
	uint32			mItemSetCount;

	ItemSetSorted	*mItemSetSorted;

	double			mPrevCountSum;
	uint32			mPrevNumCTElems;

	CandidateCTGain		*mCurrentCandidate;//current top candidate which is being merged.

	SlimMJEstStrategy mEstStrategy;

private:
	CCCPUCodeTable<BM128ItemSetType> *mCTBm128;
	CCCPUCodeTable<BAI32ItemSetType> *mCTBai32;
	CCCPUCodeTable<Uint16ItemSetType> *mCTUint16;
	double bitGain;
};


#endif // #define __SLIMMJ_H

