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
#ifndef _HASH_MAP_H
#define _HASH_MAP_H

#define DEFAULT_HASH_MAP_SIZE  1000

namespace Afopt {

	typedef struct HASH_ENTRY
	{
		int key;
		union
		{
			void* pvalue;
			int   value;
		};
		HASH_ENTRY* next;
	} HASH_ENTRY;

	typedef struct
	{
		int num_of_nodes;
		HASH_ENTRY* next;
	}HASH_NODE;

	class CHash_Map
	{
	public:
		int mnhashtable_size;

		int mnmemory_size;

		HASH_NODE *mpentry_list;

		int hashfunc(int nkey); 
		void init(int size);

		HASH_ENTRY* find(int nkey);
		HASH_ENTRY* insert(int nkey, void* nvalue);
		HASH_ENTRY* insert(int nkey, int nvalue);
		void delete_entry(int nkey);

		//	int  get_num_of_large(double threshold);
		//	void get_large_entries(double threshold, ITEM_COUNTER *plargeitems);

		void destroy();

	};

	extern CHash_Map goitem_order_map;


}

#endif
