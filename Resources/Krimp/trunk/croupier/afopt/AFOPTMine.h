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
#ifndef _AFOPT_H
#define _AFOPT_H

#include "Global.h"
//#include "max/patternsetmanager.h"
#include "closed/cfptree_outbuf.h"

namespace Afopt {

	//Tree traversal is implemented using stacks; STACK_NODE defines information to be kept in a stack node
	typedef struct
	{
		int ntrans;
		int frequency;
		int flag;
		AFOPT_NODE *pnext_child;
		AFOPT_NODE *pnode;
		AFOPT_NODE *pprev_child;
	} STACK_NODE;
	typedef STACK_NODE* STACK;

	class CAFOPTMine
	{
	public:

		STACK mpsource_stack, mpdest_stack;
		int *mpbranch, *mpfrequencies, *mptransaction, *mpntrans;

		void Init(int nmax_depth);
		void CleanUp();
		CAFOPTMine();
		~CAFOPTMine();

		void InsertTransaction(HEADER_TABLE pheader_table, int nitem_pos, ARRAY_NODE* ptransaction, int length);
		void InsertTransaction(HEADER_TABLE pheader_table, int nitem_pos, int* ptransaction, int length, int frequency, int ntrans);

		void InsertTransaction(AFOPT_NODE *ptreeroot, ARRAY_NODE* ptransaction, int length);
		//	void InsertTransaction(AFOPT_NODE *proot, int* ptransaction, int length, int frequency, int ntrans);

		void DestroyTree(AFOPT_NODE * proot);
		int  TraverseTree(AFOPT_NODE * proot);

		void DepthAFOPTGrowthAll(HEADER_TABLE pheader_table, int num_of_freqitems, int nstart_pos);
		void CFIDepthAFOPTGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, int nstart_pos, CFP_NODE *pcfp_node);
		//int MFIDepthAFOPTGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, int nstart_pos, LOCAL_PATTERN_SET *psubexistmfi, LOCAL_PATTERN_SET *pnewmfi, CHash_Map *pitem_order_map);

		void CountFreqItems(AFOPT_NODE* proot, int *pitem_sup_map, int *pitem_ntrans);
		void BuildNewTree(AFOPT_NODE *proot, HEADER_TABLE pnewheader_table, int *pitem_order_map, int num_of_newfreqitems, int *Buckets, int *tBuckets);
		void BuildNewTree(AFOPT_NODE *proot, HEADER_TABLE pnewheader_table, int *pitem_order_map);
		void PushRight(HEADER_TABLE pheader_table, int nitem_pos, int nmax_push_pos);
		void MergeTwoTree(AFOPT_NODE *pdest_root, AFOPT_NODE *psource_root);
		void BucketCountAFOPT(AFOPT_NODE *proot, int *pitem_order_map, int *Buckets, int *tBuckets);

	};

	extern CAFOPTMine goAFOPTMiner;

}
#endif
