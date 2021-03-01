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
#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#define MAX_FILENAME_LEN	200

#define CFP_TREE_FILENAME		"afopt-cls-miner.tmp"

#define DEFAULT_TREE_OUTBUF_SIZE		(1<<28)	// was 27
#define DEFAULT_TREE_BUF_PAGE_SIZE		20

#define ITEM_SUP_MAP_SIZE	10000

#define SUP_MAXLEN_MAP_SIZE 10000

namespace Afopt {
	class CParameters
	{
	public:
		char data_filename[MAX_FILENAME_LEN];
		int nmin_sup;
		bool results_to_file;
		bool bresult_name_given;
		char pat_filename[MAX_FILENAME_LEN];

		char tree_filename[MAX_FILENAME_LEN];
		char pattern_type[4];

		int ntree_outbuf_size;
		int ntree_buf_page_size;
		int ntree_buf_num_of_pages;

		void SetResultName(const char *result_name);
		void SetBufParameters(int entry_size);

	};
	extern CParameters	goparameters;
}
#endif

