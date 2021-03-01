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

void CTISList::ResetCounts() {
	islist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((ItemSet*)(*cit))->ResetUsage();
}
void CTISList::ResetCounts(islist::iterator start, islist::iterator stop) {
	for(islist::iterator cit=start; cit != stop; ++cit)
		((ItemSet*)(*cit))->ResetUsage();
}

void CTISList::BackupCounts() {
	islist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((ItemSet*)(*cit))->BackupUsage();
}
void CTISList::BackupCounts(islist::iterator start, islist::iterator stop) {
	for(islist::iterator cit = start; cit != stop; ++cit)
		((ItemSet*)(*cit))->BackupUsage();
}

void CTISList::RollbackCounts() {
	islist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((ItemSet*)(*cit))->RollbackUsage();
}
void CTISList::RollbackCounts(islist::iterator start,islist::iterator stop) {
	for(islist::iterator cit = start ; cit != stop; ++cit)
		((ItemSet*)(*cit))->RollbackUsage();
}

uint32 CTISList::RemoveZeroCountCodes() {
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
