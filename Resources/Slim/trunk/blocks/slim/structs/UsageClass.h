#ifndef __USAGECLASS_H
#define __USAGECLASS_H

#include <unordered_map>

#include "CandidateCTGain.h"

class ItemSet;

class UsageClass {
public:
	UsageClass();
	UsageClass(ItemSet *itemSet1);
	UsageClass(ItemSet *itemSet1, uint32 usageCount);
	~UsageClass();

	void		SetUsageCount(uint32 usageCount);
	void		UpdateUsageCount();
	uint32		GetUsageCount() const;
	uint32		GetID();
	void		SetItemSet(ItemSet *is);
	ItemSet*	GetItemSet();
	uint32*		GetUsages();
	double		GetStandardLength() ;
	bool		UsageChanged();
	int8		UsageDropped();
	void		AddTopCandidateI(uint32 j, CandidateCTGain *candidate);
	void		AddTopCandidateJ(uint32 j, CandidateCTGain *candidate);
	bool		HeuristicAvailable(uint32 j, uint32 i_rank, uint32 j_rank);
	//void		RemoveTopCandidates(uint32* usageChanged, uint32 count);
	//void		RemoveAllTopCandidates();
	//void		RemoveTopCandidate(uint32 j);
	
private:

	uint32		mUsageCount;
	ItemSet		*mItemSet;
	unordered_map<uint32, CandidateCTGain*> *mMyMap;
};

#endif // __USAGECLASS_H
