#ifdef BLOCK_SLIM

#include "../global.h"
#include <time.h>
#include <omp.h>
#include <unordered_set>
#include <algorithm>
#include <functional>
using namespace std;

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include <SystemUtils.h>
#include <ArrayUtils.h>
#include <Templates.h>


#include "../krimp/codetable/CodeTable.h"
#include "../krimp/codetable/CTISList.h"
#include "../krimp/codetable/coverpartial/CPAOCodeTable.h"
#include "codetable/CCPUCodeTable.h"
#include "codetable/CPUOCodeTable.h"

#include "SlimCS.h"

// TODO: See wether it is better that we swap < and > meaning ;-)
bool operator< (const Candidate &x, const Candidate &y) {
	if (x.heuristic < y.heuristic) return true;
	else if (x.heuristic > y.heuristic) return false;
	else if (x.i > y.i) return true;
	else if (x.i < y.i) return false;
	else if (x.j > y.j) return true;
	else return false;
};
bool operator> (const Candidate &x, const Candidate &y) {
	if (x.heuristic > y.heuristic) return true;
	else if (x.heuristic < y.heuristic) return false;
	else if (x.i < y.i) return true;
	else if (x.i > y.i) return false;
	else if (x.j < y.j) return true;
	else return false;
};
bool operator== (const Candidate &x, const Candidate &y) {
	return (x.i == y.i && x.j == y.j && x.heuristic == y.heuristic);
};

SlimCS::SlimCS(CodeTable* ct, HashPolicyType hashPolicy, Config* config) : SlimAlgo(ct, hashPolicy, config) {
	mWriteLogFile = true;
	mUsageCounts = NULL;
	mOrigIdx = NULL;
	mAlphabetIdx = NULL;
	mItemSetIdx = NULL;
	mStdLengths = NULL;
	mScreenReportBits = 0;

	mFriendlyCPUO = NULL;
	mFriendlyCPAO = NULL;
	mFriendlyCCPU = NULL;
	if(ct->GetConfigName().compare("coverpartial-usg-orderly") == 0)
		mFriendlyCPUO = (CPUOCodeTable*) ct;
	else if(ct->GetConfigName().compare("coverpartial-alt-orderly") == 0)
		mFriendlyCPAO = (CPAOCodeTable*) ct;
	else
		mFriendlyCCPU = (CCPUCodeTable*) ct;


	string estStrategy = config->Read<string>("estStrategy", "gain");

	if (estStrategy.compare("usg") == 0 || estStrategy.compare("usage") == 0 ) {
		mEstStrategy = SLIM_USAGE;
	} else if (estStrategy.compare("gain_basic") == 0) {
		mEstStrategy = SLIM_GAIN;
	} else if (estStrategy.compare("gain_db") == 0) {
		mEstStrategy = SLIM_NORMALIZED_GAIN;
	} else if (estStrategy.compare("gain") == 0) {
		mEstStrategy = SLIM_NORMALIZED_GAIN_CT;
	}

	mNumTopCandidates = config->Read<uint32>("numTopCandidates", 1000);
	mRejectedCandidates = NULL;

	mMaxTime = config->Read<uint32>("maxTime", 0) * 3600.0; // in hours
}

SlimCS::~SlimCS() {
	if (mUsageCounts) delete[] mUsageCounts;
	if (mOrigIdx) delete[] mOrigIdx;
	if (mAlphabetIdx && mItemSetIdx) {
		for (uint32 i = 0; i < mCurNumCTElems; ++i)
			if (mItemSetIdx[i] && mAlphabetIdx[i] != mCT->GetAlphabetSize())
				delete mItemSetIdx[i]; // TODO: we might better just update the usage of the alphabet
		delete[] mAlphabetIdx;
		delete[] mItemSetIdx;
	}
	if (mStdLengths) delete[] mStdLengths;
	if (mRejectedCandidates) {
		for (uint32 i = 0; i < mCurNumCTElems; ++i)
			mRejectedCandidates[i].clear(); // Or, is this dne automatically, when calling delete[]?
		delete[] mRejectedCandidates;
	}
}

string SlimCS::GetShortName() {
	stringstream ss;
	ss << SlimAlgo::GetShortName();
	return ss.str();
}


CodeTable* SlimCS::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */) {
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets();
	mNumCandidates = 0;
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


	double minSize;
	ItemSet* minCandidate = 0;
	uint64 numIterations = 0;
	uint32 numCandidates = 0;

	string strLatestCT = mOutDir + "ct-latest.ct";

	do {
		ProgressToScreen(numCandidates, mCT);
		if(mWriteProgressToDisk) {
			mCT->WriteToDisk(strLatestCT);
			ProgressToDisk(mCT, 0, 0, numCandidates, mReportIterType == ReportCodeTable || mReportIterType == ReportAll, mReportIterType == ReportStats || mReportIterType == ReportAll);
		}

		double elapsedTime = omp_get_wtime() - mCompressionStartTime;
		if (mMaxTime && (elapsedTime > mMaxTime))
			break;
		minSize = mCT->GetCurSize();
		minCandidate = NULL;

		UpdateLocalCTStuff();

		double heuristic = 0;
		ItemSet* candidate = NextCandidate(heuristic);
		while (!minCandidate && candidate) {
			++numCandidates;
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
				if (mWriteLogFile) fprintf(mLogFile, "Accepted: %s[%.2f, %.2f, %.2f, %d]\n", candidate->ToCodeTableString().c_str(), curStats.encSize, minSize, heuristic, numCandidates);

				minCandidate = candidate;

				UpdateLocalCTStuff();
			} else {
				list<uint32>::reverse_iterator rev_it; // Much more likely that we need to insert at the back of the list...
				for (rev_it = mRejectedCandidates[mCandidate_i].rbegin(); rev_it != mRejectedCandidates[mCandidate_i].rend() && *rev_it > mCandidate_j; ++rev_it) {}
				mRejectedCandidates[mCandidate_i].insert(rev_it.base(), mCandidate_j);

				if (mWriteLogFile) fprintf(mLogFile, "Rejected: %s[%.2f, %.2f, %.2f, %d]\n", candidate->ToCodeTableString().c_str(), curStats.encSize, minSize, heuristic, numCandidates);

				if (mPruneStrategy == PostAcceptPruneStrategy) { // Post-decide On-the-fly pruning
					mCT->RollbackAdd();
				} else { // No On-the-fly pruning
					mCT->RollbackAdd();
				}

				candidate = NextCandidate(heuristic);
			}
		}

	} while (minCandidate);
	//printf("size: %d", mRejectedCandidates.size());


	double timeCompression = omp_get_wtime() - mCompressionStartTime;
	printf(" * Time:    \t\tCompressing the database took %f seconds.\t\t\n", timeCompression);

	stats = mCT->GetCurStats();
	printf(" * Result:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
	if(mWriteProgressToDisk == true) {
		ProgressToDisk(mCT, 0, 0, numCandidates, true, true);
	}
	CloseCTLogFile();
	CloseReportFile();
	CloseLogFile();

	mCT->EndOfKrimp();

	return mCT;
}

void SlimCS::UpdateLocalCTStuff() {

	if (mUsageCounts) delete[] mUsageCounts;
	if (mOrigIdx) delete[] mOrigIdx;
	if (mAlphabetIdx && mItemSetIdx) {
		for (uint32 i = 0; i < mCurNumCTElems; ++i)
			if (mItemSetIdx[i] && mAlphabetIdx[i] != mCT->GetAlphabetSize())
				delete mItemSetIdx[i]; // TODO: we might better just update the usage of the alphabet
		delete[] mAlphabetIdx;
		delete[] mItemSetIdx;
	}
	if (mStdLengths) delete[] mStdLengths;
	// TODO: Reuse rejected candidates, but we need to do some translation because the codetable elements are shifted and/or no longer present
	if (mRejectedCandidates) {
		for (uint32 i = 0; i < mCurNumCTElems; ++i)
			mRejectedCandidates[i].clear(); // Or, is this dne automatically, when calling delete[]?
		delete[] mRejectedCandidates;
	}
	// TODO: Reuose top candidates?
	mTopCandidates.clear();
	mMoreCandidates = true;

	mCurNumCTElems = 0;
	uint32 la = (mCT->GetCurNumSets());
	uint32 bla = (mCT->GetAlphabetSize());
	mCurNumCTElems = la + bla;

	mUsageCounts = new uint32[mCurNumCTElems];
	mOrigIdx = new uint32[mCurNumCTElems];
	mAlphabetIdx = new uint32[mCurNumCTElems];
	mItemSetIdx = new ItemSet*[mCurNumCTElems];
	mRejectedCandidates = new list<uint32>[mCurNumCTElems];
	mStdLengths = new double[mCurNumCTElems];

	uint32 idx = 0;
	if(mFriendlyCPUO || mFriendlyCPAO) {
		islist *fCTList;
		if(mFriendlyCPUO)
			fCTList = mFriendlyCPUO->mCTList;
		else
			fCTList = mFriendlyCPAO->mCTList;

		//mCT->Get
		islist::iterator cit,cend=fCTList->end();
		for(cit = fCTList->begin(); cit != cend; ++cit, ++idx) {
			mUsageCounts[idx] = ((ItemSet*)(*cit))->GetUsageCount();
			mItemSetIdx[idx] = (ItemSet*)(*cit);
			mAlphabetIdx[idx] = mCT->GetAlphabetSize();
			mOrigIdx[idx] = idx;
			mStdLengths[idx] = ((ItemSet*)(*cit))->GetStandardLength();
		}
	} else {
		for(uint32 j=mFriendlyCCPU->mMaxIdx; j>=1; j--) {
			if(mFriendlyCCPU->mCT[j] != NULL) {
				islist::iterator cit, cend = mFriendlyCCPU->mCT[j]->GetList()->end();
				for(cit = mFriendlyCCPU->mCT[j]->GetList()->begin(); cit != cend; ++cit, ++idx) {
					mUsageCounts[idx] = ((ItemSet*)(*cit))->GetUsageCount();
					mItemSetIdx[idx] = (ItemSet*)(*cit);
					mAlphabetIdx[idx] = mCT->GetAlphabetSize();
					mOrigIdx[idx] = idx;
					mStdLengths[idx] = ((ItemSet*)(*cit))->GetStandardLength();
				}
			}
		}
	}
	uint32* mABCnts = mCT->GetAlphabetCounts();
	for(uint32 i=0; i<mCT->GetAlphabetSize(); ++i, ++idx) {
		mUsageCounts[idx] = mABCnts[i];
		mAlphabetIdx[idx] = i;
		mItemSetIdx[idx] = NULL;
		mOrigIdx[idx] = idx;
		mStdLengths[idx] = mCT->GetStdLength(i);
	}

	// at least for `wine` BSort is faster (tiny bit) than MSort
	ArrayUtils::BSortDesc(mUsageCounts, mCurNumCTElems, mOrigIdx);
	ArrayUtils::Permute(mAlphabetIdx, mOrigIdx, mCurNumCTElems);
	ArrayUtils::Permute(mItemSetIdx, mOrigIdx, mCurNumCTElems);
	ArrayUtils::Permute(mStdLengths, mOrigIdx, mCurNumCTElems);
}

ItemSet* SlimCS::NextCandidate(double& candidateHeuristic) {
	while (!mTopCandidates.empty() || mMoreCandidates) {
		if (!mTopCandidates.empty()) {
			mCandidate_i = mTopCandidates.back().i;
			mCandidate_j = mTopCandidates.back().j;
			candidateHeuristic = mTopCandidates.back().heuristic;
			double tmp = mTopCandidates.back().tmp;
			mTopCandidates.pop_back();

			ItemSet* i = mItemSetIdx[mCandidate_i];
			if (i == NULL) { // TODO: Do this only once for each codetable element!
				i = ItemSet::CreateSingleton(mDB->GetDataType(), mAlphabetIdx[mCandidate_i], (uint32) mDB->GetAlphabetSize());
				uint32* dbrows = new uint32[mDB->GetAlphabetNumRows()[mAlphabetIdx[mCandidate_i]]];
				memcpy(dbrows, mDB->GetAlphabetDbOccs()[mAlphabetIdx[mCandidate_i]], mDB->GetAlphabetNumRows()[mAlphabetIdx[mCandidate_i]] * sizeof(uint32));
				i->SetDBOccurrence(dbrows, mDB->GetAlphabetNumRows()[mAlphabetIdx[mCandidate_i]]);
				i->SetSupport(mDB->GetAlphabetNumRows()[mAlphabetIdx[mCandidate_i]]);
				i->SetUsageCount(mUsageCounts[mCandidate_i]);
				i->SetUsage(mCT->GetAlphabetUsage(mAlphabetIdx[mCandidate_i]), mUsageCounts[mCandidate_i]);
				i->SetStandardLength(mCT->GetStdLength(mAlphabetIdx[mCandidate_i]));
				mItemSetIdx[mCandidate_i] = i;
			}
			ItemSet* j = mItemSetIdx[mCandidate_j];
			if (j == NULL) { // TODO: Do this only once for each codetable element!
				j = ItemSet::CreateSingleton(mDB->GetDataType(), mAlphabetIdx[mCandidate_j], (uint32) mDB->GetAlphabetSize());
				uint32* dbrows = new uint32[mDB->GetAlphabetNumRows()[mAlphabetIdx[mCandidate_j]]];
				memcpy(dbrows, mDB->GetAlphabetDbOccs()[mAlphabetIdx[mCandidate_j]], mDB->GetAlphabetNumRows()[mAlphabetIdx[mCandidate_j]] * sizeof(uint32));
				j->SetDBOccurrence(dbrows, mDB->GetAlphabetNumRows()[mAlphabetIdx[mCandidate_j]]);
				j->SetSupport(mDB->GetAlphabetNumRows()[mAlphabetIdx[mCandidate_j]]);
				j->SetUsageCount(mUsageCounts[mCandidate_j]);
				j->SetUsage(mCT->GetAlphabetUsage(mAlphabetIdx[mCandidate_j]), mUsageCounts[mCandidate_j]);
				j->SetStandardLength(mCT->GetStdLength(mAlphabetIdx[mCandidate_j]));
				mItemSetIdx[mCandidate_j] = j;
			}

			ItemSet* ij = i->Union(j);

			if (!ij->GetNumOccs()) {
				delete ij;
				list<uint32>::reverse_iterator rev_it; // Much more likely that we need to insert at the back of the list...
				for (rev_it = mRejectedCandidates[mCandidate_i].rbegin(); rev_it != mRejectedCandidates[mCandidate_i].rend() && *rev_it > mCandidate_j; ++rev_it) {}
				mRejectedCandidates[mCandidate_i].insert(rev_it.base(), mCandidate_j);
				continue;
			}

			// Calculate support fast(er) using DBOccurs (which are automagically updated during union!)
			uint32* rids = ij->GetOccs();
			uint32 numoccs = ij->GetNumOccs();
			uint32 supp = 0;
			if(mDB->HasBinSizes()) {
				for (uint32 i = 0; i < numoccs; ++i)
					supp += mDB->GetRow(rids[i])->GetUsageCount();
			} else
				supp = numoccs;
			ij->SetSupport(supp);

			if (ij->GetSupport() < mMinSup) {
				delete ij;
				list<uint32>::reverse_iterator rev_it; // Much more likely that we need to insert at the back of the list...
				for (rev_it = mRejectedCandidates[mCandidate_i].rbegin(); rev_it != mRejectedCandidates[mCandidate_i].rend() && *rev_it > mCandidate_j; ++rev_it) {}
				mRejectedCandidates[mCandidate_i].insert(rev_it.base(), mCandidate_j);
				continue;
			}

			mDB->CalcStdLength(ij);

			if (mEstStrategy == SLIM_NORMALIZED_GAIN) {
				//candidateHeuristic += (tmp + ij->GetStandardLength()); // Again a sign error? Nope, gain is loss in bits ;-)
				candidateHeuristic -= (tmp + ij->GetStandardLength());
				/* // This is an early stopping criterium, but a bit of a too drastic pruning criteria, nevertheless if a candidates passes this check the estimate is very very tight.
				if (candidateHeuristic <= 0.0) {
					delete ij;
					list<uint32>::reverse_iterator rev_it; // Much more likely that we need to insert at the back of the list...
					for (rev_it = mRejectedCandidates[mCandidate_i].rbegin(); rev_it != mRejectedCandidates[mCandidate_i].rend() && *rev_it > mCandidate_j; ++rev_it) {}
					mRejectedCandidates[mCandidate_i].insert(rev_it.base(), mCandidate_j);
					continue;
				}
				*/
			}
			if (mEstStrategy == SLIM_NORMALIZED_GAIN_CT) {
				candidateHeuristic -= ij->GetStandardLength();
			}

			return ij;
		}
		if (mMoreCandidates) {
			mMoreCandidates = false;

			if(mEstStrategy == SLIM_USAGE) {
				EstimateQualityOnUsage();
			} else if (mEstStrategy == SLIM_GAIN) {
				EstimateQualityOnLocalGain();
			} else if (mEstStrategy == SLIM_NORMALIZED_GAIN) {
				// normalized gain in bits based
				EstimateQualityOnGlobalGainDB();
			} else if (mEstStrategy == SLIM_NORMALIZED_GAIN_CT) {
				// normalized gain in bits based
				EstimateQualityOnGlobalGain();
			}
			sort_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
			reverse(mTopCandidates.begin(),mTopCandidates.end());
		}
	}
	return NULL;
}


void SlimCS::ProgressToScreen(const uint64 curCandidate, CodeTable *ct) {
	if(Bass::GetOutputLevel() > 0) {
		uint64 canDif = curCandidate - mScreenReportCandidateIdx;

		if(true) {
			// Candidates per Second uitrekenen, update per 30s
			time_t curTime = time(NULL);
			if(canDif >= mScreenReportCandidateDelta) {
				uint32 timeDif = (uint32) (curTime - mScreenReportTime);
				mScreenReportCandPerSecond = (uint32) (curCandidate - mScreenReportCandidateIdx) / (timeDif > 0 ? timeDif : 1);
				mScreenReportCandidateDelta = (mScreenReportCandPerSecond + 1) * 30;
				mScreenReportTime = curTime;
				mScreenReportCandidateIdx = curCandidate;
			}

			
			double numBits = ct->GetCurStats().encSize;
			double difBits = mScreenReportBits - numBits;
			mScreenReportBits = numBits;

			// Percentage uitrekenen
			int d = curCandidate % 4;
			char c = '*';
			switch(d) {
			case 0 : c = '-'; break;
			case 1 : c = '\\'; break;
			case 2 : c = '|'; break;
			default : c = '/';
			}
			uint32 numprinted = 0;
			numprinted = printf_s(" %c Progress:\t\t%" I64d " (%d/s), %.00fb (-%.01fb)   ", c, curCandidate, mScreenReportCandPerSecond, numBits, difBits);
			if(Bass::GetOutputLevel() == 3)
				printf("\n");
			else {
				if(numprinted < 45)
					for(uint32 i=0; i< (45-numprinted); i++)
						printf(" ");
				printf("\r");
				//printf(" "%c Progress:\t\tcandidate %" I64d " (%d/s), %.02fbits (-%.02f)\r", c, curCandidate, mScreenReportCandPerSecond, numBits, difBits);
			}
			fflush(stdout);
		}
	}
}

// Heuristic quality measure for candidate itemset based on estimated usage
void SlimCS::EstimateQualityOnUsage() {
	list<uint32>::iterator rejected;
	
	uint32 minUsage;
	for(uint32 i = 0; i < mCurNumCTElems; ++i) {
		minUsage = mTopCandidates.size() == mNumTopCandidates ? (uint32) mTopCandidates.front().heuristic : 0;
		if (mUsageCounts[i] <= minUsage) {
			if (mUsageCounts[i])
				mMoreCandidates = true;
			break;
		}
		if (mUsageCounts[i] < mMinSup)
			continue;

		rejected = mRejectedCandidates[i].begin();

		uint32* usage_i = mItemSetIdx[i] ? mItemSetIdx[i]->GetUsages() : mCT->GetAlphabetUsage(mAlphabetIdx[i]);
		for(uint32 j = i + 1; j < mCurNumCTElems; ++j) {

			minUsage = mTopCandidates.size() == mNumTopCandidates ? (uint32) mTopCandidates.front().heuristic : 0;
			if (mUsageCounts[j] <= minUsage) {
				if (mUsageCounts[j])
					mMoreCandidates = true;
				break;
			}
			if (mUsageCounts[j] < mMinSup)
				break;

			if (rejected != mRejectedCandidates[i].end() && *rejected == j) {
				++rejected;
				continue;
			}
			uint32* usage_j = mItemSetIdx[j] ? mItemSetIdx[j]->GetUsages() : mCT->GetAlphabetUsage(mAlphabetIdx[j]);

			//uint32 estimatedUsage = CountUsageMatches(usage_i, mUsageCounts[i], usage_j, mUsageCounts[j], max(minUsage,mMinSup-1));
			uint32 estimatedUsage = (uint32) ArrayUtils::IntersectionLength(usage_i, mUsageCounts[i], usage_j, mUsageCounts[j], max(minUsage,mMinSup-1));
			//					uint32 estimatedUsage = CountUsageMatches(usage_i, mUsageCounts[i], usage_j, mUsageCounts[j], minUsage);

			if (estimatedUsage >= mMinSup) {
				if (mTopCandidates.size() == mNumTopCandidates) {
					mMoreCandidates = true;
					if (estimatedUsage > mTopCandidates.front().heuristic) {
						pop_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
						mTopCandidates.pop_back();
					}
				}
				if (mTopCandidates.size() < mNumTopCandidates) {
					mTopCandidates.push_back(Candidate(i, j, estimatedUsage));
					push_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
				}
			}
		}
	}
}

// Heuristic quality measure for candidate itemset based on estimated gain
void SlimCS::EstimateQualityOnLocalGain() {
	// gain in bits based

	list<uint32>::iterator rejected;
	uint64 countSum = mCT->GetCurStats().usgCountSum;
	double log_countsum = log2((double)countSum);
	double best_ij_gain;
	for(uint32 i = 0; i < mCurNumCTElems; ++i) {
		best_ij_gain = mTopCandidates.size() == mNumTopCandidates ? mTopCandidates.front().heuristic : 0.0;

		// first estimate
		uint32 i_max_usage = mUsageCounts[i];
		if(i_max_usage < mMinSup)
			continue;

		// 1. as we calculate many i and j codelengths we could cache this
		// 2. we could also add an extra field in ItemSet and store/retrieve it
		double i_log_max_usage = log2((double)i_max_usage);
		double i_code_cost = -i_log_max_usage + log_countsum;

		uint64 ii_countsum = countSum - i_max_usage;
		double ii_code_cost = -i_log_max_usage + log2((double)ii_countsum);

		double ii_i_sep_cost = i_max_usage * i_code_cost;
		double ii_combo_cost = i_max_usage * ii_code_cost;	// naively assume we can replace all i codes by ii codes

		double ii_gain = 2* ii_i_sep_cost - ii_combo_cost;
		if (ii_gain <= best_ij_gain) {
			if (ii_gain > 0.0)
				mMoreCandidates = true;
			break;
		}

		rejected = mRejectedCandidates[i].begin();

		uint32* usage_i = mItemSetIdx[i] ? mItemSetIdx[i]->GetUsages() : mCT->GetAlphabetUsage(mAlphabetIdx[i]);
		for(uint32 j = i + 1; j < mCurNumCTElems; ++j) {
			best_ij_gain = mTopCandidates.size() == mNumTopCandidates ? mTopCandidates.front().heuristic : 0.0;

			uint32 j_max_usage = mUsageCounts[j];
			if(j_max_usage < mMinSup)
				continue;

			if (rejected != mRejectedCandidates[i].end() && *rejected == j) {
				++rejected;
				continue;
			}

			double j_log_max_usage = log2((double)j_max_usage);
			double j_code_cost = -j_log_max_usage + log_countsum;

			uint32 ij_max_usage = j_max_usage;

			uint32 ij_i_remainder_usage = i_max_usage - ij_max_usage;
			uint32 ij_j_remainder_usage = j_max_usage - ij_max_usage;
			uint64 ij_countsum = countSum - i_max_usage - j_max_usage + ij_i_remainder_usage + ij_j_remainder_usage + ij_max_usage;

			double ij_log_countsum = log2((double)ij_countsum);
			double ij_i_code_cost = ij_i_remainder_usage > 0 ? -log2((double)ij_i_remainder_usage) + ij_log_countsum : 0;

			double ij_j_code_cost = ij_j_remainder_usage > 0 ? (ij_i_remainder_usage == ij_j_remainder_usage ? ij_i_code_cost : -log2((double)ij_j_remainder_usage) + ij_log_countsum) : 0;

			double ij_code_cost = ij_max_usage == ij_j_remainder_usage ? ij_j_code_cost :
									(ij_max_usage == ij_i_remainder_usage ? ij_i_code_cost : -log2((double)ij_max_usage) + ij_log_countsum ) ;

			double ij_sep_cost = ii_i_sep_cost + j_max_usage * j_code_cost;
			double ij_combo_cost = ij_i_remainder_usage * ij_i_code_cost + ij_j_remainder_usage * ij_j_code_cost + ij_max_usage * ij_code_cost;

			double ij_gain = ij_sep_cost - ij_combo_cost;
			if (ij_gain <= best_ij_gain) {
				if (ij_gain > 0.0)
					mMoreCandidates = true;
				break;
			}

			// eigenlijk nu uitrekenen hoeveel usages minimaal nodig zijn om best_gain te overtreffen
			/*
			double l1 = pow(2,-best_gain);
			double l2 = pow(2, z);
			double l = l1/l2;
			double r = (zxy-zzy-zzx+zzz) / (c-z)
			// hierover minimaliseren, is monotoon, dus bv via binary search
			// min_z = 1
			// max_z = ij_max_usage
				*/

			uint32* usage_j = mItemSetIdx[j] ? mItemSetIdx[j]->GetUsages() : mCT->GetAlphabetUsage(mAlphabetIdx[j]);
			uint32 est_ij_usage  = (uint32) ArrayUtils::IntersectionLength(usage_i, mUsageCounts[i], usage_j, mUsageCounts[j], mMinSup-1);

			if (est_ij_usage >= mMinSup) {
				uint32 est_ij_i_remainder_usage = i_max_usage - est_ij_usage;
				uint32 est_ij_j_remainder_usage = j_max_usage - est_ij_usage;
				uint64 est_ij_countsum = countSum - i_max_usage - j_max_usage + est_ij_i_remainder_usage + est_ij_j_remainder_usage + est_ij_usage;

				double est_ij_log_countsum = log2((double)est_ij_countsum);

				double est_ij_i_code_cost = est_ij_i_remainder_usage > 0 ? -log2((double) est_ij_i_remainder_usage) + est_ij_log_countsum : 0;
				double est_ij_j_code_cost = est_ij_j_remainder_usage > 0 ? (est_ij_j_remainder_usage == est_ij_i_remainder_usage ? est_ij_i_code_cost : -log2((double)est_ij_j_remainder_usage) + est_ij_log_countsum) : 0;

				double est_ij_code_cost = est_ij_usage == est_ij_j_remainder_usage ? est_ij_j_code_cost : (est_ij_usage == est_ij_i_remainder_usage ? est_ij_i_code_cost : -log2((double)est_ij_usage) + est_ij_log_countsum);
				double est_ij_combo_cost = est_ij_i_remainder_usage * est_ij_i_code_cost + est_ij_j_remainder_usage * est_ij_j_code_cost + est_ij_usage * est_ij_code_cost;

				double est_ij_gain = ij_sep_cost - est_ij_combo_cost;
				if (est_ij_gain > 0.0) {
					if (mTopCandidates.size() == mNumTopCandidates) {
						mMoreCandidates = true;
						if (est_ij_gain > mTopCandidates.front().heuristic) {
							pop_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
							mTopCandidates.pop_back();
						}
					}
					if (mTopCandidates.size() < mNumTopCandidates) {
						mTopCandidates.push_back(Candidate(i, j, est_ij_gain));
						push_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
					}
				}
			}

		}
	}
}

void SlimCS::EstimateQualityOnGlobalGainDB() {
	// normalized gain in bits based
	// normalized with what?

	list<uint32>::iterator rejected;
	uint64 countSum = mCT->GetCurStats().usgCountSum;
	double log_countsum = log2((double)countSum);
	double best_ij_gain;
	for(uint32 i = 0; i < mCurNumCTElems; ++i) {
		best_ij_gain = mTopCandidates.size() == mNumTopCandidates ? mTopCandidates.front().heuristic : 0.0;

		// first estimate
		uint32 i_max_usage = mUsageCounts[i];
		if(i_max_usage < mMinSup)
			continue;



		// 1. as we calculate many i and j codelengths we could cache this
		// 2. we could also add an extra field in ItemSet and store/retrieve it
		double i_log_max_usage = log2((double)i_max_usage);
		double i_code_cost = -i_log_max_usage + log_countsum;

		uint64 ii_countsum = countSum - i_max_usage;
		double ii_code_cost = -i_log_max_usage + log2((double)ii_countsum);

		double ii_i_sep_cost = i_max_usage * i_code_cost;
		double ii_combo_cost = i_max_usage * ii_code_cost;	// naively assume we can replace all i codes by ii codes

		double ii_gain = 2* ii_i_sep_cost - ii_combo_cost;

		if (ii_gain <= best_ij_gain) {
			if (ii_gain > 0.0)
				mMoreCandidates = true;
			break;
		}

		rejected = mRejectedCandidates[i].begin();

		uint32* usage_i = mItemSetIdx[i] ? mItemSetIdx[i]->GetUsages() : mCT->GetAlphabetUsage(mAlphabetIdx[i]);
		for(uint32 j = i + 1; j < mCurNumCTElems; ++j) {
			best_ij_gain = mTopCandidates.size() == mNumTopCandidates ? mTopCandidates.front().heuristic : 0.0;

			uint32 j_max_usage = mUsageCounts[j];
			if(j_max_usage < mMinSup)
				continue;

			if (rejected != mRejectedCandidates[i].end() && *rejected == j) {
				++rejected;
				continue;
			}

			double j_log_max_usage = log2((double)j_max_usage);
			double j_code_cost = -j_log_max_usage + log_countsum;

			uint32 ij_max_usage = j_max_usage;

			uint32 ij_i_remainder_usage = i_max_usage - ij_max_usage;
			uint32 ij_j_remainder_usage = j_max_usage - ij_max_usage;
			uint64 ij_countsum = countSum - i_max_usage - j_max_usage + ij_i_remainder_usage + ij_j_remainder_usage + ij_max_usage;

			double ij_log_countsum = log2((double)ij_countsum);
			double ij_i_code_cost = ij_i_remainder_usage > 0 ? -log2((double)ij_i_remainder_usage) + ij_log_countsum : 0;

			double ij_j_code_cost = ij_j_remainder_usage > 0 ? (ij_i_remainder_usage == ij_j_remainder_usage ? ij_i_code_cost : -log2((double)ij_j_remainder_usage) + ij_log_countsum) : 0;

			double ij_code_cost = ij_max_usage == ij_j_remainder_usage ? ij_j_code_cost :
									(ij_max_usage == ij_i_remainder_usage ? ij_i_code_cost : -log2((double)ij_max_usage) + ij_log_countsum ) ;

			double ij_sep_cost = ii_i_sep_cost + j_max_usage * j_code_cost;
			double ij_combo_cost = ij_i_remainder_usage * ij_i_code_cost + ij_j_remainder_usage * ij_j_code_cost + ij_max_usage * ij_code_cost;

			double ij_gain = ij_sep_cost - ij_combo_cost;
			if (ij_gain <= best_ij_gain) {
				if (ij_gain > 0.0)
					mMoreCandidates = true;
				break;
			}

			// eigenlijk nu uitrekenen hoeveel usages minimaal nodig zijn om best_gain te overtreffen
			/*
			double l1 = pow(2,-best_gain);
			double l2 = pow(2, z);
			double l = l1/l2;
			double r = (zxy-zzy-zzx+zzz) / (c-z)
			// hierover minimaliseren, is monotoon, dus bv via binary search
			// min_z = 1
			// max_z = ij_max_usage
				*/

			uint32* usage_j = mItemSetIdx[j] ? mItemSetIdx[j]->GetUsages() : mCT->GetAlphabetUsage(mAlphabetIdx[j]);
			uint32 est_ij_usage  = (uint32) ArrayUtils::IntersectionLength(usage_i, mUsageCounts[i], usage_j, mUsageCounts[j], mMinSup-1);

			if (est_ij_usage >= mMinSup) {
				double old_usage_i = (double) i_max_usage;
				double old_usage_j = (double) j_max_usage;
				double new_usage_ij = (double) est_ij_usage;
				double new_usage_i = old_usage_i - new_usage_ij;
				double new_usage_j = old_usage_j - new_usage_ij;
				double old_countsum = (double) countSum;
				double new_countsum = old_countsum - new_usage_ij;
				double log_old_usage_i = old_usage_i > 0.0 ? log2(old_usage_i) : 0.0;
				double log_old_usage_j = old_usage_j > 0.0 ? log2(old_usage_j) : 0.0;
				double log_new_usage_ij = new_usage_ij > 0.0 ? log2(new_usage_ij) : 0.0 ;
				double log_new_usage_i = new_usage_i > 0.0 ? log2(new_usage_i) : 0.0 ;
				double log_new_usage_j = new_usage_j > 0.0 ? log2(new_usage_j) : 0.0 ;
				double log_old_countsum = old_countsum > 0.0 ? log2(old_countsum) : 0.0 ;
				double log_new_countsum = new_countsum > 0.0 ? log2(new_countsum) : 0.0 ;
				double old_num_codes_with_non_zero_usage = (double)mCurNumCTElems; // TODO: only count the ones with non-zero usage
				double new_num_codes_with_non_zero_usage = old_num_codes_with_non_zero_usage + 1;
				if (new_usage_i == 0.0) --new_num_codes_with_non_zero_usage;
				if (new_usage_j == 0.0) --new_num_codes_with_non_zero_usage;

				double gain_db_ij = -new_usage_ij * log_new_usage_ij - new_usage_i * log_new_usage_i  + old_usage_i * log_old_usage_i  - new_usage_j * log_new_usage_j  + old_usage_j * log_old_usage_j + new_countsum * log_new_countsum - old_countsum * log_old_countsum; // TODO
				gain_db_ij *= -1.0; // TODO: Did I really make a sign error? Nope, gain is loss in bits ;-)

				double gain_ct_ij = 0.0; // TODO
				// gain_ct_ij += ij->GetStandardLength(); // TODO: we need to postpone this until after we have constructed the itemset.
				gain_ct_ij -= log_new_usage_ij;
				if (new_usage_i != old_usage_i) {
					if (new_usage_i != 0.0 && old_usage_i != 0.0) {
						gain_ct_ij -= log_new_usage_i;
						gain_ct_ij += log_old_usage_i;
					} else if (old_usage_i == 0.0) {
						gain_ct_ij += mStdLengths[i];
						gain_ct_ij -= log_new_usage_i;
					} else if (new_usage_i == 0.0) {
						gain_ct_ij -= mStdLengths[i];
						gain_ct_ij += log_old_usage_i;
					}
				}
				if (new_usage_j != old_usage_j) {
					if (new_usage_j != 0.0 && old_usage_j != 0.0) {
						gain_ct_ij -= log_new_usage_j;
						gain_ct_ij += log_old_usage_j;
					} else if (old_usage_j == 0.0) {
						gain_ct_ij += mStdLengths[j];
						gain_ct_ij -= log_new_usage_j;
					} else if (new_usage_j == 0.0) {
						gain_ct_ij += mStdLengths[j];
						gain_ct_ij -= log_old_usage_j;
					}
				}
				gain_ct_ij += new_num_codes_with_non_zero_usage * log_new_countsum;
				gain_ct_ij -= old_num_codes_with_non_zero_usage * log_old_countsum;

//							double gain_ij = gain_db_ij + gain_ct_ij; // Not yet complete so we can't use this.
//							printf("%.2f\n", gain_ij);

				if (gain_db_ij > 0.0) {
					if (mTopCandidates.size() == mNumTopCandidates) {
						mMoreCandidates = true;
						if (gain_db_ij > mTopCandidates.front().heuristic) {
							pop_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
							mTopCandidates.pop_back();
						}
					}
					if (mTopCandidates.size() < mNumTopCandidates) {
						mTopCandidates.push_back(Candidate(i, j, gain_db_ij, gain_ct_ij));
						push_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
					}
				}
			}

		}
	}

}

void SlimCS::EstimateQualityOnGlobalGain() {
	list<uint32>::iterator rejected;
	// normalized gain in bits based
	uint64 countSum = mCT->GetCurStats().usgCountSum;
	double log_countsum = log2((double)countSum);
	double best_ij_gain;
	for(uint32 i = 0; i < mCurNumCTElems; ++i) {
		best_ij_gain = mTopCandidates.size() == mNumTopCandidates ? mTopCandidates.front().heuristic : 0.0;

		// first estimate
		uint32 i_max_usage = mUsageCounts[i];
		if(i_max_usage < mMinSup)
			continue;



		// 1. as we calculate many i and j codelengths we could cache this
		// 2. we could also add an extra field in ItemSet and store/retrieve it
		double i_log_max_usage = log2((double)i_max_usage);
		double i_code_cost = -i_log_max_usage + log_countsum;

		uint64 ii_countsum = countSum - i_max_usage;
		double ii_code_cost = -i_log_max_usage + log2((double)ii_countsum);

		double ii_i_sep_cost = i_max_usage * i_code_cost;
		double ii_combo_cost = i_max_usage * ii_code_cost;	// naively assume we can replace all i codes by ii codes

		double ii_gain = 2* ii_i_sep_cost - ii_combo_cost;

/*
  		double countsum = (double)countSum;
		double max_usage_i = (double) i_max_usage;
		double log_usage_i = log2(max_usage_i);
		double usage_ii = max_usage_i;
		double log_usage_ii = log2(usage_ii);
		double countsum_ii = countsum - max_usage_i;
		double log_countsum_ii = log2(countsum_ii);
		double gain_db_ii = -usage_ii * log_usage_ii +  2*max_usage_i * log_usage_i + countsum_ii * log_countsum_ii - countsum * log_countsum;
		gain_db_ii *= -1.0; // TODO: did I really made a sign error?
		double gain_ct_ii = 0.0; // TODO
		double gain_ii = gain_db_ii + gain_ct_ii;
		printf("%.2f %.2f\n", gain_ii, ii_gain);
*/

		if (ii_gain <= best_ij_gain) {
			if (ii_gain > 0.0)
				mMoreCandidates = true;
			break;
		}




		rejected = mRejectedCandidates[i].begin();

		uint32* usage_i = mItemSetIdx[i] ? mItemSetIdx[i]->GetUsages() : mCT->GetAlphabetUsage(mAlphabetIdx[i]);
		for(uint32 j = i + 1; j < mCurNumCTElems; ++j) {
			best_ij_gain = mTopCandidates.size() == mNumTopCandidates ? mTopCandidates.front().heuristic : 0.0;

			uint32 j_max_usage = mUsageCounts[j];
			if(j_max_usage < mMinSup)
				continue;

			if (rejected != mRejectedCandidates[i].end() && *rejected == j) {
				++rejected;
				continue;
			}

			double j_log_max_usage = log2((double)j_max_usage);
			double j_code_cost = -j_log_max_usage + log_countsum;

			uint32 ij_max_usage = j_max_usage;

			uint32 ij_i_remainder_usage = i_max_usage - ij_max_usage;
			uint32 ij_j_remainder_usage = j_max_usage - ij_max_usage;
			uint64 ij_countsum = countSum - i_max_usage - j_max_usage + ij_i_remainder_usage + ij_j_remainder_usage + ij_max_usage;

			double ij_log_countsum = log2((double)ij_countsum);
			double ij_i_code_cost = ij_i_remainder_usage > 0 ? -log2((double)ij_i_remainder_usage) + ij_log_countsum : 0;

			double ij_j_code_cost = ij_j_remainder_usage > 0 ? (ij_i_remainder_usage == ij_j_remainder_usage ? ij_i_code_cost : -log2((double)ij_j_remainder_usage) + ij_log_countsum) : 0;

			double ij_code_cost = ij_max_usage == ij_j_remainder_usage ? ij_j_code_cost :
									(ij_max_usage == ij_i_remainder_usage ? ij_i_code_cost : -log2((double)ij_max_usage) + ij_log_countsum ) ;

			double ij_sep_cost = ii_i_sep_cost + j_max_usage * j_code_cost;
			double ij_combo_cost = ij_i_remainder_usage * ij_i_code_cost + ij_j_remainder_usage * ij_j_code_cost + ij_max_usage * ij_code_cost;

			double ij_gain = ij_sep_cost - ij_combo_cost;
			if (ij_gain <= best_ij_gain) {
				if (ij_gain > 0.0)
					mMoreCandidates = true;
				break;
			}

			// eigenlijk nu uitrekenen hoeveel usages minimaal nodig zijn om best_gain te overtreffen
			/*
			double l1 = pow(2,-best_gain);
			double l2 = pow(2, z);
			double l = l1/l2;
			double r = (zxy-zzy-zzx+zzz) / (c-z)
			// hierover minimaliseren, is monotoon, dus bv via binary search
			// min_z = 1
			// max_z = ij_max_usage
				*/

			uint32* usage_j = mItemSetIdx[j] ? mItemSetIdx[j]->GetUsages() : mCT->GetAlphabetUsage(mAlphabetIdx[j]);
			uint32 est_ij_usage  = (uint32) ArrayUtils::IntersectionLength(usage_i, mUsageCounts[i], usage_j, mUsageCounts[j], mMinSup-1);

			if (est_ij_usage >= mMinSup) {
				double old_usage_i = (double) i_max_usage;
				double old_usage_j = (double) j_max_usage;
				double new_usage_ij = (double) est_ij_usage;
				double new_usage_i = old_usage_i - new_usage_ij;
				double new_usage_j = old_usage_j - new_usage_ij;
				double old_countsum = (double) countSum;
				double new_countsum = old_countsum - new_usage_ij;
				double log_old_usage_i = old_usage_i > 0.0 ? log2(old_usage_i) : 0.0;
				double log_old_usage_j = old_usage_j > 0.0 ? log2(old_usage_j) : 0.0;
				double log_new_usage_ij = new_usage_ij > 0.0 ? log2(new_usage_ij) : 0.0 ;
				double log_new_usage_i = new_usage_i > 0.0 ? log2(new_usage_i) : 0.0 ;
				double log_new_usage_j = new_usage_j > 0.0 ? log2(new_usage_j) : 0.0 ;
				double log_old_countsum = old_countsum > 0.0 ? log2(old_countsum) : 0.0 ;
				double log_new_countsum = new_countsum > 0.0 ? log2(new_countsum) : 0.0 ;
				double old_num_codes_with_non_zero_usage = (double)mCurNumCTElems; // TODO: only count the ones with non-zero usage
				double new_num_codes_with_non_zero_usage = old_num_codes_with_non_zero_usage + 1;
				if (new_usage_i == 0.0) --new_num_codes_with_non_zero_usage;
				if (new_usage_j == 0.0) --new_num_codes_with_non_zero_usage;

				double gain_db_ij = -new_usage_ij * log_new_usage_ij - new_usage_i * log_new_usage_i  + old_usage_i * log_old_usage_i  - new_usage_j * log_new_usage_j  + old_usage_j * log_old_usage_j + new_countsum * log_new_countsum - old_countsum * log_old_countsum; // TODO
				gain_db_ij *= -1.0; // TODO: Did I really make a sign error? Nope, gain is loss in bits ;-)

				double gain_ct_ij = 0.0; // TODO
				// gain_ct_ij += ij->GetStandardLength(); // TODO: we need to postpone this until after we have constructed the itemset.
				gain_ct_ij -= log_new_usage_ij;
				if (new_usage_i != old_usage_i) {
					if (new_usage_i != 0.0 && old_usage_i != 0.0) {
						gain_ct_ij -= log_new_usage_i;
						gain_ct_ij += log_old_usage_i;
					} else if (old_usage_i == 0.0) {
						gain_ct_ij += mStdLengths[i];
						gain_ct_ij -= log_new_usage_i;
					} else if (new_usage_i == 0.0) {
						gain_ct_ij -= mStdLengths[i];
						gain_ct_ij += log_old_usage_i;
					}
				}
				if (new_usage_j != old_usage_j) {
					if (new_usage_j != 0.0 && old_usage_j != 0.0) {
						gain_ct_ij -= log_new_usage_j;
						gain_ct_ij += log_old_usage_j;
					} else if (old_usage_j == 0.0) {
						gain_ct_ij += mStdLengths[j];
						gain_ct_ij -= log_new_usage_j;
					} else if (new_usage_j == 0.0) {
						gain_ct_ij += mStdLengths[j];
						gain_ct_ij -= log_old_usage_j;
					}
				}
				gain_ct_ij += new_num_codes_with_non_zero_usage * log_new_countsum;
				gain_ct_ij -= old_num_codes_with_non_zero_usage * log_old_countsum;

				double gain_ij = gain_db_ij - gain_ct_ij;

//				if (gain_ij > 0.0) { // This is too strict
				if (gain_db_ij > 0.0) {
					if (mTopCandidates.size() == mNumTopCandidates) {
						mMoreCandidates = true;
						if (gain_ij > mTopCandidates.front().heuristic) {
							pop_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
							mTopCandidates.pop_back();
						}
					}
					if (mTopCandidates.size() < mNumTopCandidates) {
						mTopCandidates.push_back(Candidate(i, j, gain_ij));
						push_heap(mTopCandidates.begin(),mTopCandidates.end(), greater<Candidate>());
					}
				}
			}

		}
	}

}


#endif // BLOCK_SLIM
