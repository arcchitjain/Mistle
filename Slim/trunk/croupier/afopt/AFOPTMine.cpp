#include "../clobal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <time.h>

#include <Primitives.h>

#include "closed/cfptree_outbuf.h"
#include "AfoptMiner.h"
#include "parameters.h"
#include "fsout.h"

#include "AFOPTMine.h"
#if VERBOSE
#include "fsout.h"
#endif

using namespace Afopt;

void CAFOPTMine::CFIDepthAFOPTGrowth(HEADER_TABLE pheader_table, int num_of_freqitems, int nstart_pos, CFP_NODE *pcfp_node)
{
	int	 i;
	AFOPT_NODE	*ptreenode, *pcurroot;
	ITEM_COUNTER* pnewfreqitems;
	int *pitem_suporder_map, *pitem_ntrans_tmp;
	int num_of_newfreqitems, order, len, npbyclosed;
	int* pitemset, *pitem_sups, *pitem_ntrans;
	clock_t start;
	bool issinglepath, isSumedByLeftSib;
	HEADER_TABLE pnewheader_table;

	CFP_NODE newcfp_node;
	int bitmap, base_bitmap, num_of_prefix_node, nclass_pos;

	gntotal_call++;

	pitem_sups = new int[num_of_freqitems];
	pitem_ntrans = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));
	pnewfreqitems = new ITEM_COUNTER[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	pitem_suporder_map = new int[num_of_freqitems];
	pitem_ntrans_tmp = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));

	isSumedByLeftSib = false;
	order = nstart_pos;
	while(order<num_of_freqitems)
	{
		pcurroot = pheader_table[order].pafopt_conddb;
		num_of_prefix_node = 0;
		gpheader_itemset[gndepth] = pheader_table[order].item;
		gndepth++;
		pitemset = &(gpheader_itemset[gndepth]);

		if(isSumedByLeftSib)
		{
			isSumedByLeftSib = false;
			nclass_pos = -1;
		}
		else 
			nclass_pos = PrunedByExistCFI(0, pheader_table[order].nsupport);

		if(nclass_pos)
			pcfp_node->pentries[order].hash_bitmap = nclass_pos;
		else if(pcurroot==NULL || pcurroot->flag==-1)
			OutputOneClosePat(pheader_table[order].nsupport, pheader_table[order].ntrans);
		else
		{
			issinglepath = false;
			len = 0;
			//[begin] single path testing
			ptreenode = pcurroot;
			if(ptreenode->flag>0)
			{
				issinglepath = true;
				for(i=0;i<ptreenode->flag;i++)
				{
					if(ptreenode->pitemlist[i].support<gnmin_sup)
						break;
					gpheader_itemset[gndepth+len] = pheader_table[ptreenode->pitemlist[i].order].item;
					pitem_sups[len] = ptreenode->pitemlist[i].support;
					pitem_ntrans[len] = ptreenode->pitemlist[i].ntrans;
					len++;
				}
			}
			else
			{
				ptreenode = ptreenode->pchild;
				while(ptreenode!=NULL && ptreenode->prightsibling==NULL && ptreenode->frequency>=gnmin_sup)
				{
					gpheader_itemset[gndepth+len] = pheader_table[ptreenode->order].item;
					pitem_sups[len] = ptreenode->frequency;
					pitem_ntrans[len] = ptreenode->ntrans;
					len++;
					pcurroot = ptreenode;
					if(ptreenode->flag==0)
						ptreenode = ptreenode->pchild;
					else 
						break;
				}

				if(ptreenode==NULL || (ptreenode->prightsibling==NULL && ptreenode->frequency<gnmin_sup))
				{
					issinglepath = true;
				}
				else if(ptreenode->flag>0 && ptreenode->prightsibling==NULL)
				{
					issinglepath = true;
					for(i=0;i<ptreenode->flag;i++)
					{
						if(ptreenode->pitemlist[i].support<gnmin_sup)
							break;
						gpheader_itemset[gndepth+len] = pheader_table[ptreenode->pitemlist[i].order].item;
						pitem_sups[len] = ptreenode->pitemlist[i].support;
						pitem_ntrans[len] = ptreenode->pitemlist[i].ntrans;
						len++;
					}
				}
			}
			//[end] single path testing

			if(issinglepath)
			{
				if(len==0 || pheader_table[order].nsupport>pitem_sups[0])
					OutputOneClosePat(pheader_table[order].nsupport, pheader_table[order].ntrans);

				if(len==1)
				{
					nclass_pos = PrunedByExistCFI(1, pitem_sups[0]);
					if(nclass_pos==0)
					{
						pcfp_node->pentries[order].child = gotree_bufmanager.GetTreeSize();
						pcfp_node->pentries[order].hash_bitmap = (1<<pitemset[0]%INT_BIT_LEN);
						gotree_bufmanager.WriteLeafNode(pitemset, 1, pitem_sups[0], pitem_ntrans[0], 0);
						OutputOneClosePat(1, pitem_sups[0], pitem_ntrans[0]);
					}
				}
				else if(len>1)
				{
					pcfp_node->pentries[order].child = gotree_bufmanager.GetTreeSize();
					bitmap = OutputSingleClosePath(pitem_sups, pitem_ntrans, len, pheader_table[order].nsupport);
					if(bitmap==0)
						pcfp_node->pentries[order].child = 0;
					else 
						pcfp_node->pentries[order].hash_bitmap = bitmap;
				}
			}
			else 
			{
				npbyclosed = 0;

				start = clock();
				for(i=order+1;i<num_of_freqitems;i++) {
					pitem_suporder_map[i] = 0;
					pitem_ntrans_tmp[i] = 0;
				}
				CountFreqItems(pcurroot, pitem_suporder_map, pitem_ntrans_tmp);
				//goTimeTracer.mdAFOPT_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
				num_of_newfreqitems = 0;
				for(i=order+1;i<num_of_freqitems;i++)
				{
					if(pitem_suporder_map[i]>=gnmin_sup)
					{
						pnewfreqitems[num_of_newfreqitems].item = pheader_table[i].item;
						pnewfreqitems[num_of_newfreqitems].support = pitem_suporder_map[i];
						pnewfreqitems[num_of_newfreqitems].ntrans = pitem_ntrans_tmp[i];
						pnewfreqitems[num_of_newfreqitems].order = i;
						num_of_newfreqitems++;
					}
					pitem_suporder_map[i] = -1;
				}
				if(num_of_newfreqitems>0)
				{
					qsort(pnewfreqitems, num_of_newfreqitems, sizeof(ITEM_COUNTER), comp_item_freq_asc);
					for(i=0;i<num_of_newfreqitems;i++)
						pitem_suporder_map[pnewfreqitems[i].order] = i;
				}

				for(i=num_of_newfreqitems-1;i>=0;i--)
				{
					if(pnewfreqitems[i].support==pcurroot->frequency)
					{
						gpheader_itemset[gndepth+len+npbyclosed] = pnewfreqitems[i].item;
						pitem_sups[len+npbyclosed] = pnewfreqitems[i].support;
						pitem_ntrans[len+npbyclosed] = pnewfreqitems[i].ntrans;
						npbyclosed++;
						pitem_suporder_map[pnewfreqitems[i].order] = -1;
					}
					else 
						break;
				}
				num_of_newfreqitems -= npbyclosed;

				num_of_prefix_node = 0;
				if(len+npbyclosed==0 || pitem_sups[0]<pheader_table[order].nsupport)
					OutputOneClosePat(pheader_table[order].nsupport, pheader_table[order].ntrans);
				if(num_of_newfreqitems+len+npbyclosed>0)
					pcfp_node->pentries[order].child = gotree_bufmanager.GetTreeSize();

				base_bitmap = 0;
				for(i=0;i<num_of_newfreqitems;i++)
					base_bitmap = base_bitmap | (1<< (pnewfreqitems[i].item%INT_BIT_LEN));
				if(len+npbyclosed>0 && num_of_newfreqitems==0)
				{
					bitmap = OutputSingleClosePath(pitem_sups, pitem_ntrans, len+npbyclosed, pheader_table[order].nsupport);
					if(bitmap==0)
						pcfp_node->pentries[order].child = 0;
					else 
						pcfp_node->pentries[order].hash_bitmap = bitmap;
				}
				else if(len+npbyclosed>0 && num_of_newfreqitems>0)
				{
					bitmap = base_bitmap;
					num_of_prefix_node = OutputCommonPrefix(pitem_sups, pitem_ntrans, len+npbyclosed, &bitmap, pheader_table[order].nsupport);
				}

				if(num_of_prefix_node<0)
				{
					if(bitmap==0)
						pcfp_node->pentries[order].child = 0;
					else
						pcfp_node->pentries[order].hash_bitmap = bitmap;
				}
				else if(num_of_newfreqitems>0)
				{
					bitmap = 0;
					for(i=0;i<len+npbyclosed;i++)
						bitmap = bitmap | (1<<(pitemset[i]%INT_BIT_LEN));
					pcfp_node->pentries[order].hash_bitmap = bitmap | base_bitmap;

					if(num_of_newfreqitems==1)
					{
						gndepth += len+npbyclosed;
						gpheader_itemset[gndepth] = pnewfreqitems[0].item;
						nclass_pos = PrunedByExistCFI(1, pnewfreqitems[0].support);
						if(nclass_pos==0)
						{
							OutputOneClosePat(1, pnewfreqitems[0].support, pnewfreqitems[0].ntrans);
							gotree_bufmanager.WriteLeafNode(&(pnewfreqitems[0].item), 1, pnewfreqitems[0].support, pnewfreqitems[0].ntrans, 0);
						}
						else 
						{
							if(len+npbyclosed==0)
							{
								pcfp_node->pentries[order].child = 0;
								pcfp_node->pentries[order].hash_bitmap = 0;
							}
							else 
								pcfp_node->pentries[order].hash_bitmap = bitmap;
							base_bitmap = 0;
						}
						gndepth = gndepth-len-npbyclosed;

					}
					else if(num_of_newfreqitems>1)
					{
						gndepth += len+npbyclosed;

						start = clock();
						pnewheader_table = new HEADER_NODE[num_of_newfreqitems];
						//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*num_of_newfreqitems);
						for(i=0;i<num_of_newfreqitems;i++)
						{
							pnewheader_table[i].item = pnewfreqitems[i].item;
							pnewheader_table[i].nsupport = pnewfreqitems[i].support;
							pnewheader_table[i].ntrans = pnewfreqitems[i].ntrans;
							pnewheader_table[i].pafopt_conddb = NULL;
							pnewheader_table[i].order = i;
							pnewheader_table[i].flag = AFOPT_FLAG;
						}
						BuildNewTree(pcurroot, pnewheader_table, pitem_suporder_map);
						//goTimeTracer.mdAFOPT_construct_time += (double)(clock()-start)/CLOCKS_PER_SEC;

						newcfp_node.pos = gotree_bufmanager.GetTreeSize();
						newcfp_node.cur_order = 0;
						newcfp_node.length = num_of_newfreqitems;
						newcfp_node.pentries = new ENTRY[newcfp_node.length];
						//goMemTracer.IncCFPSize(newcfp_node.length*sizeof(ENTRY));
						for(i=0;i<newcfp_node.length;i++)
						{
							newcfp_node.pentries[i].item = pnewfreqitems[i].item;
							newcfp_node.pentries[i].support = pnewfreqitems[i].support;
							newcfp_node.pentries[i].ntrans = pnewfreqitems[i].ntrans;
							newcfp_node.pentries[i].hash_bitmap = 0;
							newcfp_node.pentries[i].child = 0;
						}
						gotree_bufmanager.InsertActiveNode(gntree_level, &newcfp_node);
						gntree_level++;
						CFIDepthAFOPTGrowth(pnewheader_table, num_of_newfreqitems, 0, &newcfp_node);
						gntree_level--;
						gotree_bufmanager.WriteInternalNode(&newcfp_node);
						delete []newcfp_node.pentries;
						//goMemTracer.DecCFPSize(newcfp_node.length*sizeof(ENTRY));

						delete []pnewheader_table;
						//goMemTracer.DecBuffer(sizeof(HEADER_NODE)*num_of_newfreqitems);

						gndepth = gndepth-len-npbyclosed;
					}

					if(len+npbyclosed>0 && num_of_prefix_node>0)
					{
						gotree_bufmanager.WriteCommonPrefixNodes(num_of_prefix_node, base_bitmap);
						gntree_level -= num_of_prefix_node;
					}
				}
			}
		}
		gndepth--;

		if(gndepth==gnfirst_level_depth && gpitem_sup_map[order]!=NULL)
		{
			delete []gpitem_sup_map[order];
			//goMemTracer.DecTwoLayerMap(MIN(SUP_MAXLEN_MAP_SIZE, pheader_table[order].nsupport-(int)gnmin_sup+1)*sizeof(MAP_NODE));
			gpitem_sup_map[order] = NULL;
		}

		pcurroot = pheader_table[order].pafopt_conddb;
		if(pcurroot!=NULL && order<num_of_freqitems-1 && pheader_table[order+1].pafopt_conddb==NULL && 
			(pcurroot->flag==0 && pcurroot->pchild->order==order+1 && pcurroot->pchild->frequency==pheader_table[order+1].nsupport ||
			pcurroot->flag>0 && pcurroot->pitemlist[0].order==order+1 && pcurroot->pitemlist[0].support==pheader_table[order+1].nsupport))
			isSumedByLeftSib = true;

		start = clock();
		if(pcurroot!=NULL && pcurroot->flag!=-1)
			PushRight(pheader_table, order, num_of_freqitems);
		else if(pcurroot!=NULL)
		{
			delete pcurroot;
			//goMemTracer.DelOneAFOPTNode();
		}
		//goTimeTracer.mdAFOPT_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;

		order++;
		pcfp_node->cur_order++;
	}

	delete []pitem_sups;
	delete []pitem_ntrans;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));
	delete []pnewfreqitems;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	delete []pitem_suporder_map;
	delete []pitem_ntrans_tmp;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));

	return;
}

//-----------------------------------------------------------------------------------
//mining frequent itemsets from a conditional database represented by an AFOPT-tree
//------------------------------------------------------------------------------------
void CAFOPTMine::DepthAFOPTGrowthAll(HEADER_TABLE pheader_table, int num_of_freqitems, int nstart_pos)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::DepthAFOPTGrowth\n");
#endif
	int	i;
	AFOPT_NODE *pcurroot;
	ITEM_COUNTER* pnewfreqitems;
	int *pitem_suporder_map, *pitem_ntrans;
	int num_of_newfreqitems;
	HEADER_TABLE pnewheader_table;
	int k, nend_pos;
	ARRAY_NODE *pitem_list;

	clock_t start;

	gntotal_call++;

	pnewfreqitems = new ITEM_COUNTER[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	pitem_suporder_map = new int[num_of_freqitems];
	pitem_ntrans = new int[num_of_freqitems];
	//goMemTracer.IncBuffer(num_of_freqitems*sizeof(int));

	if(gnmax_trans_len<gndepth+1)
		gnmax_trans_len = gndepth+1;
	nend_pos = num_of_freqitems-MAX_BUCKET_SIZE;
	for(k=nstart_pos;k<nend_pos;k++)
	{
		OutputOnePattern(pheader_table[k].item, pheader_table[k].nsupport, pheader_table[k].ntrans);
		pcurroot = pheader_table[k].pafopt_conddb;
		if(pcurroot==NULL)
			continue;

		gpheader_itemset[gndepth] = pheader_table[k].item;
		gndepth++;

		//if the tree contains only one single branch, then directly enumerate all frequent itemsets from the tree
		if(pcurroot->flag>0)
		{
			pitem_list = pcurroot->pitemlist;
			for(i=0;i<pcurroot->flag && pitem_list[i].support>=gnmin_sup;i++)
			{
				pnewfreqitems[i].item = pheader_table[pitem_list[i].order].item;
				pnewfreqitems[i].support = pitem_list[i].support;
				pnewfreqitems[i].ntrans = pitem_list[i].ntrans;
			}

			OutputSinglePath(pnewfreqitems, i);

			if(pcurroot->flag>1 && pitem_list[0].order<nend_pos)
			{
				InsertTransaction(pheader_table, pitem_list[0].order, &(pitem_list[1]), pcurroot->flag-1);
			}
			delete []pitem_list;
			//goMemTracer.DelSingleBranch(pcurroot->flag);
			delete pcurroot;
			//goMemTracer.DelOneAFOPTNode();
			pheader_table[k].pafopt_conddb = NULL;
		}
		else if(pcurroot->flag==0)
		{

			//count frequent items from AFOPT-tree
			for(i=k+1;i<num_of_freqitems;i++) {
				pitem_suporder_map[i] = 0;
				pitem_ntrans[i] = 0;
			}
			start = clock();
			CountFreqItems(pcurroot, pitem_suporder_map, pitem_ntrans);
			//goTimeTracer.mdAFOPT_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
			num_of_newfreqitems = 0;
			for(i=k+1;i<num_of_freqitems;i++)
			{
				if(pitem_suporder_map[i]>=gnmin_sup)
				{
					pnewfreqitems[num_of_newfreqitems].item = pheader_table[i].item;
					pnewfreqitems[num_of_newfreqitems].support = pitem_suporder_map[i];
					pnewfreqitems[num_of_newfreqitems].ntrans = pitem_ntrans[i];
					pnewfreqitems[num_of_newfreqitems].order = i;
					num_of_newfreqitems++;
				}
				pitem_suporder_map[i] = -1;
			}
			if(num_of_newfreqitems>0)
			{
				qsort(pnewfreqitems, num_of_newfreqitems, sizeof(ITEM_COUNTER), comp_item_freq_asc);
				for(i=0;i<num_of_newfreqitems;i++)
					pitem_suporder_map[pnewfreqitems[i].order] = i;
			}
			if(num_of_newfreqitems==1)
			{
				if(gnmax_trans_len<gndepth+1)
					gnmax_trans_len = gndepth+1;
				OutputOnePattern(pnewfreqitems[0].item, pnewfreqitems[0].support, pnewfreqitems[0].ntrans);
			}
			else if(num_of_newfreqitems>1)
			{
				if(num_of_newfreqitems<=MAX_BUCKET_SIZE)
				{
					start = clock();
					memset(gpbuckets, 0, sizeof(int)*(uint32)(1<<(uint32)num_of_newfreqitems));
					memset(gtbuckets, 0, sizeof(int)*(uint32)(1<<(uint32)num_of_newfreqitems));
					BucketCountAFOPT(pcurroot, pitem_suporder_map, gpbuckets, gtbuckets);
					bucket_count(gpbuckets, gtbuckets, num_of_newfreqitems, pnewfreqitems);
					//goTimeTracer.mdBucket_count_time += (double)(clock()-start)/CLOCKS_PER_SEC;
				}
				else 
				{
					start = clock();
					pnewheader_table = new HEADER_NODE[num_of_newfreqitems];
					//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*num_of_newfreqitems);
					for(i=0;i<num_of_newfreqitems;i++)
					{
						pnewheader_table[i].item = pnewfreqitems[i].item;
						pnewheader_table[i].nsupport = pnewfreqitems[i].support;
						pnewheader_table[i].ntrans = pnewfreqitems[i].ntrans;
						pnewheader_table[i].pafopt_conddb = NULL;
						pnewheader_table[i].order = i;
						pnewheader_table[i].flag = AFOPT_FLAG;
					}

					memset(gpbuckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
					memset(gtbuckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
					BuildNewTree(pcurroot, pnewheader_table, pitem_suporder_map, num_of_newfreqitems, gpbuckets, gtbuckets); 
					//goTimeTracer.mdAFOPT_construct_time += (double)(clock()-start)/CLOCKS_PER_SEC;
					bucket_count(gpbuckets, gtbuckets, MAX_BUCKET_SIZE, &(pnewfreqitems[num_of_newfreqitems-MAX_BUCKET_SIZE]));

					DepthAFOPTGrowthAll(pnewheader_table, num_of_newfreqitems, 0);
					delete []pnewheader_table;
					//goMemTracer.DecBuffer(sizeof(HEADER_NODE)*num_of_newfreqitems);
				}
			}

			start = clock();
			PushRight(pheader_table, k, nend_pos);
			//if(gndepth==1)
			//goTimeTracer.mdInitial_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;
			//else
			//goTimeTracer.mdAFOPT_pushright_time += (double)(clock()-start)/CLOCKS_PER_SEC;
		}
		else 
		{
			delete pcurroot;
			//goMemTracer.DelOneAFOPTNode();
			pheader_table[k].pafopt_conddb = NULL;
		}
		gndepth--;
	}


	delete []pnewfreqitems;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(ITEM_COUNTER));
	delete []pitem_suporder_map;
	delete []pitem_ntrans;
	//goMemTracer.DecBuffer(num_of_freqitems*sizeof(int));

#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::DepthAFOPTGrowth\n");
#endif
}

//-------------------------------------------------------------------------------
//Count frequent items from an AFOPT-tree by depth-first traversal of the tree.
//It is implemented using stack.
//-------------------------------------------------------------------------------
void CAFOPTMine::CountFreqItems(AFOPT_NODE* proot, int *pitem_sup_map, int *pitem_ntrans)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::CountFreqItems\n");
#endif
	AFOPT_NODE *ptreenode, *pnextchild;

	int source_top;
	int i;
	ARRAY_NODE* pitem_list;
	
	ptreenode = proot;
	source_top = 0;
	mpsource_stack[source_top].pnext_child = ptreenode->pchild;
	mpsource_stack[source_top].pnode = ptreenode;
	source_top++;

	while(source_top>0)
	{
		source_top--;
		pnextchild = mpsource_stack[source_top].pnext_child;
		if(pnextchild!=NULL)
		{
			pitem_sup_map[pnextchild->order] += pnextchild->frequency;
			pitem_ntrans[pnextchild->order] += pnextchild->ntrans;
			mpsource_stack[source_top].pnext_child = pnextchild->prightsibling;
			source_top++;
			if(pnextchild->flag==0)
			{
				mpsource_stack[source_top].pnext_child = pnextchild->pchild;
				mpsource_stack[source_top].pnode = pnextchild;
				source_top++;
			}
			else if(pnextchild->flag>0) 
			{
				pitem_list = pnextchild->pitemlist;
				for(i=0;i<pnextchild->flag;i++)
				{
					pitem_sup_map[pitem_list[i].order] += pitem_list[i].support;
					pitem_ntrans[pitem_list[i].order] += pitem_list[i].ntrans;
				}
			}
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::CountFreqItems\n");
#endif
}

//---------------------------------------------------------------------------------------
//Build new conditional databases from an AFOPT-tree by depth-first traversal of the tree, 
//and at the same time bucket counting the most frequent items. Implemented using stack;
//----------------------------------------------------------------------------------------
void CAFOPTMine::BuildNewTree(AFOPT_NODE *proot, HEADER_TABLE pnewheader_table, int *pitem_order_map, int num_of_newfreqitems, int *Buckets, int *tBuckets)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::BuildNewTree\n");
#endif
	AFOPT_NODE *ptreenode;
	int i, order;
	int source_top, ntrans_len, ntemp_len, ntemp_k;
	unsigned int k;
	int num_of_countitems;
	
	num_of_countitems = num_of_newfreqitems-MAX_BUCKET_SIZE;

	source_top = 0;
	mpsource_stack[source_top].pnext_child = proot->pchild;
	mpsource_stack[source_top].frequency = proot->frequency;
	mpsource_stack[source_top].ntrans = proot->ntrans;
	mpsource_stack[source_top].flag = 0;
	source_top++;
	ntrans_len = 0;

	k = 0;
	while(source_top>0)
	{
		source_top--;
		ptreenode = mpsource_stack[source_top].pnext_child;
		if(ptreenode!=NULL)
		{
			mpsource_stack[source_top].frequency -= ptreenode->frequency;
			mpsource_stack[source_top].ntrans -= ptreenode->ntrans;
			mpsource_stack[source_top].pnext_child = ptreenode->prightsibling;
			source_top++;

			if(ptreenode->flag==0)
			{
				mpsource_stack[source_top].pnext_child = ptreenode->pchild;
				mpsource_stack[source_top].frequency = ptreenode->frequency;
				mpsource_stack[source_top].ntrans = ptreenode->ntrans;
				order = pitem_order_map[ptreenode->order];
				if(order>=0)
				{
					mpsource_stack[source_top].flag = 1;
					mpbranch[ntrans_len] = order;
					ntrans_len++;
					if(order>=num_of_countitems)
						k += (1<<(order-num_of_countitems));
				}
				else 
					mpsource_stack[source_top].flag = 0;
				source_top++;
			}
			else if(ptreenode->flag==-1) //the node has no children
			{
				order = pitem_order_map[ptreenode->order];
				if(order>=0)
				{
					mpbranch[ntrans_len] = order;
					ntrans_len++;
					if(order>=num_of_countitems)
						k += (1<<(order-num_of_countitems));
				}

				if(k>0) {
					Buckets[k] += ptreenode->frequency;
					tBuckets[k] += ptreenode->ntrans;
				}

				if(ntrans_len>1)
				{
					memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
					qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc); 
					if(mptransaction[0]<num_of_countitems)
					{
						InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, ptreenode->frequency, ptreenode->ntrans);
					}
				}

				if(order>=0)
				{
					ntrans_len--;
					if(order>=num_of_countitems)
						k -= (1<<(order-num_of_countitems));
				}

				//pop out nodes whose children are all visited
				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
					{
						ntrans_len--;
						if(mpbranch[ntrans_len]>=num_of_countitems)
							k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
					}
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
			else //the node points to a single branch
			{
				ntemp_len = ntrans_len;
				ntemp_k = k;

				order = pitem_order_map[ptreenode->order];
				if(order>=0)
				{
					mpbranch[ntrans_len] = order;
					ntrans_len++;
					if(order>=num_of_countitems)
						k += (1<<(order-num_of_countitems));
				}
				if(ntrans_len>0) {
					mpfrequencies[ntrans_len-1] = ptreenode->frequency;
					mpntrans[ntrans_len-1] = ptreenode->ntrans;
				}

				for(i=0;i<ptreenode->flag;i++)
				{
					order = pitem_order_map[ptreenode->pitemlist[i].order];
					if(order>=0)
					{
						mpbranch[ntrans_len] = order;
						mpfrequencies[ntrans_len] = ptreenode->pitemlist[i].support;
						mpntrans[ntrans_len] = ptreenode->pitemlist[i].ntrans;
						ntrans_len++;
						if(order>=num_of_countitems)
							k += (1<<(order-num_of_countitems));
					}
				}

				if(ntrans_len>0)
				{
					mpfrequencies[ntrans_len] = 0;
					mpntrans[ntrans_len] = 0;
					while(ntrans_len>0 && ntrans_len>=ntemp_len)
					{
						memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
						qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc);
						if(ntrans_len>1 && mptransaction[0]<num_of_countitems)
						{
							InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, mpfrequencies[ntrans_len-1]-mpfrequencies[ntrans_len], mpntrans[ntrans_len-1]-mpntrans[ntrans_len]);
						}
						if(k>0) {
							Buckets[k] += mpfrequencies[ntrans_len-1]-mpfrequencies[ntrans_len];
							tBuckets[k] += mpntrans[ntrans_len-1]-mpntrans[ntrans_len];
						}

						if(mpfrequencies[ntrans_len-1]==ptreenode->frequency)
						{
							ntrans_len = ntemp_len;
							k = ntemp_k;
							break;
						}
						ntrans_len--;
						if(mpbranch[ntrans_len]>=num_of_countitems)
							k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
						while(ntrans_len>ntemp_len && mpfrequencies[ntrans_len-1]==mpfrequencies[ntrans_len])
						{
							ntrans_len--;
							if(mpbranch[ntrans_len]>=num_of_countitems)
								k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
						}
					}
				}

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
					{
						ntrans_len--;
						if(mpbranch[ntrans_len]>=num_of_countitems)
							k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
					}
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
		}
		else
		{
			if(mpsource_stack[source_top].frequency>0)
			{
				if(k>0) {
					Buckets[k] += mpsource_stack[source_top].frequency;
					tBuckets[k] += mpsource_stack[source_top].ntrans;
				}
				if(ntrans_len>1)
				{
					memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
					qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc);
					if(mptransaction[0]<num_of_countitems)
					{
						InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, mpsource_stack[source_top].frequency, mpsource_stack[source_top].ntrans);
					}
				}
				mpsource_stack[source_top].frequency = 0;
				mpsource_stack[source_top].ntrans = 0;
			}

			while(source_top>=0 && mpsource_stack[source_top].frequency==0)
			{
				if(mpsource_stack[source_top].pnext_child!=NULL)
				{
					printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
					exit(-1);
				}
				if(mpsource_stack[source_top].flag)
				{
					ntrans_len--;
					if(mpbranch[ntrans_len]>=num_of_countitems)
						k -= (1<<(mpbranch[ntrans_len]-num_of_countitems));
				}
				source_top--;
			}
			if(source_top>=0)
				source_top++;
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::BuildNewTree (all)\n");
#endif
}

//-----------------------------------------------------------------
//build new conditional databases from an AFOPT-tree; used by closed miner
//-----------------------------------------------------------------
void CAFOPTMine::BuildNewTree(AFOPT_NODE *proot, HEADER_TABLE pnewheader_table, int *pitem_order_map)
{
	AFOPT_NODE *ptreenode;
	int i;
	int source_top, ntrans_len, ntemp_len;
	
	source_top = 0;
	mpsource_stack[source_top].pnext_child = proot->pchild;
	mpsource_stack[source_top].frequency = proot->frequency;
	mpsource_stack[source_top].ntrans = proot->ntrans;
	mpsource_stack[source_top].flag = 0;
	source_top++;
	ntrans_len = 0;

	while(source_top>0)
	{
		source_top--;
		ptreenode = mpsource_stack[source_top].pnext_child;
		if(ptreenode!=NULL)
		{
			mpsource_stack[source_top].frequency -= ptreenode->frequency;
			mpsource_stack[source_top].ntrans -= ptreenode->ntrans;
			mpsource_stack[source_top].pnext_child = ptreenode->prightsibling;
			source_top++;

			if(ptreenode->flag==0)
			{
				mpsource_stack[source_top].pnext_child = ptreenode->pchild;
				mpsource_stack[source_top].frequency = ptreenode->frequency;
				mpsource_stack[source_top].ntrans = ptreenode->ntrans;
				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpsource_stack[source_top].flag = 1;
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}
				else 
					mpsource_stack[source_top].flag = 0;
				source_top++;
			}
			else if(ptreenode->flag==-1)
			{
				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}

				if(ntrans_len>1)
				{
					memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
					qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc); 
					InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, ptreenode->frequency, ptreenode->ntrans);
				}

				if(pitem_order_map[ptreenode->order]>=0)
					ntrans_len--;

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
						ntrans_len--;
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
			else
			{
				ntemp_len = ntrans_len;

				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}
				if(ntrans_len>0) {
					mpfrequencies[ntrans_len-1] = ptreenode->frequency;
					mpntrans[ntrans_len-1] = ptreenode->ntrans;
				}

				for(i=0;i<ptreenode->flag;i++)
				{
					if(pitem_order_map[ptreenode->pitemlist[i].order]>=0)
					{
						mpbranch[ntrans_len] = pitem_order_map[ptreenode->pitemlist[i].order];
						mpfrequencies[ntrans_len] = ptreenode->pitemlist[i].support;
						mpntrans[ntrans_len] = ptreenode->pitemlist[i].ntrans;
						ntrans_len++;
					}
				}

				if(ntrans_len>1)
				{
					mpfrequencies[ntrans_len] = 0;
					mpntrans[ntrans_len] = 0;
					while(ntrans_len>0 && ntrans_len>=ntemp_len)
					{
						if(ntrans_len>1)
						{
							memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
							qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc);
							InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, mpfrequencies[ntrans_len-1]-mpfrequencies[ntrans_len], mpntrans[ntrans_len-1]-mpntrans[ntrans_len]);
						}
						if(mpfrequencies[ntrans_len-1]==ptreenode->frequency)
						{
							ntrans_len = ntemp_len;
							break;
						}
						ntrans_len--;
						while(ntrans_len>ntemp_len && mpfrequencies[ntrans_len-1]==mpfrequencies[ntrans_len])
							ntrans_len--;
					}
				}
				else 
					ntrans_len = ntemp_len;

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
						ntrans_len--;
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
		}
		else
		{
			if(mpsource_stack[source_top].frequency>0)
			{
				if(ntrans_len>0)
				{
					memcpy(mptransaction, mpbranch, sizeof(int)*ntrans_len);
					qsort(mptransaction, ntrans_len, sizeof(int), comp_int_asc);
					InsertTransaction(pnewheader_table, mptransaction[0], &(mptransaction[1]), ntrans_len-1, mpsource_stack[source_top].frequency, mpsource_stack[source_top].ntrans);
				}
				mpsource_stack[source_top].frequency = 0;
				mpsource_stack[source_top].ntrans = 0;
			}

			while(source_top>=0 && mpsource_stack[source_top].frequency==0)
			{
				if(mpsource_stack[source_top].pnext_child!=NULL)
				{
					printf("Error[BuildSubTree]: the frequency of the item is 0 while it still has some children\n");
					exit(-1);
				}
				if(mpsource_stack[source_top].flag)
					ntrans_len--;
				source_top--;
			}
			if(source_top>=0)
				source_top++;
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::BuildNewTree (closed)\n");
#endif
}

//---------------------------------------------------------
//bucket count all the frequent itemsets in an AFOPT-tree; 
//---------------------------------------------------------
void CAFOPTMine::BucketCountAFOPT(AFOPT_NODE *proot, int* pitem_order_map, int *Buckets, int *tBuckets)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::BucketCountAFOPT\n");
#endif
	int i;
	unsigned int k;
	AFOPT_NODE *ptreenode;

	int source_top, ntrans_len, ntemp_len;

	source_top = 0;
	mpsource_stack[source_top].pnext_child = proot->pchild;
	mpsource_stack[source_top].frequency = proot->frequency;
	mpsource_stack[source_top].ntrans = proot->ntrans;
	mpsource_stack[source_top].flag = 0;
	source_top++;
	ntrans_len = 0;

	while(source_top>0)
	{
		source_top--;
		ptreenode = mpsource_stack[source_top].pnext_child;
		if(ptreenode!=NULL)
		{
			mpsource_stack[source_top].frequency -= ptreenode->frequency;
			mpsource_stack[source_top].ntrans -= ptreenode->ntrans;
			mpsource_stack[source_top].pnext_child = ptreenode->prightsibling;
			source_top++;

			if(ptreenode->flag==0)//the node has child nodes
			{
				mpsource_stack[source_top].pnext_child = ptreenode->pchild;
				mpsource_stack[source_top].frequency = ptreenode->frequency;
				mpsource_stack[source_top].ntrans = ptreenode->ntrans;
				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpsource_stack[source_top].flag = 1;
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}
				else 
					mpsource_stack[source_top].flag = 0;
				source_top++;
			}
			else if(ptreenode->flag==-1) //the node has no child nodes
			{
				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}

				if(ntrans_len>0)
				{
					k = 0;
					for(i=0;i<ntrans_len;i++)
						k += (1<<mpbranch[i]);
					Buckets[k] += ptreenode->frequency;
					tBuckets[k] += ptreenode->ntrans;
				}

				if(pitem_order_map[ptreenode->order]>=0)
					ntrans_len--;

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BucketCountAFOPT]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
						ntrans_len--;
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
			else // the node points to a single branch
			{
				ntemp_len = ntrans_len;

				if(pitem_order_map[ptreenode->order]>=0)
				{
					mpbranch[ntrans_len] = pitem_order_map[ptreenode->order];
					ntrans_len++;
				}
				if(ntrans_len>0) {
					mpfrequencies[ntrans_len-1] = ptreenode->frequency;
					mpntrans[ntrans_len-1] = ptreenode->ntrans;
				}

				for(i=0;i<ptreenode->flag;i++)
				{
					if(pitem_order_map[ptreenode->pitemlist[i].order]>=0)
					{
						mpbranch[ntrans_len] = pitem_order_map[ptreenode->pitemlist[i].order];
						mpfrequencies[ntrans_len] = ptreenode->pitemlist[i].support;
						mpntrans[ntrans_len] = ptreenode->pitemlist[i].ntrans;
						ntrans_len++;
					}
				}

				if(ntrans_len>0)
				{
					k = 0;
					for(i=0;i<ntrans_len;i++)
						k += (1<<mpbranch[i]);
					mpfrequencies[ntrans_len] = 0;
					mpntrans[ntrans_len] = 0;
					while(ntrans_len>0 && ntrans_len>=ntemp_len)
					{
						Buckets[k] += (mpfrequencies[ntrans_len-1]-mpfrequencies[ntrans_len]);
						tBuckets[k] += (mpntrans[ntrans_len-1]-mpntrans[ntrans_len]);
						if(mpfrequencies[ntrans_len-1]==ptreenode->frequency)
						{
							ntrans_len = ntemp_len;
							break;
						}
						ntrans_len--;
						k -= (1<<mpbranch[ntrans_len]);						
						while(ntrans_len>ntemp_len && mpfrequencies[ntrans_len-1]==mpfrequencies[ntrans_len])
						{
							ntrans_len--;
							k -= (1<<mpbranch[ntrans_len]);
						}
					}
				}

				source_top--;
				while(source_top>=0 && mpsource_stack[source_top].frequency==0)
				{
					if(mpsource_stack[source_top].pnext_child!=NULL)
					{
						printf("Error[BucketCountAFOPT]: the frequency of the item is 0 while it still has some children\n");
						exit(-1);
					}
					if(mpsource_stack[source_top].flag)
						ntrans_len--;
					source_top--;
				}
				if(source_top>=0)
					source_top++;
			}
		}
		else
		{
			if(mpsource_stack[source_top].frequency>0)
			{
				if(ntrans_len>0)
				{
					k = 0;
					for(i=0;i<ntrans_len;i++)
						k += (1 << mpbranch[i]);
					Buckets[k] += mpsource_stack[source_top].frequency;
					tBuckets[k] += mpsource_stack[source_top].ntrans;
				}
				mpsource_stack[source_top].frequency = 0;
				mpsource_stack[source_top].ntrans = 0;
			}

			while(source_top>=0 && mpsource_stack[source_top].frequency==0)
			{
				if(mpsource_stack[source_top].pnext_child!=NULL)
				{
					printf("Error[BucketCountAFOPT]: the frequency of the item is 0 while it still has some children\n");
					exit(-1);
				}
				if(mpsource_stack[source_top].flag)
					ntrans_len--;
				source_top--;
			}
			if(source_top>=0)
				source_top++;
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::BucketCountAFOPT\n");
#endif
}

//-------------------------------------------------------------------------------
//Push right an AFOPT-tree. The children of the root are merged with corresponding 
//siblings of the root
//-------------------------------------------------------------------------------
void CAFOPTMine::PushRight(HEADER_TABLE pheader_table, int nitem_pos, int nmax_push_pos)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::PushRight\n");
#endif
	ARRAY_NODE	*pitem_list;
	AFOPT_NODE *psource_root, *psource_child, *pnext_source_child;

	psource_root = pheader_table[nitem_pos].pafopt_conddb;
	if(psource_root->flag==-1)
	{
		printf("Error with flag\n");
	}
	else if(psource_root->flag>0)
	{
		pitem_list = psource_root->pitemlist;
		if(psource_root->flag>1 && pitem_list[0].order<nmax_push_pos)
		{
			InsertTransaction(pheader_table, pitem_list[0].order, &(pitem_list[1]), psource_root->flag-1);
		}
		
		delete []pitem_list;
		//goMemTracer.DelSingleBranch(psource_root->flag);
	}
	else 
	{
		psource_child = psource_root->pchild;
		while(psource_child!=NULL)
		{
			pnext_source_child = psource_child->prightsibling;
			if(psource_child->order<nmax_push_pos)
			{
				if(pheader_table[psource_child->order].pafopt_conddb==NULL)
				{
					psource_child->prightsibling = NULL;
					pheader_table[psource_child->order].pafopt_conddb = psource_child;
				}
				else if(psource_child->flag>=0)
					MergeTwoTree(psource_child, pheader_table[psource_child->order].pafopt_conddb);
				else 
				{
					delete psource_child;
					//goMemTracer.DelOneAFOPTNode();
				}
			}
			else	
				DestroyTree(psource_child);
			psource_child = pnext_source_child;
		}
	}
	
	delete psource_root;
	//goMemTracer.DelOneAFOPTNode();
	pheader_table[nitem_pos].pafopt_conddb = NULL;
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::PushRight\n");
#endif
}

//-------------------------------------------------------------------------------
//Merge two trees togather
//-------------------------------------------------------------------------------
void CAFOPTMine::MergeTwoTree(AFOPT_NODE *psource_root, AFOPT_NODE *pdest_root)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::MergeTwoTree\n");
#endif
	ARRAY_NODE	*pitem_list;
	int nlen;
	AFOPT_NODE *psource_child, *pnext_source_child, *pdest_child, *pprev_dest_child;

	pdest_root->frequency += psource_root->frequency;
	pdest_root->ntrans += psource_root->ntrans;

	pprev_dest_child = NULL;
	pnext_source_child = NULL;
	if(psource_root->flag==-1)//if the source root has no child, output error message;
	{
		printf("Error with flag\n");
	}
	else if(psource_root->flag>0) //the source root points to a single branch, insert this single branch into destination tree
	{
		InsertTransaction(pdest_root, psource_root->pitemlist, psource_root->flag);
		delete []psource_root->pitemlist;
		//goMemTracer.DelSingleBranch(psource_root->flag);
	}
	else if(pdest_root->flag==-1) //the source root has an AFOPT-node as a child and the destination tree has no child, adjust child pointer
	{
		pdest_root->pchild = psource_root->pchild;
		pdest_root->flag = 0;
	}
	else if(pdest_root->flag>0) //the destination root has a single branch, first exchange child pointer, then insert the single branch
	{
		pitem_list = pdest_root->pitemlist;
		nlen = pdest_root->flag;
		pdest_root->pchild = psource_root->pchild;
		pdest_root->flag = 0;
		InsertTransaction(pdest_root, pitem_list, nlen);
		delete []pitem_list;
		//goMemTracer.DelSingleBranch(nlen);
	}
	else //both source root and destination root points to AFOPT-node, recursively call itself
	{
		psource_child = psource_root->pchild;
		pdest_child = pdest_root->pchild;
		while(psource_child!=NULL)
		{
			while(pdest_child!=NULL && pdest_child->order<psource_child->order)
			{
				pprev_dest_child = pdest_child;
				pdest_child = pdest_child->prightsibling;
			}

			if(pdest_child==NULL)
			{
				pprev_dest_child->prightsibling = psource_child;
				break;
			}
			else if(psource_child->order==pdest_child->order)
			{
				pnext_source_child = psource_child->prightsibling;
				if(psource_child->flag>=0)
					MergeTwoTree(psource_child, pdest_child);
				else 
				{
					pdest_child->frequency += psource_child->frequency;
					pdest_child->ntrans += psource_child->ntrans;
					delete psource_child;
					//goMemTracer.DelOneAFOPTNode();
				}
				psource_child = pnext_source_child;
				pprev_dest_child = pdest_child;
				pdest_child = pdest_child->prightsibling;
			}
			else 
			{
				pnext_source_child = psource_child->prightsibling;
				psource_child->prightsibling = pdest_child;
				if(pprev_dest_child==NULL)
					pdest_root->pchild = psource_child;
				else 
					pprev_dest_child->prightsibling = psource_child;
				pprev_dest_child = psource_child;
				psource_child = pnext_source_child;
			}
		}
	}
	
	delete psource_root;
	//goMemTracer.DelOneAFOPTNode();
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::MergeTwoTree\n");
#endif
}

//-------------------------------------------------------------------------------
//insert a transaction into an AFOPT-tree
//-------------------------------------------------------------------------------
void CAFOPTMine::InsertTransaction(HEADER_TABLE pheader_table, int nitem_pos, int* ptransaction, int length, int frequency, int ntrans)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::InsertTransaction(pht, nip, pt, l, f, t)\n");
#endif
	int i, j, order, orig_frequency, list_len , t;
	AFOPT_NODE *pnewnode, *pcurnode, *pprevnode, *ptnode;
	ARRAY_NODE *pitem_list, *pnewitem_list;

	if(pheader_table[nitem_pos].pafopt_conddb==NULL)
	{
		pnewnode = new AFOPT_NODE;
		//goMemTracer.AddOneAFOPTNode();
		pnewnode->order = nitem_pos;
		pnewnode->frequency = 0;
		pnewnode->ntrans = 0;
		pnewnode->flag = -1;
		pnewnode->pchild = NULL;
		pnewnode->prightsibling = NULL;
		pheader_table[nitem_pos].pafopt_conddb = pnewnode;
	}

	pprevnode = NULL;
	pcurnode = pheader_table[nitem_pos].pafopt_conddb;
	orig_frequency = pcurnode->frequency;
	pcurnode->frequency += frequency;
	pcurnode->ntrans += ntrans;
	
	for(i=0;i<length;i++)
	{
		if(pcurnode->flag == -1)
		{
			pnewitem_list = new ARRAY_NODE[length-i];
			//goMemTracer.AddSingleBranch(length-i);
			for(j=0;j<length-i;j++)
			{
				pnewitem_list[j].order = ptransaction[j+i];
				pnewitem_list[j].support = frequency;
				pnewitem_list[j].ntrans = ntrans;
			}
			pcurnode->pitemlist = pnewitem_list;
			pcurnode->flag = length-i;
			break;
		}
		else if(pcurnode->flag == 0)
		{
			order = ptransaction[i];
			if(pcurnode->pchild->order > order)
			{
				//goMemTracer.AddOneAFOPTNode();
				pnewnode = new AFOPT_NODE;
				pnewnode->frequency = frequency;
				pnewnode->ntrans = ntrans;
				pnewnode->prightsibling = pcurnode->pchild;
				pnewnode->order = order;
				pcurnode->pchild = pnewnode;
				if(i==length-1)
				{
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
				}
				else
				{
					pnewitem_list = new ARRAY_NODE[length-i-1];
					//goMemTracer.AddSingleBranch(length-i-1);
					for(j=0;j<length-i-1;j++)
					{
						pnewitem_list[j].order = ptransaction[j+i+1];
						pnewitem_list[j].support = frequency;
						pnewitem_list[j].ntrans = ntrans;
					}
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = length-i-1;
					break;
				}
			}
			else 
			{
				pcurnode = pcurnode->pchild;
				while(pcurnode!=NULL && pcurnode->order<order)
				{
					pprevnode = pcurnode;
					pcurnode = pcurnode->prightsibling;
				}

				if(pcurnode!=NULL && pcurnode->order==order)
				{
					pcurnode->frequency += frequency;
					pcurnode->ntrans += ntrans;
				}
				else 
				{
					//goMemTracer.AddOneAFOPTNode();
					pnewnode = new AFOPT_NODE;
					pnewnode->frequency = frequency;
					pnewnode->ntrans = ntrans;
					pnewnode->prightsibling = pcurnode;
					pnewnode->order = order;
					pprevnode->prightsibling = pnewnode;
					if(i==length-1)
					{
						pnewnode->pchild = NULL;
						pnewnode->flag = -1;
					}
					else
					{
						pnewitem_list = new ARRAY_NODE[length-i-1];
						//goMemTracer.AddSingleBranch(length-i-1);
						for(j=0;j<length-i-1;j++)
						{
							pnewitem_list[j].order = ptransaction[j+i+1];
							pnewitem_list[j].support = frequency;
							pnewitem_list[j].ntrans = ntrans;
						}
						pnewnode->pitemlist = pnewitem_list;
						pnewnode->flag = length-i-1;
						break;
					}				
				}
			}
		}
		else 
		{
			pitem_list = pcurnode->pitemlist;
			list_len = pcurnode->flag;

			j = 0;
			while(j<list_len && (i+j< length) && pitem_list[j].order == ptransaction[i+j])
			{
				pitem_list[j].support += frequency;
				pitem_list[j].ntrans += ntrans;
				j++;
			}

			if(i+j==length)
			{
				break;
			}
			else if(j==list_len)
			{
				pnewitem_list = new ARRAY_NODE[length-i];
				//goMemTracer.AddSingleBranch(length-i);
				for(t=0;t<j;t++)
					pnewitem_list[t] = pitem_list[t];
				for(t=j;t<length-i;t++)
				{
					pnewitem_list[t].order = ptransaction[i+t];
					pnewitem_list[t].support = frequency;
					pnewitem_list[t].ntrans = ntrans;
				}
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);
				pcurnode->pitemlist = pnewitem_list;
				pcurnode->flag = length-i;
				break;
			}
			else 
			{
				for(t=0;t<j;t++)
				{
					//goMemTracer.AddOneAFOPTNode();
					pnewnode = new AFOPT_NODE;
					pnewnode->frequency = pitem_list[t].support;
					pnewnode->ntrans = pitem_list[t].ntrans;
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
					pnewnode->prightsibling = NULL;
					pnewnode->order = pitem_list[t].order;
					pcurnode->pchild = pnewnode;
					pcurnode->flag = 0;
					pcurnode = pnewnode;
				}

				pnewnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				pnewnode->frequency = pitem_list[j].support;
				pnewnode->ntrans = pitem_list[j].ntrans;
				pnewnode->order = pitem_list[j].order;
				pnewnode->prightsibling = NULL;
				if(j==list_len-1)
				{
					pnewnode->flag = -1;
					pnewnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[list_len-1-j];
					//goMemTracer.AddSingleBranch(list_len-j-1);
					for(t=0;t<list_len-j-1;t++)
						pnewitem_list[t] = pitem_list[t+j+1];
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = list_len-1-j;
				}

				ptnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				ptnode->frequency = frequency;
				ptnode->ntrans = ntrans;
				ptnode->order = ptransaction[i+j];
				ptnode->prightsibling = NULL;
				if(i+j == length-1)
				{
					ptnode->flag = -1;
					ptnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[length-1-j-i];
					//goMemTracer.AddSingleBranch(length-1-j-i);
					for(t=0;t<length-1-j-i;t++)
					{
						pnewitem_list[t].order = ptransaction[t+j+1+i];
						pnewitem_list[t].support = frequency;
						pnewitem_list[t].ntrans = ntrans;
					}
					ptnode->pitemlist = pnewitem_list;
					ptnode->flag = length-1-i-j;
				}

				if(pnewnode->order<ptnode->order)
				{
					pcurnode->pchild = pnewnode;
					pnewnode->prightsibling = ptnode;
				}
				else 
				{
					pcurnode->pchild = ptnode;
					ptnode->prightsibling = pnewnode;
				}
				pcurnode->flag = 0;
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);

				break;
			}
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::InsertTransaction(pht, nip, pt, l, f, t)\n");
#endif
}

//-------------------------------------------------------------------------------
//insert a single branch in an AFOPT-tree
//-------------------------------------------------------------------------------

void CAFOPTMine::InsertTransaction(HEADER_TABLE pheader_table, int nitem_pos, ARRAY_NODE* ptransaction, int length)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::InsertTransaction(pht, nip, pt, l)\n");
#endif
	int i, j, order, orig_frequency, list_len , t;
	AFOPT_NODE *pnewnode, *pcurnode, *pprevnode, *ptnode;
	ARRAY_NODE *pitem_list, *pnewitem_list;

	if(pheader_table[nitem_pos].pafopt_conddb==NULL)
	{
		pnewnode = new AFOPT_NODE;
		//goMemTracer.AddOneAFOPTNode();
		pnewnode->order = nitem_pos;
		pnewnode->frequency = 0;
		pnewnode->ntrans = 0;
		pnewnode->flag = -1;
		pnewnode->pchild = NULL;
		pnewnode->prightsibling = NULL;
		pheader_table[nitem_pos].pafopt_conddb = pnewnode;
	}

	pprevnode = NULL;
	pcurnode = pheader_table[nitem_pos].pafopt_conddb;
	pcurnode->frequency += ptransaction[0].support;
	pcurnode->ntrans += ptransaction[0].ntrans;
	
	for(i=0;i<length;i++)
	{
		if(pcurnode->flag == -1)
		{
			pnewitem_list = new ARRAY_NODE[length-i];
			//goMemTracer.AddSingleBranch(length-i);
			for(j=0;j<length-i;j++)
			{
				pnewitem_list[j].order = ptransaction[j+i].order;
				pnewitem_list[j].support = ptransaction[j+i].support;
				pnewitem_list[j].ntrans = ptransaction[j+i].ntrans;
			}
			pcurnode->pitemlist = pnewitem_list;
			pcurnode->flag = length-i;
			break;
		}
		else if(pcurnode->flag == 0)
		{
			order = ptransaction[i].order;
			if(pcurnode->pchild->order > order)
			{
				//goMemTracer.AddOneAFOPTNode();
				pnewnode = new AFOPT_NODE;
				pnewnode->frequency = ptransaction[i].support;
				pnewnode->ntrans = ptransaction[i].ntrans;
				pnewnode->prightsibling = pcurnode->pchild;
				pnewnode->order = order;
				pcurnode->pchild = pnewnode;
				if(i==length-1)
				{
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
				}
				else
				{
					pnewitem_list = new ARRAY_NODE[length-i-1];
					//goMemTracer.AddSingleBranch(length-i-1);
					for(j=0;j<length-i-1;j++)
					{
						pnewitem_list[j].order = ptransaction[j+i+1].order;
						pnewitem_list[j].support = ptransaction[j+i+1].support;
						pnewitem_list[j].ntrans = ptransaction[j+i+1].ntrans;
					}
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = length-i-1;
					break;
				}
			}
			else 
			{
				pcurnode = pcurnode->pchild;
				while(pcurnode!=NULL && pcurnode->order<order)
				{
					pprevnode = pcurnode;
					pcurnode = pcurnode->prightsibling;
				}

				if(pcurnode!=NULL && pcurnode->order==order)
				{
					pcurnode->frequency += ptransaction[i].support;
					pcurnode->ntrans += ptransaction[i].ntrans;
				}
				else 
				{
					//goMemTracer.AddOneAFOPTNode();
					pnewnode = new AFOPT_NODE;
					pnewnode->frequency = ptransaction[i].support;
					pnewnode->ntrans = ptransaction[i].ntrans;
					pnewnode->prightsibling = pcurnode;
					pnewnode->order = order;
					pprevnode->prightsibling = pnewnode;
					if(i==length-1)
					{
						pnewnode->flag = -1;
						pnewnode->pchild = NULL;
					}
					else
					{
						pnewitem_list = new ARRAY_NODE[length-i-1];
						//goMemTracer.AddSingleBranch(length-i-1);
						for(j=0;j<length-i-1;j++)
						{
							pnewitem_list[j].order = ptransaction[j+i+1].order;
							pnewitem_list[j].support = ptransaction[j+i+1].support;
							pnewitem_list[j].ntrans = ptransaction[j+i+1].ntrans;
						}
						pnewnode->pitemlist = pnewitem_list;
						pnewnode->flag = length-i-1;
						break;
					}				
				}
			}
		}
		else 
		{
			pitem_list = pcurnode->pitemlist;
			list_len = pcurnode->flag;
			orig_frequency = pcurnode->frequency;

			j = 0;
			while(j<list_len && (i+j< length) && pitem_list[j].order == ptransaction[i+j].order)
			{
				pitem_list[j].support += ptransaction[i+j].support;
				pitem_list[j].ntrans += ptransaction[i+j].ntrans;
				j++;
			}

			if(i+j==length)
			{
				break;
			}
			else if(j==list_len)
			{
				pnewitem_list = new ARRAY_NODE[length-i];
				//goMemTracer.AddSingleBranch(length-i);
				for(t=0;t<j;t++)
					pnewitem_list[t] = pitem_list[t];
				for(t=j;t<length-i;t++)
				{
					pnewitem_list[t] = ptransaction[i+t];
				}
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);
				pcurnode->pitemlist = pnewitem_list;
				pcurnode->flag = length-i;
				break;
			}
			else 
			{
				for(t=0;t<j;t++)
				{
					pnewnode = new AFOPT_NODE;
					//goMemTracer.AddOneAFOPTNode();
					pnewnode->frequency = pitem_list[t].support;
					pnewnode->ntrans = pitem_list[t].ntrans;
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
					pnewnode->prightsibling = NULL;
					pnewnode->order = pitem_list[t].order;
					pcurnode->pchild = pnewnode;
					pcurnode->flag = 0;
					pcurnode = pnewnode;
				}

				pnewnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				pnewnode->frequency = pitem_list[j].support;
				pnewnode->ntrans = pitem_list[j].ntrans;
				pnewnode->order = pitem_list[j].order;
				pnewnode->prightsibling = NULL;
				if(j==list_len-1)
				{
					pnewnode->flag = -1;
					pnewnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[list_len-1-j];
					//goMemTracer.AddSingleBranch(list_len-j-1);
					for(t=0;t<list_len-j-1;t++)
						pnewitem_list[t] = pitem_list[t+j+1];
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = list_len-1-j;
				}

				ptnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				ptnode->frequency = ptransaction[i+j].support;
				ptnode->ntrans = ptransaction[i+j].ntrans;
				ptnode->order = ptransaction[i+j].order;
				ptnode->prightsibling = NULL;
				if(i+j == length-1)
				{
					ptnode->flag = -1;
					ptnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[length-1-j-i];
					//goMemTracer.AddSingleBranch(length-1-j-i);
					for(t=0;t<length-1-j-i;t++)
					{
						pnewitem_list[t].order = ptransaction[t+j+1+i].order;
						pnewitem_list[t].support = ptransaction[t+j+1+i].support;
						pnewitem_list[t].ntrans = ptransaction[t+j+1+i].ntrans;
					}
					ptnode->pitemlist = pnewitem_list;
					ptnode->flag = length-1-i-j;
				}

				if(pnewnode->order<ptnode->order)
				{
					pcurnode->pchild = pnewnode;
					pnewnode->prightsibling = ptnode;
				}
				else 
				{
					pcurnode->pchild = ptnode;
					ptnode->prightsibling = pnewnode;
				}
				pcurnode->flag = 0;
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);

				break;
			}
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::InsertTransaction(pht, nip, pt, l)\n");
#endif
}

//-------------------------------------------------------------------------------
//insert a single branch in an AFOPT-tree. 
//This procedure is called by MergeTwoTree procedure
//-------------------------------------------------------------------------------
void CAFOPTMine::InsertTransaction(AFOPT_NODE *ptreeroot, ARRAY_NODE* ptransaction, int length)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::InsertTransaction(ptr, pt, l)\n");
#endif
	int i, j, order, orig_frequency, list_len , t;
	AFOPT_NODE *pnewnode, *pcurnode, *pprevnode, *ptnode;
	ARRAY_NODE *pitem_list, *pnewitem_list;


	pprevnode = NULL;
	pcurnode = ptreeroot;
//	pcurnode->frequency += ptransaction[0].support;
	
	for(i=0;i<length;i++)
	{
		if(pcurnode->flag == -1)
		{
			pnewitem_list = new ARRAY_NODE[length-i];
			//goMemTracer.AddSingleBranch(length-i);
			for(j=0;j<length-i;j++)
			{
				pnewitem_list[j].order = ptransaction[j+i].order;
				pnewitem_list[j].support = ptransaction[j+i].support;
				pnewitem_list[j].ntrans = ptransaction[j+i].ntrans;
			}
			pcurnode->pitemlist = pnewitem_list;
			pcurnode->flag = length-i;
			break;
		}
		else if(pcurnode->flag == 0)
		{
			order = ptransaction[i].order;
			if(pcurnode->pchild->order > order)
			{
				//goMemTracer.AddOneAFOPTNode();
				pnewnode = new AFOPT_NODE;
				pnewnode->frequency = ptransaction[i].support;
				pnewnode->ntrans = ptransaction[i].ntrans;
				pnewnode->prightsibling = pcurnode->pchild;
				pnewnode->order = order;
				pcurnode->pchild = pnewnode;
				if(i==length-1)
				{
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
				}
				else
				{
					pnewitem_list = new ARRAY_NODE[length-i-1];
					//goMemTracer.AddSingleBranch(length-i-1);
					for(j=0;j<length-i-1;j++)
					{
						pnewitem_list[j].order = ptransaction[j+i+1].order;
						pnewitem_list[j].support = ptransaction[j+i+1].support;
						pnewitem_list[j].ntrans = ptransaction[j+i+1].ntrans;
					}
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = length-i-1;
					break;
				}
			}
			else 
			{
				pcurnode = pcurnode->pchild;
				while(pcurnode!=NULL && pcurnode->order<order)
				{
					pprevnode = pcurnode;
					pcurnode = pcurnode->prightsibling;
				}

				if(pcurnode!=NULL && pcurnode->order==order)
				{
					pcurnode->frequency += ptransaction[i].support;
					pcurnode->ntrans += ptransaction[i].ntrans;
				}
				else 
				{
					//goMemTracer.AddOneAFOPTNode();
					pnewnode = new AFOPT_NODE;
					pnewnode->frequency = ptransaction[i].support;
					pnewnode->ntrans = ptransaction[i].ntrans;
					pnewnode->prightsibling = pcurnode;
					pnewnode->order = order;
					pprevnode->prightsibling = pnewnode;
					if(i==length-1)
					{
						pnewnode->flag = -1;
						pnewnode->pchild = NULL;
					}
					else
					{
						pnewitem_list = new ARRAY_NODE[length-i-1];
						//goMemTracer.AddSingleBranch(length-i-1);
						for(j=0;j<length-i-1;j++)
						{
							pnewitem_list[j].order = ptransaction[j+i+1].order;
							pnewitem_list[j].support = ptransaction[j+i+1].support;
							pnewitem_list[j].ntrans = ptransaction[j+i+1].ntrans;
						}
						pnewnode->pitemlist = pnewitem_list;
						pnewnode->flag = length-i-1;
						break;
					}				
				}
			}
		}
		else 
		{
			pitem_list = pcurnode->pitemlist;
			list_len = pcurnode->flag;
			orig_frequency = pcurnode->frequency;

			j = 0;
			while(j<list_len && (i+j< length) && pitem_list[j].order == ptransaction[i+j].order)
			{
				pitem_list[j].support += ptransaction[i+j].support;
				pitem_list[j].ntrans += ptransaction[i+j].ntrans;
				j++;
			}

			if(i+j==length)
			{
				break;
			}
			else if(j==list_len)
			{
				pnewitem_list = new ARRAY_NODE[length-i];
				//goMemTracer.AddSingleBranch(length-i);
				for(t=0;t<j;t++)
					pnewitem_list[t] = pitem_list[t];
				for(t=j;t<length-i;t++)
				{
					pnewitem_list[t] = ptransaction[i+t];
				}
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);
				pcurnode->pitemlist = pnewitem_list;
				pcurnode->flag = length-i;
				break;
			}
			else 
			{
				for(t=0;t<j;t++)
				{
					pnewnode = new AFOPT_NODE;
					//goMemTracer.AddOneAFOPTNode();
					pnewnode->frequency = pitem_list[t].support;
					pnewnode->ntrans = pitem_list[t].ntrans;
					pnewnode->pchild = NULL;
					pnewnode->flag = -1;
					pnewnode->prightsibling = NULL;
					pnewnode->order = pitem_list[t].order;
					pcurnode->pchild = pnewnode;
					pcurnode->flag = 0;
					pcurnode = pnewnode;
				}

				pnewnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				pnewnode->frequency = pitem_list[j].support;
				pnewnode->ntrans = pitem_list[j].ntrans;
				pnewnode->order = pitem_list[j].order;
				pnewnode->prightsibling = NULL;
				if(j==list_len-1)
				{
					pnewnode->flag = -1;
					pnewnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[list_len-1-j];
					//goMemTracer.AddSingleBranch(list_len-j-1);
					for(t=0;t<list_len-j-1;t++)
						pnewitem_list[t] = pitem_list[t+j+1];
					pnewnode->pitemlist = pnewitem_list;
					pnewnode->flag = list_len-1-j;
				}

				ptnode = new AFOPT_NODE;
				//goMemTracer.AddOneAFOPTNode();
				ptnode->frequency = ptransaction[i+j].support;
				ptnode->ntrans = ptransaction[i+j].ntrans;
				ptnode->order = ptransaction[i+j].order;
				ptnode->prightsibling = NULL;
				if(i+j == length-1)
				{
					ptnode->flag = -1;
					ptnode->pchild = NULL;
				}
				else 
				{
					pnewitem_list = new ARRAY_NODE[length-1-j-i];
					//goMemTracer.AddSingleBranch(length-1-j-i);
					for(t=0;t<length-1-j-i;t++)
					{
						pnewitem_list[t].order = ptransaction[t+j+1+i].order;
						pnewitem_list[t].support = ptransaction[t+j+1+i].support;
						pnewitem_list[t].ntrans = ptransaction[t+j+1+i].ntrans;
					}
					ptnode->pitemlist = pnewitem_list;
					ptnode->flag = length-1-i-j;
				}

				if(pnewnode->order<ptnode->order)
				{
					pcurnode->pchild = pnewnode;
					pnewnode->prightsibling = ptnode;
				}
				else 
				{
					pcurnode->pchild = ptnode;
					ptnode->prightsibling = pnewnode;
				}
				pcurnode->flag = 0;
				delete []pitem_list;
				//goMemTracer.DelSingleBranch(list_len);

				break;
			}
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::InsertTransaction(ptr, pt, l)\n");
#endif
}

//-------------------------------------------------------------------------------
//release the space occupied by an AFOPT-tree
//-------------------------------------------------------------------------------
void CAFOPTMine::DestroyTree(AFOPT_NODE * proot)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering CAFOPTMine::DestroyTree\n");
#endif
	AFOPT_NODE *ptreenode, *pnextchild;
	int source_top;

	if(proot->flag == -1)
	{
		delete proot;
		//goMemTracer.DelOneAFOPTNode();
		return;
	}
	else if(proot->flag>0)
	{
		delete []proot->pitemlist;
		//goMemTracer.DelSingleBranch(proot->flag);
		delete proot;
		//goMemTracer.DelOneAFOPTNode();
		return;
	}
	
	ptreenode = proot;
	source_top = 0;

	mpsource_stack[source_top].pnext_child = ptreenode->pchild;
	mpsource_stack[source_top].pnode = ptreenode;
	source_top++;

	while(source_top>0)
	{
		source_top--;
		pnextchild = mpsource_stack[source_top].pnext_child;
		if(pnextchild!=NULL)
		{
			mpsource_stack[source_top].pnext_child = pnextchild->prightsibling;
			source_top++;
			if(pnextchild->flag==0)
			{
				mpsource_stack[source_top].pnext_child = pnextchild->pchild;
				mpsource_stack[source_top].pnode = pnextchild;
				source_top++;
			}
			else if(pnextchild->flag==-1)
			{
				delete pnextchild;
				//goMemTracer.DelOneAFOPTNode();
			}
			else 
			{
				delete []pnextchild->pitemlist;
				//goMemTracer.DelSingleBranch(pnextchild->flag);
				delete pnextchild;
				//goMemTracer.DelOneAFOPTNode();
			}
		}
		else 
		{
			delete mpsource_stack[source_top].pnode;
			//goMemTracer.DelOneAFOPTNode();
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving CAFOPTMine::DestroyTree\n");
#endif
}

//-------------------------------------------------------------------------------
//initialize some varibles used when traversal a tree
//-------------------------------------------------------------------------------
void CAFOPTMine::Init(int nmax_depth)
{
	mpbranch = new int[nmax_depth];
	mptransaction = new int[nmax_depth];
	mpfrequencies = new int[nmax_depth+1];
	mpntrans = new int[nmax_depth+1];
	mpsource_stack = new STACK_NODE[nmax_depth];
	mpdest_stack = new STACK_NODE[nmax_depth];
}
void CAFOPTMine::CleanUp() {
	if(mpsource_stack!=NULL)
		delete []mpsource_stack;
	if(mpdest_stack!=NULL)
		delete []mpdest_stack;
	if(mptransaction!=NULL)
		delete []mptransaction;
	if(mpbranch!=NULL)
		delete []mpbranch;
	if(mpfrequencies!=NULL)
		delete []mpfrequencies;
	if(mpntrans!=NULL)
		delete []mpntrans;
	mpsource_stack = NULL;
	mpdest_stack = NULL;
	mptransaction = NULL;
	mpbranch = NULL;
	mpfrequencies = NULL;
	mpntrans = NULL;
}

CAFOPTMine::CAFOPTMine()
{
	mpsource_stack = NULL;
	mpdest_stack = NULL;
	mptransaction = NULL;
	mpbranch = NULL;
	mpfrequencies = NULL;
	mpntrans = NULL;
}

CAFOPTMine::~CAFOPTMine()
{
	CleanUp();
}
