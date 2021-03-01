#include "../clobal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ScanDBMine.h"
#include "AFOPTMine.h"
#include "parameters.h"
#include "data.h"
//#include "MemTracer.h"
//#include "TimeTracer.h"

using namespace Afopt;


// Enumerate all the frequent items in the database
int CScanDBMine::ScanDBCountFreqItems(ITEM_COUNTER **ppfreqitems)
{
	Transaction* ptransaction;
	int *pitem_trans_map, *pitem_sup_map, *ptemp_map, ncapacity, nnewcapacity;
	int i, k;

	ncapacity = ITEM_SUP_MAP_SIZE;
	pitem_sup_map = new int[ncapacity];
	pitem_trans_map = new int[ncapacity];
	memset(pitem_sup_map, 0, sizeof(int)*ncapacity);
	memset(pitem_trans_map, 0, sizeof(int)*ncapacity);

	gndb_size = 0;
	gndb_ntrans = 0;
	gnmax_trans_len = 0;
	gnmax_item_id = 0;

	ptransaction = gpdata->getNextTransaction();
	while(ptransaction)
	{
		if(ptransaction->length>0)
		{
			gndb_size += ptransaction->count;
			gndb_ntrans++;
			if(gnmax_trans_len<ptransaction->length)
				gnmax_trans_len = ptransaction->length;
			for(i=0;i<ptransaction->length;i++)
			{
				if(gnmax_item_id<ptransaction->t[i])
					gnmax_item_id = ptransaction->t[i];

				if(ptransaction->t[i]>=ncapacity)
				{
					nnewcapacity = MAX(2*ncapacity, ptransaction->t[i]+1);
					ptemp_map = new int[nnewcapacity];
					memcpy(ptemp_map, pitem_sup_map, sizeof(int)*ncapacity);
					memset(&(ptemp_map[ncapacity]), 0, sizeof(int)*(nnewcapacity-ncapacity));
					delete []pitem_sup_map;
					pitem_sup_map = ptemp_map;
					ptemp_map = new int[nnewcapacity];
					memcpy(ptemp_map, pitem_trans_map, sizeof(int)*ncapacity);
					memset(&(ptemp_map[ncapacity]), 0, sizeof(int)*(nnewcapacity-ncapacity));
					delete []pitem_trans_map;
					pitem_trans_map = ptemp_map;
					ncapacity = nnewcapacity;
				}
				pitem_sup_map[ptransaction->t[i]] += ptransaction->count;
				pitem_trans_map[ptransaction->t[i]]++;
			}
		}
		delete ptransaction;
		ptransaction=gpdata->getNextTransaction();
	}

	gnmax_trans_len++;
	gnmax_item_id++;

	gntotal_freqitems  = 0;
	for(i=0;i<gnmax_item_id;i++)
	{
		if(pitem_sup_map[i]>=gnmin_sup)
			gntotal_freqitems++;
	}
	
	if(gntotal_freqitems>0)
	{
		(*ppfreqitems) = new ITEM_COUNTER[gntotal_freqitems];
		//goMemTracer.IncBuffer(gntotal_freqitems*sizeof(ITEM_COUNTER));
		k = 0;
		for(i=0;i<gnmax_item_id;i++)
		{
			if(pitem_sup_map[i]>=gnmin_sup)
			{
				(*ppfreqitems)[k].item = i;
				(*ppfreqitems)[k].support = pitem_sup_map[i];
				(*ppfreqitems)[k].ntrans = pitem_trans_map[i];
				k++;
			}
		}
		
		qsort((void*)(*ppfreqitems), gntotal_freqitems, sizeof(ITEM_COUNTER), comp_item_freq_asc);
	}

	delete []pitem_sup_map;
	delete []pitem_trans_map;
	return gntotal_freqitems;

}

void CScanDBMine::ScanDBBuildCondDB(HEADER_TABLE pheader_table, int *pitem_order_map, int num_of_freqitems, int *Buckets, int *tBuckets)
{
	Transaction *ptransaction;
	int *pproj_trans, *pnew_trans;
	int ntrans_len;
	TRANS_HEAD_NODE *pthnode;
	int i, order, num_of_countitems;
	unsigned int k;

	num_of_countitems = num_of_freqitems-MAX_BUCKET_SIZE;

	pproj_trans = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));

	ptransaction = gpdata->getNextTransaction();
	while(ptransaction)
	{
		if(ptransaction->length>0)
		{
			k = 0;
			ntrans_len = 0;
			for(i=0;i<ptransaction->length;i++)
			{
				order = pitem_order_map[ptransaction->t[i]];
				if(order>=0)
				{
					if(order>=num_of_countitems)
						k += (1<<(order-num_of_countitems));
					pproj_trans[ntrans_len] = order;
					ntrans_len++;
				}
			}

			if(k>0) {
				Buckets[k] += ptransaction->count;
				++tBuckets[k];
			}

			if(ntrans_len>1)
			{
				qsort((void*)pproj_trans, ntrans_len, sizeof(int), comp_int_asc);

				order = pproj_trans[0];
				if(order<num_of_countitems)
				{
					if(pheader_table[order].flag & AFOPT_FLAG)
					{
						goAFOPTMiner.InsertTransaction(pheader_table, order, &(pproj_trans[1]), ntrans_len-1, ptransaction->count, 1);
					}
					else
					{
						pthnode = new TRANS_HEAD_NODE;
						//goMemTracer.AddOneTransHeadNode();
						pthnode->length = ntrans_len-1;
						pthnode->nstart_pos = 0;
						pthnode->next = pheader_table[order].parray_conddb;
						pheader_table[order].parray_conddb = pthnode;

						pnew_trans = new int[pthnode->length];
						//goMemTracer.AddOneTransaction(pthnode->length);
						memcpy(pnew_trans, &(pproj_trans[1]), (pthnode->length)*sizeof(int));
						pthnode->ptransaction = pnew_trans;
					}
				}
			}
		}
		delete ptransaction;
		ptransaction = gpdata->getNextTransaction();
	}

	delete []pproj_trans;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));

}

void CScanDBMine::ScanDBBuildCondDB(HEADER_TABLE pheader_table, int *pitem_order_map)
{
	Transaction *ptransaction;
	int *pproj_trans; //, *pnew_trans;
	int ntrans_len;
	//TRANS_HEAD_NODE *pthnode;
	int i, order;


	pproj_trans = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));

	ptransaction = gpdata->getNextTransaction();
	while(ptransaction)
	{
		if(ptransaction->length>0)
		{
			ntrans_len = 0;
			for(i=0;i<ptransaction->length;i++)
			{
				order = pitem_order_map[ptransaction->t[i]];
				if(order>=0)
				{
					pproj_trans[ntrans_len] = order;
					ntrans_len++;
				}
			}

			if(ntrans_len>1)
			{
				qsort((void*)pproj_trans, ntrans_len, sizeof(int), comp_int_asc);

				order = pproj_trans[0];
//				if(pheader_table[order].flag & AFOPT_FLAG)
//				{
					goAFOPTMiner.InsertTransaction(pheader_table, order, &(pproj_trans[1]), ntrans_len-1, ptransaction->count, 1);
//				}
/*				else
				{
					pthnode = new TRANS_HEAD_NODE;
					//goMemTracer.AddOneTransHeadNode();
					pthnode->length = ntrans_len-1;
					pthnode->nstart_pos = 0;
					pthnode->next = pheader_table[order].parray_conddb;
					pheader_table[order].parray_conddb = pthnode;

					pnew_trans = new int[pthnode->length];
					//goMemTracer.AddOneTransaction(pthnode->length);
					memcpy(pnew_trans, &(pproj_trans[1]), pthnode->length*sizeof(int));
					pthnode->ptransaction = pnew_trans;
				}*/
			}
		}
		delete ptransaction;
		ptransaction = gpdata->getNextTransaction();
	}

	delete []pproj_trans;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));

}

void CScanDBMine::ScanDBBucketCount(int *pitem_order_map, int *Buckets, int *transBuckets)
{
	Transaction *ptransaction;
	unsigned int k;
	int i;

	ptransaction = gpdata->getNextTransaction();
	while(ptransaction)
	{
		if(ptransaction->length>0)
		{
			k = 0;
			for(i=0;i<ptransaction->length;i++)
			{
				if(pitem_order_map[ptransaction->t[i]]>=0)
				{
					k += (1<<pitem_order_map[ptransaction->t[i]]);
				}
			}
			Buckets[k] += ptransaction->count;
			transBuckets[k]++;
		}
		delete ptransaction;
		ptransaction = gpdata->getNextTransaction();
	}

}

