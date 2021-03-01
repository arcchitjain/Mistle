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

