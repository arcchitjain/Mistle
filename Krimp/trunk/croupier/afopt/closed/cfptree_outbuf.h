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
#ifndef OUTBUF_MANAGER
#define OUTBUF_MANAGER
#include <stdio.h>

namespace Afopt {

	typedef struct
	{
		int support;
		int ntrans;
		int hash_bitmap;
		int child;
		union
		{
			int item;
			int *pitems;
		};
	} ENTRY;

	typedef struct
	{
		int length; // length>1: an internal node with multiple entires; 
		// length=1: a node with only one entry, and the entry contains only one item;
		// length<-1: a node with only one entry and the entry contains |length| items;
		ENTRY *pentries;
		int pos;
		int cur_order;
	} CFP_NODE;


	typedef struct
	{
		int level;
		bool in_mem;
		int  mem_pos;
		int  cfpnode_size;
		CFP_NODE *pcfp_node;
		int	 disk_pos;
	} ACTIVE_NODE;

	class CTreeOutBufManager
	{
		int mnbuffer_size, mncapacity;
		int mnstart_pos, mnend_pos;
		int mnmin_inmem_level; 
		int mnwrite_start_pos, mndisk_write_start_pos;

		char **mptree_buffer;

		ACTIVE_NODE *mpactive_stack;

		FILE* mfpcfp_file, *mfpread_file;
		int mntree_size; // in bytes


		void Dump(int newnode_size);
		void DumpAll();

		unsigned int SearchInBuf(int ndisk_pos, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos);
		unsigned int SearchOnDisk(int ndisk_pos, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos);

	public: 
		//for pruning
		int mpat_len, mnactive_top;


		void Destroy();

		void Init(int nstack_size);
		void PrintfStatistics(FILE *fpsum_file);
		int  GetTreeSize();
		void IncTreeSize(int ninc_size);

		void InsertActiveNode(int level, CFP_NODE *pcfp_node);
		void WriteInternalNode(CFP_NODE *pcfp_node);
		void WriteLeafNode(int *pitems, int num_of_items, int nsupport, int ntrans, int hash_bitmap);

		void InsertCommonPrefixNode(int level, int *pitems, int num_of_items, int nsupport, int ntrans, int hash_bitmap);
		int WriteCommonPrefixNodes(int num_of_nodes, int base_hashmap);

		int bsearch(int mnmin_sup, ENTRY * pentries, int num_of_freqitems);
		unsigned int SearchActiveStack(int nactive_level, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos);
	};

	extern int gnprune_times;
	extern int gnmap_pruned_times;
}

#endif
