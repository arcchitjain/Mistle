#ifndef _MINER_H
#define _MINER_H

#include "Global.h"	

namespace Afopt {
	class CScanDBMine
	{
	public:
		int ScanDBCountFreqItems(ITEM_COUNTER **ppfreqitems);

		void ScanDBBuildCondDB(HEADER_TABLE pheader_table, int *pitem_order_map, int num_of_freqitems, int *Buckets, int *tBuckets);
		void ScanDBBuildCondDB(HEADER_TABLE pheader_table, int *pitem_order_map);

		void ScanDBBucketCount(int *pitem_order_map, int *Buckets, int *tBuckets);

	};

	extern CScanDBMine  goDBMiner;
}
#endif

