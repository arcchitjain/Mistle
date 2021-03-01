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
