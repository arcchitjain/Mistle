
#include "../clobal.h"

#include <time.h>

#include <Bass.h>
#include <db/Database.h>
#include <TimeUtils.h>
#include <Primitives.h>

#include "closed/cfptree_outbuf.h"
#include "ScanDBMine.h"
#include "../Croupier.h"
#include "AFOPTMine.h"
#include "parameters.h"
#include "fsout.h"
#include "data.h"

#include "AfoptMiner.h"

using namespace Afopt;

// Generic
int *Afopt::gpbuckets;
int *Afopt::gtbuckets;

// Closed
ITEM_COUNTER *Afopt::gpfreqitems;
int  Afopt::gntree_level;
int *Afopt::gpitem_order_map;
MAP_NODE** Afopt::gpitem_sup_map;
int Afopt::gnprune_times;
int Afopt::gnmap_pruned_times;
CTreeOutBufManager Afopt::gotree_bufmanager;

AfoptMiner::AfoptMiner(Database *db, AfoptCroupier *cr, const uint32 minSup, const string &type, MineGoal goal) {
	mDatabase = db;
	mMinSup = minSup;

	goparameters.results_to_file = false;
	goparameters.bresult_name_given = true;
	if(goal == MINE_GOAL_HISTOGRAM_SUPPORT)
		gpout = new HistogramOut(db, cr);		
	else if(goal == MINE_GOAL_HISTOGRAM_SETLEN)
		gpout = new HistogramOut(db, cr, true);
	else // MINE_GOAL_PATTERNS
		gpout = new MemoryOut(db, cr);
	strcpy_s(goparameters.data_filename, MAX_FILENAME_LEN, db->GetName().c_str());

	if(type.compare("all") == 0) {
		mType = ALL_FREQUENT_PATTERNS;
	} else if(type.compare("closed") == 0) {
		mType = CLOSED_FREQUENT_PATTERNS;
	} else 
		THROW("Can only mine all or closed frequent item sets.");

	strcpy_s(goparameters.pattern_type, 4, GetPatternTypeStr().c_str());

}
AfoptMiner::~AfoptMiner() {
	delete gpout;
}

string AfoptMiner::GetPatternTypeStr() {
	if(mType == ALL_FREQUENT_PATTERNS)
		return "all";
	else if(mType == CLOSED_FREQUENT_PATTERNS)
		return "cls";
	THROW("Unknown FrequentPatternType");
}

void AfoptMiner::MineOnline() {
	if(mType == ALL_FREQUENT_PATTERNS) {
		AfoptMiner::MineAll(mDatabase, mMinSup);
	} else if(mType == CLOSED_FREQUENT_PATTERNS) {
		AfoptMiner::MineClosed(mDatabase, mMinSup);
	}
}

void AfoptMiner::MineHistogram() {
	if(mType == ALL_FREQUENT_PATTERNS) {
		AfoptMiner::MineAll(mDatabase, mMinSup);
	} else if(mType == CLOSED_FREQUENT_PATTERNS) {
		AfoptMiner::MineClosed(mDatabase, mMinSup);
	}
}

void AfoptMiner::MineAllPatternsToFile(Database *db, const uint32 minSup, const string &outputFile) {
	strcpy_s(goparameters.pattern_type, MAX_FILENAME_LEN, "all");
	goparameters.results_to_file = true;
	goparameters.bresult_name_given = true;
	strcpy_s(goparameters.data_filename, MAX_FILENAME_LEN, db->GetName().c_str());
	strcpy_s(goparameters.pat_filename, MAX_FILENAME_LEN, outputFile.c_str());

	gpout = new FSout(goparameters.pat_filename);

	MineAll(db, minSup);

	delete gpout;
}

void AfoptMiner::MineClosedPatternsToFile(Database *db, const uint32 minSup, const string &outputFile) {
	goparameters.results_to_file = true;
	goparameters.bresult_name_given = true;
	strcpy_s(goparameters.pattern_type, MAX_FILENAME_LEN, "cls");
	strcpy_s(goparameters.data_filename, MAX_FILENAME_LEN, db->GetName().c_str());
	goparameters.SetResultName(outputFile.c_str());	// sets pat_filename
	gpout = new FSout(goparameters.pat_filename);

	MineClosed(db, minSup);

	delete gpout;
}

void AfoptMiner::MineAll(Database *db, const uint32 minSup) {
	goparameters.nmin_sup = minSup;
	gnmin_sup = goparameters.nmin_sup;

	ITEM_COUNTER* pfreqitems;
	int *pitem_order_map;
	double ntotal_occurrences;

	int i;

	gntotal_patterns = 0;
	gnmax_pattern_len = 0;
	gntotal_singlepath = 0;

	gndepth = 0;
	gntotal_call = 0;
	gpdata = new Data(db, gpout);

	//count frequent items in original database
	pfreqitems = NULL;
	goDBMiner.ScanDBCountFreqItems(&pfreqitems);
	//goTimeTracer.mdInitial_count_time = (double)(clock()-start)/CLOCKS_PER_SEC;

	gppat_counters = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));
	for(i=0;i<gnmax_trans_len;i++)
		gppat_counters[i] = 0;

	bool initedGoAFOPTMiner = false;
	goAFOPTMiner.Init(MIN(gnmax_trans_len, gntotal_freqitems+1));
	initedGoAFOPTMiner = true;

	if(gntotal_freqitems==0)
		delete gpdata;
	else if(gntotal_freqitems==1)
	{
		gnmax_pattern_len = 1;
		gpheader_itemset = new int[1];
		OutputOnePattern(pfreqitems[0].item, pfreqitems[0].support, pfreqitems[0].ntrans);
		delete[] gpheader_itemset;
		delete gpdata;
	}
	else if(gntotal_freqitems>1)
	{
		gpheader_itemset = new int[gntotal_freqitems];
		//goMemTracer.IncBuffer(gntotal_freqitems*sizeof(int));

		//an array which maps an item to its frequency order; the most infrequent item has order 0
		pitem_order_map = new int[gnmax_item_id];
		for(i=0;i<gnmax_item_id;i++)
			pitem_order_map[i] = -1;
		for(i=0;i<gntotal_freqitems;i++)
			pitem_order_map[pfreqitems[i].item] = i;

		//if the number of frequent items in a conditonal database is smaller than maximal bucket size, use bucket counting technique
		if(gntotal_freqitems <= MAX_BUCKET_SIZE) {
			gpbuckets = new int[(uint32)(1<<gntotal_freqitems)];
			gtbuckets = new int[(uint32)(1<<gntotal_freqitems)];
			memset(gpbuckets, 0, sizeof(int)*(uint32)(1<<gntotal_freqitems));
			memset(gtbuckets, 0, sizeof(int)*(uint32)(1<<gntotal_freqitems));
			goDBMiner.ScanDBBucketCount(pitem_order_map, gpbuckets, gtbuckets);
			bucket_count(gpbuckets, gtbuckets, gntotal_freqitems, pfreqitems);
			delete []gtbuckets;
			delete []gpbuckets;
			delete gpdata;
			delete []pitem_order_map;
		}
		else
		{
			HEADER_TABLE pheader_table;

			gtbuckets = new int[(1<<MAX_BUCKET_SIZE)];
			gpbuckets = new int[(1<<MAX_BUCKET_SIZE)];
			//goMemTracer.IncBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

			//initialize header table
			pheader_table = new HEADER_NODE[gntotal_freqitems];
			//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*gntotal_freqitems);
			ntotal_occurrences = 0.0;
			for(i=0;i<gntotal_freqitems;i++)
			{
				pheader_table[i].item = pfreqitems[i].item;
				pheader_table[i].nsupport = pfreqitems[i].support;
				pheader_table[i].ntrans = pfreqitems[i].ntrans;
				pheader_table[i].parray_conddb = NULL;
				pheader_table[i].order = i;
				ntotal_occurrences += pfreqitems[i].support;
			}

			//to choose a proper representation format for each conditional database
			//			if((double)goparameters.nmin_sup/gndb_size>=BUILD_TREE_MINSUP_THRESHOLD || 
			//				ntotal_occurrences/(gndb_size*gntotal_freqitems)>=BUILD_TREE_AVGSUP_THRESHOLD ||
			//				gntotal_freqitems<=BUILD_TREE_ITEM_THRESHOLD )
			//			{
			for(i=0;i<gntotal_freqitems;i++)
				pheader_table[i].flag = AFOPT_FLAG;
			//			}
			//			else
			//			{
			//				for(i=0;i<gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD;i++)
			//					pheader_table[i].flag = 0;
			//				for(i=MAX(0, gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD);i<gntotal_freqitems;i++)
			//					pheader_table[i].flag = AFOPT_FLAG;
			//			}

			//scan database to construct conditional databases and do bucket counting
			memset(gpbuckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
			memset(gtbuckets, 0, sizeof(int)*(1<<MAX_BUCKET_SIZE));
			goDBMiner.ScanDBBuildCondDB(pheader_table, pitem_order_map, gntotal_freqitems, gpbuckets, gtbuckets);
			//goTimeTracer.mdInitial_construct_time = (double)(clock()-start)/CLOCKS_PER_SEC;
			//goMemTracer.mninitial_struct_size = //goMemTracer.mnArrayDB_size+//goMemTracer.mnAFOPTree_size+sizeof(int)*(1<<MAX_BUCKET_SIZE);			
			bucket_count(gpbuckets, gtbuckets, MAX_BUCKET_SIZE, &(pfreqitems[gntotal_freqitems-MAX_BUCKET_SIZE]));

			delete []pitem_order_map;
			delete gpdata;

			//mining frequent itemsets in depth first order
			//			if((double)goparameters.nmin_sup/gndb_size>=BUILD_TREE_MINSUP_THRESHOLD ||
			//				ntotal_occurrences/(gndb_size*gntotal_freqitems)>=BUILD_TREE_AVGSUP_THRESHOLD ||
			//				gntotal_freqitems<=BUILD_TREE_ITEM_THRESHOLD)
			goAFOPTMiner.DepthAFOPTGrowthAll(pheader_table, gntotal_freqitems, 0);
			//			else
			//			{
			//				goArrayMiner.DepthArrayGrowth(pheader_table, gntotal_freqitems);
			//				goAFOPTMiner.DepthAFOPTGrowth(pheader_table, gntotal_freqitems, gntotal_freqitems-BUILD_TREE_ITEM_THRESHOLD);
			//			}

			delete []pheader_table;
			//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(HEADER_NODE));
			delete []gpbuckets;
			delete []gtbuckets;
			//goMemTracer.DecBuffer(sizeof(int)*(1<<MAX_BUCKET_SIZE));

		}
		delete []gpheader_itemset;
		//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(int));
	}
	delete []pfreqitems;
	//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(ITEM_COUNTER));

	//if(goparameters.bresult_name_given)
	gpout->finished();
	//delete gpout;
	//goTimeTracer.mdtotal_running_time = (double)(clock()-start_mining)/CLOCKS_PER_SEC;

	//PrintSummary();
	delete []gppat_counters;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));

	if(initedGoAFOPTMiner == true)
		goAFOPTMiner.CleanUp();
}

void AfoptMiner::MineClosed(Database *db, const uint32 minSup) {
	goparameters.nmin_sup = minSup;
	gnmin_sup = goparameters.nmin_sup;

	int i, base_bitmap;
	double dtotal_occurences;
	CFP_NODE cfp_node;

	gntotal_freqitems = 0;
	gnmax_pattern_len = 0;
	gntotal_singlepath = 0;
	gntotal_patterns = 0;

	gntree_level = 0; 
	gnfirst_level = 0;
	gnfirst_level_depth = 0;
	gntotal_call = 0;

	gpdata = new Data(db, gpout);

	gpfreqitems = NULL;
	goDBMiner.ScanDBCountFreqItems(&gpfreqitems);
	//goTimeTracer.mdInitial_count_time = (double)(clock()-start_mining)/CLOCKS_PER_SEC;

	goAFOPTMiner.Init(MIN(gnmax_trans_len, gntotal_freqitems+1));

	string tempFile = Bass::GetWorkingDir() + "afopt-cls-miner" + TimeUtils::GetTimeStampString() + ".tmp";
	strcpy_s(goparameters.tree_filename, MAX_FILENAME_LEN, tempFile.c_str());
	goparameters.SetBufParameters(sizeof(ENTRY));
	gotree_bufmanager.Init(gnmax_trans_len);

	gpheader_itemset = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));
	gndepth = 0;
	gppat_counters = new int[gnmax_trans_len];
	//goMemTracer.IncBuffer(gnmax_trans_len*sizeof(int));
	for(i=0;i<gnmax_trans_len;i++)
		gppat_counters[i] = 0;

	gnprune_times = 0;
	gnmap_pruned_times = 0;

	while(gntotal_freqitems>0 && gpfreqitems[gntotal_freqitems-1].support==gndb_size)
	{
		gpheader_itemset[gndepth] = gpfreqitems[gntotal_freqitems-1].item;
		gndepth++;
		gntotal_freqitems--;
	}

	base_bitmap = 0;
	if(gndepth>0)
	{
		gnfirst_level = 1;
		gnfirst_level_depth = gndepth;

		OutputOneClosePat(gndb_size, gndb_ntrans);
		for(i=0;i<gntotal_freqitems;i++)
			base_bitmap = base_bitmap | (1<<(gpfreqitems[i].item%INT_BIT_LEN));
		gotree_bufmanager.InsertCommonPrefixNode(gntree_level, gpheader_itemset, gndepth, gndb_size, gndb_ntrans, base_bitmap);
		gntree_level++;
	}

	if(gntotal_freqitems==0)
		delete gpdata;
	else if(gntotal_freqitems==1)
	{
		gpheader_itemset[gndepth] = gpfreqitems[0].item;
		OutputOneClosePat(1, gpfreqitems[0].support, gpfreqitems[0].ntrans);
		gotree_bufmanager.WriteLeafNode(&(gpfreqitems[0].item), 1, gpfreqitems[0].support, gpfreqitems[0].ntrans, 0);
		delete gpdata;
	}
	else if(gntotal_freqitems>1)
	{
		HEADER_TABLE pheader_table;

		gpitem_order_map = new int[gnmax_item_id];
		for(i=0;i<gnmax_item_id;i++)
			gpitem_order_map[i] = -1;
		for(i=0;i<gntotal_freqitems;i++)
			gpitem_order_map[gpfreqitems[i].item] = i;

		gpitem_sup_map = new MAP_NODE*[gntotal_freqitems];
		//goMemTracer.IncTwoLayerMap(gntotal_freqitems*sizeof(MAP_NODE*));
		for(i=0;i<gntotal_freqitems;i++)
			gpitem_sup_map[i] = NULL;

		pheader_table = new HEADER_NODE[gntotal_freqitems];
		//goMemTracer.IncBuffer(sizeof(HEADER_NODE)*gntotal_freqitems);
		dtotal_occurences = 0;
		for(i=0;i<gntotal_freqitems;i++)
		{
			pheader_table[i].item = gpfreqitems[i].item;
			pheader_table[i].nsupport = gpfreqitems[i].support;
			pheader_table[i].ntrans = gpfreqitems[i].ntrans;
			pheader_table[i].order = i;
			pheader_table[i].pafopt_conddb = NULL;
			dtotal_occurences += gpfreqitems[i].support;
		}
		for(i=0;i<gntotal_freqitems;i++)
			pheader_table[i].flag = AFOPT_FLAG;
		goDBMiner.ScanDBBuildCondDB(pheader_table, gpitem_order_map);
		//goTimeTracer.mdInitial_construct_time = (double)(clock()-start)/CLOCKS_PER_SEC;
		//goMemTracer.mninitial_struct_size = //goMemTracer.mnAFOPTree_size+//goMemTracer.mnArrayDB_size;
		delete gpdata;

		cfp_node.pos = gotree_bufmanager.GetTreeSize();
		cfp_node.cur_order = 0;
		cfp_node.length = gntotal_freqitems;
		cfp_node.pentries = new ENTRY[cfp_node.length];
		//goMemTracer.IncCFPSize(cfp_node.length*sizeof(ENTRY));
		for(i=0;i<cfp_node.length;i++)
		{
			cfp_node.pentries[i].item = pheader_table[i].item;
			cfp_node.pentries[i].support = pheader_table[i].nsupport;
			cfp_node.pentries[i].ntrans = pheader_table[i].ntrans;
			cfp_node.pentries[i].hash_bitmap = 0;
			cfp_node.pentries[i].child = 0;
		}
		gotree_bufmanager.InsertActiveNode(gntree_level, &cfp_node);
		gntree_level++;

		goAFOPTMiner.CFIDepthAFOPTGrowth(pheader_table, gntotal_freqitems, 0, &cfp_node);

		gntree_level--;
		gotree_bufmanager.WriteInternalNode(&cfp_node);
		delete []cfp_node.pentries;
		//goMemTracer.DecCFPSize(cfp_node.length*sizeof(ENTRY));

		delete []pheader_table;
		//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(HEADER_NODE));

		delete []gpitem_order_map;
		for(i=0;i<gntotal_freqitems;i++)
		{
			if(gpitem_sup_map[i]!=NULL)
			{
				printf("The two-layer map for item %d should be empty\n", gpfreqitems[i].item);
				delete []gpitem_sup_map[i];
				//goMemTracer.DecTwoLayerMap(MIN(SUP_MAXLEN_MAP_SIZE, gpfreqitems[i].support-(int)gnmin_sup+1)*sizeof(MAP_NODE));
			}
		}
		delete []gpitem_sup_map;
		//goMemTracer.DecTwoLayerMap(gntotal_freqitems*sizeof(MAP_NODE*));
	}

	if(gndepth>0)
	{
		gotree_bufmanager.WriteCommonPrefixNodes(1, base_bitmap);
		gntree_level--;
	}

	gpout->finished();
	//delete gpout;
	goAFOPTMiner.CleanUp();

	delete []gpheader_itemset;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));
	gntotal_freqitems += gndepth;
	delete []gpfreqitems;
	//goMemTracer.DecBuffer(gntotal_freqitems*sizeof(ITEM_COUNTER));
	gotree_bufmanager.Destroy();

	//goTimeTracer.mdtotal_running_time = (double)(clock()-start_mining)/CLOCKS_PER_SEC;

	//PrintSummary();
	delete []gppat_counters;
	//goMemTracer.DecBuffer(gnmax_trans_len*sizeof(int));
}

// ----------------------- All -----------------------

int get_combnum(int n, int m)
{
	int i, ncombnum;

	if(m==n || m==0)
		return 1;
	if(m==1 || m==n-1)
		return n;

	if(m>n-m)
		m = n-m;
	ncombnum = 1; 
	for(i=0;i<m;i++)
		ncombnum = ncombnum*(n-i)/(i+1);

	return ncombnum;
}


void Afopt::OutputSinglePath(ITEM_COUNTER* pfreqitems, int length)
{
	SNODE*  pstack;
	int top;
	int next;
	int i;

	gntotal_patterns += (1<<length)-1;
	for(i=1;i<=length;i++)
		gppat_counters[gndepth+i] += get_combnum(length, i);
	if(gnmax_pattern_len<gndepth+length)
		gnmax_pattern_len = gndepth+length;

	if(length<=0)
		return;

	if(goparameters.bresult_name_given)
	{
		pstack = new SNODE[length+1];
		//goMemTracer.IncBuffer((length+1)*sizeof(SNODE));
		top = 0;

		pstack[top].item = -1;
		pstack[top].frequency = -1;
		pstack[top].ntrans = -1;
		pstack[top].next = top;

		top++;
		while(top>0)
		{
			top--;
			next = pstack[top].next;
			if(next<length)
			{
				pstack[top].next++;
				top++;
				pstack[top].item = pfreqitems[next].item;
				pstack[top].frequency = pfreqitems[next].support;
				pstack[top].ntrans = pfreqitems[next].ntrans;
				pstack[top].next = next+1;

				if(top>0)
				{
					for(i=1;i<=top;i++)
						gpheader_itemset[gndepth+i-1] = pstack[i].item;
					gpout->printSet(gndepth+top, gpheader_itemset, pstack[top].frequency, pstack[top].ntrans);
				}
				top++;
			}
			else if(next==length)
			{
				while(top>=0 && pstack[top].next==length)
					top--;
				top++;
			}

		}
		delete []pstack;
		//goMemTracer.DecBuffer((length+1)*sizeof(SNODE));
	}

}

int Afopt::bucket_count(int *Buckets, int *transBuckets, int no_of_freqitems, ITEM_COUNTER* pfreqitems)
{
#if VERBOSE
	fprintf(gpout->GetFile(), "Entering bucket_count\n");
#endif
	int i, j;
	unsigned int k;
	int num_of_freqpats, npat_len;

	int num_buckets;

	num_buckets = (1 << no_of_freqitems);

	for(i=0;i<no_of_freqitems;i++)
	{
		k = 1 << i;
		for(j=0;j<num_buckets;j++)
		{
			if(!(k & j))
			{
				Buckets[j] = Buckets[j] + Buckets[j+k];
				transBuckets[j] = transBuckets[j] + transBuckets[j+k];
			}
		}
	}

	num_of_freqpats = 0;
	for(i=1;i<num_buckets;i++)
	{
		if(Buckets[i]>=gnmin_sup)
		{
			npat_len = 0;
			for(j=0;j<no_of_freqitems;j++)
			{
				if(i & (1 << j))
				{
					gpheader_itemset[gndepth+npat_len] = pfreqitems[j].item;
					npat_len++;
				}
			}
			if(npat_len>0)
			{
				//if(goparameters.bresult_name_given)
					gpout->printSet(gndepth+npat_len, gpheader_itemset, Buckets[i], transBuckets[i]);
				gntotal_patterns++;
				gppat_counters[gndepth+npat_len]++;
				if(gnmax_pattern_len < gndepth+npat_len)
					gnmax_pattern_len = gndepth+npat_len;
				num_of_freqpats++;
			}
		}
	}
#if VERBOSE
	fprintf(gpout->GetFile(), "Leaving bucket_count\n");
#endif
	return num_of_freqpats;
}

// ----------------------------- Closed --------------------------


int Afopt::OutputSingleClosePath(int* pitem_sups, int* pitem_ntrans, int length, int nroot_sup)
{
	int i, nprev_i, j;
	int *pitemset;
	int base_bitmap, bitmap;
	int num_of_nodes;
	int nclass_pos;

	if(length<=0)
		return 0;

	pitemset = &(gpheader_itemset[gndepth]);

	nprev_i = 0;
	i = 0;
	num_of_nodes = 0;
	nclass_pos = 0;
	while(i<length)
	{
		nprev_i = i;
		while(i<length-1 && pitem_sups[i]==pitem_sups[i+1])
		{
			i++;
		}

		if(pitem_sups[i]==nroot_sup)
		{
			nclass_pos = 0;
			update_suplen_map(i+1, pitem_sups[i]);
		}
		else 
			nclass_pos = PrunedByExistCFI(i+1, pitem_sups[i]);
		if(nclass_pos==0)
		{
			OutputOneClosePat(i+1, pitem_sups[i], pitem_ntrans[i]);
			if(i<length-1)
			{
				gotree_bufmanager.InsertCommonPrefixNode(gntree_level, &(pitemset[nprev_i]), (i+1-nprev_i), pitem_sups[i], pitem_ntrans[i], 0);
				gntree_level++;
				num_of_nodes++;
			}
			else
				break;
		}
		else
			break;
		i++;
	}

	base_bitmap = 0;
	if(nclass_pos==0)
	{
		gotree_bufmanager.WriteLeafNode(&(pitemset[nprev_i]), (i+1-nprev_i), pitem_sups[i], pitem_ntrans[i], 0);
		for(j=nprev_i;j<i+1;j++)
			base_bitmap = base_bitmap | (1<<pitemset[j]%INT_BIT_LEN);
	}

	if(num_of_nodes>0)
	{
		bitmap = gotree_bufmanager.WriteCommonPrefixNodes(num_of_nodes, base_bitmap);
		gntree_level -= num_of_nodes;
	}
	else 
		bitmap = base_bitmap;

	return bitmap;
}

int Afopt::OutputCommonPrefix(int* pitem_sups, int *pitem_ntrans, int length, int *bitmap, int nroot_sup)
{
	int i, nprev_i, j;
	int *pitemset;
	int base_bitmap;
	int num_of_nodes;
	int nclass_pos;

	if(length<=0)
		return 0;

	num_of_nodes = 0;

	pitemset = &(gpheader_itemset[gndepth]);

	i = 0;
	while(i<length)
	{
		nprev_i = i;
		while(i<length-1 && pitem_sups[i]==pitem_sups[i+1])
		{
			i++;
		}

		if(pitem_sups[i]==nroot_sup)
		{
			nclass_pos = 0;
			update_suplen_map(i+1, pitem_sups[i]);
		}
		else 
			nclass_pos = PrunedByExistCFI(i+1, pitem_sups[i]);
		if(nclass_pos==0)
		{
			OutputOneClosePat(i+1, pitem_sups[i], pitem_ntrans[i]);
			base_bitmap = (*bitmap);
			for(j=i+1;j<length;j++)
				base_bitmap = base_bitmap | (1<<pitemset[j]%INT_BIT_LEN);
			gotree_bufmanager.InsertCommonPrefixNode(gntree_level, &(pitemset[nprev_i]), (i+1-nprev_i), pitem_sups[i], pitem_ntrans[i], base_bitmap);
			gntree_level++;
			num_of_nodes++;
		}
		else
		{
			if(num_of_nodes>0)
			{
				(*bitmap) = gotree_bufmanager.WriteCommonPrefixNodes(num_of_nodes, 0);
				gntree_level -= num_of_nodes;
			}
			else
				(*bitmap) = 0;

			num_of_nodes = -1;
			break; 
		}

		i++;
	}

	return num_of_nodes;
}

void Afopt::update_suplen_map(int nsuffix_len, int nsupport)
{
	int i, npat_len, order, nmap_size, offset;
	int nmax_item_id, nmin_item_id, bitmap;
	clock_t start;

	start = clock();
	npat_len = gndepth+nsuffix_len;

	nmax_item_id = gpheader_itemset[gnfirst_level_depth];
	nmin_item_id = gpheader_itemset[gnfirst_level_depth];
	bitmap = 0;
	for(i=gnfirst_level_depth;i<npat_len;i++)
	{
		if(gpheader_itemset[i]>nmax_item_id)
			nmax_item_id = gpheader_itemset[i];
		if(gpheader_itemset[i]<nmin_item_id)
			nmin_item_id = gpheader_itemset[i];
		bitmap = bitmap | (1<< (gpheader_itemset[i]%INT_BIT_LEN));
	}

	for(i=gnfirst_level_depth;i<npat_len;i++)
	{
		order = gpitem_order_map[gpheader_itemset[i]];
		if(order<0)
			printf("Error[update_suplen_map]: cannot find item %d in item-order-map\n", gpheader_itemset[i]);
		else 
		{
			nmap_size = MIN(SUP_MAXLEN_MAP_SIZE, gpfreqitems[order].support-(int)gnmin_sup+1);
			offset = (nsupport-(int)gnmin_sup)%nmap_size;
			if(gpitem_sup_map[order]==NULL)
			{
				gpitem_sup_map[order] = new MAP_NODE[nmap_size];
				//goMemTracer.IncTwoLayerMap(nmap_size*sizeof(MAP_NODE));
				memset(gpitem_sup_map[order], 0, sizeof(MAP_NODE)*nmap_size);
				gpitem_sup_map[order][offset].nmax_len = npat_len;
				gpitem_sup_map[order][offset].nmax_item_id = nmax_item_id;
				gpitem_sup_map[order][offset].nmin_item_id = nmin_item_id;
				gpitem_sup_map[order][offset].bitmap = bitmap;
			}
			else 
			{
				if(gpitem_sup_map[order][offset].nmax_len<npat_len)
					gpitem_sup_map[order][offset].nmax_len = npat_len;
				if(gpitem_sup_map[order][offset].nmax_item_id<nmax_item_id)
					gpitem_sup_map[order][offset].nmax_item_id = nmax_item_id;
				if(gpitem_sup_map[order][offset].nmin_item_id>nmin_item_id)
					gpitem_sup_map[order][offset].nmin_item_id = nmin_item_id;
				gpitem_sup_map[order][offset].bitmap = gpitem_sup_map[order][offset].bitmap | bitmap;
			}
		}
	}

	//goTimeTracer.mdpruning_time += (double)(clock()-start)/CLOCKS_PER_SEC;
}

int Afopt::PrunedByExistCFI(int nsuffix_len, int nsupport)
{
	unsigned int found;
	int npat_len, i, order, nmap_size, offset, nmax_item_id, nmin_item_id, bitmap;
	clock_t start;

	start = clock();
	gnprune_times++;
	npat_len = gndepth+nsuffix_len;
	found = 1;

	nmin_item_id = gpheader_itemset[gnfirst_level_depth];
	nmax_item_id = gpheader_itemset[gnfirst_level_depth];
	bitmap = 0;
	for(i=gnfirst_level_depth;i<npat_len;i++)
	{
		if(gpheader_itemset[i]>nmax_item_id)
			nmax_item_id = gpheader_itemset[i];
		if(gpheader_itemset[i]<nmin_item_id)
			nmin_item_id = gpheader_itemset[i];
		bitmap = bitmap | (1<< (gpheader_itemset[i]%INT_BIT_LEN));
	}

	for(i=gnfirst_level_depth;i<npat_len;i++)
	{
		order = gpitem_order_map[gpheader_itemset[i]];
		if(order<0)
			printf("Error[PrunedByExistCFI]: cannot find item %d in item-order-map\n", gpheader_itemset[i]);
		else 
		{
			nmap_size = MIN(SUP_MAXLEN_MAP_SIZE, gpfreqitems[order].support-(int)gnmin_sup+1);
			offset = (nsupport-(int)gnmin_sup)%nmap_size;
			if(gpitem_sup_map[order]==NULL)
			{
				found = 0;
				gpitem_sup_map[order] = new MAP_NODE[nmap_size];
				//goMemTracer.IncTwoLayerMap(nmap_size*sizeof(MAP_NODE));
				memset(gpitem_sup_map[order], 0, sizeof(MAP_NODE)*nmap_size);
				gpitem_sup_map[order][offset].nmax_len = npat_len;
				gpitem_sup_map[order][offset].nmax_item_id = nmax_item_id;
				gpitem_sup_map[order][offset].nmin_item_id = nmin_item_id;
				gpitem_sup_map[order][offset].bitmap = bitmap;
			}
			else 
			{
				if(gpitem_sup_map[order][offset].nmax_len<npat_len)
				{
					found = 0;
					gpitem_sup_map[order][offset].nmax_len = npat_len;
				}
				else if(gpitem_sup_map[order][offset].nmax_len==npat_len)
					found = 0;

				if(gpitem_sup_map[order][offset].nmax_item_id<nmax_item_id)
				{
					found = 0;
					gpitem_sup_map[order][offset].nmax_item_id = nmax_item_id;
				}
				if(gpitem_sup_map[order][offset].nmin_item_id>nmin_item_id)
				{
					found = 0;
					gpitem_sup_map[order][offset].nmin_item_id = nmin_item_id;
				}
				if(found)
				{
					if((gpitem_sup_map[order][offset].bitmap & bitmap) != bitmap)
						found = 0;
				}
				gpitem_sup_map[order][offset].bitmap = gpitem_sup_map[order][offset].bitmap | bitmap;
			}
		}
	}

	if(found==0)
	{
		gnmap_pruned_times++;
		return 0;
	}

	found = 0;
	gotree_bufmanager.mpat_len = gnfirst_level_depth;

	if(npat_len-gnfirst_level_depth==0)
	{
		printf("Error with pattern length\n");
	}
	else if(npat_len>gnfirst_level_depth)
		found = gotree_bufmanager.SearchActiveStack(gnfirst_level, &(gpheader_itemset[gnfirst_level_depth]), npat_len-gnfirst_level_depth, nsupport, gndb_size, 0);
	else
	{
		printf("Error with level\n");
	}		

	if(found)
	{
		if(npat_len>gotree_bufmanager.mpat_len)
			printf("Error: pattern length are not consistent %d %d\n", npat_len, gotree_bufmanager.mpat_len);
	}
	else if(gotree_bufmanager.mpat_len!=gnfirst_level_depth)
		printf("Error with the length of the pattern\n");

	//goTimeTracer.mdpruning_time += (double)(clock()-start)/CLOCKS_PER_SEC;

	return found;

}
