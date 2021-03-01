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

#include <string.h>

#include "Global.h"
#include "ScanDBMine.h"
#include "AFOPTMine.h"
//#include "ArrayMine.h"
#include "fsout.h"
#include "data.h"
#include "parameters.h"

//#include "MemTracer.h"
//#include "TimeTracer.h"


namespace Afopt {
	//parameter settings
	int gnmin_sup;

	//data statistics
	int gndb_size;
	int gndb_ntrans;
	int	gnmax_trans_len;
	int gnmax_item_id;

	//pattern statistics
	int gntotal_freqitems;
	unsigned long long gntotal_patterns;
	int	gnmax_pattern_len;
	int gntotal_singlepath;
	int *gppat_counters;

	//program running statistics
	int	 gndepth;
	int gntotal_call;
	int  gnfirst_level;
	int gnfirst_level_depth;

	int* gpheader_itemset;

	//global variables
	CScanDBMine  goDBMiner;
	CAFOPTMine	goAFOPTMiner;
	//CArrayMine goArrayMiner;
	CParameters goparameters;
	Data* gpdata;
	Output	*gpout;

	//CTimeTracer goTimeTracer;
	//CMemTracer  goMemTracer;


	void PrintSummary()
	{
		int i, ntotal_pats;

		ntotal_pats = 0;
		if(gppat_counters[0]!=0)
			printf("Error: pattern length is 0\n");
		for(i=1;i<=gnmax_pattern_len;i++)
		{
			printf("%d\n", gppat_counters[i]);
			ntotal_pats += gppat_counters[i];
		}
		for(i=gnmax_pattern_len+1;i<gnmax_trans_len;i++)
		{
			if(gppat_counters[i]!=0)
				printf("Error with pattern length\n");
		}

		//	if(ntotal_pats!=gntotal_patterns)
		//		printf("Error: inconsistent total pattern numbers %d %d\n", ntotal_pats, gntotal_patterns);

		/*
		FILE *summary_file;

		summary_file = fopen(SUMMARY_FILENAME, "a+");
		if(summary_file == NULL)
		{
		printf("Error[MinePattern]: cannot open file %s\n", SUMMARY_FILENAME);
		return;
		}
		fprintf(summary_file, "%s ", goparameters.data_filename);
		fprintf(summary_file, "%f ", ((double)goparameters.nmin_sup*100/gndb_size));
		//	fprintf(summary_file, "%d\t", gnmax_pattern_len);
		fprintf(summary_file, "%d\t", gntotal_patterns);
		fprintf(summary_file, "%d\t", gntotal_call);
		fprintf(summary_file, "%d\t", gntotal_singlepath);
		goTimeTracer.PrintStatistics(summary_file);
		goMemTracer.PrintStatistics(summary_file);

		fprintf(summary_file, "\n");
		fclose(summary_file);
		*/
	}


	//sort items in ascending frequency order
	int comp_item_freq_asc(const void *e1, const void *e2)
	{
		ITEM_COUNTER pair1,pair2;
		pair1=*(ITEM_COUNTER *) e1;
		pair2=*(ITEM_COUNTER *) e2;

		if ((pair1.support>pair2.support) )
			return (1);
		else if (pair1.support<pair2.support)
			return(-1);
		return (0);
	}



	int comp_int_asc(const void *e1, const void *e2)
	{
		int pair1,pair2;
		pair1=*(int *) e1;
		pair2=*(int *) e2;

		if (pair1>pair2)
			return (1);
		else if (pair1<pair2)
			return(-1);
		else
			return (0);
	}
}

