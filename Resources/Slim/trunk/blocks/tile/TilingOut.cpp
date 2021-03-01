#ifdef BLOCK_TILING
#include <db/Database.h>

#include "TilingMiner.h"

#include "TilingOut.h"

using namespace tlngmnr;

MemoryOut::MemoryOut(Database *db, TilingMiner *cr) : Output(cr) {
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
void MemoryOut::OutputSet(uint16 length, uint16 *iset, uint32 support, uint32 area, uint32 ntrans) {
	double stdLen = 0.0;
	for(int i=0; i<length; i++) {
		mSetBuffer[i] = iset[i];
		stdLen += mStdLengths[iset[i]];
	}

	//BM128ItemSet::BM128ItemSet(const uint16 *set, uint32 setlen, uint32 cnt,uint32 freq, uint32 numOcc, float stdLen, uint32 *occurrences) {
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
void MemoryOut::SetFinished() {
	mCroupier->MinerIsErMooiKlaarMeeCBFunc(mBuffer, mNumInBuffer);
	mFinished = true;
}

FSout::FSout(string filename, TilingMiner *cr) : Output(cr) {
	if(fopen_s(&mFile,filename.c_str(),"wt") != 0)
		THROW("Dude, where should I leave my patterns?");
	fprintf(mFile, "ficfi-1.1\n");
	fprintf(mFile, "dbFilename: %s\n", cr->GetDatabaseName().c_str());
	fprintf(mFile, "patternType: %s\n", cr->GetPatternTypeStr().c_str());
	mNumSetPos = ftell(mFile);
	fprintf(mFile, "unfinished                    \n");		// `numFound: %d`
	mMaxSetLenPos = ftell(mFile);
	fprintf(mFile, "unfinished                        \n");	// `maxSetLength: %d`
	mMinAreaPos = ftell(mFile);
	fprintf(mFile, "unfinished                      \n");	// `minArea: %d`
}

FSout::~FSout() {
	if(mFile != NULL) {
		fseek(mFile, (long)mNumSetPos, SEEK_SET);
		fprintf(mFile, "numFound: %" I64d "", mCroupier->GetNumTilesMined());
		fseek(mFile, (long)mMaxSetLenPos, SEEK_SET);
		fprintf(mFile, "maxSetLength: %d", mCroupier->GetMaxSetLength());
		fseek(mFile, (long)mMinAreaPos, SEEK_SET);
		fprintf(mFile, "minArea: %d", mCroupier->GetMinArea());
		fclose(mFile);
	}
}

void FSout::OutputParameters() {
	fprintf(mFile, "hasSepNumRows: %d\n", mHasBinSizes?1:0);
}

void FSout::OutputSet(uint16 length, uint16 *iset, uint32 support, uint32 area, uint32 ntrans) {
	fprintf(mFile, "%d: ", length);
	for(int i=0; i<length; i++)
		fprintf(mFile, "%d ", iset[i]);
	if(mHasBinSizes) {
		fprintf(mFile, "(%d,%d)\n", area, ntrans);
	} else
		fprintf(mFile, "(%d)\n", area);
}
/*
// HistogramOut
HistogramOut::HistogramOut(Database *db, AfoptCroupier *cr) : Output() {
	uint32 numT = db->GetNumTransactions();
	mHistoLen = numT + 1;
	mHistogram = new uint64[mHistoLen];
	memset(mHistogram, 0, sizeof(uint64) * mHistoLen);
	mCroupier = cr;
}
HistogramOut::~HistogramOut() {
	delete[] mHistogram;
}
void HistogramOut::printSet(int length, int *iset, int support, int ntrans)
{
	mHistogram[support]++;
}
uint64* HistogramOut::GetHistogram() {
	return mHistogram;
}
void HistogramOut::finished() {
	mCroupier->HistogramMiningIsHistoryCBFunc(mHistogram, mHistoLen);
	mFinished = true;
}
*/
#endif // BLOCK_TILING
