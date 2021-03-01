#include "../clobal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <glibc_s.h>
#endif

#include "parameters.h"
#include "Global.h"

using namespace Afopt;

void CParameters::SetResultName(const char *result_name)
{
	if(goparameters.bresult_name_given)
		strcpy_s(pat_filename, MAX_FILENAME_LEN, result_name);
}

void CParameters::SetBufParameters(int entry_size)
{
	ntree_buf_page_size = (1<<DEFAULT_TREE_BUF_PAGE_SIZE);
	if((unsigned int)ntree_buf_page_size<entry_size+sizeof(int)*(gnmax_trans_len+1))
	{
		while((unsigned int)ntree_buf_page_size<entry_size+sizeof(int)*(gnmax_trans_len+1))
			ntree_buf_page_size = ntree_buf_page_size*2;
	}
	if((unsigned int)ntree_buf_page_size<entry_size*gntotal_freqitems+2*sizeof(int))
	{
		while((unsigned int)ntree_buf_page_size<entry_size*gntotal_freqitems+2*sizeof(int))
			ntree_buf_page_size = ntree_buf_page_size*2;
	}

	ntree_outbuf_size = DEFAULT_TREE_OUTBUF_SIZE;

	ntree_buf_num_of_pages = (ntree_outbuf_size-1)/ntree_buf_page_size+1;
	ntree_outbuf_size = ntree_buf_page_size*ntree_buf_num_of_pages;

}
