#ifdef BROKEN

#include "../../global.h"

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>
#include <omp.h>

#include "ParallelCFCodeTable.h"
#include "../CTISList.h"

ParallelCFCodeTable::ParallelCFCodeTable() : CFCodeTable() {
	mNumThreads = omp_get_max_threads();
}

ParallelCFCodeTable::~ParallelCFCodeTable() {
	for (uint32 i=0; i<mNumThreads; i++) {
		delete mCoverSets[i];
	}
}

void ParallelCFCodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) {
	CFCodeTable::UseThisStuff(db, type, ctinit, maxCTElemLength, toMinSup);

	mLocalCounts.resize(mNumThreads, vector<uint32>(1));
	mLocalAlphabet.resize(mNumThreads, vector<uint32>(mABLen, 0));
	mCoverSets.resize(mNumThreads);
	mCoverSets[0] = mCoverSet;
	for (uint32 i=1; i<mNumThreads; i++) {
		mCoverSets[i] = mCoverSet->Clone();
	}
	mCoverSet = NULL;
}

void ParallelCFCodeTable::CommitAdd(bool updateLog) {
	CFCodeTable::CommitAdd(updateLog);
	for(uint32 i=0; i<mNumThreads; i++) {
		mLocalCounts[i].resize(mCurNumSets + 1);
	}
}

void ParallelCFCodeTable::CoverDB(CoverStats &stats) {
	memcpy(mOldAlphabet, mAlphabet, mABLen * sizeof(uint32));

	uint64 countSum = mABUnusedCount;
	if(mSTSpecialCode)
		++countSum;

#ifdef PROFILE
	double time1 = omp_get_wtime(), time2, time3, time4, time5;
#endif

    #pragma omp parallel
	{
#ifdef PROFILE
		#pragma omp master
		{
			time2 = omp_get_wtime();
			mTimeParallelOverhead += time2 - time1;
		}
#endif

		int tid = omp_get_thread_num();
		CoverSet* coverSet = mCoverSets[tid];
		std::vector<uint32> &counts = mLocalCounts[tid];
		std::vector<uint32> &alphabet = mLocalAlphabet[tid];

		for (uint32 i=0; i<mCurNumSets; i++) {
			counts[i] = 0;
		}
		for (uint32 i=0; i<mABUsedLen; i++) {
			alphabet[mABUsed[i]] = 0;
		}

		// full cover db
		#pragma omp for reduction(+:countSum) nowait
		for(int32 i=0; (uint32) i<mNumDBRows; i++) {
			coverSet->InitSet(mDB->GetRow(i));
			uint32 binSize = mDB->GetRow(i)->GetCount();
			uint32 idx = 0;
			for(uint32 j=mMaxIdx; j>=1; j--) {
				if(mCT[j] != NULL) {
					islist::iterator cend = mCT[j]->GetList()->end();
					for(islist::iterator cit = mCT[j]->GetList()->begin(); cit != cend; ++cit, ++idx) {
						if(coverSet->Cover(j+1,*cit)) {
							counts[idx] += binSize;
							countSum += binSize;
						}
					}					
				}
			}
			// alphabet items
			countSum += coverSet->CoverAlphabet(mABLen, &alphabet[0], binSize); 
		}

#ifdef PROFILE
		#pragma omp master
		{
			time3 = omp_get_wtime();
			mTimeCoverDB += time3 - time2;
		}
	}
	time4 = omp_get_wtime();
	mTimeSyncOverhead += time4 - time3;
#else
	}
#endif

	// calculate global counts
	uint32 idx = 0;
	for(uint32 i=mMaxIdx; i>=1; i--) {
		if(mCT[i] != NULL) {
			islist::iterator cend = mCT[i]->GetList()->end();
			for(islist::iterator cit = mCT[i]->GetList()->begin(); cit != cend; ++cit, ++idx) {
				(*cit)->ResetCount();
				for(uint32 j=0; j<mNumThreads; j++) {
					(*cit)->AddCount(mLocalCounts[j][idx]);
				}
			}
		}
	}

	for(uint32 i=0; i<mABUsedLen; i++) {
		mAlphabet[mABUsed[i]] = 0;
		for(uint32 j=0; j<mNumThreads; j++) {
			mAlphabet[mABUsed[i]] += mLocalAlphabet[j][mABUsed[i]];
		}
	}

#ifdef PROFILE
	time5 = omp_get_wtime();
	mTimeSumLocalCounts += time5 - time4;
#endif

	stats.countSum = countSum;
	stats.numSetsUsed = 0;
	stats.alphItemsUsed = 0;
	stats.dbSize = 0;
	stats.ctSize = 0;
	CalcTotalSize(stats);

#ifdef PROFILE
	mTimeCalcTotalSize += omp_get_wtime() - time5;
#endif
}

#endif // BROKEN
