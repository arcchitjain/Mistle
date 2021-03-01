#ifdef BLOCK_SLIM

#include "../global.h"
#include <time.h>
#include <omp.h>
#include <unordered_set>
#include <cassert>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include <SystemUtils.h>

#include <Templates.h>

#include <unordered_set>


#include "SlimPlus.h"

SlimPlus::SlimPlus(CodeTable* ct, HashPolicyType hashPolicy, Config* config)
: SlimAlgo(ct, hashPolicy, config), mCandidates(compareCandidates)
{
	mWriteLogFile = true;

	string scoreItemSetType = mConfig->Read<string>("scoreItemSet", "exact");
	if (scoreItemSetType == "exact")
		mScoreItemSetType = EXACT;
	else if (scoreItemSetType == "support_db")
		mScoreItemSetType = SUPPORT_DB;
	else if (scoreItemSetType == "support_cs")
		mScoreItemSetType = SUPPORT_CS;
}

SlimPlus::~SlimPlus() {
/* TODO: Fix memory leak
	for (CandidatesScores::iterator it = mCandidatesScores.begin(); it != mCandidatesScores.end(); ++it)
		delete it->first;
*/
	mCandidatesScores.clear();

	if (mSingletonList) {
		for (islist::iterator cit = mSingletonList->begin(); cit != mSingletonList->end(); ++cit)
			delete *cit;
		mSingletonList->clear();
		delete mSingletonList;
	}

	if (mItemSetList)
		mItemSetList->clear();
}

string SlimPlus::GetShortName() {
	stringstream ss;
	ss << mConfig->Read<string>("scoreItemSet", "exact") << "-";
	ss << SlimAlgo::GetShortName();
	return ss.str();
}


CodeTable* SlimPlus::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */)
{
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets();
	mNumCandidates = numM;
	//ItemSet *m;
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

	// Initialise candidates
	mSingletonList = mCT->GetSingletonList();

	// Initialise occurence lists
	for (islist::iterator cit = mSingletonList->begin(); cit != mSingletonList->end(); ++cit)
		(*cit)->DetermineDBOccurence(mDB, true);

	islist::iterator cend_i=mSingletonList->end(); cend_i--;
	islist::iterator cend_j=mSingletonList->end();

	pair<CandidatesScores::iterator,bool> ret;
	for (islist::iterator cit = mSingletonList->begin(); cit != cend_i; ++cit) {
		for (islist::iterator cjt = cit; cjt != cend_j; ++cjt) {
			if (cit != cjt) {
				ItemSet* is = (*cit)->Union(*cjt);

				if (!is->GetNumOccs()) {
					delete is;
					continue;
				}

				// Calculate support fast(er) using DBOccurs (which are automagically updated during union!)
				uint32* rids = is->GetOccs();
				uint32 supp = 0;
				for (size_t i = 0; i < is->GetNumOccs(); ++i)
					supp += mDB->GetRow(rids[i])->GetUsageCount();
				is->SetSupport(supp);

				if (is->GetSupport() < mMinSup) {
					delete is;
					continue;
				}

				mDB->CalcStdLength(is); // BUGFIX: itemset has no valid standard encoded size

				ret=mCandidatesScores.insert(pair<const ItemSet*, double>(is, 0.0));
				if (ret.second) {
					ret.first->second = ScoreItemSet(ret.first->first);
					mCandidates.push(ret.first->first);
//					if (mWriteLogFile) fprintf(mLogFile, "Candidate: %s \n", const_cast<ItemSet*>(ret.first->first)->ToCodeTableString().c_str());
				}
			}
		}
	}

	double minSize;
	ItemSet* minCandidate;
	uint64 numIterations = 0;
	do {
		if(mWriteProgressToDisk) {
			if (numIterations == 0) {
				ProgressToDisk(mCT, 0 /* TODO: should read maximum support */, 0 /* TODO: curLength */, numIterations, true, true);
			}
			else if (mReportIteration) {
				ProgressToDisk(mCT, minCandidate->GetSupport(), minCandidate->GetLength(), mCandidates.size(), mReportIterType == ReportCodeTable || mReportIterType == ReportAll, mReportIterType == ReportStats || mReportIterType == ReportAll);
			}
		}

		minSize = mCT->GetCurSize();
		minCandidate = NULL;
		while (!mCandidates.empty() && !minCandidate) {
			ItemSet* candidate = const_cast<ItemSet*>(mCandidates.top());
			mCandidates.pop();

			if (mWriteLogFile) fprintf(mLogFile, "Candidate: %s\n\n", candidate->ToCodeTableString().c_str());

			if (mScoreItemSetType == EXACT) {
				CandidatesScores::iterator res = mCandidatesScores.find(candidate);
				if (res->second >= minSize) {
					if (mWriteLogFile) fprintf(mLogFile, "Rejected: %s [%.2f, %d]\n\n", candidate->ToCodeTableString().c_str(), curStats.encSize, mCandidates.size());
					break; // We KNOW there isn't a better candidate!
				}
			}

			mCT->Add(candidate);
			candidate->SetUsageCount(0);
			mCT->CoverDB(curStats);
			if(curStats.encDbSize < 0) {
				THROW("dbSize < 0. Dat is niet goed.");
			}

			if (mCT->GetCurSize() < minSize) {
				if (mPruneStrategy == PostAcceptPruneStrategy) { // Post-decide On-the-fly pruning
					mCT->CommitAdd(mWriteCTLogFile);
					PrunePostAccept(mCT);
				} else { // No On-the-fly pruning
					mCT->CommitAdd(mWriteCTLogFile);
				}

				minCandidate = candidate;
			}
			else {
				if (mWriteLogFile) fprintf(mLogFile, "Rejected: %s [%.2f, %d]\n\n", candidate->ToCodeTableString().c_str(), curStats.encSize, mCandidates.size());
				// TODO: check whether we can truly ditch the candidate!
				mCandidatesScores.erase(candidate);

				// TODO: I don't think we need to keep this one...
				//ItemSet* m =  candidate->Clone();  // HACK: RollbackAdd() will not delete m!

				if (mPruneStrategy == PostAcceptPruneStrategy) { // Post-decide On-the-fly pruning
					mCT->RollbackAdd();
				} else { // No On-the-fly pruning
					mCT->RollbackAdd();
				}
			}
		}

		if (minCandidate) {
			mAcceptedCandidate = minCandidate;
			UpdateCandidates();

			++numIterations;
			if (mWriteLogFile) fprintf(mLogFile, "Accepted: %s [%.2f, %d]\n\n", minCandidate->ToCodeTableString().c_str(), curStats.encSize, mCandidates.size());
		}

	} while (minCandidate);

	double timeCompression = omp_get_wtime() - mCompressionStartTime;
	printf(" * Time:    \t\tCompressing the database took %f seconds.\t\t\n", timeCompression);

	stats = mCT->GetCurStats();
	printf(" * Result:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
	if(mWriteProgressToDisk == true) {
		ProgressToDisk(mCT, mISC->GetMinSupport(), 0 /* TODO: curLength */, mCandidates.size(), true, true);
	}
	CloseCTLogFile();
	CloseReportFile();
	CloseLogFile();

	mCT->EndOfKrimp();

	return mCT;
}

void SlimPlus::UpdateCandidates() {
	mCandidatesScores.erase(mAcceptedCandidate);

	unordered_set<const ItemSet*> ct_elements;
	mItemSetList = mCT->GetItemSetList();
	for (islist::iterator cit = mItemSetList->begin(); cit != mItemSetList->end(); ++cit) {
		ItemSet* is = (*cit)->Union(mAcceptedCandidate);

		if (!is->GetNumOccs()) {
			delete is;
			continue;
		}

		// Calculate support fast(er) using DBOccurs (which are automagically updated during union!)
		uint32* rids = is->GetOccs();
		uint32 supp = 0;
		for (size_t i = 0; i < is->GetNumOccs(); ++i)
			supp += mDB->GetRow(rids[i])->GetUsageCount();
		is->SetSupport(supp);

		if (is->GetSupport() < mMinSup) {
			delete is;
			continue;
		}

		mDB->CalcStdLength(is); // BUGFIX: itemset has no valid standard encoded size

		mCandidatesScores.insert(pair<ItemSet*, double>(is, 0.0));

		ct_elements.insert(*cit);
	}
	mItemSetList->clear();
	delete mItemSetList;
	mItemSetList = NULL;
	for (islist::iterator cit = mSingletonList->begin(); cit != mSingletonList->end(); ++cit) {
		ItemSet* is = (*cit)->Union(mAcceptedCandidate);

		if (!is->GetNumOccs()) {
			delete is;
			continue;
		}

		// Calculate support fast(er) using DBOccurs (which are automagically updated during union!)
		uint32* rids = is->GetOccs();
		uint32 supp = 0;
		for (size_t i = 0; i < is->GetNumOccs(); ++i)
			supp += mDB->GetRow(rids[i])->GetUsageCount();
		is->SetSupport(supp);

		if (is->GetSupport() < mMinSup) {
			delete is;
			continue;
		}

		mDB->CalcStdLength(is); // BUGFIX: itemset has no valid standard encoded size

		mCandidatesScores.insert(pair<ItemSet*, double>(is, 0.0));
	}

	while (!mCandidates.empty())
		mCandidates.pop();

//	for (unordered_set<const ItemSet*>::iterator it = ct_elements.begin(); it != ct_elements.end(); ++it) {
//		printf("CT elements: %s\n", const_cast<ItemSet*>((*it))->ToCodeTableString().c_str());
//	}

	// Prune all candidates that are
	// 1) present in current code table
	// ...
	queue<const ItemSet*> superfluous;
	for (CandidatesScores::iterator it = mCandidatesScores.begin(); it != mCandidatesScores.end(); ++it) {
//		printf("Superflous? %s\n", const_cast<ItemSet*>(it->first)->ToCodeTableString().c_str());
		if (ct_elements.find(it->first) != ct_elements.end()) {
			superfluous.push(it->first);
//			printf("Superflous: %s\n", const_cast<ItemSet*>(it->first)->ToCodeTableString().c_str());
		}
	}
	while (!superfluous.empty()) {
		const ItemSet* is = superfluous.front();
		mCandidatesScores.erase(is);
		superfluous.pop();
		delete is;
	}

	for (CandidatesScores::iterator it = mCandidatesScores.begin(); it != mCandidatesScores.end(); ++it) {
		it->second = ScoreItemSet(it->first);
		mCandidates.push(it->first);
//		if (mWriteLogFile) fprintf(mLogFile, "Candidate: %s \n", const_cast<ItemSet*>(it->first)->ToCodeTableString().c_str());
	}

	mAcceptedCandidate = NULL;
}

double SlimPlus::ScoreItemSet(const ItemSet* is)
{
	switch (mScoreItemSetType) {
	case EXACT: {
		double size = 0;
		ItemSet* m =  is->Clone();  // HACK: RollbackAdd() will not delete m!
		CoverStats& curStats = mCT->GetCurStats();

		mCT->Add(m);
		m->SetUsageCount(0);
		mCT->CoverDB(curStats);
		if(curStats.encDbSize < 0) {
			THROW("dbSize < 0. Dat is niet goed.");
		}

		if (mPruneStrategy == PostAcceptPruneStrategy) { // Post-decide On-the-fly pruning
			size = mCT->GetCurSize();
			mCT->RollbackAdd();
		} else { // No On-the-fly pruning
			size = mCT->GetCurSize();
			mCT->RollbackAdd();
		}

		return size;
	}
	case SUPPORT_DB: {
		return mDB->GetNumTransactions() - is->GetSupport(); // As we want to consider the one with highest support first in priority queue!
	}
	case SUPPORT_CS: {
		return 0.0;
	}
	default:
		return 0.0;
	}
}

SlimPlus::CandidatesScores SlimPlus::mCandidatesScores;
bool SlimPlus::compareCandidates(const ItemSet* a, const ItemSet* b)
{
	return mCandidatesScores[a] > mCandidatesScores[b];
}


#endif // BLOCK_SLIM
