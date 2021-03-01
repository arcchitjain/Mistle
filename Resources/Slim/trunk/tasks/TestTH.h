#ifndef __TESTTASKHANDLER_H
#define __TESTTASKHANDLER_H

#include "TaskHandler.h"

class TestTH : public TaskHandler {
public:
	TestTH (Config *conf);
	virtual ~TestTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	void	TestCandidateSetRobustness();
	void	TestShizzle();
	void	ArnoKPatterns();
	void	Dealer();

	void	SetupEclat();
	void	CleanupEclat();
	void	AdjustEclatParameters();

	void	TestMineFrequentItemSets();
	void	MineFIS();
	void	MineFISDFS(uint16 *posItems, uint32 numPosItems, uint32** posItemTids, uint32* posItemNumTids, uint32 prefixSup, uint16 *prefix, uint32 prefixLen, uint16 *comb, uint32 combLen);

	void	OutputSet(uint16 *is, uint32 isLen, uint32 supp);
	void	OutputSet(uint16 *is, uint32 isLen, uint32 supp, uint16 *comb, uint32 combLen);

	void	TestMineMostFrequentItemsets();
	void	MineMFIS();
	void	MineMFISDFS(uint16 *posItems, uint32 numPosItems, uint32** posItemTids, uint32* posItemNumTids, uint32 prefixSup, uint16 *prefix, uint32 prefixLen, uint16 *comb, uint32 combLen);
	void	BufferSet(uint16 *is, uint32 isLen, uint32 supp, uint16 *comb, uint32 combLen);
	void	OutputBufferedSets();

	void	TestKrimpOnTheFly();

#ifdef BLOCK_TILING
	void	TestMineTilesToMemory();
#endif

	void	TestArraySort();

	Database *mDB;
	ItemSet	**mRows;
	uint32	mNumRows;
	uint32	mABSize;

	uint16	*mItems;
	uint32	mNumItems;
	uint32	**mItemTids;
	uint32	*mItemNumTids;

	uint32	mMinSup;
	uint32	mMaxSup;
	uint32	mMinLen;
	uint32	mMaxLen;

	FILE	*mFile;

	// mISBuffer
	uint16	**mISBuffer;
	uint32	*mISBufferLen;
	uint32	mISBufferMax;
	uint32	mISBufferNum;
	uint32	mISBufferSup;

};

#endif // __TESTTASKHANDLER_H
