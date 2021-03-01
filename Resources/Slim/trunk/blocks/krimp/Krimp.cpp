#ifdef BLOCK_SLIM

/*
	Work in progress to refactor regular Krimp from Algo into this file, and make Algo the base Algorithm construct for all Krimp and Slim variants --JV
*/

#include "../global.h"
#include <time.h>
#include <omp.h>
#include <unordered_set>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "Krimp.h"

Krimp::Krimp(CodeTable* ct, HashPolicyType hashPolicy, Config* config)
: SlimAlgo(ct, hashPolicy, config)
{

}

CodeTable* Krimp::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */) {
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets(), nextReportCnd;
	mNumCandidates = numM;
	int32 curSup=-1, nextReportSup, prevSup=-1;
	uint32 accSup=0;
	uint32 accLength=0;
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

	nextReportSup = (mReportSupDelta == 0) ? 0 : startSup - mReportSupDelta;
	nextReportCnd = (mReportCndDelta == 0) ? numM : mReportCndDelta;

	for(uint64 curM=candidateOffset; curM<numM; curM++) {
		KrimpAlgo::ProgressToScreen(curM, numM);

		m = mISC->GetNextItemSet();
		if(mNeedsOccs)
			mDB->DetermineOccurrences(m);
		m->SetID(curM);
		prevSup = curSup;
		curSup = m->GetSupport();

		if(mWriteProgressToDisk == true) {
			if(curM == 0) {
				ProgressToDisk(mCT, curSup, 0 /* TODO: curLength */, curM, true, true);
				// !!! nextReportSup moet niet 0-based, maar minSup-based zijn (dus (curSup - minSup) % mRepSupD == 0, ofzo
				nextReportSup = (mReportSupDelta == 0) ? 0 : curSup - (((curSup-mISC->GetMinSupport()) % mReportSupDelta) == 0 ? mReportSupDelta : (curSup-mISC->GetMinSupport()) % mReportSupDelta);
			} else if(mReportOnAccept == true && candidateAccepted == true) {
				// we moeten alle accepts rapporteren
				ProgressToDisk(mCT, accSup, accLength, curM, mReportAccType == ReportCodeTable || mReportAccType == ReportAll, mReportAccType == ReportStats || mReportAccType == ReportAll);
			} else if(curSup < nextReportSup) {		// curSup < nRS, dus we moeten volledig reporten!
				ProgressToDisk(mCT, nextReportSup, 0 /* TODO: curLength */, curM, mReportSupType == ReportCodeTable || mReportSupType == ReportAll, mReportSupType == ReportStats || mReportSupType == ReportAll);
				nextReportSup -= mReportSupDelta;
				if(mReportCndDelta != 0)
					nextReportCnd = curM + mReportCndDelta;
				if(nextReportSup < 1)
					nextReportSup = 1;
				while(nextReportSup > curSup) {
					ProgressToDisk(mCT, nextReportSup, (uint32) curM, 0 /* TODO: curLength */, mReportSupType == ReportCodeTable || mReportSupType == ReportAll, mReportSupType == ReportStats || mReportSupType == ReportAll);
					nextReportSup -= mReportSupDelta;
				}
			} else if(curM == nextReportCnd) {
				ProgressToDisk(mCT, curSup, (uint32) curM, 0 /* TODO: curLength */, mReportCndType == ReportCodeTable || mReportCndType == ReportAll, mReportCndType == ReportStats || mReportCndType == ReportAll);
				nextReportCnd += mReportCndDelta;
			}
		}


		candidateAccepted = false; // voor de singleton-skip, ivm re-report on accept

		if(m->GetLength() <= 1)	{	// Skip singleton Itemsets: already in alphabet
			delete m;
			continue;
		}
		
		mCT->Add(m, curM);
		m->SetUsageCount(0);
		mCT->CoverDB(curStats);
		if(curStats.encDbSize < 0) {
			THROW("dbSize < 0. Dat is niet goed.");
		}

		if ((mPruneStrategy == PostAcceptPruneStrategy) || (mPruneStrategy == PostEstimationPruning)) {						// Post-decide On-the-fly pruning
			if(mCT->GetCurSize() < mCT->GetPrevSize()) {
				mCT->CommitAdd(mWriteCTLogFile);
				PrunePostAccept(mCT);
				candidateAccepted = true;
			} else {
				mCT->RollbackAdd();
			}
		} else if(mPruneStrategy == PreAcceptPruneStrategy) {				// Pre-decide On-the-fly pruning
			PrunePreAccept(mCT, m);

			if(mCT->GetCurSize() < mCT->GetPrevSize()) {
				mCT->CommitAdd(mWriteCTLogFile);
				mCT->CommitAllDels(mWriteCTLogFile);
				candidateAccepted = true;
			} else {
				mCT->RollbackAllDels();
				mCT->RollbackAdd();
			}

		} else {												// No On-the-fly pruning
			if(mCT->GetCurSize() < mCT->GetPrevSize()) {
				mCT->CommitAdd(mWriteCTLogFile);
				candidateAccepted = true;
			} else {
				mCT->RollbackAdd();
			}
		}

		if (candidateAccepted) {
			accSup = m->GetSupport();
			accLength = m->GetLength();
		}

	}
	if(mPruneStrategy == SanitizeOfflinePruneStrategy) {				// Sanitize post-pruning
		PruneSanitize(mCT);
	}

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
