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
	int PrunedByExistCFI(int nsuffix_len, int nsupport);
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
