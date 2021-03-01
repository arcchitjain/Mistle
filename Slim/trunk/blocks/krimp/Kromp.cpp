#ifdef BLOCK_SLIM

#include "../global.h"
#include <time.h>
#include <omp.h>
#include <unordered_set>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "Kromp.h"

Kromp::Kromp(CodeTable* ct, HashPolicyType hashPolicy, Config* config)
: SlimAlgo(ct, hashPolicy, config)
{

}

CodeTable* Kromp::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */)
{
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets();
	mNumCandidates = numM;
	bool candidateAccepted = false;
	ItemSet *m;
	mProgress = -1;

	if(mWriteReportFile == true)
		OpenReportFile(true);
	if(mWriteCTLogFile == true) {
		OpenCTLogFile();
		mCT->SetCTLogFile(mCTLogFile);
	}

	mStartTime = time(NULL);
	mScreenReportTime = time(NULL);
	mScreenReportCandidateIdx = 0;
	mScreenReportCandPerSecond = 0;
	mScreenReportCandidateDelta = 5000;

	CoverStats stats = mCT->GetCurStats();
	stats.numCandidates = mNumCandidates;
	printf(" * Start:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);

	CoverStats &curStats = mCT->GetCurStats();

	double prevIterSize;
	uint64 numIterations = 0;
	uint64 numCandidatesAccepted = 0;
	// TODO: check whether we really need hash set!
	std::unordered_set<uint64> hashedCandidates;
	do {
		prevIterSize = mCT->GetCurSize();

		if(mWriteProgressToDisk) {
			if (numIterations == 0) {
				ProgressToDisk(mCT, 0 /* TODO: should read maximum support */, 0 /* TODO: curLength */, numIterations, true, true);
			}
			else if (mReportIteration && !mReportOnAccept) {
				ProgressToDisk(mCT, 0, 0 /* TODO: curLength */, numIterations, mReportIterType == ReportCodeTable || mReportIterType == ReportAll, mReportIterType == ReportStats || mReportIterType == ReportAll);
			}
		}

		for(uint64 curM=candidateOffset; curM<numM; curM++) {
//			ProgressToScreen(curM, numM); // TODO: Fix!

			m = mISC->GetNextItemSet();
			if(mNeedsOccs)
				mDB->DetermineOccurrences(m);
			m->SetID(curM);

			if(m->GetLength() <= 1)	{	// Skip singleton Itemsets: already in alphabet
				delete m;
				continue;
			}
			if(mHashPolicy != hashNoCandidates && hashedCandidates.find(m->GetID()) != hashedCandidates.end()) {
				delete m;
				continue;
			}

			mCT->Add(m, curM);
			m->SetUsageCount(0);
			mCT->CoverDB(curStats);
			if(curStats.encDbSize < 0) {
				THROW("dbSize < 0. Dat is niet goed.");
			}

			if(mPruneStrategy == PostAcceptPruneStrategy) {						// Post-decide On-the-fly pruning
				if(mCT->GetCurSize() < mCT->GetPrevSize()) {
					mCT->CommitAdd(mWriteCTLogFile);
					PrunePostAccept(mCT);
					candidateAccepted = true;
					if (mHashPolicy != hashNoCandidates)
						hashedCandidates.insert(m->GetID());
				} else {
					mCT->RollbackAdd();
				}
			} else {												// No On-the-fly pruning
				if(mCT->GetCurSize() < mCT->GetPrevSize()) {
					mCT->CommitAdd(mWriteCTLogFile);
					candidateAccepted = true;
					if (mHashPolicy != hashNoCandidates)
						hashedCandidates.insert(m->GetID());
				} else {
					mCT->RollbackAdd();
				}
			}
			if (candidateAccepted) {
				++numCandidatesAccepted;
				if (mReportOnAccept)
					ProgressToDisk(mCT, m->GetSupport(), m->GetLength(), numCandidatesAccepted, mReportAccType == ReportCodeTable || mReportAccType == ReportAll, mReportAccType == ReportStats || mReportAccType == ReportAll);
			}
			candidateAccepted = false;

		}
		if(mPruneStrategy == SanitizeOfflinePruneStrategy) {				// Sanitize post-pruning
			PruneSanitize(mCT);
		}

		mISC->Rewind();
		++numIterations;
	} while (mCT->GetCurSize() < prevIterSize);

	double timeCompression = omp_get_wtime() - mCompressionStartTime;
	printf(" * Time:    \t\tCompressing the database took %f seconds.\t\t\n", timeCompression);

	stats = mCT->GetCurStats();
	printf(" * Result:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
	if(mWriteProgressToDisk == true) {
		ProgressToDisk(mCT, mISC->GetMinSupport(), 0 /* TODO: curLength */, numM, true, true);
	}
	CloseCTLogFile();
	CloseReportFile();

	mCT->EndOfKrimp();

	return mCT;
}

#endif // BLOCK_COMPRESS_NG
