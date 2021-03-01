#ifndef _FOUT_H
#define _FOUT_H

#include <stdio.h>
#if defined (_WINDOWS)
	#include <Windows.h>
#endif

#include <Primitives.h>

#include "../Croupier.h"
#include "AfoptCroupier.h"

#include "Global.h"
#include "parameters.h"

class Database;
class ItemSet;
enum ItemSetType;

namespace Afopt {
	class Output
	{
	public:
		Output() { mHasBinSizes = false; mFinished = false; }
		virtual ~Output() {}

		virtual int isOpen() { return true; }

		void setParameters(bool hbs) { mHasBinSizes = hbs; }
		bool HasBinSizes() { return mHasBinSizes; }
		virtual void printParameters() {}
		virtual void printSet(int length, int *iset, int support, int ntrans) =0;
		virtual void finished() { mFinished = true; }
		virtual bool isFinished() { return mFinished; }
	protected:
		bool mHasBinSizes;
		bool mFinished;
	};

	class MemoryOut : public Output
	{
	public:
		MemoryOut(Database *db, AfoptCroupier *cr);
		virtual ~MemoryOut();

		void	InitBuffer(const uint32 size);
		void	ResizeBuffer(const uint32 size);
		void printSet(int length, int *iset, int support, int ntrans);
		virtual void finished();

		uint32	GetBufferSize()		{ return mBufferSize; }
		uint32	GetNumInBuffer()	{ return mNumInBuffer; }
		ItemSet **GetBuffer()		{ return mBuffer; }
		void	EmptyBuffer()		{ mNumInBuffer = 0; }

	protected:
		ItemSet	**mBuffer;
		double*	mStdLengths;
		uint32	mBufferSize;
		uint32	mNumInBuffer;
		ItemSetType mDataType;
		uint32	mAlphLen;
		uint16	*mSetBuffer;
		AfoptCroupier *mCroupier;
		uint32  mTotalNum;
		uint32  mScreenUpdateCounter;

	friend class AfoptCroupier;
	friend class Croupier;
	};
	class FSout : public Output
	{
	public:

		FSout(char *filename);
		virtual ~FSout();

		virtual int isOpen();
		virtual void printParameters();
		virtual void printSet(int length, int *iset, int support, int ntrans);
		virtual void finished();

		FILE *GetFile() { return mOut; }

	protected:
		FILE *mOut;
		long long mNumSetPos;
		long long mMinSupPos;
		long long mMaxSetLenPos;
	};
	class HistogramOut : public Output
	{
	public:
		HistogramOut(Database *db, AfoptCroupier *cr, const bool setLenMode = false); // in setLenMode, mine histogram of itemset lengths instead of their supports
		virtual ~HistogramOut();

		void printSet(int length, int *iset, int support, int ntrans);
		virtual uint64* GetHistogram();
		virtual void finished();

	protected:
		bool	mSetLenMode;
		uint64	*mHistogram;
		uint32	mHistoLen;
		AfoptCroupier *mCroupier;
	};

	extern Output *gpout;

	void inline OutputOnePattern(int nitem, int nsupport, int ntrans)
	{
		gntotal_patterns++;
		gppat_counters[gndepth+1]++;

		if(goparameters.bresult_name_given)
		{
			if(gnmax_pattern_len<gndepth+1)
				gnmax_pattern_len = gndepth+1;

			gpheader_itemset[gndepth] = nitem;
			gpout->printSet(gndepth+1, gpheader_itemset, nsupport, ntrans);
		}
	}

	void inline OutputOneClosePat(int nsupport, int ntrans)
	{
		gntotal_patterns++;
		gppat_counters[gndepth]++;
		if(gnmax_pattern_len<gndepth)
			gnmax_pattern_len = gndepth;

		if(goparameters.bresult_name_given)
			gpout->printSet(gndepth, gpheader_itemset, nsupport, ntrans);
	}

	void inline OutputOneClosePat(int length, int nsupport, int ntrans)
	{
		gntotal_patterns++;
		gppat_counters[gndepth+length]++;
		if(gnmax_pattern_len<gndepth+length)
			gnmax_pattern_len = gndepth+length;

		if(goparameters.bresult_name_given)
			gpout->printSet(gndepth+length, gpheader_itemset, nsupport, ntrans);
	}

	void inline OutputOneMaxPat(int length, int nsupport, int ntrans)
	{
		gntotal_patterns++;
		gppat_counters[gndepth+length]++;
		if(gnmax_pattern_len<gndepth+length)
			gnmax_pattern_len = gndepth+length;

		if(goparameters.bresult_name_given)
			gpout->printSet(gndepth+length, gpheader_itemset, nsupport, ntrans);
	}


	/*void inline OutputOneMaxPat(int *ppattern, int npat_len, int nsupport, int ntrans)
	{
		gppat_counters[npat_len]++;

		if(goparameters.bresult_name_given)
			gpout->printSet(npat_len, ppattern, nsupport, ntrans);
	}*/



}
#endif
