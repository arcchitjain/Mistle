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
