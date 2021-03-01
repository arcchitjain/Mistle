#ifndef __ITEMSETSORTED_H
#define __ITEMSETSORTED_H


#include "UsageClass.h"
#include <vector>
#include <list>
#include <unordered_map>
#include "Primitives.h"

using namespace std;

class ItemSetSorted {
public:
	ItemSetSorted();
	~ItemSetSorted();

	void SortElements();
	void Insert(uint32 key, UsageClass &usageClass);
	void Remove(std::list<uint32> *deletedIDs);

	ItemSet* GetItemSet(uint32 id);
	uint32 GetUsageCount(uint32 id);

	std::vector<std::pair<uint32, UsageClass>> *mCurrentCT;
};

// with index
class ItemSetSortedIdx
{
public:
	ItemSetSortedIdx();
	~ItemSetSortedIdx();

	void SortElements();
	void Insert(uint32 key, UsageClass &usageClass);
	void Remove(std::list<uint32> *deletedIDs);

	ItemSet* GetItemSet(uint32 id);
	uint32 GetUsageCount(uint32 id);

	std::vector<std::pair<uint32, UsageClass>> *mCurrentCT;
	unordered_map<uint32,uint32> *mCTIdxMap;
};



// without usageclass
class ItemSetSortedJV
{
public:
	ItemSetSortedJV();
	~ItemSetSortedJV();

	void SortElements();
	void Insert(uint32 key, ItemSet *is);
	void Remove(uint32 key);
	void Remove(std::list<uint32> *deletedIDs);


	ItemSet* GetItemSet(uint32 id);
	uint32 GetUsageCount(uint32 id);

	std::vector<std::pair<uint32, ItemSet*>> *currentCT;
};

#endif