#define _CRT_SECURE_NO_DEPRECATE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cfptree_outbuf.h"

#include "../Global.h"
#include "../parameters.h"
#include "../AfoptMiner.h"

using namespace Afopt;

//------------------------------------------------------
//Initialize variables used for cfp-tree read and write
//--------------------------------------------------------
void CTreeOutBufManager::Init(int nstack_size)
{
	mpactive_stack = new ACTIVE_NODE[nstack_size];
	mnactive_top = 0;

	mfpcfp_file = fopen(goparameters.tree_filename, "wb"); // NOT fopen_s, as this opens files exclusively; fails below
	if(mfpcfp_file == NULL) {
		printf("Error: cannot open file %s for write\n", goparameters.tree_filename);
		exit(-1);
	}
	mntree_size = 0;

	mnbuffer_size = goparameters.ntree_outbuf_size;
	mncapacity = mnbuffer_size-1;

	mptree_buffer = new char*[goparameters.ntree_buf_num_of_pages];
	for(int i=0;i<goparameters.ntree_buf_num_of_pages;i++)
		mptree_buffer[i] = NULL;

	mnstart_pos = 0;
	mnend_pos = 0;
	mnwrite_start_pos = 0;
	mndisk_write_start_pos = 0;
	mnmin_inmem_level = -1;

	mfpread_file = fopen(goparameters.tree_filename, "rb"); // NOT fopen_s; see above
	if(mfpread_file == NULL) {
		printf("Error: cannot open file %s for read\n", goparameters.tree_filename);
		exit(-1);
	}
}

//------------------------------------------------------------------
//Release space occupied by variables after finish mining
//------------------------------------------------------------------
void CTreeOutBufManager::Destroy()
{
	delete []mpactive_stack;
	mpactive_stack = NULL;

	for(int i=0;i<goparameters.ntree_buf_num_of_pages;i++)
	{
		if(mptree_buffer[i]!=NULL)
		{
			delete []mptree_buffer[i];
			mptree_buffer[i] = NULL;
			//goMemTracer.DecCFPSize(goparameters.ntree_buf_page_size);
		}
	}
	delete []mptree_buffer;
	mptree_buffer = NULL;

	fclose(mfpcfp_file);
	mfpcfp_file = NULL;
	fclose(mfpread_file);
	mfpread_file = NULL;

	remove(goparameters.tree_filename);
}

void CTreeOutBufManager::IncTreeSize(int ninc_size)
{
	mntree_size += ninc_size;
}

int  CTreeOutBufManager::GetTreeSize()
{
	return mntree_size;
}

void CTreeOutBufManager::PrintfStatistics(FILE *fpsum_file)
{
	fprintf(fpsum_file, "%.2f MB\t", (double)mntree_size/(1<<20));
}


//----------------------------------------------------------------------------------
//Insert an internal node into CFP-tree. The child position of entries in this node
//is unknown when the node is inserted. Therefore when a CFP-node is first inserted,
//it will be inserted into the active stack. After all its children node are created 
//and the information in the CFP-node is complete. It will be popped out from the 
//active stack.
//----------------------------------------------------------------------------------
void CTreeOutBufManager::InsertActiveNode(int level, CFP_NODE *pcfp_node)
{
	int node_size, norig_end_pos;

//	if(mnend_pos!=mntree_size%mnbuffer_size)
//		printf("The values of mnend_pos and mntree_size are not consistent\n");
//	if(mnwrite_start_pos!=mndisk_write_start_pos%mnbuffer_size)
//		printf("The value of the mnwrite_start_pos and mndisk_write_start_pos are not consistent\n");
//	if(level!=mnactive_top)
//		printf("Error: the values of level and mnactive_top are not consistent\n");

	norig_end_pos = mnend_pos;

	node_size = sizeof(int) + pcfp_node->length*sizeof(ENTRY);

	mpactive_stack[mnactive_top].level = level;
	mpactive_stack[mnactive_top].disk_pos = mntree_size;
	mpactive_stack[mnactive_top].pcfp_node = pcfp_node;
	mpactive_stack[mnactive_top].cfpnode_size = node_size;

	if(node_size>=mncapacity)
	{
		if(mnend_pos!=mnstart_pos)
			Dump(node_size);
		mnend_pos = (mnend_pos+node_size)%mnbuffer_size;
		mnstart_pos = mnend_pos;
		mnwrite_start_pos = mnend_pos;
		mnmin_inmem_level = -1;
		mndisk_write_start_pos += node_size;
		mpactive_stack[mnactive_top].in_mem = false;
		mpactive_stack[mnactive_top].mem_pos = -1;
	}
	else
	{
		if(mnend_pos>=mnstart_pos && mnend_pos+node_size-mnstart_pos>mncapacity ||
			mnend_pos<mnstart_pos && mnend_pos+mnbuffer_size-mnstart_pos+node_size>mncapacity)
			Dump(node_size);
		mpactive_stack[mnactive_top].in_mem = true;
		mpactive_stack[mnactive_top].mem_pos = mnend_pos;
		mnend_pos = (mnend_pos+node_size)%mnbuffer_size;
		if(mnwrite_start_pos==norig_end_pos)
		{
			mnwrite_start_pos = mnend_pos;
			mndisk_write_start_pos += node_size;
		}
		if(mnmin_inmem_level==-1)
			mnmin_inmem_level = mnactive_top;

//		if(mnmin_inmem_level>mnactive_top)
//		{
//			printf("Error[InsertActiveNode]: with mnmin_inmem_level length\n");
//			mnmin_inmem_level = mnactive_top;
//		}
	}

	mnactive_top++;
	mntree_size += node_size;

}

//--------------------------------------------------------------------------------
//Write an internal CFP-tree into output buffer, and pop it out from active stack
//--------------------------------------------------------------------------------
void CTreeOutBufManager::WriteInternalNode(CFP_NODE *pcfp_node)
{
	int node_size, part1_size;
	int nwrite_cur_pos;
	int noffset;
	int npage_no, npage_offset;

//	if(mnend_pos!=mntree_size%mnbuffer_size)
//		printf("The values of mnend_pos and mntree_size are not consistent\n");
//	if(mnwrite_start_pos!=mndisk_write_start_pos%mnbuffer_size)
//		printf("The value of the mnwrite_start_pos and mndisk_write_start_pos are not consistent\n");
//	if(pcfp_node!=mpactive_stack[mnactive_top-1].pcfp_node)
//		printf("Error: wrong internal node\n");

	mnactive_top--;
	if(mnmin_inmem_level>=mnactive_top)
		mnmin_inmem_level = -1;

	if(!mpactive_stack[mnactive_top].in_mem)
	{
		DumpAll();
		int nfile_pos;

		nfile_pos = ftell(mfpcfp_file);
		noffset = pcfp_node->pos-nfile_pos;
		if(noffset!=0)
			fseek(mfpcfp_file, noffset, SEEK_CUR);

		fwrite(&(pcfp_node->length), sizeof(int), 1, mfpcfp_file);
		fwrite(pcfp_node->pentries, sizeof(ENTRY), pcfp_node->length, mfpcfp_file);
		fflush(mfpcfp_file);
		fseek(mfpcfp_file, nfile_pos-ftell(mfpcfp_file), SEEK_CUR);
		
		return;
	}

	noffset = pcfp_node->pos-mntree_size;

//	if(pcfp_node!=mpactive_stack[mnactive_top].pcfp_node)
//		printf("Wrong CFP-tree node\n");
//	if((mnend_pos+noffset+mnbuffer_size)%mnbuffer_size!=mpactive_stack[mnactive_top].mem_pos%mnbuffer_size)
//		printf("Error: the memory position of the node does not match\n");

	node_size = sizeof(int) + sizeof(ENTRY)*pcfp_node->length;

	if(mnend_pos>=mnwrite_start_pos && mnend_pos+noffset<mnwrite_start_pos ||
		mnend_pos<mnwrite_start_pos && mnend_pos+noffset<0 && mnend_pos+noffset+mnbuffer_size<mnwrite_start_pos)
	{
		mnwrite_start_pos = (mnend_pos+noffset+mnbuffer_size)%mnbuffer_size;
		mndisk_write_start_pos = pcfp_node->pos;
		
//		if(mnwrite_start_pos!=mndisk_write_start_pos%mnbuffer_size)
//			printf("Error: the values of mnwrite_start_pos and mndisk_write_start_pos are not consistent\n");
	}

	node_size = sizeof(ENTRY)*pcfp_node->length;
	nwrite_cur_pos = (mnend_pos+noffset+mnbuffer_size)%mnbuffer_size;

	npage_no = nwrite_cur_pos/goparameters.ntree_buf_page_size;
	npage_offset = nwrite_cur_pos & (goparameters.ntree_buf_page_size-1);
	if(mptree_buffer[npage_no]==NULL)
	{
		mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
		//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
	}			

	memcpy(&(mptree_buffer[npage_no][npage_offset]), &(pcfp_node->length), sizeof(int));
	npage_offset += sizeof(int);
	if(npage_offset==goparameters.ntree_buf_page_size)
	{
		npage_offset = 0;
		npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
		if(mptree_buffer[npage_no]==NULL)
		{
			mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
			//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
		}			
		memcpy(mptree_buffer[npage_no], pcfp_node->pentries, node_size);
	}
	else if(npage_offset+node_size<=goparameters.ntree_buf_page_size)
	{
		memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries, node_size);
	}
	else 
	{
		char *pentries;
		pentries = (char*)(pcfp_node->pentries);
		part1_size = goparameters.ntree_buf_page_size-npage_offset;
		memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries, part1_size);
		npage_offset = 0;
		npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
		if(mptree_buffer[npage_no]==NULL)
		{
			mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
			//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
		}			
		memcpy(mptree_buffer[npage_no], &(pentries[part1_size]), node_size-part1_size);
	}

//	if(mnactive_top==0)
//		DumpAll();
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void CTreeOutBufManager::WriteLeafNode(int *pitems, int num_of_items, int nsupport, int ntrans, int hash_bitmap)
{
	ENTRY cfp_entry;
	int node_size, itemset_size, singleton_entry_size;
	int npage_offset, npage_no;


//	if(mnend_pos!=mntree_size%mnbuffer_size)
//		printf("The values of mnend_pos and mntree_size are not consistent\n");
//	if(mnwrite_start_pos!=mndisk_write_start_pos%mnbuffer_size)
//		printf("The value of the mnwrite_start_pos and mndisk_write_start_pos are not consistent\n");

	cfp_entry.support = nsupport;
	cfp_entry.ntrans = ntrans;
	cfp_entry.hash_bitmap = hash_bitmap;
	cfp_entry.child = 0;

	if(num_of_items==1)
	{
		node_size = sizeof(int)+sizeof(ENTRY);
		mntree_size += node_size;
		cfp_entry.item = pitems[0];

		if(mnend_pos>mnstart_pos && mnend_pos-mnstart_pos+node_size>mncapacity ||
			mnend_pos<mnstart_pos && mnend_pos+mnbuffer_size-mnstart_pos+node_size>mncapacity)
			Dump(node_size);

		npage_no = mnend_pos/goparameters.ntree_buf_page_size;
		npage_offset = mnend_pos & (goparameters.ntree_buf_page_size-1);
		if(mptree_buffer[npage_no]==NULL)
		{
			mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
			//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
		}

		memcpy(&(mptree_buffer[npage_no][npage_offset]), &num_of_items, sizeof(int));
		npage_offset += sizeof(int);
		if(npage_offset==goparameters.ntree_buf_page_size)
		{
			npage_offset = 0;
			npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
			if(mptree_buffer[npage_no]==NULL)
			{
				mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
				//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
			}
			memcpy(mptree_buffer[npage_no], &cfp_entry, sizeof(ENTRY));
		}
		else if(npage_offset+sizeof(ENTRY)<=(unsigned int)(goparameters.ntree_buf_page_size))
		{
			memcpy(&(mptree_buffer[npage_no][npage_offset]), &cfp_entry, sizeof(ENTRY));
		}
		else 
		{
			char *pentries;
			int part1_size;
			pentries = (char*)(&cfp_entry);
			part1_size = goparameters.ntree_buf_page_size-npage_offset;
			memcpy(&(mptree_buffer[npage_no][npage_offset]), &cfp_entry, part1_size);
			npage_offset = 0;
			npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
			if(mptree_buffer[npage_no]==NULL)
			{
				mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
				//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
			}
			memcpy(mptree_buffer[npage_no], &(pentries[part1_size]), sizeof(ENTRY)-part1_size);
		}

		mnend_pos = (mnend_pos+node_size)%mnbuffer_size;
	}
	else
	{
		qsort(pitems, num_of_items, sizeof(int), comp_int_asc);

		itemset_size = sizeof(int)*num_of_items;
		node_size = itemset_size+sizeof(ENTRY);
		singleton_entry_size = sizeof(ENTRY)-sizeof(int);
		mntree_size += node_size;

		num_of_items = -num_of_items;

		if(mnend_pos>mnstart_pos && mnend_pos-mnstart_pos+node_size>mncapacity ||
			mnend_pos<mnstart_pos && mnend_pos+mnbuffer_size-mnstart_pos+node_size>mncapacity)
			Dump(node_size);

		npage_no = mnend_pos/goparameters.ntree_buf_page_size;
		npage_offset = mnend_pos & (goparameters.ntree_buf_page_size-1);
		if(mptree_buffer[npage_no]==NULL)
		{
			mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
			//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
		}

		memcpy(&(mptree_buffer[npage_no][npage_offset]), &num_of_items, sizeof(int));
		npage_offset += sizeof(int);
		if(npage_offset==goparameters.ntree_buf_page_size)
		{
			npage_offset = 0;
			npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
			if(mptree_buffer[npage_no]==NULL)
			{
				mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
				//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
			}
			memcpy(mptree_buffer[npage_no], &cfp_entry, singleton_entry_size);
			npage_offset += singleton_entry_size;
			memcpy(&(mptree_buffer[npage_no][npage_offset]), pitems, itemset_size);
		}
		else if(npage_offset+node_size<=goparameters.ntree_buf_page_size) 
		{
			memcpy(&(mptree_buffer[npage_no][npage_offset]), &cfp_entry, singleton_entry_size);
			npage_offset += singleton_entry_size;
			memcpy(&(mptree_buffer[npage_no][npage_offset]), pitems, itemset_size);
		}
		else if(npage_offset+singleton_entry_size==goparameters.ntree_buf_page_size)
		{
			memcpy(&(mptree_buffer[npage_no][npage_offset]), &cfp_entry, singleton_entry_size);
			npage_offset = 0;
			npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
			if(mptree_buffer[npage_no]==NULL)
			{
				mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
				//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
			}
			memcpy(mptree_buffer[npage_no], pitems, itemset_size);
		}
		else if(npage_offset+singleton_entry_size<goparameters.ntree_buf_page_size)
		{
			char *ptemp_itemset;
			int part1_size;
			memcpy(&(mptree_buffer[npage_no][npage_offset]), &cfp_entry, singleton_entry_size);
			npage_offset += singleton_entry_size;
			ptemp_itemset = (char*)pitems;
			part1_size = goparameters.ntree_buf_page_size-npage_offset;
			memcpy(&(mptree_buffer[npage_no][npage_offset]), pitems, part1_size);
			npage_offset = 0;
			npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
			if(mptree_buffer[npage_no]==NULL)
			{
				mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
				//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
			}
			memcpy(mptree_buffer[npage_no], &(ptemp_itemset[part1_size]), itemset_size-part1_size);
		}
		else
		{
			char *ptemp_entry;
			int part1_size;
			ptemp_entry = (char*)(&cfp_entry);
			part1_size = goparameters.ntree_buf_page_size-npage_offset;
			memcpy(&(mptree_buffer[npage_no][npage_offset]), &cfp_entry, part1_size);
			npage_offset = 0;
			npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
			if(mptree_buffer[npage_no]==NULL)
			{
				mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
				//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
			}
			memcpy(mptree_buffer[npage_no], &(ptemp_entry[part1_size]), singleton_entry_size-part1_size);
			npage_offset += singleton_entry_size-part1_size;
			memcpy(&(mptree_buffer[npage_no][npage_offset]), pitems, itemset_size);
		}
		mnend_pos = (mnend_pos+node_size)%mnbuffer_size;
	}
}

//-------------------------------------------------------------------------------------
//When buffer is full, dump some of its contents on disk to release space for future
//CFP-tree nodes
//-------------------------------------------------------------------------------------
void CTreeOutBufManager::Dump(int newnode_size)
{
	int node_size, nwrite_end_pos, nlevel;
	int npage_offset, npage_no, nend_page_no, nend_page_offset;
	int nfile_pos;

	while(mnend_pos>mnstart_pos && mnend_pos-mnstart_pos+newnode_size>mncapacity ||
		mnend_pos<mnstart_pos && mnend_pos+mnbuffer_size-mnstart_pos+newnode_size>mncapacity)
	{
		if(mnstart_pos!=mnwrite_start_pos)
		{
			if(mnstart_pos!=mpactive_stack[mnmin_inmem_level].mem_pos)
				printf("Error\n");
			if(mnend_pos>mnstart_pos)
			{
//				if(mnstart_pos>mnwrite_start_pos || mnend_pos<mnwrite_start_pos)
//					printf("Error\n");

				while(mnstart_pos!=mnwrite_start_pos && mnend_pos-mnstart_pos+newnode_size>mncapacity)
				{
					node_size = mpactive_stack[mnmin_inmem_level].cfpnode_size;
					mnstart_pos += node_size;
					mpactive_stack[mnmin_inmem_level].in_mem = false;
					mpactive_stack[mnmin_inmem_level].mem_pos = -1;
					if(mnmin_inmem_level>=0)
						mnmin_inmem_level++;
					if(mnmin_inmem_level>=mnactive_top)
						mnmin_inmem_level = -1;
				}
				if(mnend_pos-mnstart_pos+newnode_size<=mncapacity)
					break;
			}
			else if(mnend_pos<mnstart_pos)
			{
				if(mnwrite_start_pos>mnend_pos && mnwrite_start_pos<mnstart_pos)
					printf("Error\n");

				while(mnstart_pos!=mnwrite_start_pos && mnend_pos+mnbuffer_size-mnstart_pos+newnode_size>mncapacity)
				{
					node_size = mpactive_stack[mnmin_inmem_level].cfpnode_size;
					mnstart_pos = (mnstart_pos+node_size)%mnbuffer_size;
					mpactive_stack[mnmin_inmem_level].in_mem = false;
					mpactive_stack[mnmin_inmem_level].mem_pos = -1;
					if(mnmin_inmem_level>=0)
						mnmin_inmem_level++;
					if(mnmin_inmem_level>=mnactive_top)
						mnmin_inmem_level = -1;
				}
				if(mnend_pos+mnbuffer_size-mnstart_pos+newnode_size<=mncapacity)
					break;
			}
		}

		if(mnstart_pos==mnend_pos)
		{
			mnwrite_start_pos = mnend_pos;
			mnmin_inmem_level = -1;
			break;
		}

		if(mnmin_inmem_level==-1)
		{
			DumpAll();
			break;
		}

		nwrite_end_pos = mpactive_stack[mnmin_inmem_level].mem_pos;

		if(nwrite_end_pos==mnwrite_start_pos)
		{
			DumpAll();
			break;
		}

		nfile_pos = ftell(mfpcfp_file);
		if(mndisk_write_start_pos!=nfile_pos)
			fseek(mfpcfp_file, mndisk_write_start_pos-nfile_pos, SEEK_CUR);

		npage_no = mnwrite_start_pos/goparameters.ntree_buf_page_size;
		npage_offset = mnwrite_start_pos & (goparameters.ntree_buf_page_size-1);
		nend_page_no = nwrite_end_pos/goparameters.ntree_buf_page_size;
		nend_page_offset = nwrite_end_pos & (goparameters.ntree_buf_page_size-1);

		if(nwrite_end_pos>mnwrite_start_pos)
		{
			if(nend_page_no==npage_no)
				fwrite(&(mptree_buffer[npage_no][npage_offset]), sizeof(char), nend_page_offset-npage_offset, mfpcfp_file);
			else 
			{
				fwrite(&(mptree_buffer[npage_no][npage_offset]), sizeof(char), goparameters.ntree_buf_page_size-npage_offset, mfpcfp_file);
				npage_no++;
				while(npage_no<nend_page_no)
				{
					fwrite(mptree_buffer[npage_no], sizeof(char), goparameters.ntree_buf_page_size, mfpcfp_file);
					npage_no++;
				}
				if(nend_page_offset>0)
					fwrite(mptree_buffer[nend_page_no], sizeof(char), nend_page_offset, mfpcfp_file);
			}
		}
		else if(nwrite_end_pos<mnwrite_start_pos)
		{
			fwrite(&(mptree_buffer[npage_no][npage_offset]), sizeof(char), goparameters.ntree_buf_page_size-npage_offset, mfpcfp_file);
			npage_no++;
			while(npage_no<goparameters.ntree_buf_num_of_pages)
			{
				fwrite(mptree_buffer[npage_no], sizeof(char), goparameters.ntree_buf_page_size, mfpcfp_file);
				npage_no++;
			}
			npage_no = 0;
			while(npage_no<nend_page_no)
			{
				fwrite(mptree_buffer[npage_no], sizeof(char), goparameters.ntree_buf_page_size, mfpcfp_file);
				npage_no++;
			}
			if(nend_page_offset>0)
				fwrite(mptree_buffer[nend_page_no], sizeof(char), nend_page_offset, mfpcfp_file);
		}

		mnstart_pos = nwrite_end_pos;
		mnwrite_start_pos = (nwrite_end_pos + mpactive_stack[mnmin_inmem_level].cfpnode_size)%mnbuffer_size;
		mndisk_write_start_pos = mpactive_stack[mnmin_inmem_level].disk_pos+mpactive_stack[mnmin_inmem_level].cfpnode_size;

		nlevel = mnmin_inmem_level+1;
		while(nlevel>=0 && nlevel<mnactive_top)
		{
			if(mpactive_stack[nlevel].mem_pos!=mnwrite_start_pos)
				break;
			else
			{
				mnwrite_start_pos = (mnwrite_start_pos+mpactive_stack[nlevel].cfpnode_size)%mnbuffer_size;
				mndisk_write_start_pos += mpactive_stack[nlevel].cfpnode_size;
			}
			nlevel++;
		}
	}

	fflush(mfpcfp_file);
}

//------------------------------------------------------------------------
//Dump all the contents in buffer on disk
//------------------------------------------------------------------------
void CTreeOutBufManager::DumpAll()
{
	int npage_no, npage_offset, nend_page_no, nend_page_offset;

	if(mnstart_pos!=mnwrite_start_pos)
		printf("Error\n");

	if(mnstart_pos==mnend_pos)
		return;

	int nfile_pos;
	nfile_pos = ftell(mfpcfp_file);

	if(mndisk_write_start_pos!=nfile_pos)
		fseek(mfpcfp_file, mndisk_write_start_pos-nfile_pos, SEEK_CUR);

	npage_no = mnstart_pos/goparameters.ntree_buf_page_size;
	npage_offset = mnstart_pos & (goparameters.ntree_buf_page_size-1);
	nend_page_no = mnend_pos/goparameters.ntree_buf_page_size;
	nend_page_offset = mnend_pos & (goparameters.ntree_buf_page_size-1);

	if(mnend_pos>mnstart_pos)
	{
		if(nend_page_no==npage_no)
			fwrite(&(mptree_buffer[npage_no][npage_offset]), sizeof(char), nend_page_offset-npage_offset, mfpcfp_file);
		else 
		{
			fwrite(&(mptree_buffer[npage_no][npage_offset]), sizeof(char), goparameters.ntree_buf_page_size-npage_offset, mfpcfp_file);
			npage_no++;
			while(npage_no<nend_page_no)
			{
				fwrite(mptree_buffer[npage_no], sizeof(char), goparameters.ntree_buf_page_size, mfpcfp_file);
				npage_no++;
			}
			if(nend_page_offset>0)
				fwrite(mptree_buffer[nend_page_no], sizeof(char), nend_page_offset, mfpcfp_file);
		}
	}
	else if(mnend_pos<mnstart_pos)
	{
		fwrite(&(mptree_buffer[npage_no][npage_offset]), sizeof(char), goparameters.ntree_buf_page_size-npage_offset, mfpcfp_file);
		npage_no++;
		while(npage_no<goparameters.ntree_buf_num_of_pages)
		{
			fwrite(mptree_buffer[npage_no], sizeof(char), goparameters.ntree_buf_page_size, mfpcfp_file);
			npage_no++;
		}
		npage_no = 0;
		while(npage_no<nend_page_no)
		{
			fwrite(mptree_buffer[npage_no], sizeof(char), goparameters.ntree_buf_page_size, mfpcfp_file);
			npage_no++;
		}
		if(nend_page_offset>0)
			fwrite(mptree_buffer[nend_page_no], sizeof(char), nend_page_offset, mfpcfp_file);
	}

	mnstart_pos = mnend_pos;
	mnwrite_start_pos = mnend_pos;
	mnmin_inmem_level = -1;
}

//-----------------------------------------------------------------------------------------
//Insert a CFP-tree node into output buffer and active stack. This node either appears in 
//every transaction of a conditional database, or is the only child of a CFP-tree root.
//-----------------------------------------------------------------------------------------
void CTreeOutBufManager::InsertCommonPrefixNode(int level, int *pitems, int num_of_items, int nsupport, int ntrans, int hash_bitmap)
{
	int node_size, norig_end_pos;
	ENTRY *pcfp_entry;
	CFP_NODE *pcfp_node;

	if(num_of_items==1)
		node_size = sizeof(int) + sizeof(ENTRY);
	else 
		node_size = num_of_items*sizeof(int)+sizeof(ENTRY);

//	if(mnend_pos!=mntree_size%mnbuffer_size)
//		printf("Error[InsertCommonPrefixNode]: The values of mnend_pos and mntree_size are not consistent\n");
//	if(mnwrite_start_pos!=mndisk_write_start_pos%mnbuffer_size)
//		printf("Error[InsertCommonPrefixNode]: The value of the mnwrite_start_pos and mndisk_write_start_pos are not consistent\n");
//	if(level!=mnactive_top)
//		printf("Error[InsertCommonPrefixNode]: the values of level and mnactive_top are not consistent\n");

	norig_end_pos = mnend_pos;

	pcfp_node = new CFP_NODE;
	//goMemTracer.IncCFPSize(sizeof(CFP_NODE));
	pcfp_node->pos = mntree_size;
	pcfp_node->cur_order = 1;
	pcfp_entry = new ENTRY;
	//goMemTracer.IncCFPSize(sizeof(ENTRY));
	pcfp_entry->support = nsupport;
	pcfp_entry->ntrans = ntrans;
	pcfp_entry->hash_bitmap = hash_bitmap;
	pcfp_entry->child = mntree_size+node_size;
	pcfp_node->pentries = pcfp_entry;

	if(num_of_items==1)
	{
		pcfp_node->length = 1;
		pcfp_entry->item = pitems[0];
	}
	else
	{
		pcfp_node->length = -num_of_items;
		pcfp_entry->pitems = new int[num_of_items];
		//goMemTracer.IncCFPSize(num_of_items*sizeof(int));
		memcpy(pcfp_entry->pitems, pitems, sizeof(int)*num_of_items);
		qsort(pcfp_entry->pitems, num_of_items, sizeof(int), comp_int_asc);
	}

	mpactive_stack[mnactive_top].level = level;
	mpactive_stack[mnactive_top].disk_pos = mntree_size;
	mpactive_stack[mnactive_top].pcfp_node = pcfp_node;
	mpactive_stack[mnactive_top].cfpnode_size = node_size;

	if(node_size>=mncapacity)
	{
		if(mnend_pos!=mnstart_pos)
			Dump(node_size);
		mnend_pos = (mnend_pos+node_size)%mnbuffer_size;
		mnstart_pos = mnend_pos;
		mnwrite_start_pos = mnend_pos;
		mnmin_inmem_level = -1;
		mndisk_write_start_pos += node_size;
		mpactive_stack[mnactive_top].in_mem = false;
		mpactive_stack[mnactive_top].mem_pos = -1;
	}
	else
	{
		if(mnend_pos>mnstart_pos && mnend_pos+node_size-mnstart_pos>mncapacity ||
			mnend_pos<mnstart_pos && mnend_pos+mnbuffer_size-mnstart_pos+node_size>mncapacity)
			Dump(node_size);
		mpactive_stack[mnactive_top].in_mem = true;
		mpactive_stack[mnactive_top].mem_pos = mnend_pos;
		mnend_pos = (mnend_pos+node_size)%mnbuffer_size;

		if(mnwrite_start_pos==norig_end_pos)
		{
			mnwrite_start_pos = mnend_pos;
			mndisk_write_start_pos += node_size;
		}
		if(mnmin_inmem_level==-1)
			mnmin_inmem_level = mnactive_top;
		if(mnmin_inmem_level>mnactive_top)
		{
			printf("Error[InsertCommonPrefixNode]: with mnmin_inmem_level\n");
			mnmin_inmem_level = mnactive_top;
		}
	}

	mnactive_top++;
	mntree_size += node_size;

}

//------------------------------------------------------------------------------
//Write a CFP-tree node to output buffer and pop it out from active stack
//------------------------------------------------------------------------------
int CTreeOutBufManager::WriteCommonPrefixNodes(int num_of_nodes, int base_bitmap)
{
	int i, j, noffset;
	CFP_NODE *pcfp_node;
	int node_size, itemset_size, singleton_entry_size, part1_size;
	int nwrite_cur_pos, npage_no, npage_offset;
	int bitmap;
	
//	if(mnend_pos!=mntree_size%mnbuffer_size)
//		printf("Error[WriteCommonPrefixNodes]: The values of mnend_pos and mntree_size are not consistent\n");
//	if(mnwrite_start_pos!=mndisk_write_start_pos%mnbuffer_size)
//		printf("Error[WriteCommonPrefixNodes]: The value of the mnwrite_start_pos and mndisk_write_start_pos are not consistent\n");

	singleton_entry_size = sizeof(ENTRY)-sizeof(int);

	itemset_size = 0;

	bitmap = base_bitmap;
	for(i=0;i<num_of_nodes;i++)
	{
		mnactive_top--;
		if(mnmin_inmem_level>=mnactive_top)
			mnmin_inmem_level = -1;

		pcfp_node = mpactive_stack[mnactive_top].pcfp_node;

		if(bitmap==0)
			pcfp_node->pentries->child = 0;
		pcfp_node->pentries->hash_bitmap = bitmap;
		if(pcfp_node->length==1)
			bitmap = bitmap | (1<<pcfp_node->pentries->item%INT_BIT_LEN);
		else
		{
			for(j=0;j<-pcfp_node->length;j++)
				bitmap = bitmap | (1<<pcfp_node->pentries->pitems[j]%INT_BIT_LEN);
		}

		if(!mpactive_stack[mnactive_top].in_mem)
		{
			if(mnstart_pos!=mnend_pos)
				DumpAll();

			int nfile_pos;
			nfile_pos = ftell(mfpcfp_file);
			if(pcfp_node->pos!=nfile_pos)
				fseek(mfpcfp_file, pcfp_node->pos-nfile_pos, SEEK_CUR);

			if(pcfp_node->length==1)
			{
				fwrite(&(pcfp_node->length), sizeof(int), 1, mfpcfp_file);
				fwrite(pcfp_node->pentries, sizeof(ENTRY), 1, mfpcfp_file);
			}
			else
			{
				fwrite(&(pcfp_node->length), sizeof(int), 1, mfpcfp_file);
				fwrite(pcfp_node->pentries, sizeof(int), sizeof(ENTRY)/sizeof(int)-1, mfpcfp_file);
				fwrite(pcfp_node->pentries->pitems, sizeof(int), -pcfp_node->length, mfpcfp_file);
			}
			fflush(mfpcfp_file);
			fseek(mfpcfp_file, nfile_pos-ftell(mfpcfp_file), SEEK_CUR);
		}
		else
		{
			noffset = pcfp_node->pos-mntree_size;

//			if(pcfp_node!=mpactive_stack[mnactive_top].pcfp_node)
//				printf("Error[WriteCommonPrefixNodes]: Wrong CFP-tree node\n");
//			if((mnend_pos+noffset+mnbuffer_size)%mnbuffer_size!=mpactive_stack[mnactive_top].mem_pos)
//				printf("Error[WriteCommonPRefixNodes]: the memory position do not match\n");

			if(pcfp_node->length==1)
				node_size = sizeof(int) + sizeof(ENTRY);
			else
			{
				itemset_size = (-pcfp_node->length)*sizeof(int);
				node_size =  (-pcfp_node->length)*sizeof(int) + sizeof(ENTRY);
			}

			if(mnend_pos>=mnwrite_start_pos && mnend_pos+noffset<mnwrite_start_pos ||
				mnend_pos<mnwrite_start_pos && mnend_pos+noffset<0 && mnend_pos+noffset+mnbuffer_size<mnwrite_start_pos)
			{
				mnwrite_start_pos = (mnend_pos+noffset+mnbuffer_size)%mnbuffer_size;
				mndisk_write_start_pos = pcfp_node->pos;
				
//				if(mnwrite_start_pos!=mndisk_write_start_pos%mnbuffer_size)
//					printf("Error[WriteCommonPRefixNodes]: the values of mnwrite_start_pos and mndisk_write_start_pos are not consistent\n");
			}

			nwrite_cur_pos = mnend_pos+noffset;
			if(nwrite_cur_pos<0)
				nwrite_cur_pos += mnbuffer_size;

			npage_no = nwrite_cur_pos/goparameters.ntree_buf_page_size;
			npage_offset = nwrite_cur_pos & (goparameters.ntree_buf_page_size-1);
			if(mptree_buffer[npage_no]==NULL)
			{
				mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
				//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
			}			

			memcpy(&(mptree_buffer[npage_no][npage_offset]), &(pcfp_node->length), sizeof(int));
			npage_offset += sizeof(int);
			if(pcfp_node->length==1)
			{
				if(npage_offset==goparameters.ntree_buf_page_size)
				{
					npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
					if(mptree_buffer[npage_no]==NULL)
					{
						mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
						//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
					}			
					memcpy(mptree_buffer[npage_no], pcfp_node->pentries, sizeof(ENTRY));
				}
				else if(npage_offset+sizeof(ENTRY)<=(unsigned int)(goparameters.ntree_buf_page_size))
				{
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries, sizeof(ENTRY));
				}
				else 
				{
					char *ptemp_entry;
					ptemp_entry = (char*)(pcfp_node->pentries);
					part1_size = goparameters.ntree_buf_page_size-npage_offset;
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries, part1_size);
					npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
					if(mptree_buffer[npage_no]==NULL)
					{
						mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
						//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
					}			
					memcpy(mptree_buffer[npage_no], &(ptemp_entry[part1_size]), sizeof(ENTRY)-part1_size);
				}
			}
			else if(pcfp_node->length<-1)
			{
				node_size = node_size-sizeof(int);
				if(npage_offset==goparameters.ntree_buf_page_size)
				{
					npage_offset = 0;
					npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
					if(mptree_buffer[npage_no]==NULL)
					{
						mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
						//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
					}			
					memcpy(mptree_buffer[npage_no], pcfp_node->pentries, singleton_entry_size);
					npage_offset += singleton_entry_size;
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries->pitems, itemset_size);
				}
				else if(npage_offset+node_size<=goparameters.ntree_buf_page_size)
				{
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries, singleton_entry_size);
					npage_offset += singleton_entry_size;
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries->pitems, itemset_size);
				}
				else if(npage_offset+singleton_entry_size==goparameters.ntree_buf_page_size)
				{
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries, singleton_entry_size);
					npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
					if(mptree_buffer[npage_no]==NULL)
					{
						mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
						//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
					}			
					memcpy(mptree_buffer[npage_no], pcfp_node->pentries->pitems, itemset_size);
				}
				else if(npage_offset+singleton_entry_size<goparameters.ntree_buf_page_size)
				{
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries, singleton_entry_size);
					npage_offset += singleton_entry_size;
					char* ptemp_itemset;
					ptemp_itemset = (char*)(pcfp_node->pentries->pitems);
					part1_size = goparameters.ntree_buf_page_size-npage_offset;
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries->pitems, part1_size);
					npage_offset = 0;
					npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
					if(mptree_buffer[npage_no]==NULL)
					{
						mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
						//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
					}			
					memcpy(mptree_buffer[npage_no], &(ptemp_itemset[part1_size]), itemset_size-part1_size);
				}
				else 
				{
					char *ptemp_entry;
					ptemp_entry = (char*)(pcfp_node->pentries);
					part1_size = goparameters.ntree_buf_page_size-npage_offset;
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries, part1_size);
					npage_offset = 0;
					npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
					if(mptree_buffer[npage_no]==NULL)
					{
						mptree_buffer[npage_no] = new char[goparameters.ntree_buf_page_size];
						//goMemTracer.IncCFPSize(goparameters.ntree_buf_page_size);
					}
					memcpy(mptree_buffer[npage_no], &(ptemp_entry[part1_size]), singleton_entry_size-part1_size);
					npage_offset += (singleton_entry_size-part1_size);
					memcpy(&(mptree_buffer[npage_no][npage_offset]), pcfp_node->pentries->pitems, itemset_size);
				}
			}

//			if(mnactive_top==0)
//				DumpAll();
		}

		if(pcfp_node->length<-1)
		{
			delete [](pcfp_node->pentries->pitems);
			//goMemTracer.DecCFPSize((-pcfp_node->length)*sizeof(int));
		}
		delete pcfp_node->pentries;
		//goMemTracer.DecCFPSize(sizeof(ENTRY));
		delete pcfp_node;
		//goMemTracer.DecCFPSize(sizeof(CFP_NODE));
	}

	return bitmap;
}

//---------------------------------------------------------------------------
//Returns the first entry in a CFP-node with support no less than mnmin_sup
//---------------------------------------------------------------------------
int CTreeOutBufManager::bsearch(int mnmin_sup, ENTRY * pentries, int num_of_freqitems)
{
	int low, high, mid;

	low = 0;
	high = num_of_freqitems;

	if(pentries[0].support==mnmin_sup)
		return 0;
	else if(pentries[num_of_freqitems-1].support<mnmin_sup)
		return -1;

	while (low <= high)
	{
		mid = (low + high)/2; 
		if (pentries[mid].support==mnmin_sup)
		{
			while(mid>0 && pentries[mid].support==mnmin_sup)
				mid--;
			if(pentries[mid].support<mnmin_sup)
				mid++;
			return mid;
		}
 
		if(pentries[mid].support<mnmin_sup)
			low = mid+1;
		else 
			high = mid-1;
	}

	if(pentries[low].support>mnmin_sup)
	{
		while(low>0 && pentries[low].support>mnmin_sup)
			low--;
		if(pentries[low].support<mnmin_sup)
			low++;
	}
	else if(pentries[low].support<mnmin_sup)
	{
		while(low<num_of_freqitems-1 && pentries[low].support<mnmin_sup)
			low++;
	}

	return low;
}

//--------------------------------------------------------------------------------------------------
//A CFP-node can either be in active stack, in output buffer or on disk. 
//If a node is in active stack, then its descendant can be in active stack, output buffer or on disk.
//If a node is on disk, then its descendant can be on disk or in output buffer.
//If a node is in output buffer but not in active stack, then all of its desendants are in output buffer.
//---------------------------------------------------------------------------------------------------
unsigned int CTreeOutBufManager::SearchActiveStack(int nactive_level, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos)
{
	int num_of_entries, i, j;
	int found;
	int *pnewpattern;
	int ncur_pos;

	found = 0;
	num_of_entries = mpactive_stack[nactive_level].pcfp_node->length;

	if(num_of_entries==1)
	{
		ENTRY *pcfp_entry;
		int  item_pos_inpat, num_of_items_inentry;

		pcfp_entry = mpactive_stack[nactive_level].pcfp_node->pentries;
		ncur_pos = mpactive_stack[nactive_level].pcfp_node->pos+sizeof(int);

		if(npat_len==0)
		{
			if(pcfp_entry->support==nparent_sup)
				return ncur_pos;
			else 
				return nparent_pos;
		}

		if(pcfp_entry->support<nsupport || pcfp_entry->child==0 && 
			(num_of_entries<npat_len || pcfp_entry->hash_bitmap!=0))
			return 0;

		item_pos_inpat = -1;
		num_of_items_inentry = 0;
		for(i=0;i<npat_len;i++)
		{
			if(ppattern[i]==pcfp_entry->item)
			{
				item_pos_inpat = i;
				num_of_items_inentry++;
				break;
			}
		}

		if(item_pos_inpat>=0 && npat_len==1 && pcfp_entry->support==nsupport)
		{
			mpat_len++;
			return ncur_pos;
		}
		else if(item_pos_inpat>=0 && npat_len==1)
		{
			printf("Error with the support of the entry %d %d\n", pcfp_entry->support, nsupport);
		}

		if(pcfp_entry->child!=0)
		{
			for(i=0;i<npat_len;i++)
			{
				if(i!=item_pos_inpat && !(pcfp_entry->hash_bitmap & (1<<(ppattern[i]%INT_BIT_LEN))))
					break;
			}
			if(i>=npat_len)
			{
				mpat_len++;
				if(item_pos_inpat>=0)
				{
					pnewpattern = new int[npat_len-1];
					i = 0;
					for(j=0;j<npat_len;j++)
					{
						if(j!=item_pos_inpat)
						{
							pnewpattern[i] = ppattern[j];
							i++;
						}
					}
				}
				else 
					pnewpattern = ppattern;
				if(nactive_level+1>gotree_bufmanager.mnactive_top)
				{
					printf("Error: wrong active_level value\n");
					exit(-1);
				}
				else if(nactive_level+1==gotree_bufmanager.mnactive_top)
				{
					if(pcfp_entry->child<gotree_bufmanager.mndisk_write_start_pos)
						found = SearchOnDisk(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
					else if(pcfp_entry->child<gotree_bufmanager.mntree_size)
						found = SearchInBuf(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
				}
				else
				{
					if(pcfp_entry->child==mpactive_stack[nactive_level+1].disk_pos)
						found = SearchActiveStack(nactive_level+1, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
					else if(pcfp_entry->child<gotree_bufmanager.mndisk_write_start_pos)
						found = SearchOnDisk(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
					else if(pcfp_entry->child<mpactive_stack[nactive_level+1].disk_pos)
						found = SearchInBuf(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
				}
				if(item_pos_inpat>=0)
					delete []pnewpattern;
				if(found)
				{
					return found;
				}
				mpat_len--;
			}
		}
		else
			return 0;
	}
	else if(num_of_entries<-1 && num_of_entries>-gnmax_pattern_len)
	{
		ENTRY *pcfp_entry;
		int *pitems, *ppat_bitmap, num_of_items_inentry;

		pcfp_entry = mpactive_stack[nactive_level].pcfp_node->pentries;
		ncur_pos = mpactive_stack[nactive_level].pcfp_node->pos+sizeof(int);

		if(npat_len==0)
		{
			if(pcfp_entry->support==nparent_sup)
				return ncur_pos;
			else 
				return nparent_pos;
		}

		num_of_entries = -num_of_entries;
		if(pcfp_entry->support<nsupport || pcfp_entry->child==0 && 
			(num_of_entries<npat_len || pcfp_entry->hash_bitmap!=0))
			return 0;

		ppat_bitmap = new int[npat_len];
		for(i=0;i<npat_len;i++)
			ppat_bitmap[i] = 0;
		num_of_items_inentry = 0;

		pitems = pcfp_entry->pitems;
		for(j=0;j<num_of_entries;j++)
		{
			for(i=0;i<npat_len;i++)
			{
				if(ppat_bitmap[i]==0 && ppattern[i]==pitems[j])
				{
					ppat_bitmap[i] = 1;
					num_of_items_inentry++;
					break;
				}
				else if(ppat_bitmap[i]==1 && ppattern[i]==pitems[j])
				{
					printf("Error: duplicated items in an entry\n");
				}
			}
		}

		if(npat_len==num_of_items_inentry && pcfp_entry->support==nsupport)
		{
			mpat_len += num_of_entries;
			delete []ppat_bitmap;
			return ncur_pos;
		}
		else if(npat_len==num_of_items_inentry)
		{
			printf("Error with the support of the entry %d %d\n", pcfp_entry->support, nsupport);
		}

		if(pcfp_entry->child!=0)
		{
			for(i=0;i<npat_len;i++)
			{
				if(!ppat_bitmap[i] && !(pcfp_entry->hash_bitmap & (1<<(ppattern[i]%INT_BIT_LEN))))
					break;
			}
			if(i>=npat_len)
			{
				mpat_len += num_of_entries;
				if(num_of_items_inentry>0)
				{
					pnewpattern = new int[npat_len-num_of_items_inentry];
					i = 0;
					for(j=0;j<npat_len;j++)
					{
						if(!ppat_bitmap[j])
						{
							pnewpattern[i] = ppattern[j];
							i++;
						}
					}
				}
				else 
					pnewpattern = ppattern;
				if(nactive_level+1>gotree_bufmanager.mnactive_top)
				{
					printf("Error: wrong active_level value\n");
					exit(-1);
				}
				else if(nactive_level+1==gotree_bufmanager.mnactive_top)
				{
					if(pcfp_entry->child<gotree_bufmanager.mndisk_write_start_pos)
						found = SearchOnDisk(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
					else if(pcfp_entry->child<gotree_bufmanager.mntree_size)
						found = SearchInBuf(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
				}
				else
				{
					if(pcfp_entry->child==mpactive_stack[nactive_level+1].disk_pos)
						found = SearchActiveStack(nactive_level+1, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
					else if(pcfp_entry->child<gotree_bufmanager.mndisk_write_start_pos)
						found = SearchOnDisk(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
					else if(pcfp_entry->child<mpactive_stack[nactive_level+1].disk_pos)
						found = SearchInBuf(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
				}
				if(num_of_items_inentry>0)
					delete []pnewpattern;
				if(found)
				{
					delete []ppat_bitmap;
					return found;
				}
				mpat_len -= num_of_entries;
			}
		}
		else
		{
			delete []ppat_bitmap;
			return 0;
		}
		delete []ppat_bitmap;
	}
	else if(num_of_entries>1)
	{
		ENTRY *pentries;
		int nfirst_sup_entry, nfirst_item_entry, nfirst_pat_item_pos, finished_order;

		if(npat_len==0)
			return nparent_pos;

		if(num_of_entries<npat_len)
			return 0;

		pentries = mpactive_stack[nactive_level].pcfp_node->pentries;
		if(nactive_level+1>=mnactive_top)
			finished_order = mpactive_stack[nactive_level].pcfp_node->cur_order-1;
		else
			finished_order = mpactive_stack[nactive_level].pcfp_node->cur_order;

		if(finished_order<0)
			return 0;
		else if(finished_order>num_of_entries)
			printf("Error with finished_order\n");

		nfirst_pat_item_pos = -1;
		nfirst_item_entry = -1;
		if(mpat_len==gnfirst_level_depth)
		{
			for(i=0;i<num_of_entries;i++)
			{
				if(pentries[i].item==ppattern[0])
				{
					nfirst_item_entry = i;
					nfirst_pat_item_pos = 0;
					break;
				}
			}
			if(i==num_of_entries)
			{
				printf("Error: cannot find the first item\n");
				exit(-1);
			}
		}
		else
		{
			for(j=0;j<npat_len;j++)
			{
				for(i=0;i<num_of_entries;i++)
				{
					if(pentries[i].item==ppattern[j])
					{
						if(nfirst_item_entry==-1 || nfirst_item_entry>i)
						{
							nfirst_item_entry = i;
							nfirst_pat_item_pos = j;
						}
						break;
					}
				}
				if(i==num_of_entries)
					return 0;
			}
		}

		if(nfirst_item_entry<=finished_order)
		{ 
			if(pentries[nfirst_item_entry].support<nsupport)
				return 0;
		}
		else if(pentries[finished_order].support<nsupport)
			return 0;

		nfirst_sup_entry = bsearch(nsupport, pentries, num_of_entries);
		if(nfirst_sup_entry==-1)
			return 0;

		if(nfirst_item_entry<=finished_order)
		{
			ncur_pos = mpactive_stack[nactive_level].pcfp_node->pos+sizeof(int)+sizeof(ENTRY)*nfirst_item_entry;
			if(pentries[nfirst_item_entry].child!=0)
			{
				for(j=0;j<npat_len;j++)
				{
					if(j!=nfirst_pat_item_pos && !(pentries[nfirst_item_entry].hash_bitmap & (1<<(ppattern[j]%INT_BIT_LEN))))
						break;
				}
				if(j>=npat_len)
				{
					mpat_len++;
					pnewpattern = new int[npat_len];
					for(i=0;i<nfirst_pat_item_pos;i++)
						pnewpattern[i] = ppattern[i];
					for(i=nfirst_pat_item_pos+1;i<npat_len;i++)
						pnewpattern[i-1] = ppattern[i];
					if(nactive_level+1>gotree_bufmanager.mnactive_top)
					{
						printf("Error: wrong active_level value\n");
						exit(-1);
					}
					else if(nactive_level+1==gotree_bufmanager.mnactive_top)
					{
						if(pentries[nfirst_item_entry].child<gotree_bufmanager.mndisk_write_start_pos)
							found = SearchOnDisk(pentries[nfirst_item_entry].child, pnewpattern, npat_len-1, nsupport, pentries[nfirst_item_entry].support, ncur_pos);
						else if(pentries[nfirst_item_entry].child<gotree_bufmanager.mntree_size)
							found = SearchInBuf(pentries[nfirst_item_entry].child, pnewpattern, npat_len-1, nsupport, pentries[nfirst_item_entry].support, ncur_pos);
					}
					else
					{
						if(pentries[nfirst_item_entry].child==mpactive_stack[nactive_level+1].disk_pos)
							found = SearchActiveStack(nactive_level+1, pnewpattern, npat_len-1, nsupport, pentries[nfirst_item_entry].support, ncur_pos);
						else if(pentries[nfirst_item_entry].child<gotree_bufmanager.mndisk_write_start_pos)
							found = SearchOnDisk(pentries[nfirst_item_entry].child, pnewpattern, npat_len-1, nsupport, pentries[nfirst_item_entry].support, ncur_pos);
						else if(pentries[nfirst_item_entry].child<mpactive_stack[nactive_level+1].disk_pos)
							found = SearchInBuf(pentries[nfirst_item_entry].child, pnewpattern, npat_len-1, nsupport, pentries[nfirst_item_entry].support, ncur_pos);
					}
					delete []pnewpattern;
					if(found)
						return found;
					mpat_len--;
				}
			}
			else  
			{
				if(npat_len==1 && pentries[nfirst_item_entry].support==nsupport)
				{
//					if(pentries[nfirst_item_entry].child==0 && pentries[nfirst_item_entry].hash_bitmap!=0)
//						printf("Error when pruning class\n");
					mpat_len++;
					return ncur_pos;
				}
				else if(npat_len==1)
				{
					printf("Error with the support of the entry %d %d\n", pentries[nfirst_item_entry].support, nsupport);
				}
			}
		}

		int nstart_order;
		if(finished_order>=nfirst_item_entry)
			nstart_order = nfirst_item_entry-1;
		else
			nstart_order = finished_order;
		for(i=nstart_order;i>=nfirst_sup_entry;i--)
		{
			ncur_pos = mpactive_stack[nactive_level].pcfp_node->pos+sizeof(int)+i*sizeof(ENTRY);
			if(pentries[i].child!=0)
			{
				for(j=0;j<npat_len;j++)
				{
					if(!(pentries[i].hash_bitmap & (1<<(ppattern[j]%INT_BIT_LEN))))
						break;
				}
				if(j>=npat_len)
				{
					mpat_len++;
					if(nactive_level+1>gotree_bufmanager.mnactive_top)
					{
						printf("Error: wrong active_level value\n");
						exit(-1);
					}
					else if(nactive_level+1==gotree_bufmanager.mnactive_top)
					{
						if(pentries[i].child<gotree_bufmanager.mndisk_write_start_pos)
							found = SearchOnDisk(pentries[i].child, ppattern, npat_len, nsupport, pentries[i].support, ncur_pos);
						else if(pentries[i].child<gotree_bufmanager.mntree_size)
							found = SearchInBuf(pentries[i].child, ppattern, npat_len, nsupport, pentries[i].support, ncur_pos);
					}
					else
					{
						if(pentries[i].child==mpactive_stack[nactive_level+1].disk_pos)
							found = SearchActiveStack(nactive_level+1, ppattern, npat_len, nsupport, pentries[i].support, ncur_pos);
						else if(pentries[i].child<gotree_bufmanager.mndisk_write_start_pos)
							found = SearchOnDisk(pentries[i].child, ppattern, npat_len, nsupport, pentries[i].support, ncur_pos);
						else if(pentries[i].child<mpactive_stack[nactive_level+1].disk_pos)
							found = SearchInBuf(pentries[i].child, ppattern, npat_len, nsupport, pentries[i].support, ncur_pos);
					}
					if(found)
						return found;
					mpat_len--;
				}
			}
		}
	}
	else
	{
		printf("Error with number of entires\n");
	}

	return 0;
}


//-----------------------------------------------------------------------------------------------------------
//Search supersets of ppattern with support nsupport on disk
//-----------------------------------------------------------------------------------------------------------
unsigned int CTreeOutBufManager::SearchOnDisk(int ndisk_pos, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos)
{
	int num_of_entries;
	int nfile_pos;
	int i, j;
	int *pnewpattern;
	int found;
	int ncur_pos;

	found = 0;
	nfile_pos = ftell(mfpread_file);
	if(ndisk_pos!=nfile_pos)
		fseek(mfpread_file, ndisk_pos-nfile_pos, SEEK_CUR);
	fread(&num_of_entries, sizeof(int), 1, mfpread_file);
	if(num_of_entries<-gnmax_pattern_len)
		printf("Error when reading from disk\n");

	if(num_of_entries==1)
	{
		ENTRY cfp_entry;
		int item_pos_inpat, num_of_items_inentry;

		fread(&cfp_entry, sizeof(ENTRY), 1, mfpread_file);

		ncur_pos = ndisk_pos+sizeof(int);
		if(npat_len==0)
		{
			if(cfp_entry.support==nparent_sup)
				return ncur_pos;
			else 
				return nparent_pos;
		}

		if(cfp_entry.support<nsupport || cfp_entry.child==0 && 
			(num_of_entries<npat_len || cfp_entry.hash_bitmap!=0))
			return 0;

		num_of_items_inentry = 0;
		item_pos_inpat = -1;
		for(i=0;i<npat_len;i++)
		{
			if(ppattern[i]==cfp_entry.item)
			{
				item_pos_inpat = i;
				num_of_items_inentry++;
				break;
			}
		}

		if(item_pos_inpat>=0 && npat_len==1 && cfp_entry.support==nsupport)
		{
			mpat_len++;
			return ncur_pos;
		}
		else if(item_pos_inpat>=0 && npat_len==1)
		{
			printf("Error with the support of the entry %d %d\n", cfp_entry.support, nsupport);
		}

		if(cfp_entry.child!=0)
		{
			for(i=0;i<npat_len;i++)
			{
				if(i!=item_pos_inpat && !(cfp_entry.hash_bitmap & (1<<(ppattern[i]%INT_BIT_LEN))))
					break;
			}
			if(i>=npat_len)
			{
				mpat_len++;
				if(item_pos_inpat>=0)
				{
					pnewpattern = new int[npat_len-1];
					i = 0;
					for(j=0;j<npat_len;j++)
					{
						if(j!=item_pos_inpat)
						{
							pnewpattern[i] = ppattern[j];
							i++;
						}
					}
				}
				else 
					pnewpattern = ppattern;
				if(cfp_entry.child<gotree_bufmanager.mndisk_write_start_pos)
					found = SearchOnDisk(cfp_entry.child, pnewpattern, npat_len-num_of_items_inentry, nsupport, cfp_entry.support, ncur_pos);
				else
					found = SearchInBuf(cfp_entry.child, pnewpattern, npat_len-num_of_items_inentry, nsupport, cfp_entry.support, ncur_pos);
				if(item_pos_inpat>=0)
					delete []pnewpattern;
				if(found)
					return found;
				mpat_len--;
			}
		}
		else
			return 0;
	}
	else if(num_of_entries<-1 && num_of_entries>-gnmax_pattern_len)
	{
		ENTRY cfp_entry;
		int *pitems, *ppat_bitmap;
		int num_of_items_inentry;

		num_of_entries = -num_of_entries;
		fread(&cfp_entry, sizeof(int), sizeof(ENTRY)/sizeof(int)-1, mfpread_file);

		ncur_pos = ndisk_pos+sizeof(int);
		if(npat_len==0)
		{
			if(cfp_entry.support==nparent_sup)
				return ncur_pos;
			else 
				return nparent_pos;
		}

		if(cfp_entry.support<nsupport || cfp_entry.child==0 && num_of_entries<npat_len)
			return 0;

		pitems = new int[num_of_entries];
		fread(pitems, sizeof(int), num_of_entries, mfpread_file);

		ppat_bitmap = new int[npat_len];
		for(i=0;i<npat_len;i++)
			ppat_bitmap[i] = 0;
		num_of_items_inentry = 0;

		for(j=0;j<num_of_entries;j++)
		{
			for(i=0;i<npat_len;i++)
			{
				if(ppat_bitmap[i]==0 && ppattern[i]==pitems[j])
				{
					ppat_bitmap[i] = 1;
					num_of_items_inentry++;
					break;
				}
				else if(ppat_bitmap[i]==1 && ppattern[i]==pitems[j])
				{
					printf("Error: duplicated items in an entry\n");
				}
			}
		}

		if(npat_len==num_of_items_inentry && cfp_entry.support==nsupport)
		{
			mpat_len += num_of_entries;
			delete []pitems;
			delete []ppat_bitmap;
			return ncur_pos;
		}
		else if(npat_len==num_of_items_inentry)
		{
			printf("Error with the support of the entry %d %d\n", cfp_entry.support, nsupport);
		}

		if(cfp_entry.child!=0)
		{
			for(i=0;i<npat_len;i++)
			{
				if(!ppat_bitmap[i] && !(cfp_entry.hash_bitmap & (1<<(ppattern[i]%INT_BIT_LEN))))
					break;
			}
			if(i>=npat_len)
			{
				mpat_len += num_of_entries;
				if(num_of_items_inentry>0)
				{
					pnewpattern = new int[npat_len-num_of_items_inentry];
					i = 0;
					for(j=0;j<npat_len;j++)
					{
						if(!ppat_bitmap[j])
						{
							pnewpattern[i] = ppattern[j];
							i++;
						}
					}
				}
				else 
					pnewpattern = ppattern;
				if(cfp_entry.child<gotree_bufmanager.mndisk_write_start_pos)
					found = SearchOnDisk(cfp_entry.child, pnewpattern, npat_len-num_of_items_inentry, nsupport , cfp_entry.support, ncur_pos);
				else
					found = SearchInBuf(cfp_entry.child, pnewpattern, npat_len-num_of_items_inentry, nsupport, cfp_entry.support, ncur_pos);
				if(num_of_items_inentry>0)
					delete []pnewpattern;
				if(found)
				{
					delete []pitems;
					delete []ppat_bitmap;
					return found;
				}
				mpat_len -= num_of_entries;
			}
		}
		else
		{
			delete []pitems;
			delete []ppat_bitmap;
			return 0;
		}
		delete []pitems;
		delete []ppat_bitmap;
	}
	else if(num_of_entries>1)
	{
		ENTRY* pentries;
		int nfirst_sup_entry, nfirst_item_entry, nfirst_pat_item_pos;

		if(npat_len==0)
			return nparent_pos;

		pentries = new ENTRY[num_of_entries];
		fread(pentries, sizeof(ENTRY), num_of_entries, mfpread_file);

		nfirst_pat_item_pos = -1;
		nfirst_item_entry = -1;
		if(num_of_entries<npat_len)
		{
			delete []pentries;
			return 0;
		}

		if(mpat_len==gnfirst_level_depth)
		{
			for(i=0;i<num_of_entries;i++)
			{
				if(pentries[i].item==ppattern[0])
				{
					nfirst_item_entry = i;
					nfirst_pat_item_pos = 0;
					break;
				}
			}
			if(i==num_of_entries)
			{
				printf("Error: cannot find the first item\n");
				exit(-1);
			}
		}
		else
		{
			for(j=0;j<npat_len;j++)
			{
				for(i=0;i<num_of_entries;i++)
				{
					if(pentries[i].item==ppattern[j])
					{
						if(nfirst_item_entry==-1 || nfirst_item_entry>i)
						{
							nfirst_item_entry = i;
							nfirst_pat_item_pos = j;
						}
						break;
					}
				}
				if(i==num_of_entries)
				{
					delete []pentries;
					return 0;
				}
			}
		}

		if(pentries[nfirst_item_entry].support<nsupport)
		{
			delete []pentries;
			return 0;
		}

		nfirst_sup_entry = bsearch(nsupport, pentries, num_of_entries);
		if(nfirst_sup_entry==-1)
		{
			delete []pentries;
			return 0;
		}

		for(i=nfirst_sup_entry;i<nfirst_item_entry;i++)
		{
			ncur_pos = ndisk_pos+sizeof(int)+i*sizeof(ENTRY);
			if(i>num_of_entries)
				printf("Error with value of i %d %d\n", i, num_of_entries);
			if(pentries[i].child!=0)
			{
				for(j=0;j<npat_len;j++)
				{
					if(!(pentries[i].hash_bitmap & (1<<(ppattern[j]%INT_BIT_LEN))))
						break;
				}
				if(j>=npat_len)
				{
					mpat_len++;

					if(pentries[i].child<gotree_bufmanager.mndisk_write_start_pos)
						found = SearchOnDisk(pentries[i].child, ppattern, npat_len, nsupport, pentries[i].support, ncur_pos);
					else
						found = SearchInBuf(pentries[i].child, ppattern, npat_len, nsupport, pentries[i].support, ncur_pos);
					if(found)
					{
						delete []pentries;
						return found;
					}
					mpat_len--;
				}
			}
		}

		ncur_pos = ndisk_pos+sizeof(int)+nfirst_item_entry*sizeof(ENTRY);
		if(pentries[nfirst_item_entry].child!=0)
		{
			for(j=0;j<npat_len;j++)
			{
				if(j!=nfirst_pat_item_pos && !(pentries[nfirst_item_entry].hash_bitmap & (1<<(ppattern[j]%INT_BIT_LEN))))
					break;
			}
			if(j>=npat_len)
			{
				mpat_len++;
				pnewpattern = new int[npat_len];
				for(i=0;i<nfirst_pat_item_pos;i++)
					pnewpattern[i] = ppattern[i];
				for(i=nfirst_pat_item_pos+1;i<npat_len;i++)
					pnewpattern[i-1] = ppattern[i];
				if(pentries[nfirst_item_entry].child==0)
					printf("Error with child\n");
				else if(pentries[nfirst_item_entry].child<gotree_bufmanager.mndisk_write_start_pos)
					found = SearchOnDisk(pentries[nfirst_item_entry].child, pnewpattern, npat_len-1, nsupport, pentries[nfirst_item_entry].support, ncur_pos);
				else
					found = SearchInBuf(pentries[nfirst_item_entry].child, pnewpattern, npat_len-1, nsupport, pentries[nfirst_item_entry].support, ncur_pos);
				delete []pnewpattern;
				if(found)
				{
					delete []pentries;
					return found;
				}
				mpat_len--;
			}
		}
		else
		{
			if(npat_len==1 && pentries[nfirst_item_entry].support==nsupport)
			{
//				if(pentries[nfirst_item_entry].child==0 && pentries[nfirst_item_entry].hash_bitmap!=0)
//					printf("Error when pruning class\n");
				mpat_len++;
				delete []pentries;
				return ncur_pos;
			}
			else if(npat_len==1)
			{
				printf("Error with entry support %d %d\n", pentries[nfirst_item_entry].support, nsupport);
			}
		}
		delete []pentries;
	}
	else 
	{
		printf("Error with number of entries\n");
	}
	return 0;
}

//-----------------------------------------------------------------------------------------------------------
//Search supersets of ppattern with support nsupport in output buffer.
//-----------------------------------------------------------------------------------------------------------
unsigned int CTreeOutBufManager::SearchInBuf(int ndisk_pos, int *ppattern, int npat_len, int nsupport, int nparent_sup, int nparent_pos)
{
	int num_of_entries, node_size, part1_size;
	int nmem_pos, i, j, found;
	int *pnewpattern;
	char* ptemp_str;
	bool overflow;
	int ncur_pos, npage_offset, npage_no;

	nmem_pos = ndisk_pos%mnbuffer_size;
	if(nmem_pos!=(gotree_bufmanager.mnend_pos-(gotree_bufmanager.mntree_size-ndisk_pos)+mnbuffer_size)%mnbuffer_size)
		printf("Error[SearchInBuf]: with memeory position\n");

	npage_no = nmem_pos/goparameters.ntree_buf_page_size;
	npage_offset = nmem_pos & (goparameters.ntree_buf_page_size-1);

	memcpy(&num_of_entries, &(mptree_buffer[npage_no][npage_offset]), sizeof(int));
	if(num_of_entries<-gnmax_pattern_len)
		printf("Error when reading output buffer\n");
	npage_offset += sizeof(int);
	if(npage_offset==goparameters.ntree_buf_page_size)
	{
		npage_offset = 0;
		npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
	}

	if(num_of_entries==1)
	{
		ENTRY *pcfp_entry;
		int num_of_items_inentry, item_pos_inpat;

		node_size = sizeof(ENTRY);
		if(npage_offset+node_size<=goparameters.ntree_buf_page_size)
		{
			overflow = false;
			pcfp_entry = (ENTRY*)&(mptree_buffer[npage_no][npage_offset]);
		}
		else 
		{
			overflow = true;
			pcfp_entry = new ENTRY;
			ptemp_str = (char*)pcfp_entry;
			part1_size = goparameters.ntree_buf_page_size-npage_offset;
			memcpy(pcfp_entry, &(mptree_buffer[npage_no][npage_offset]), part1_size);
			npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
			memcpy(&(ptemp_str[part1_size]), mptree_buffer[npage_no], node_size-part1_size);
		}
		
		ncur_pos = ndisk_pos+sizeof(int);
		if(npat_len==0)
		{
			if(pcfp_entry->support==nparent_sup)
			{
				if(overflow)
					delete pcfp_entry;
				return ncur_pos;
			}
			else 
			{
				if(overflow)
					delete pcfp_entry;
				return nparent_pos;
			}
		}

		if(pcfp_entry->support<nsupport || pcfp_entry->child==0 && 
			(num_of_entries<npat_len || pcfp_entry->hash_bitmap!=0))
		{
			if(overflow)
				delete pcfp_entry;
			return 0;
		}

		num_of_items_inentry = 0;
		item_pos_inpat = -1;
		for(i=0;i<npat_len;i++)
		{
			if(ppattern[i]==pcfp_entry->item)
			{
				item_pos_inpat = i;
				num_of_items_inentry++;
				break;
			}
		}

		if(item_pos_inpat>=0 && npat_len==1 && pcfp_entry->support==nsupport)
		{
			if(overflow)
				delete pcfp_entry;
			mpat_len++;
			return ncur_pos;
		}
		else if(item_pos_inpat>=0 && npat_len==1)
		{
			printf("Error with the support of the entry %d %d\n", pcfp_entry->support, nsupport);
		}

		if(pcfp_entry->child!=0)
		{
			for(i=0;i<npat_len;i++)
			{
				if(i!=item_pos_inpat && !(pcfp_entry->hash_bitmap & (1<<(ppattern[i]%INT_BIT_LEN))))
					break;
			}
			if(i>=npat_len)
			{
				mpat_len++;
				if(item_pos_inpat>=0)
				{
					pnewpattern = new int[npat_len-1];
					i = 0;
					for(j=0;j<npat_len;j++)
					{
						if(j!=item_pos_inpat)
						{
							pnewpattern[i] = ppattern[j];
							i++;
						}
					}
				}
				else 
					pnewpattern = ppattern;
				found = SearchInBuf(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
				if(item_pos_inpat>=0)
					delete []pnewpattern;
				if(found)
				{
					if(overflow)
						delete pcfp_entry;
					return found;
				}
				mpat_len--;
			}
		}
		else
		{
			if(overflow)
				delete pcfp_entry;
			return 0;
		}
		if(overflow)
			delete pcfp_entry;
	}
	else if(num_of_entries<-1 && num_of_entries>-gnmax_pattern_len)
	{
		ENTRY *pcfp_entry;
		int *pitems, *ppat_bitmap;
		int num_of_items_inentry, itemset_size, nentry_size;

		num_of_entries = -num_of_entries;
		itemset_size = sizeof(int)*num_of_entries;
		nentry_size = sizeof(ENTRY)-sizeof(int);
		node_size = itemset_size+nentry_size;

		if(npage_offset+node_size<=goparameters.ntree_buf_page_size)
		{
			overflow = false;
			pcfp_entry = (ENTRY*)&(mptree_buffer[npage_no][npage_offset]);
			pitems = (int*)&(mptree_buffer[npage_no][npage_offset+nentry_size]);
		}
		else 
		{
			overflow = true;
			pcfp_entry = new ENTRY;
			pitems = new int[num_of_entries];
			if(npage_offset+nentry_size<goparameters.ntree_buf_page_size)
			{
				memcpy(pcfp_entry, &(mptree_buffer[npage_no][npage_offset]), nentry_size);
				npage_offset += nentry_size;
				ptemp_str = (char*)pitems;
				part1_size = goparameters.ntree_buf_page_size-npage_offset;
				memcpy(pitems, &(mptree_buffer[npage_no][npage_offset]), part1_size);
				npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
				memcpy(&(ptemp_str[part1_size]), mptree_buffer[npage_no], itemset_size-part1_size);
			}
			else if (npage_offset+nentry_size==goparameters.ntree_buf_page_size)
			{
				memcpy(pcfp_entry, &(mptree_buffer[npage_no][npage_offset]), nentry_size);
				npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
				memcpy(pitems, mptree_buffer[npage_no], itemset_size);
			}
			else
			{
				ptemp_str = (char*)pcfp_entry;
				part1_size = goparameters.ntree_buf_page_size-npage_offset;
				memcpy(pcfp_entry, &(mptree_buffer[npage_no][npage_offset]), part1_size);
				npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
				memcpy(&(ptemp_str[part1_size]), mptree_buffer[npage_no], nentry_size-part1_size);
				npage_offset = nentry_size-part1_size;
				memcpy(pitems, &(mptree_buffer[npage_no][npage_offset]), itemset_size);
			}
		}

		ncur_pos = ndisk_pos+sizeof(int);
		if(npat_len==0)
		{
			if(pcfp_entry->support==nparent_sup)
			{
				if(overflow)
				{
					delete pcfp_entry;
					delete []pitems;
				}
				return ncur_pos;
			}
			else 
			{
				if(overflow)
				{
					delete pcfp_entry;
					delete []pitems;
				}
				return nparent_pos;
			}
		}
		
		if(pcfp_entry->support<nsupport || pcfp_entry->child==0 && num_of_entries<npat_len)
		{
			if(overflow)
			{
				delete pcfp_entry;
				delete []pitems;
			}
			return 0;
		}

		ppat_bitmap = new int[npat_len];
		for(i=0;i<npat_len;i++)
			ppat_bitmap[i] = 0;
		num_of_items_inentry = 0;

		for(j=0;j<num_of_entries;j++)
		{
			for(i=0;i<npat_len;i++)
			{
				if(ppat_bitmap[i]==0 && ppattern[i]==pitems[j])
				{
					ppat_bitmap[i] = 1;
					num_of_items_inentry++;
					break;
				}
				else if(ppat_bitmap[i]==1 && ppattern[i]==pitems[j])
				{
					printf("Error: duplicated items in an entry\n");
				}
			}
		}

		if(npat_len==num_of_items_inentry && pcfp_entry->support==nsupport)
		{
			if(overflow)
			{
				delete pcfp_entry;
				delete []pitems;
			}
			delete []ppat_bitmap;
			mpat_len += num_of_entries;
			return ncur_pos;
		}
		else if(npat_len==num_of_items_inentry)
		{
			printf("Error with the support of the entry %d %d\n", pcfp_entry->support, nsupport);
		}

		if(pcfp_entry->child!=0)
		{
			for(i=0;i<npat_len;i++)
			{
				if(!ppat_bitmap[i] && !(pcfp_entry->hash_bitmap & (1<<(ppattern[i]%INT_BIT_LEN))))
					break;
			}
			if(i>=npat_len)
			{
				mpat_len += num_of_entries;
				if(num_of_items_inentry>0)
				{
					pnewpattern = new int[npat_len-num_of_items_inentry];
					i = 0;
					for(j=0;j<npat_len;j++)
					{
						if(ppat_bitmap[j]==0)
						{
							pnewpattern[i] = ppattern[j];
							i++;
						}
					}
				}
				else 
					pnewpattern = ppattern;
				found = SearchInBuf(pcfp_entry->child, pnewpattern, npat_len-num_of_items_inentry, nsupport, pcfp_entry->support, ncur_pos);
				if(num_of_items_inentry>0)
					delete []pnewpattern;
				if(found)
				{
					if(overflow)
					{
						delete pcfp_entry;
						delete []pitems;
					}
					delete []ppat_bitmap;
					return found;
				}
				mpat_len -=num_of_entries;
			}
		}
		else
		{
			if(overflow)
			{
				delete pcfp_entry;
				delete []pitems;
			}
			delete []ppat_bitmap;
			return 0;
		}
		if(overflow)
		{
			delete pcfp_entry;
			delete []pitems;
		}
		delete []ppat_bitmap;
	}
	else if(num_of_entries>1)
	{
		ENTRY *pentries;
		int nfirst_sup_entry, nfirst_item_entry, nfirst_pat_item_pos;

		if(npat_len==0)
			return nparent_pos;

		node_size = num_of_entries*sizeof(ENTRY);
		if(npage_offset+node_size<=goparameters.ntree_buf_page_size)
		{
			overflow = false;
			pentries = (ENTRY*)&(mptree_buffer[npage_no][npage_offset]);
		}
		else 
		{
			overflow = true;
			pentries = new ENTRY[num_of_entries];
			ptemp_str = (char*)pentries;
			part1_size = goparameters.ntree_buf_page_size-npage_offset;
			memcpy(pentries, &(mptree_buffer[npage_no][npage_offset]), part1_size);
			npage_no = (npage_no+1)%goparameters.ntree_buf_num_of_pages;
			memcpy(&(ptemp_str[part1_size]), mptree_buffer[npage_no], node_size-part1_size);
		}

		found = 0;
		nfirst_pat_item_pos = -1;
		nfirst_item_entry = -1;

		for(j=0;j<npat_len;j++)
		{
			for(i=0;i<num_of_entries;i++)
			{
				if(pentries[i].item==ppattern[j])
				{
					if(nfirst_item_entry==-1 || nfirst_item_entry>i)
					{
						nfirst_item_entry = i;
						nfirst_pat_item_pos = j;
					}
					break;
				}
			}
			if(i==num_of_entries)
			{
				if(overflow)
					delete []pentries;
				return 0;
			}
		}

		if(pentries[nfirst_item_entry].support<nsupport)
		{
			if(overflow)
				delete []pentries;
			return 0;
		}
		
		nfirst_sup_entry = bsearch(nsupport, pentries, num_of_entries);
		if(nfirst_sup_entry==-1)
		{
			if(overflow)
				delete []pentries;
			return 0;
		}
		
		for(i=nfirst_sup_entry;i<nfirst_item_entry;i++)
		{
			ncur_pos = ncur_pos = ndisk_pos+sizeof(int)+i*sizeof(ENTRY);
			if(pentries[i].child!=0)
			{
				for(j=0;j<npat_len;j++)
				{
					if(!(pentries[i].hash_bitmap & (1<<(ppattern[j]%INT_BIT_LEN))))
						break;
				}
				if(j>=npat_len)
				{
					mpat_len++;
					found = SearchInBuf(pentries[i].child, ppattern, npat_len, nsupport, pentries[i].support, ncur_pos);
					if(found)
					{
						if(overflow)
							delete []pentries;
						return found;
					}
					mpat_len--;
				}
			}
		}

		ncur_pos = ndisk_pos+sizeof(int)+nfirst_item_entry*sizeof(ENTRY);
		if(pentries[nfirst_item_entry].child!=0)
		{
			for(j=0;j<npat_len;j++)
			{
				if(j!=nfirst_pat_item_pos && !(pentries[nfirst_item_entry].hash_bitmap & (1<<(ppattern[j]%INT_BIT_LEN))))
					break;
			}
			if(j>=npat_len)
			{
				mpat_len++;
				pnewpattern = new int[npat_len];
				for(i=0;i<nfirst_pat_item_pos;i++)
					pnewpattern[i] = ppattern[i];
				for(i=nfirst_pat_item_pos+1;i<npat_len;i++)
					pnewpattern[i-1] = ppattern[i];
				found = SearchInBuf(pentries[nfirst_item_entry].child, pnewpattern, npat_len-1, nsupport, pentries[nfirst_item_entry].support, ncur_pos);
				delete []pnewpattern;
				if(found)
				{
					if(overflow)
						delete []pentries;
					return found;
				}
				mpat_len--;
			}
		}
		else 
		{
			if(npat_len==1 && pentries[nfirst_item_entry].support==nsupport)
			{
//				if(pentries[nfirst_item_entry].child==0 && pentries[nfirst_item_entry].hash_bitmap!=0)
//					printf("Error when pruning class\n");
				mpat_len++;
				if(overflow)
					delete []pentries;
				return ncur_pos;
			}
		}

		if(overflow)
			delete []pentries;
	}
	else
	{
		printf("Error with number of entries\n");
	}
	return 0;
}
