#include "../../global.h"
#include <time.h>
#include <omp.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "codetable/CodeTable.h"

#include "CoverFullAlgo.h"
#include "ParallelCoverFull.h"

ParallelCoverFull::ParallelCoverFull(CodeTable *ct) : CoverFullAlgo(ct) {
	mNumRechecked = 0;
}

CodeTable* ParallelCoverFull::DoeJeDing(const uint64 candidateOffset, const uint32 startSup) {
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets(), nextReportCnd;
	int32 nextReportSup;
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
	printf(" * Start:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);

	nextReportSup = (mReportSupDelta == 0) ? 0 : startSup - mReportSupDelta;
	nextReportCnd = (mReportCndDelta == 0) ? numM : mReportCndDelta;

	uint64 curM = candidateOffset;

	uint32 numThreads = omp_get_max_threads();
	std::vector<uint64> curCandidate(numThreads);
	std::vector<uint32> abort(numThreads);
	std::deque<ItemSet*> candidatesBuffer;
	uint64 bufferOffset = curM;
	uint32 changed = 0;

    #pragma omp parallel
	{
		uint32 tid = omp_get_thread_num();
		ItemSet *m = NULL;
		CodeTable *ct = NULL;
		uint32 localChanged;

		#pragma omp critical
		if(curM<numM) {
			m = mISC->GetNextItemSet();
			candidatesBuffer.push_back(m->Clone());
			curCandidate[tid] = curM;
			abort[tid] = false;
			curM++;

			ct = mCT->Clone();
			localChanged = changed;
		}

		while(m != NULL) {
			int32 curSup = m->GetSupport();
			CoverStats &curStats = ct->GetCurStats();
			bool commit = false;

			#pragma omp master
			{
				ProgressToScreen(curCandidate[tid], numM);

				// Er kan een codetable worden geschreven die niet geldig is, omdat een andere
				// thread een kandidaat kan toevoegen die eerder komt dan de huidige kandidaat.
				if(mWriteProgressToDisk) {
					if(curCandidate[tid] == 0) {
						ProgressToDisk(ct, curSup, 0 /* TODO: curLength */, curCandidate[tid], true, true);
						// !!! nextReportSup moet niet 0-based, maar minSup-based zijn (dus (curSup - minSup) % mRepSupD == 0, ofzo
						nextReportSup = (mReportSupDelta == 0) ? 0 : curSup - (((curSup-mISC->GetMinSupport()) % mReportSupDelta) == 0 ? mReportSupDelta : (curSup-mISC->GetMinSupport()) % mReportSupDelta);
					} else if(curSup < nextReportSup) {		// curSup < nRS, dus we moeten volledig reporten!
						ProgressToDisk(ct, nextReportSup, 0 /* TODO: curLength */, curCandidate[tid], true, true);
						nextReportSup -= mReportSupDelta;
						if(mReportCndDelta != 0)
							nextReportCnd = curCandidate[tid] + mReportCndDelta;
						if(nextReportSup < 1)
							nextReportSup = 1;
						while(nextReportSup > curSup) {
							ProgressToDisk(ct, nextReportSup, 0 /* TODO: curLength */, curCandidate[tid], false, true);
							nextReportSup -= mReportSupDelta;
						}
					} else if(curCandidate[tid] >= nextReportCnd) {
						ProgressToDisk(ct, nextReportSup+mReportSupDelta, 0 /* TODO: curLength */, curCandidate[tid], true, false);
						nextReportCnd += mReportCndDelta;
					}
				}
			}


			if(m->GetLength() <= 1)	{	// Skip singleton Itemsets: already in alphabet
				delete m;
			} else {

				//printf("\nConsidering m : %s\n",m->ToString().c_str());
				ct->Add(m, curCandidate[tid]);
				m->SetUsageCount(0);
				ct->CoverDB(curStats);
				if(curStats.encDbSize < 0) {
					uint32 wow = 10;
					THROW("dbSize < 0. Dit is niet goed.");
				}

				if(mPruneStrategy == PreAcceptPruneStrategy) {						// Pre-decide On-the-fly pruning
					PrunePreAccept(ct, m);
		 
					if(ct->GetCurSize() < ct->GetPrevSize()) {
						ct->CommitAdd();
						ct->CommitAllDels();
						commit = true;
					} else {
						ct->RollbackAllDels();
						ct->RollbackAdd();
					}

				} else if(mPruneStrategy == PostAcceptPruneStrategy) {				// Post-decide On-the-fly pruning
					if(ct->GetCurSize() < ct->GetPrevSize()) {
						ct->CommitAdd();
						PrunePostAccept(ct);
						ct->BackupStats();
						commit = true;
					} else {
						ct->RollbackAdd();
						ct->RollbackCounts();
					}
				} else {												// No On-the-fly pruning
					if(ct->GetCurSize() < ct->GetPrevSize()) {
						ct->CommitAdd();
						commit = true;
					} else {
						ct->RollbackAdd();
					}
				}
			}
            #pragma omp critical
			{
				if(!abort[tid] && commit) {
					//printf("Thread %d: Accepted candidate %I64d\n", tid, curCandidate[tid]);
					delete mCT;
					mCT = ct->Clone();
					changed++;
					localChanged = changed;

					for(uint32 i=0; i<numThreads; i++) {
						if(curCandidate[i] > curCandidate[tid]) {
							abort[i] = true;
						}
					}
					curM = curCandidate[tid] + 1;
				}

				if(curCandidate[tid] == bufferOffset) {
					uint64 min = curM;
					for (uint32 j=0; j<numThreads; j++) {
						if (j != tid && curCandidate[j] < min) {
							min = curCandidate[j];
						}
					}
					while (bufferOffset < min) {
						delete candidatesBuffer.front();
						candidatesBuffer.pop_front();
						bufferOffset++;
					}
				}

				if(curM<numM) {
					if(curM < bufferOffset + candidatesBuffer.size()) {
						m = candidatesBuffer[(uint32) (curM - bufferOffset)]->Clone();
						mNumRechecked++;
					} else {
						m = mISC->GetNextItemSet();
						candidatesBuffer.push_back(m->Clone());
					}
					curCandidate[tid] = curM;
					abort[tid] = false;
					curM++;

					if(localChanged != changed) {
						delete ct;
						ct = mCT->Clone();
						localChanged = changed;
					}
				} else {
					m = NULL;
				}
			}
		}
		delete ct;
	}
	if(mPruneStrategy == SanitizeOfflinePruneStrategy) {				// Sanitize post-pruning
		PruneSanitize(mCT);
	}

	for(uint32 i=0; i<candidatesBuffer.size(); i++) {
		delete candidatesBuffer[i];
	}

	printf(" * Parallel:\t%d candidates had to be rechecked.\t\t\n", mNumRechecked);
	double timeCompression = omp_get_wtime() - mCompressionStartTime;
	printf(" * Time:\t\tCompressing the database took %f seconds.\n", timeCompression);
	stats = mCT->GetCurStats();
	printf(" * Result:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
	if(mWriteProgressToDisk) {
		//ProgressToDisk(mCT, curSup, numM, true, true); // J? : waarom dubbel? levert dubbele files en entries op, leluk.
		ProgressToDisk(mCT, mISC->GetMinSupport(), 0 /* TODO: curLength */, numM, true, true);
	}
	CloseCTLogFile();
	CloseReportFile();


	return mCT;
}

