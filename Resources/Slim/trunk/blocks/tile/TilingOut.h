#ifndef _TILINGOUT_H
#define _TILINGOUT_H

#include <stdio.h>
#if defined (_WINDOWS)
#include <Windows.h>
#endif

#include <Primitives.h>

#include <Croupier.h>
#include "TilingMiner.h"

class Database;
class ItemSet;
enum ItemSetType;

namespace tlngmnr {
	class Output
	{
	public:
		Output(TilingMiner *cr) { mCroupier = cr; mHasBinSizes = false; mFinished = false; }
		virtual ~Output() {
			mCroupier = NULL;
		}

		void SetParameters(bool hbs) { mHasBinSizes = hbs; }

		bool HasBinSizes() { return mHasBinSizes; }

		virtual void SetFinished() { mFinished = true; }
		virtual bool IsFinished() { return mFinished; }

		virtual void OutputParameters() {}
		virtual void OutputSet(uint16 length, uint16 *iset, uint32 support, uint32 area, uint32 ntrans) =0;

	protected:
		bool mHasBinSizes;
		bool mFinished;
		TilingMiner *mCroupier;
	};

	class MemoryOut : public Output
	{
	public:
		MemoryOut(Database *db, TilingMiner *cr);
		virtual ~MemoryOut();

		void	InitBuffer(const uint32 size);
		void	ResizeBuffer(const uint32 size);

		virtual void OutputSet(uint16 length, uint16 *iset, uint32 support, uint32 area, uint32 ntrans);

		virtual void SetFinished();

		ItemSet **GetBuffer()		{ return mBuffer; }
		uint32	GetBufferSize()		{ return mBufferSize; }
		uint32	GetNumInBuffer()	{ return mNumInBuffer; }
		void	EmptyBuffer()		{ mNumInBuffer = 0; }


	protected:
		ItemSet**	mBuffer;
		double*		mStdLengths;
		uint32		mBufferSize;
		uint32		mNumInBuffer;
		ItemSetType	mDataType;
		uint32		mAlphLen;
		uint16*		mSetBuffer;
		uint32		mTotalNum;
		uint32		mScreenUpdateCounter;

		friend class Croupier;
	};

	class FSout : public Output
	{
	public:

		FSout(string filename, TilingMiner *cr);
		virtual ~FSout();

		virtual void OutputParameters();
		virtual void OutputSet(uint16 length, uint16 *iset, uint32 support, uint32 area, uint32 ntrans);

	protected:
		FILE *mFile;
		long long mNumSetPos;
		long long mMinAreaPos;
		long long mMaxSetLenPos;
	};

	/*
	class TileHistogramOut : public TileOutput
	{
	public:
		TileHistogramOut(Database *db, TileMiner *cr);
		virtual ~TileHistogramOut();

		void printSet(int length, int *iset, int support, int ntrans);
		virtual uint64* GetHistogram();
		virtual void finished();

	protected:
		uint64	*mHistogram;
		uint32	mHistoLen;
		TileMiner *mCroupier;
	};

	extern TileOutput *gpout;

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
*/
}
#endif
