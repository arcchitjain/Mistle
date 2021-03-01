#ifdef BLOCK_SLIM

#include "../global.h"
#include <time.h>
#include <omp.h>
#include <unordered_set>
#include <cassert>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "Slim.h"

Slim::Slim(CodeTable* ct, HashPolicyType hashPolicy, Config* config)
: SlimAlgo(ct, hashPolicy, config)
{
	mWriteLogFile = true;
}

CodeTable* Slim::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */)
{
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets();
	mNumCandidates = numM;
	ItemSet *m;
	mProgress = -1;

	{ // Extract minsup parameter from tag
		string dbName;
		string settings;
		string type;
		IscOrderType order;
		ItemSetCollection::ParseTag(mTag, dbName, type, settings, order);
		float minsupfloat;
		if(settings.find('.') != string::npos && (minsupfloat = (float)atof(settings.c_str())) <= 1.0f)
			mMinSup = (uint32) floor(minsupfloat * mDB->GetNumTransactions());
		else
			mMinSup = (uint32) atoi(settings.c_str());
	}

	if(mWriteReportFile == true)
		OpenReportFile(true);
	if(mWriteCTLogFile == true) {
		OpenCTLogFile();
		mCT->SetCTLogFile(mCTLogFile);
	}
	if (mWriteLogFile)
		OpenLogFile();

	mStartTime = time(NULL);
	mScreenReportTime = time(NULL);
	mScreenReportCandidateIdx = 0;
	mScreenReportCandPerSecond = 0;
	mScreenReportCandidateDelta = 5000;

	CoverStats stats = mCT->GetCurStats();
	stats.numCandidates = mNumCandidates;
	printf(" * Start:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);

	CoverStats &curStats = mCT->GetCurStats();

	// TODO: Find out why there is a difference OR implement for CCCPCodeTable also!
	// mCT->CoverDB(curStats); // BUG FIX: apparently there is a small difference in encCTsize, if we don't coverdb first!

	ItemSet* minCandidate;
	double minSize = 0;
	uint64 numIterations = 0;
	uint64 numCandidates;
	do {

		if(mWriteProgressToDisk) {
			if (numIterations == 0) {
				ProgressToDisk(mCT, 0 /* TODO: should read maximum support */, 0 /* TODO: curLength */, numIterations, true, true);
			} else if (mReportIteration) {
				ProgressToDisk(mCT, minCandidate->GetSupport(), minCandidate->GetLength(), numCandidates, mReportIterType == ReportCodeTable || mReportIterType == ReportAll, mReportIterType == ReportStats || mReportIterType == ReportAll);
			}
		}

		minSize = mCT->GetCurSize();
		minCandidate = NULL;
		numCandidates = 0;

/*
		{ // Debug section
			char buf[255];
			sprintf_s(buf, 255, "%sct-%s-%d-%lu.ct", mOutDir.c_str(),mTag.c_str(),0,numIterations);
			string ctFilename = buf;

			CodeTable *ct = CodeTable::LoadCodeTable(ctFilename, mDB, "coverfull");
			if(ct == NULL)
				throw string("Codetable not found.");

			CoverStats &debugStats = ct->GetCurStats();
			ct->CoverDB(debugStats);

			assert(debugStats.alphItemsUsed == curStats.alphItemsUsed);
			assert(debugStats.numSetsUsed == curStats.numSetsUsed);
			assert(debugStats.usgCountSum == curStats.usgCountSum);
			assert(debugStats.encDbSize == curStats.encDbSize);
			if (debugStats.encCTSize != curStats.encCTSize) {
				islist *ilist = ct->GetItemSetList();
				ilist->splice(ilist->end(), *ct->GetSingletonList());
				islist *jlist = mCT->GetItemSetList();
				jlist->splice(jlist->end(), *mCT->GetSingletonList());
				islist::iterator i = ilist->begin();
				islist::iterator j = jlist->begin();
				for (; i != ilist->end(); ++i,++j) {
					printf("%f == %f? %d\n", (*i)->GetStandardLength(), (*j)->GetStandardLength(), (*i)->GetStandardLength() == (*j)->GetStandardLength());
				}
			}
			assert(debugStats.encCTSize == curStats.encCTSize);
			assert(debugStats.encSize == curStats.encSize);

			delete ct;
		}
*/

		islist *ctlist = mCT->GetItemSetList();
		ctlist->splice(ctlist->end(), *mCT->GetSingletonList());

		islist::iterator cend_i=ctlist->end(); cend_i--;
		islist::iterator cend_j=ctlist->end();

		for (islist::iterator cit = ctlist->begin(); cit != cend_i; ++cit) {
			for (islist::iterator cjt = cit; cjt != cend_j; ++cjt) {
				if (cit != cjt) {
					m = (*cit)->Union(*cjt);

					if (m->GetLength() > mDB->GetMaxSetLength()) {
						delete m;
						continue;
					}

					++numCandidates;

					if(mNeedsOccs)
						mDB->DetermineOccurrences(m);
					mDB->CalcSupport(m);
					mDB->CalcStdLength(m); // BUGFIX: itemset has no valid standard encoded size

					if (m->GetSupport() < mMinSup) {
						delete m;
						continue;
					}

					if (mWriteLogFile) fprintf(mLogFile, "Candidate: %s\n", m->ToCodeTableString().c_str());

					mCT->Add(m);
					m->SetUsageCount(0);
					mCT->CoverDB(curStats);
					if(curStats.encDbSize < 0) {
						THROW("L(D|M) < 0. That's not good.");
					}

					if (mWriteLogFile) fprintf(mLogFile, "%.2f < %.2f ? %d\n", mCT->GetCurSize(), minSize, mCT->GetCurSize() < minSize);

					if (mPruneStrategy == PostAcceptPruneStrategy) { // Post-decide On-the-fly pruning
						if (mCT->GetCurSize() < minSize) {
							minSize = mCT->GetCurSize();

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

							if (minCandidate) delete minCandidate; // HACK: Delete previous minCandidate
							minCandidate = m->Clone();  // HACK: RollbackAdd() will not delete minCandidate!
						}
						mCT->RollbackAdd();
					}
				}
			}
		}

		if(minCandidate) {
			mCT->Add(minCandidate);
			minCandidate->SetUsageCount(0);
			mCT->CoverDB(curStats);
			if(curStats.encDbSize < 0) {
				THROW("L(D|M) < 0. That's not good.");
			}
			if (mPruneStrategy == PostAcceptPruneStrategy) { // Post-decide On-the-fly pruning
				mCT->CommitAdd(mWriteCTLogFile);
				PrunePostAccept(mCT);
			} else { // No On-the-fly pruning
				mCT->CommitAdd(mWriteCTLogFile);
			}

			++numIterations;
			if (mWriteLogFile) fprintf(mLogFile, "Accepted: %s [%.2f]\n\n", minCandidate->ToCodeTableString().c_str(), curStats.encSize);
		}

	} while (minCandidate);

	double timeCompression = omp_get_wtime() - mCompressionStartTime;
	printf(" * Time:    \t\tCompressing the database took %f seconds.\t\t\n", timeCompression);

	stats = mCT->GetCurStats();
	printf(" * Result:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
	if(mWriteProgressToDisk == true) {
		ProgressToDisk(mCT, mISC->GetMinSupport(), 0 /* TODO: curLength */, numCandidates, true, true);
	}
	CloseCTLogFile();
	CloseReportFile();
	CloseLogFile();

	mCT->EndOfKrimp();

	return mCT;
}

#endif // BLOCK_SLIM
