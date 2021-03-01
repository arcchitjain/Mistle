#include "../global.h"

#include <itemstructs/ItemSet.h>

#include "CTISList.h"

CTISList::CTISList(CTISList *nxt) {
	mNextCTISList = nxt;
	mList = new islist();
}

CTISList::~CTISList() {
	delete mList;
}

void CTISList::ResetUsages() {
	islist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((ItemSet*)(*cit))->ResetUsage();
}
void CTISList::ResetUsages(islist::iterator start, islist::iterator stop) {
	for(islist::iterator cit=start; cit != stop; ++cit)
		((ItemSet*)(*cit))->ResetUsage();
}

void CTISList::BackupUsages() {
	islist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((ItemSet*)(*cit))->BackupUsage();
}
void CTISList::BackupUsages(islist::iterator start, islist::iterator stop) {
	for(islist::iterator cit = start; cit != stop; ++cit)
		((ItemSet*)(*cit))->BackupUsage();
}

void CTISList::RollbackUsageCounts() {
	islist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((ItemSet*)(*cit))->RollbackUsage();
}
void CTISList::RollbackUsageCounts(islist::iterator start,islist::iterator stop) {
	for(islist::iterator cit = start ; cit != stop; ++cit)
		((ItemSet*)(*cit))->RollbackUsage();
}

uint32 CTISList::RemoveZeroUsageCountCodes() {
	uint32 c = 0;
	islist::iterator cit = mList->begin(),cend=mList->end(), cache;
	while(cit != cend) {
		cache = cit;
		++cit;
		if(((ItemSet*)(*cache))->GetUsageCount() == 0) {
			delete *cache;
			mList->erase(cache);
			++c;
		}
	}
	return c;
}

void CTISList::BuildItemSetList(islist *ilist) {
	islist::iterator ci, cend=mList->end();
	for(ci=mList->begin(); ci != cend; ++ci)
		ilist->push_back(*ci);
}

void CTISList::BuildSanitizePruneList(islist *ilist) {
	islist::iterator ci, cend=mList->end();
	for(ci=mList->begin(); ci != cend; ++ci) {
		if((ItemSet*)(*ci)->GetUsageCount() == 0)
			ilist->push_back(*ci);
	}
}

void CTISList::BuildPrePruneList(islist *ilist) {
	if(mList->size() == 0)
		return;
	uint32 cnt;
	islist::iterator ins, first, itend;
	ItemSet *is;
	first = mList->begin();
	ins = ilist->begin();
	islist::iterator it = mList->end();
	--it;
	while(true) {
		is = (ItemSet*)(*it);
		cnt = is->GetUsageCount();
		itend = ilist->end();
		for(; ins != itend; ins++) {
			if((((ItemSet*)(*ins))->GetUsageCount()) > cnt)
				break;
		}
		ilist->insert(ins, is);
		if(it == first)
			break;
		--it;
	}
}

void CTISList::BuildPostPruneList(islist *pruneList, const islist::iterator &after) {
	uint32 cnt;
	islist::iterator lstart, iter, lend1, lend2;
	ItemSet *is1;
	lend1 = mList->end();
	bool insert;
	for(islist::iterator it = mList->begin(); it != lend1; it++) {
		is1 = (ItemSet*)(*it);
		if((cnt = is1->GetUsageCount()) < is1->GetPrevUsageCount()) {
			lend2 = pruneList->end();
			//printf("Add to prunelist: %s (%d)\n", is1->ToString().c_str(), is1->GetPrevCount());
			insert = true;
			iter = after;

			if(iter != lend2)
				for(++iter; iter != lend2; ++iter)
					if(*iter == is1) {
						insert = false;
						break;
					}
			if(insert) {
				iter = after;
				if(iter != lend2) {
					for(++iter; iter != lend2; ++iter)
						if((*iter)->GetUsageCount() > cnt)
							break;
				}
				pruneList->insert(iter, is1);
			}
		}
	}
}
