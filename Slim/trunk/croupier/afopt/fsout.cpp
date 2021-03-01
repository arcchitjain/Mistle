#include "../clobal.h"

#include <db/Database.h>
#include <itemstructs/ItemSet.h>

#include "fsout.h"

using namespace Afopt;


MemoryOut::MemoryOut(Database *db, AfoptCroupier *cr) : Output() {
	mBufferSize = 0;
	mBuffer = NULL;
	mStdLengths = db->GetStdLengths();
	mNumInBuffer = 0;
	mDataType = db->GetDataType();
	mAlphLen = (uint32) db->GetAlphabetSize();
	mSetBuffer = new uint16[mAlphLen];
	mCroupier = cr;
	mTotalNum = 0;
	mScreenUpdateCounter = 10000;
}
MemoryOut::~MemoryOut() {
	if(mBufferSize > 0)
		delete[] mBuffer;
	delete[] mSetBuffer;
}
void MemoryOut::InitBuffer(const uint32 size) {
	mBufferSize = size;
	mBuffer = new ItemSet *[mBufferSize];
}
void MemoryOut::ResizeBuffer(const uint32 size) {
	if(size > mBufferSize) {
		delete[] mBuffer;
		mBuffer = new ItemSet *[size];
	}
	mBufferSize = size;
}
void MemoryOut::printSet(int length, int *iset, int support, int ntrans) {
	double stdLen = 0.0;
	for(int i=0; i<length; i++) {
		mSetBuffer[i] = iset[i];
		stdLen += mStdLengths[iset[i]];
	}

	//BM128ItemSet::BM128ItemSet(const uint16 *set, uint32 setlen, uint32 cnt, uint32 freq, uint32 numOcc, float stdLen, uint32 *occurrences) {
	//ItemSet *ItemSet::Create(ItemSetType type, const uint16 *set, uint32 setlen, uint32 abetlen, uint32 cnt, uint32 freq, uint32 numOcc, float stdLen, uint32 *occurrences) {

	if(mHasBinSizes)
		mBuffer[mNumInBuffer++] = ItemSet::Create(mDataType, mSetBuffer, length, mAlphLen, 0, support, ntrans, stdLen);
	else 
		mBuffer[mNumInBuffer++] = ItemSet::Create(mDataType, mSetBuffer, length, mAlphLen, 0, support, support, stdLen);

	mTotalNum++;
	--mScreenUpdateCounter;
	if(mScreenUpdateCounter == 0) {
		int d = (mTotalNum/10000) % 4;
		char c = '*';
		switch(d) {
				case 0 : c = '-'; break;
				case 1 : c = '\\'; break;
				case 2 : c = '|'; break;
				default : c = '/';
		}
		ECHO(2,printf(" %c Mining:\t\tin progress... (%d0k)\r", c, mTotalNum/10000));
		mScreenUpdateCounter = 10000;
	}

	if(mNumInBuffer == mBufferSize) {
		mCroupier->MinerIsErVolVanCBFunc(mBuffer, mNumInBuffer);
		EmptyBuffer();
	}
}
void MemoryOut::finished() {
	mCroupier->MinerIsErMooiKlaarMeeCBFunc(mBuffer, mNumInBuffer);
	mFinished = true;
}


FSout::FSout(char *filename) : Output() {
	if(fopen_s(&mOut,filename,"wt") != 0)
		THROW("Dude, where should I leave my patterns?");
	fprintf(mOut, "ficfi-1.1\n");
	fprintf(mOut, "dbFilename: %s\n", goparameters.data_filename);
	fprintf(mOut, "patternType: %s\n", goparameters.pattern_type);
	mNumSetPos = ftell(mOut);
	fprintf(mOut, "unfinished                    \n");		// `numFound: %d`
	mMaxSetLenPos = ftell(mOut);
	fprintf(mOut, "unfinished                        \n");	// `maxSetLength: %d`
	mMinSupPos = ftell(mOut);
	fprintf(mOut, "unfinished                      \n");	// `minSupport: %d`
}

FSout::~FSout() {
	if(mOut != NULL) { 
		fseek(mOut, (long)mNumSetPos, SEEK_SET);
		fprintf(mOut, "numFound: %" I64d, gntotal_patterns);
		fseek(mOut, (long)mMaxSetLenPos, SEEK_SET);
		fprintf(mOut, "maxSetLength: %d", gnmax_pattern_len);
		fseek(mOut, (long)mMinSupPos, SEEK_SET);
		fprintf(mOut, "minSupport: %d", gnmin_sup);
		fclose(mOut);
	}
}

int FSout::isOpen() {
	if(mOut) return 1;
	else return 0;
}

void FSout::printParameters() {
	fprintf(mOut, "hasSepNumRows: %d\n", mHasBinSizes?1:0);
}

void FSout::printSet(int length, int *iset, int support, int ntrans) {
	fprintf(mOut, "%d: ", length);
	for(int i=0; i<length; i++) 
		fprintf(mOut, "%d ", iset[i]);
	if(mHasBinSizes) {
		fprintf(mOut, "(%d,%d)\n", support, ntrans);
	} else
		fprintf(mOut, "(%d)\n", support);
}
void FSout::finished() {
	if(mOut != NULL) { 
		fseek(mOut, (long)mNumSetPos, SEEK_SET);
		fprintf(mOut, "numFound: %" I64d, gntotal_patterns);
		fseek(mOut, (long)mMaxSetLenPos, SEEK_SET);
		fprintf(mOut, "maxSetLength: %d", gnmax_pattern_len);
		fseek(mOut, (long)mMinSupPos, SEEK_SET);
		fprintf(mOut, "minSupport: %d", gnmin_sup);
		fclose(mOut);
	}
	mOut = NULL;
	mFinished = true;
}

// HistogramOut
HistogramOut::HistogramOut(Database *db, AfoptCroupier *cr, const bool setLenMode /*= false*/) : Output() {
	uint32 num;
	if(setLenMode) {
		num = db->GetMaxSetLength();
	} else {
		num = db->GetNumTransactions();
	}
	mSetLenMode = setLenMode;
	mHistoLen = num + 1;
	mHistogram = new uint64[mHistoLen];
	memset(mHistogram, 0, sizeof(uint64) * mHistoLen);
	mCroupier = cr;
}
HistogramOut::~HistogramOut() {
	delete[] mHistogram;
}
void HistogramOut::printSet(int length, int *iset, int support, int ntrans) {
	mHistogram[mSetLenMode ? length : support]++;
}
uint64* HistogramOut::GetHistogram() {
	return mHistogram;
}
void HistogramOut::finished() {
	mCroupier->HistogramMiningIsHistoryCBFunc(mHistogram, mHistoLen);
	mFinished = true;
}
