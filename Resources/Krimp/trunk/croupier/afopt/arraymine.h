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
#ifndef _ARRAY_H
#define _ARRAY_H

#include "Global.h"
//#include "max/patternsetmanager.h"
#include "closed/cfptree_outbuf.h"

namespace Afopt {

	class CArrayMine
	{
	public:
		void DepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems);
		void CFIDepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, CFP_NODE *pcfp_node);
#ifdef BROKEN
		void MFIDepthArrayGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, LOCAL_PATTERN_SET *psubexistmfi, LOCAL_PATTERN_SET *pnewmfi, CHash_Map *pitem_order_map);
#endif // BROKEN

		void CountFreqItems(TRANS_HEAD_NODE *pcond_db, int* pitem_sup_map, int* pitem_ntrans);
		void BuildNewCondDB(TRANS_HEAD_NODE *pcond_db, HEADER_TABLE pnewheader_table, int* pitem_order_map, int num_of_freqitems, int *Buckets, int *tBuckets);
		//void BuildNewCondDB(TRANS_HEAD_NODE *pcond_db, HEADER_TABLE pnewheader_table, int* pitem_order_map);
		void PushRight(HEADER_TABLE pheader_table, int item_pos, int max_push_pos);
		void BucketCount(TRANS_HEAD_NODE *pcond_db, int *pitem_order_map, int *Buckets, int *tBuckets);

	};

	extern CArrayMine goArrayMiner;
}
#endif

