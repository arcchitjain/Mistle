#ifdef BLOCK_LESS

#include "../../global.h"

#include <itemstructs/ItemSet.h>
#include "LowEntropySet.h"

#include "LESList.h"

LESList::LESList() {
	mList = new leslist();
}

LESList::~LESList() {
	delete mList;
}

void LESList::ResetCounts() {
	leslist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((LowEntropySet*)(*cit))->ResetCount();
}
void LESList::ResetCounts(leslist::iterator start, leslist::iterator stop) {
	for(leslist::iterator cit=start; cit != stop; ++cit)
		((LowEntropySet*)(*cit))->ResetCount();
}

void LESList::BackupCounts() {
	leslist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((LowEntropySet*)(*cit))->BackupCount();
}
void LESList::BackupCounts(leslist::iterator start, leslist::iterator stop) {
	for(leslist::iterator cit = start; cit != stop; ++cit)
		((LowEntropySet*)(*cit))->BackupCount();
}

void LESList::RollbackCounts() {
	leslist::iterator cit = mList->begin(),cend=mList->end();
	for( ; cit != cend; ++cit)
		((LowEntropySet*)(*cit))->RollbackCount();
}
void LESList::RollbackCounts(leslist::iterator start,leslist::iterator stop) {
	for(leslist::iterator cit = start ; cit != stop; ++cit)
		((LowEntropySet*)(*cit))->RollbackCount();
}

uint32 LESList::RemoveZeroCountCodes() {
	uint32 c = 0;
	leslist::iterator cit = mList->begin(),cend=mList->end(), cache;
	while(cit != cend) {
		cache = cit;
		++cit;
		if(((LowEntropySet*)(*cache))->GetCount() == 0) {
			delete *cache;
			mList->erase(cache);
			++c;
		}
	}
	return c;
}

void LESList::BuildLowEntropySetList(leslist *llist) {
	leslist::iterator ci, cend=mList->end();
	for(ci=mList->begin(); ci != cend; ++ci)
		llist->push_back(*ci);
}

void LESList::BuildPostPruneList(leslist *pruneList, const leslist::iterator &after) {
	uint32 cnt;
	leslist::iterator lstart, iter, lend1, lend2;
	LowEntropySet *les1;
	lend1 = mList->end();
	bool insert;
	for(leslist::iterator it = mList->begin(); it != lend1; it++) {
		les1 = (LowEntropySet*)(*it);
		uint32 la = 10;
		//if((cnt = les1->GetCount()) < les1->GetPrevCount() || les1->GetCount() == 0) {
		if((cnt = les1->GetCount()) < les1->GetPrevCount()) {
			lend2 = pruneList->end();
			//printf("Add to prunelist: %s (%d)\n", is1->ToString().c_str(), is1->GetPrevCount());
			insert = true;
			iter = after;

			if(iter != lend2) {
				for(++iter; iter != lend2; ++iter)
					if(*iter == les1) {
						insert = false;
						break;
					}
			}
			if(insert) {
				iter = after;
				if(iter != lend2) {
					for(++iter; iter != lend2; ++iter)
						if((*iter)->GetCount() > cnt)
							break;
				}
				pruneList->insert(iter, les1);
			}
		}
	}
}

#endif // BLOCK_LESS
