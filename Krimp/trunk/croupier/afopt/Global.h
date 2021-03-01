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
#ifndef _GLOBAL_H
#define _GLOBAL_H

#define VERBOSE 0

#define INT_BIT_LEN	32

#define SUMMARY_FILENAME	"afopt.sum.txt"

//four parameters for choose proper structures for conditional database representation.
/*
#define MAX_BUCKET_SIZE 10
#define BUILD_TREE_ITEM_THRESHOLD 20
#define BUILD_TREE_MINSUP_THRESHOLD 0.05
#define BUILD_TREE_AVGSUP_THRESHOLD 0.10
*/

#define MAX_BUCKET_SIZE 10
//#define BUILD_TREE_ITEM_THRESHOLD 2000
//#define BUILD_TREE_MINSUP_THRESHOLD 2
//#define BUILD_TREE_AVGSUP_THRESHOLD 2

#define MAX(x,y)   x>=y?x:y
#define MIN(x,y)   x<=y?x:y

namespace Afopt {
	//parameter settings
	extern int gnmin_sup;

	//some statistic information about the data
	extern int gndb_size;
	extern int gndb_ntrans;
	extern int gnmax_trans_len;
	extern int gnmax_item_id;

	//pattern statistics
	extern int  gntotal_freqitems;
	extern unsigned long long gntotal_patterns;
	extern int	gnmax_pattern_len;
	extern int  gntotal_singlepath;
	extern int *gppat_counters;

	//program running statistics
	extern int gndepth;
	extern int gntotal_call;
	extern int gnfirst_level;
	extern int gnfirst_level_depth;
	extern int *gpheader_itemset;

	typedef struct
	{
		int item;
		int support;
		int ntrans;
		int order;
	} ITEM_COUNTER;

	typedef struct
	{
		int nmax_len;
		int nmax_item_id;
		int nmin_item_id;
		int bitmap;
	} MAP_NODE;

#define AFOPT_FLAG  0x0002

	//header node of a transaction stored in array structure
	typedef struct TRANS_HEAD_NODE
	{
		int   length;
		int   frequency;
		int	  nstart_pos; 
		int	  *ptransaction;
		TRANS_HEAD_NODE *next; // the next node with the same item.
	} TRANS_HEAD_NODE;


	//arry node in single branches of AFOPT-tree 
	typedef struct
	{
		int order;
		int support;
		int ntrans;
	} ARRAY_NODE;

	//AFOPT-tree node. 
	//An AFOPT-node can either point to a single branch stored as arrays or point to a child AFOPT-node

	typedef struct AFOPT_NODE
	{
		int	frequency;
		int ntrans;
		short order;
		short flag; // -1: pchild==NULL; 0: point to child node; >0: length of the single branch;
		union 
		{
			ARRAY_NODE *pitemlist;
			AFOPT_NODE *pchild;
		};
		AFOPT_NODE* prightsibling;
	} AFOPT_NODE;


	//the information stored in a header node;
	typedef struct
	{
		int item; 
		int nsupport;
		int ntrans;
		union 
		{
			TRANS_HEAD_NODE *parray_conddb;
			AFOPT_NODE *pafopt_conddb;
			int *pbuckets;
		}; 
		short order; 
		short flag; //to indicate whether the conditional database is an AFOPT-tree
	} HEADER_NODE;

	typedef HEADER_NODE *HEADER_TABLE;

	int comp_item_freq_asc(const void *e1, const void *e2);
	int comp_int_asc(const void *e1, const void *e2);


	void PrintSummary();
}
#endif
