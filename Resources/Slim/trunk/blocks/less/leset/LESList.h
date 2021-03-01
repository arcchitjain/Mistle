#ifndef __LESLIST_H
#define __LESLIST_H

#include "LowEntropySet.h"

class LESList {
public:
	LESList();
	~LESList();

	leslist*		GetList() { return mList; }

	/* Resets the counts of all ItemSets in the list to 0 */
	void			ResetCounts();
	/* Resets the counts in the list to 0 from 'start' up till, but -not- including 'stop' */
	void			ResetCounts(leslist::iterator start, leslist::iterator stop);

	/* Backs up current count of all ItemSets in the list */
	void			BackupCounts();
	/* Backs up current count of ItemSets in the list from 'start' up till, but -not- including 'stop' */
	void			BackupCounts(leslist::iterator start, leslist::iterator stop);

	/* Rolls back counts of all ItemSets in the list to previousCount */
	void			RollbackCounts();
	/* Rolls back counts of ItemSets in the list to previousCount from 'start' up till, but -not- including 'stop' */
	void			RollbackCounts(leslist::iterator start, leslist::iterator stop);

	uint32			RemoveZeroCountCodes();

	void			BuildLowEntropySetList(leslist *ilist);
	void			BuildPostPruneList(leslist *pruneList, const leslist::iterator &after);

protected:
	leslist	 *mList;
};

#endif // __LESLIST_H
