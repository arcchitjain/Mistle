#ifdef BLOCK_SLIM

#include "../global.h"

#include <algorithm>

#include "CandidateCTGain.h"


struct FindCandidate {
	FindCandidate(uint32 i) : toFind(i) { }
	uint32 toFind;
	bool operator() 
		( const std::pair<uint32, CandidateCTGain*> &p ) {
			return p.first == toFind;
	}
};

CandidateCTGain::CandidateCTGain(uint32 I, uint32 J, double Heuristic, uint32 New_countsum, uint32 New_usage_ij, uint32 I_rank, uint32 J_rank){
	mI				= I;
	mJ				= J;
	mHeuristic		= Heuristic;
	mRankI			= I_rank;
	mRankJ			= J_rank;
	mNewUsageIJ	= New_usage_ij;
	mNewCountSum	= New_countsum;
	mActualGain		= 0;
	mItI			= NULL;
	mItJ			= NULL;
}
/*
CandidateCTGain::CandidateCTGain(const CandidateCTGain &obj) {

new_usage_ij	= obj.new_usage_ij;
new_countsum	= obj.new_countsum;
actualGain		= obj.actualGain;
i_it			= obj.i_it;
j_it			= obj.j_it;
}
*/

void CandidateCTGain::AddIteratorI(unordered_map<uint32, CandidateCTGain*> *i_It){
	mItI = i_It;
}

void CandidateCTGain::AddIteratorJ(unordered_map<uint32, CandidateCTGain*> *j_It){
	mItJ = j_It;
}


void CandidateCTGain::RemoveIterators(){
	if(mItI){
		std::unordered_map<uint32, CandidateCTGain*>::iterator it = std::find_if(mItI->begin(), mItI->end(), FindCandidate(mJ));
		if(it != mItI->end()){
			mItI->erase(it);
		}	
	}
	if(mItJ){
		std::unordered_map<uint32, CandidateCTGain*>::iterator it2 = std::find_if(mItJ->begin(), mItJ->end(), FindCandidate(mI));
		if(it2 != mItJ->end()){
			mItJ->erase(it2);
		}
	}
}

CandidateCTGain::~CandidateCTGain(void) {
	RemoveIterators();
}

#endif // BLOCK_SLIM