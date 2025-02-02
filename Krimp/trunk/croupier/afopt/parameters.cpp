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
#include "../clobal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined (__unix__)
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
