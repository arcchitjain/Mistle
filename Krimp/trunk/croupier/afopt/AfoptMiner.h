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
#ifndef _AFOPTMINER_H
#define _AFOPTMINER_H

#if defined (_WINDOWS)
	#include <windows.h>
#endif

#include <Primitives.h>
#include <Thread.h>

#include "../clobal.h"
#include "../CroupierApi.h"

#include "Global.h"

class Database;
class Croupier;
class AfoptCroupier;

typedef struct 
{
	int frequency;
	int ntrans;
	int next;
	int item;
} SNODE;

/* ----------------------------------------------------------------------- //
Modified Afopt note:
	support == to be used for minsup, total number of virtual transactions
	trans == total number of physical rows, # occurrences
// ----------------------------------------------------------------------- */

namespace Afopt {
	class CTreeOutBufManager;
	enum FrequentPatternType {
		ALL_FREQUENT_PATTERNS,
		CLOSED_FREQUENT_PATTERNS
	};
	enum MineGoal {
		MINE_GOAL_PATTERNS,
		MINE_GOAL_HISTOGRAM_SUPPORT,
		MINE_GOAL_HISTOGRAM_SETLEN
	};

	class CROUPIER_API AfoptMiner
	{
	public:
		// Mine In Memory
		AfoptMiner(Database *db, AfoptCroupier *cr, const uint32 minSup, const std::string &type, MineGoal goal = MINE_GOAL_PATTERNS);
		~AfoptMiner();
		uint32 GetMinSup() { return mMinSup; }
		void MineOnline();
		void MineHistogram();

		// To file (static!)
		static void	MineAllPatternsToFile(Database *db, const uint32 minSup, const std::string &outputFile);
		static void	MineClosedPatternsToFile(Database *db, const uint32 minSup, const std::string &outputFile);

		string GetPatternTypeStr();

	protected:
		// Always used
		static void MineAll(Database *db, const uint32 minSup);
		static void MineClosed(Database *db, const uint32 minSup);

		// In memory
		Database *mDatabase;
		uint32 mMinSup;
		FrequentPatternType mType;
		string mPatternTypeStr;
	};

	// All
	int bucket_count(int *Buckets, int *tBuckets, int no_of_freqitems, ITEM_COUNTER* pfreqitems);
	void OutputSinglePath(ITEM_COUNTER* pfreqitems, int length);

	// Closed
	int inline PrunedByExistCFI(int nsuffix_len, int nsupport);
	void update_suplen_map(int nsuffix_len, int nsupport);
	int OutputSingleClosePath(int* pitem_sups, int* pitem_ntrans, int length, int nroot_sup);
	int OutputCommonPrefix(int* pitem_sups, int* pitem_ntrans, int length, int *bitmap, int nroot_sup);

	// Generic
	extern int *gpbuckets;
	extern int *gtbuckets;

	// Closed
	extern ITEM_COUNTER *gpfreqitems;
	extern int  gntree_level;
	extern int *gpitem_order_map;
	extern MAP_NODE** gpitem_sup_map;
	extern int gnprune_times;
	extern int gnmap_pruned_times;
	extern CTreeOutBufManager gotree_bufmanager;
}
#endif // _AFOPTMINER_H
