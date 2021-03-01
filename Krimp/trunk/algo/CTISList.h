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
	void			ResetCounts();
	/* Resets the counts in the list to 0 from 'start' up till, but -not- including 'stop' */
	void			ResetCounts(islist::iterator start, islist::iterator stop);

	/* Backs up current count of all ItemSets in the list */
	void			BackupCounts();
	/* Backs up current count of ItemSets in the list from 'start' up till, but -not- including 'stop' */
	void			BackupCounts(islist::iterator start, islist::iterator stop);

	/* Rolls back counts of all ItemSets in the list to previousCount */
	void			RollbackCounts();
	/* Rolls back counts of ItemSets in the list to previousCount from 'start' up till, but -not- including 'stop' */
	void			RollbackCounts(islist::iterator start, islist::iterator stop);

	uint32			RemoveZeroCountCodes();

	void			BuildItemSetList(islist *ilist);
	void			BuildPrePruneList(islist *ilist);
	void			BuildPostPruneList(islist *pruneList, const islist::iterator &after);
	void			BuildSanitizePruneList(islist *ilist);

protected:
	CTISList *mNextCTISList;
	islist	 *mList;
};

#endif // __CTISLIST_H
