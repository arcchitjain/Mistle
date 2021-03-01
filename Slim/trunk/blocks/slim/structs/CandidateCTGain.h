#ifndef __CANDIDATECTGAIN_H
#define __CANDIDATECTGAIN_H

#include <unordered_map>
#include <Primitives.h>

using namespace std;

class CandidateCTGain;
typedef unordered_map<uint32, CandidateCTGain*> u32cnd_map;

class CandidateCTGain {
public:
	CandidateCTGain(uint32 I, uint32 J, double Heuristic, uint32 new_countsum, uint32 new_usage_ij, uint32 I_rank, uint32 J_rank);
	~CandidateCTGain(void);

	void		AddIteratorI(unordered_map<uint32, CandidateCTGain*> *i_it);
	void		AddIteratorJ(unordered_map<uint32, CandidateCTGain*> *j_it);
	void		RemoveIterators();

//protected:
	uint32		mNewCountSum;
	uint32		mNewUsageIJ;
	double		mActualGain;
	uint32		mI;
	uint32		mJ;
	uint32		mRankI;
	uint32		mRankJ;
	double		mHeuristic;
	unordered_map<uint32, CandidateCTGain*> *mItI;
	unordered_map<uint32, CandidateCTGain*> *mItJ;
};

#endif // __CANDIDATECTGAIN_H
