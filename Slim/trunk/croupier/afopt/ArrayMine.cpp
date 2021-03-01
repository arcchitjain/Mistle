#include <stdlib.h>
#include <stdio.h>

#include "arraymine.h"
#include "AFOPTMine.h"
#include "parameters.h"
//#include "MemTracer.h"

#if defined (__GNUC__)
namespace Afopt {
#endif

//-------------------------------------------------------------------------------
//count frequent items from a conditional database represented by arrays
//-------------------------------------------------------------------------------
void CArrayMine::CountFreqItems(TRANS_HEAD_NODE* pcond_db, int *pitem_sup_map, int *pitem_ntrans)
{
	TRANS_HEAD_NODE *pthnode;
	int i;

	pthnode = pcond_db;
	while(pthnode!=NULL)
	{
		for(i=pthnode->nstart_pos;i<pthnode->length;i++)
		{
			pitem_sup_map[pthnode->ptransaction[i]] += pthnode->frequency;
			pitem_ntrans[pthnode->ptransaction[i]]++;
		}
		pthnode = pthnode->next;
	}
}

//------------------------------------------------------------------------------------
//push right a conditional database stored as arrays
//------------------------------------------------------------------------------------
void CArrayMine::PushRight(HEADER_TABLE pheader_table, int item_pos, int nmax_push_pos)
{
	TRANS_HEAD_NODE *pthnode, *pthnextnode;
	int nstart_pos, norder;

	pthnode = pheader_table[item_pos].parray_conddb;
	while(pthnode!=NULL)
	{
		pthnextnode = pthnode->next;
		nstart_pos = pthnode->nstart_pos;
		if(nstart_pos<pthnode->length-1)
		{
			norder = pthnode->ptransaction[nstart_pos];
			if(norder>=nmax_push_pos)
			{
				delete []pthnode->ptransaction;
				//goMemTracer.DelOneTransaction(pthnode->length);
				delete pthnode;
				//goMemTracer.DelOneTransHeadNode();
			}
			else if(pheader_table[norder].flag & AFOPT_FLAG)
			{
				nstart_pos++;
				goAFOPTMiner.InsertTransaction(pheader_table, norder, &(pthnode->ptransaction[nstart_pos]), pthnode->length-nstart_pos, pthnode->frequency, 1);
				delete []pthnode->ptransaction;
				//goMemTracer.DelOneTransaction(pthnode->length);
				delete pthnode;
				//goMemTracer.DelOneTransHeadNode();
			}
			else
			{
				pthnode->next = pheader_table[norder].parray_conddb;
				pheader_table[norder].parray_conddb = pthnode;
				pthnode->nstart_pos++;
			}
		}
		else 
		{
			delete []pthnode->ptransaction;
			//goMemTracer.DelOneTransaction(pthnode->length);
			delete pthnode;
			//goMemTracer.DelOneTransHeadNode();
		}
		pthnode = pthnextnode;
	}
}

//-------------------------------------------------------------------------------
//bucket count all frequent itemsets in a conditional database stored as arrays
//-------------------------------------------------------------------------------
void CArrayMine::BucketCount(TRANS_HEAD_NODE *pcond_db, int* pitem_order_map, int *Buckets, int *tBuckets)
{
	int i;
	unsigned int k;
	TRANS_HEAD_NODE *pthnode;

	pthnode = pcond_db;
	while(pthnode!=NULL)
	{
		k = 0;
		for(i=pthnode->nstart_pos;i<pthnode->length;i++)
		{
			if(pitem_order_map[pthnode->ptransaction[i]]>=0)
				k += (1 << pitem_order_map[pthnode->ptransaction[i]]);
		}
		Buckets[k] += pthnode->frequency;
		++tBuckets[k];
		pthnode = pthnode->next;
	}
}

//-----------------------------------------------------------------------------------------
//build new conditional databases from a conditional database stored as arrays, 
//and at the same time, do bucket counting
//-----------------------------------------------------------------------------------------
void CArrayMine::BuildNewCondDB(TRANS_HEAD_NODE *pcond_db, HEADER_TABLE pnewheader_table, int* pitem_order_map, int num_of_freqitems, int *Buckets, int *tBuckets)
{
	int i;
	int *ptransaction, *pnewtransaction;
	int ntrans_len;
	TRANS_HEAD_NODE *pthnode, *pnewthnode;
	int num_of_countitems, order;
	unsigned int k;

	num_of_countitems = num_of_freqitems-MAX_BUCKET_SIZE;

	ptransaction = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));

	pthnode = pcond_db;
	while(pthnode!=NULL)
	{	
		ntrans_len = 0;
		k = 0;
		for(i=pthnode->nstart_pos;i<pthnode->length;i++)
		{
			order = pitem_order_map[pthnode->ptransaction[i]];
			if(order>=0)
			{
				if(order>=num_of_countitems)
					k += (1<<(order-num_of_countitems));
				ptransaction[ntrans_len] = order;
				ntrans_len++;
			}
		}

		if(k>0) {
			Buckets[k] += pthnode->frequency;
			++tBuckets[k];
		}

		if(ntrans_len>1)
		{
			qsort((void*)ptransaction, ntrans_len, sizeof(int), comp_int_asc);

			order = ptransaction[0];
			if(order<num_of_countitems)
			{
				if(pnewheader_table[order].flag & AFOPT_FLAG)
				{
					goAFOPTMiner.InsertTransaction(pnewheader_table, order, &(ptransaction[1]), ntrans_len-1, pthnode->frequency, 1);
				}
				else
				{
					pnewthnode = new TRANS_HEAD_NODE;
					//goMemTracer.AddOneTransHeadNode();
					pnewthnode->nstart_pos = 0;
					pnewthnode->length = ntrans_len-1;
					pnewthnode->frequency = pthnode->frequency;
					pnewthnode->next = pnewheader_table[order].parray_conddb;
					pnewheader_table[order].parray_conddb = pnewthnode;

					pnewtransaction = new int[pnewthnode->length];
					//goMemTracer.AddOneTransaction(pnewthnode->length);
					for(i=0;i<pnewthnode->length;i++)
						pnewtransaction[i] = ptransaction[i+1];
					pnewthnode->ptransaction = pnewtransaction;
				}
			}
		}
		pthnode = pthnode->next;
	}

	delete []ptransaction;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));


}
/*
//---------------------------------------------------------------------------------
//construct new conditional databases from a conditional database stored as arrays
//---------------------------------------------------------------------------------
void CArrayMine::BuildNewCondDB(TRANS_HEAD_NODE *pcond_db, HEADER_TABLE pnewheader_table, int* pitem_order_map)
{
	int i;
	int *ptransaction, *pnewtransaction;
	int ntrans_len;
	TRANS_HEAD_NODE *pthnode, *pnewthnode;
	int order;

	ptransaction = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));

	pthnode = pcond_db;
	while(pthnode!=NULL)
	{	
		ntrans_len = 0;
		for(i=pthnode->nstart_pos;i<pthnode->length;i++)
		{
			order = pitem_order_map[pthnode->ptransaction[i]];
			if(order>=0)
			{
				ptransaction[ntrans_len] = order;
				ntrans_len++;
			}
		}

		if(ntrans_len>1)
		{
			qsort((void*)ptransaction, ntrans_len, sizeof(int), comp_int_asc);

			order = ptransaction[0];
			if(pnewheader_table[order].flag & AFOPT_FLAG)
			{
				goAFOPTMiner.InsertTransaction(pnewheader_table, order, &(ptransaction[1]), ntrans_len-1, 1);
			}
			else
			{
				pnewthnode = new TRANS_HEAD_NODE;
				//goMemTracer.AddOneTransHeadNode();
				pnewthnode->nstart_pos = 0;
				pnewthnode->length = ntrans_len-1;
				pnewthnode->next = pnewheader_table[order].parray_conddb;
				pnewheader_table[order].parray_conddb = pnewthnode;

				pnewtransaction = new int[pnewthnode->length];
				//goMemTracer.AddOneTransaction(pnewthnode->length);
				for(i=0;i<pnewthnode->length;i++)
					pnewtransaction[i] = ptransaction[i+1];
				pnewthnode->ptransaction = pnewtransaction;
			}
		}
		pthnode = pthnode->next;
	}

	delete []ptransaction;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));
}
*/

#if defined (__GNUC__)
} // namespace Afopt 
#endif
