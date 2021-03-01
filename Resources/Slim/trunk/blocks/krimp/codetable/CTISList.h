#ifndef __CTISLIST_H
#define __CTISLIST_H

class CTISList {
public:
	CTISList(CTISList *nxt=NULL);
	~CTISList();

	islist*			GetList() { return mList; }

	CTISList*		GetNext() { return mNextCTISList;	}
	void __inline	SetNext(CTISList* nxt) { mNextCTISList = nxt; }

	/* Resets the counts of all ItemSets in the list to 0 */
	void			ResetUsages();
	/* Resets the counts in the list to 0 from 'start' up till, but -not- including 'stop' */
	void			ResetUsages(islist::iterator start, islist::iterator stop);

	/* Backs up current count of all ItemSets in the list */
	void			BackupUsages();
	/* Backs up current count of ItemSets in the list from 'start' up till, but -not- including 'stop' */
	void			BackupUsages(islist::iterator start, islist::iterator stop);

	/* Rolls back counts of all ItemSets in the list to previousCount */
	void			RollbackUsageCounts();
	/* Rolls back counts of ItemSets in the list to previousCount from 'start' up till, but -not- including 'stop' */
	void			RollbackUsageCounts(islist::iterator start, islist::iterator stop);

	uint32			RemoveZeroUsageCountCodes();

	void			BuildItemSetList(islist *ilist);
	void			BuildPrePruneList(islist *ilist);
	void			BuildPostPruneList(islist *pruneList, const islist::iterator &after);
	void			BuildSanitizePruneList(islist *ilist);

protected:
	CTISList *mNextCTISList;
	islist	 *mList;
};

#endif // __CTISLIST_H
