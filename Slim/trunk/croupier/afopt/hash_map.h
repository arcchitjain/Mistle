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
