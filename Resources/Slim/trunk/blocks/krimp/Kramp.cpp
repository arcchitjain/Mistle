#ifdef BLOCK_SLIM

#include "../global.h"
#include <time.h>
#include <omp.h>
#include <unordered_set>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "Kramp.h"

Kramp::Kramp(CodeTable* ct, HashPolicyType hashPolicy, Config* config)
: SlimAlgo(ct, hashPolicy, config)
{

}

CodeTable* Kramp::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */)
{
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets();
	mNumCandidates = numM;
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


	ItemSet* minCandidate;
	uint64 numIterations = 0;
	// TODO: check whether we really need hash set!
	std::unordered_set<uint64> hashedCandidates;
	do {

		if(mWriteProgressToDisk) {
			if (numIterations == 0) {
				ProgressToDisk(mCT, 0 /* TODO: should read maximum support */, 0 /* TODO: curLength */, numIterations, true, true);
			} else if (mReportIteration) {
				ProgressToDisk(mCT, minCandidate->GetSupport(), minCandidate->GetLength(), numIterations, mReportIterType == ReportCodeTable || mReportIterType == ReportAll, mReportIterType == ReportStats || mReportIterType == ReportAll);
			}
		}

		double minSize = mCT->GetCurSize();
		uint64 minM = 0;
		minCandidate = NULL;

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

			if (mPruneStrategy == PostAcceptPruneStrategy) { // Post-decide On-the-fly pruning
				if (mCT->GetCurSize() < minSize) {
					minSize = mCT->GetCurSize();
					minM = curM;

					if (minCandidate) delete minCandidate; // HACK: Delete previous minCandidate
					minCandidate = m->Clone();  // HACK: RollbackAdd() will not delete minCandidate!
					/* BUG: ItemSet::~ItemSet() throws "Deleting itemset with multiple references" with coverfull and coverpartial-orderly
					if (minCandidate) minCandidate->UnRef(); // HACK: Delete previous minCandidate
					minCandidate = m;
					minCandidate->Ref(); // HACK: RollbackAdd() will not delete minCandidate!
					 */
				}
				mCT->RollbackAdd();
			} else { // No On-the-fly pruning
				if (mCT->GetCurSize() < minSize) {
					minSize = mCT->GetCurSize();
					minM = curM;

					if (minCandidate) delete minCandidate; // HACK: Delete previous minCandidate
					minCandidate = m->Clone();  // HACK: RollbackAdd() will not delete minCandidate!
				}
				mCT->RollbackAdd();
			}
		}

		if(minCandidate) {
			mCT->Add(minCandidate, minM);
			minCandidate->SetUsageCount(0);
			mCT->CoverDB(curStats);
			if(curStats.encDbSize < 0) {
				THROW("dbSize < 0. Dat is niet goed.");
			}
			if (mPruneStrategy == PostAcceptPruneStrategy) { // Post-decide On-the-fly pruning
				mCT->CommitAdd(mWriteCTLogFile);
				PrunePostAccept(mCT);
			} else { // No On-the-fly pruning
				mCT->CommitAdd(mWriteCTLogFile);
			}

			if (mHashPolicy != hashNoCandidates)
				hashedCandidates.insert(minCandidate->GetID());
			++numIterations;
		}

		mISC->Rewind();
	} while(minCandidate);

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
