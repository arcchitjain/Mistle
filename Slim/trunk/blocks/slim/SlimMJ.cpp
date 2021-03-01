#ifdef BLOCK_SLIM

#include "../global.h"

#include <algorithm>
#include <time.h>
#include <omp.h>
#include <unordered_set>
#include <functional>

using namespace std;

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include <SystemUtils.h>
#include <Templates.h>
#include <ArrayUtils.h>

#include "../krimp/codetable/CodeTable.h"
#include "SlimMJ.h"
#include "SlimCS.h"


struct GreaterComparer {
	bool operator()(const CandidateCTGain *x, const CandidateCTGain *y) {
		if (x->mHeuristic > y->mHeuristic) return true;
		else if (x->mHeuristic < y->mHeuristic) return false;
		else if (x->mRankI < y->mRankI) return true;
		else if (x->mRankI > y->mRankI) return false;
		else if (x->mRankJ < y->mRankJ) return true;
		else return false;
	}
};

SlimMJ::SlimMJ(CodeTable* ct, HashPolicyType hashPolicy, Config* config) : SlimAlgo(ct, hashPolicy, config) {
	mScreenReportBits = 0;

	string estStrategy = config->Read<string>("estStrategy", "gain");
	if (estStrategy.compare("usg") == 0 || estStrategy.compare("usage") == 0) {
		mEstStrategy = SlimMJ_USAGE;
	} else {
		mEstStrategy = SlimMJ_GAIN;
	}

	mUsgChangedIds = NULL;
	mUsgChangedIdsLen = 0;	
	mUsgChangedLU = NULL;
	mUsgChangedLULen = 0;
	
	/*mOMGCounter = 0;
	mLOLCounter = 0;
	mROFLCounter = 0;
	mCopterCounter = 0;*/

	mMinGain = config->Read<double>("minGain", 0.0);
	mMinGainEst = config->Read<double>("minGainEst", 0.0);
	mMaxTime = config->Read<uint32>("maxTime", 0) * 3600.0; // in hours
	
	mCurMinGainEst = 0.0;
	mItemSetSorted = new ItemSetSorted();
	mCurrentCandidate = NULL;
	mNoCa = new unordered_map<uint32,unordered_set<uint32>>;
}

SlimMJ::~SlimMJ() {
	uint32 abLen = mCT->GetAlphabetSize();
	for(uint32 i=0; i<abLen; ++i) {
		delete mItemSetSorted->GetItemSet(i);	// we created it
	}
	delete mItemSetSorted;

	size_t end = mTopCandidates.size();
	for(size_t i =0; i<end; i++) {
		delete mTopCandidates.at(i);
	}

	delete[] mUsgChangedIds;
	delete[] mUsgChangedLU;

	delete mNoCa;
}

CodeTable* SlimMJ::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */) {
	/*if(mDB->GetDataType() == BM128ItemSetType){
		mCTBm128 = (CCCPUCodeTable<BM128ItemSetType>*) (mCT);
		mCTBm128->SetCodeTable(mItemSetSorted);
	} else if(mDB->GetDataType() == BAI32ItemSetType){
		mCTBai32 = (CCCPUCodeTable<BAI32ItemSetType>*) (mCT);
		mCTBai32->SetCodeTable(mItemSetSorted);
	} else if(mDB->GetDataType() == Uint16ItemSetType){
		mCTUint16 = (CCCPUCodeTable<Uint16ItemSetType>*) (mCT);
		mCTUint16->SetCodeTable(mItemSetSorted);
	}*/
	mCT->SetCodeTable(mItemSetSorted);

	bitGain = mConfig->Read<double>("thresholdBits", 0);//threshold bits for stopping compression
	
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets();
	mNumCandidates = 0;
	mProgress = -1;

	mItemSetCount = mCT->GetAlphabetSize();//MG //set it to alphabet length for the first use
	uint32 candidateCount = 0;//MG

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
	if (mWriteLogFile == true)
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
	double curSize;
	ItemSet* minCandidate = 0;
	uint64 numIterations = 0;
	uint32 numCandidates = 0;

	//string strLatestCT = mOutDir + "ct-latest.ct";

	FillCodeTable();
	std::list<uint32> deletedIDs1;
	UpdateLocalCTStuff(deletedIDs1);
	do {
		bool stopCompression = false;
		ProgressToScreen(numCandidates, mCT);
		if(mWriteProgressToDisk) {
			if(numIterations == 0) {
				ProgressToDisk(mCT, 0, 0, numCandidates, mReportIterType == ReportCodeTable || mReportIterType == ReportAll, mReportIterType == ReportStats || mReportIterType == ReportAll);
			} else if(mReportOnAccept == true && minCandidate) {
				ProgressToDisk(mCT, 0, 0, numCandidates, mReportIterType == ReportCodeTable || mReportIterType == ReportAll, mReportIterType == ReportStats || mReportIterType == ReportAll);
			}
		}
		double elapsedTime = omp_get_wtime() - mCompressionStartTime;
		if (mMaxTime && (elapsedTime > mMaxTime))
			break;

		mPrevCountSum = (double) curStats.usgCountSum;
		mPrevNumCTElems = mCT->GetCurNumSets();

		minSize = mCT->GetCurSize();
		minCandidate = NULL;

		numIterations++;
		double heuristic = 0;
		ItemSet* candidate = NULL;

		candidateCount = 0;
		GenerateCandidates();
		candidate = NextCandidate(heuristic, candidateCount);

		while (!minCandidate && candidate) {
			++numCandidates;
			mCT->Add(candidate);
			candidate->SetUsageCount(0);
			mCT->CoverDB(curStats);
			if(curStats.encDbSize < 0) {
				THROW("L(D|M) < 0. That's not good.");
			}
			std::list<uint32> deletedIDs;
			curSize = mCT->GetCurSize();
			if (curSize < minSize) {
				if (minSize - curSize <= bitGain){
					mCT->RollbackAdd();
					stopCompression = true;
					break;
				}

				mCT->CommitAdd(mWriteCTLogFile);
				if ((mPruneStrategy == PostAcceptPruneStrategy) || (mPruneStrategy == PostEstimationPruning)) {	// Post-decide On-the-fly pruning
					deletedIDs = PrunePostAccept(mCT);
				}
				if (mWriteLogFile) fprintf(mLogFile, "Accepted: %s[%.2f, %.2f, %.2f, %d]\n", candidate->ToCodeTableString().c_str(), curStats.encSize, minSize, heuristic, numCandidates);

				candidate->SetUniqueID(mItemSetCount);
				mItemSetCount++;
				UsageClass uc = UsageClass(candidate, 0);
				mItemSetSorted->Insert(candidate->GetUniqueID(), uc);//MG-insert new itemset in the list
				minCandidate = candidate;

				UpdateLocalCTStuff(deletedIDs);

			} else {
				if (mWriteLogFile) fprintf(mLogFile, "Rejected: %s[%.2f, %.2f, %.2f, %d]\n", candidate->ToCodeTableString().c_str(), curStats.encSize, minSize, heuristic, numCandidates);

				mCT->RollbackAdd();

				mCurrentCandidate->mActualGain = minSize - curSize;

				// remove from mTopCandidates
				// insert into baca
				
				candidate = NextCandidate(heuristic, candidateCount);
			}
		}
		if (stopCompression){
			break;
		}

	} while (minCandidate);


	double timeCompression = omp_get_wtime() - mCompressionStartTime;
	printf(" * Time:    \t\tCompressing the database took %f seconds.\t\t\n", timeCompression);
	//printf(" * Cache Hits:\t\t%lu (%.2f%%)  \n", mNumCacheHits, ((100.0 * (double)mNumCacheHits)/((double)mNumCacheHits + (double)mNumGainCalculations)));

	stats = mCT->GetCurStats();
	printf(" * Result:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);

	//printf(" * Amount of Fun:\t%d / %d OMGLOL\t%d / %d ROFLCOPTER\n", mLOLCounter, mOMGCounter, mCopterCounter, mROFLCounter);

	if(mWriteProgressToDisk == true) {
		ProgressToDisk(mCT, 0, 0, numCandidates, true, true);
	}
	CloseCTLogFile();
	CloseReportFile();
	CloseLogFile();

	mCT->EndOfKrimp();

	return mCT;
}

void SlimMJ::UpdateLocalCTStuff(std::list<uint32> deletedIDs) {
	if(deletedIDs.size() > 0) {
		mItemSetSorted->Remove(&deletedIDs);
	}

	uint32 ctNumSets = (mCT->GetCurNumSets());
	uint32 abSize = (mCT->GetAlphabetSize());
	mCurNumCTElems = ctNumSets + abSize;

	// list of ids of changed code table elements
	if(mUsgChangedIdsLen < mCurNumCTElems) {
		delete[] mUsgChangedIds;
		mUsgChangedIdsLen = mCurNumCTElems * 2;
		mUsgChangedIds = new uint32[mUsgChangedIdsLen];
	}

	// lookup table for changed code table elements
	if(mUsgChangedLULen < mItemSetCount) {
		delete[] mUsgChangedLU;
		mUsgChangedLULen = mItemSetCount * 2;
		mUsgChangedLU = new bool[mUsgChangedLULen];
	} 
	memset(mUsgChangedLU,0,sizeof(bool)*mItemSetCount); // all set to 0

	uint32 numUsgChangeLU = 0;
	uint32 numUsgChanged = 0;

	//uint32* abCnts = mCT->GetAlphabetCounts();
	uint32 curCTSize = mItemSetSorted->mCurrentCT->size();
	for(uint32 i = 0; i < curCTSize; i++) {
		std::pair<uint32,UsageClass> &i_candidate = mItemSetSorted->mCurrentCT->at(i);
		UsageClass &i_usgclass = i_candidate.second;

		if(i_candidate.first < abSize) {
			ItemSet *i_itemset = i_usgclass.GetItemSet();
			//uint32 abCnt = mCT->GetAlphabetCount(i_candidate.first);
			uint32 abCnt = i_itemset->GetUsageCount();
			if(abCnt != i_usgclass.GetUsageCount()){
				mUsgChangedIds[numUsgChanged++] = i_candidate.first;
				mUsgChangedLU[i_candidate.first] = 1;

				//ItemSet *i_itemset = i_usgclass.GetItemSet();
				//i_itemset->SetUsageCount(abCnt);
				i_usgclass.SetUsageCount(abCnt);
				//i_itemset->SetUsage(mCT->GetAlphabetUsage(i_candidate.first), abCnt);
			}
		} else if(i_usgclass.UsageChanged()) {
			mUsgChangedIds[numUsgChanged++] = i_candidate.first;
			mUsgChangedLU[i_candidate.first] = 1;
			i_usgclass.UpdateUsageCount();
		}
	}

	uint32 numChanged = numUsgChanged;
	uint32 numZapped = (uint32) deletedIDs.size();
	std::list<uint32>::iterator cit, cend = deletedIDs.end();
	for(cit = deletedIDs.begin(); cit != cend; cit++){
		mUsgChangedIds[numUsgChanged++] = *cit;
		mUsgChangedLU[*cit] = 1;
	}

	// Remove items from TopCandidates whose usage is changed or pruned
	size_t numCandidates = mTopCandidates.size();
	if(mEstStrategy == SlimMJ_USAGE) {
		for(uint32 i = 0; i < numCandidates; i++){
			CandidateCTGain *candidate = mTopCandidates.at(i);
			for(uint32 m = 0; m < numUsgChanged; m++){
				if(candidate->mI == mUsgChangedIds[m] || candidate->mJ == mUsgChangedIds[m]){
					delete candidate;
					mTopCandidates.erase(mTopCandidates.begin() + i);
					i--;
					numCandidates--;
					break;
				}
			}
		}
	} else if(mEstStrategy == SlimMJ_GAIN){

		unordered_map<uint32,unordered_set<uint32>>::iterator nce = mNoCa->end();
		for(uint32 i = 0; i < numCandidates; i++){
			bool updateHeuristic = true;
			CandidateCTGain *candidate = mTopCandidates.at(i);
				
			if(mUsgChangedLU[candidate->mI] == 1 || mUsgChangedLU[candidate->mJ] == 1) {
				delete candidate;	// we created it, we delete it
				mTopCandidates.erase(mTopCandidates.begin() + i);
				i--;
				numCandidates--;
				updateHeuristic = false;
			} 

			if(updateHeuristic){ //if not removed, update Heuristic
				//gain_db_ij
				double log_previous_countsum1 = log2(mPrevCountSum);//s
				double log_previous_new_countsum1 = log2((double)candidate->mNewCountSum);//s dash
				double log_countsum1 = log2((double)mCT->GetCurStats().usgCountSum);//s1
				double new_countsum1 = (double)mCT->GetCurStats().usgCountSum - mPrevCountSum + candidate->mNewCountSum;
				double log_new_countsum1 = log2(new_countsum1);//s1 dash

				double difference = - mPrevCountSum * log_previous_countsum1 + candidate->mNewCountSum * log_previous_new_countsum1 + mCT->GetCurStats().usgCountSum * log_countsum1 - new_countsum1 * log_new_countsum1;
				candidate->mHeuristic += difference;
				candidate->mNewCountSum = (uint32) new_countsum1;

				//gain_ct_ij
				double new_usage_i = (double) mItemSetSorted->GetUsageCount(candidate->mI) - candidate->mNewUsageIJ;
				double new_usage_j = (double) mItemSetSorted->GetUsageCount(candidate->mJ) - candidate->mNewUsageIJ;
				double previous_old_num_codes_with_non_zero_usage = (double)mPrevNumCTElems; // TODO: only count the ones with non-zero usage
				double previous_new_num_codes_with_non_zero_usage = previous_old_num_codes_with_non_zero_usage + 1;
				double old_num_codes_with_non_zero_usage = (double)mCurNumCTElems; // TODO: only count the ones with non-zero usage
				double new_num_codes_with_non_zero_usage = old_num_codes_with_non_zero_usage + 1;

				if (new_usage_i == 0.0){
					--previous_new_num_codes_with_non_zero_usage;
					--new_num_codes_with_non_zero_usage;
				}
				if (new_usage_j == 0.0){
					--previous_new_num_codes_with_non_zero_usage;
					--new_num_codes_with_non_zero_usage;
				}

				candidate->mHeuristic += log_previous_new_countsum1 * previous_new_num_codes_with_non_zero_usage - log_previous_countsum1 * previous_old_num_codes_with_non_zero_usage - log_new_countsum1 * new_num_codes_with_non_zero_usage + log_countsum1 * old_num_codes_with_non_zero_usage;
			}
		}
	}
	make_heap(mTopCandidates.begin(), mTopCandidates.end(), GreaterComparer());
	// -- update mNoCa

	// 1) zap lists corresponding to deleted items
	unordered_map<uint32,unordered_set<uint32>>::iterator nce = mNoCa->end();
	for(uint32 i=numChanged; i<numUsgChanged; i++) {
		if(mNoCa->find(mUsgChangedIds[i]) != nce)
			mNoCa->erase(mUsgChangedIds[i]);
	}

	// 2) empty list corresponding to our id
	for(uint32 i=0; i<numChanged; i++) {
		unordered_map<uint32,unordered_set<uint32>>::iterator nci = mNoCa->find(mUsgChangedIds[i]);
		if(nci != nce)
			nci->second.clear();
	}
	// 3) filter out our ids from other lists
	size_t curCTLen = mItemSetSorted->mCurrentCT->size();
	for(size_t j=0; j<curCTLen; j++) {

		pair<uint32,UsageClass> &elem = mItemSetSorted->mCurrentCT->at(j);
		unordered_map<uint32,unordered_set<uint32>>::iterator nci = mNoCa->find(elem.first);

		if(nci != nce) {
			unordered_set<uint32> &ncs = nci->second;
			unordered_set<uint32>::iterator ncse = ncs.end();
			for(size_t i=0; i<numUsgChanged; i++)
				if(ncs.find(mUsgChangedIds[i]) != ncse)
					ncs.erase(mUsgChangedIds[i]);
		} else {
			// insert
			mNoCa->insert(pair<uint32,unordered_set<uint32>>(elem.first,unordered_set<uint32>()));
		}
	}

	deletedIDs.clear();

	// Sort according to usage counts
	mItemSetSorted->SortElements();
}

ItemSet* SlimMJ::NextCandidate(double& candidateHeuristic, uint32 &candidateCount) {
	while (!mTopCandidates.empty()) { // || mMoreCandidates) {
		candidateCount++;
		if(candidateCount > mTopCandidates.size()){
			candidateCount = 0;
			return NULL;
		}

		CandidateCTGain *candidate = mTopCandidates.at(mTopCandidates.size() - candidateCount);
		mCurrentCandidate = candidate;
		if(candidate->mActualGain < 0){
			continue;
		}
		candidateHeuristic = candidate->mHeuristic;
		//double tmp = candidate->tmp;

		ItemSet* i = mItemSetSorted->GetItemSet(candidate->mI);
		ItemSet* j = mItemSetSorted->GetItemSet(candidate->mJ);

		ItemSet* ij = i->Union(j);

		// JV warns: only generate occurrences if CT needs these ! (CCCP doesnt)


		// Calculate support fast(er) using DBOccurs (which are automagically updated during union!)
		uint32 numoccs = ij->GetNumOccs();
		uint32 supp = 0;
		if(mDB->HasBinSizes()) {
			uint32* rids = ij->GetOccs();
			for (uint32 i = 0; i < numoccs; ++i)
				supp += mDB->GetRow(rids[i])->GetUsageCount();
		} else
			supp = numoccs;
		ij->SetSupport(supp);

		if (ij->GetSupport() < mMinSup) {
			delete ij;
			continue;
		}

		mDB->CalcStdLength(ij);

		if (mEstStrategy == SlimMJ_GAIN) {
			candidateHeuristic -= ij->GetStandardLength() + min(i->GetStandardLength(),j->GetStandardLength());	// this is already included in candidate generation
		}
		return ij;		
	}
	return NULL;
}

void SlimMJ::GenerateCandidates(){

	//list<uint32>::iterator rejected;

	#pragma region SLIM_USAGE
	if(mEstStrategy == SlimMJ_USAGE) {
		// pure usage based
		uint32 minUsage = 1;
		
		for(uint32 i = 0; i < mCurNumCTElems; ++i) {

			std::pair<uint32,UsageClass> &i_candidate = mItemSetSorted->mCurrentCT->at(i);
			UsageClass &i_usgclass = i_candidate.second;

			uint32 i_usage_count = i_usgclass.GetUsageCount();
			if(i_usage_count < mMinSup || i_usage_count < minUsage)
				continue;

			uint32* usage_i  = i_usgclass.GetUsages();

			for(uint32 j = i + 1; j < mCurNumCTElems; ++j) {
				minUsage = 1;

				std::pair<uint32,UsageClass> &j_candidate = mItemSetSorted->mCurrentCT->at(j);
				UsageClass &j_usgclass = j_candidate.second;

				uint32 minUniqueId = min(i_candidate.first,j_candidate.first);
				uint32 maxUniqueId = max(i_candidate.first,j_candidate.first);

				unordered_map<uint32,unordered_set<uint32>>::iterator nci = mNoCa->find(minUniqueId);
				unordered_set<uint32> &us = nci->second;
				if(us.find(maxUniqueId) != us.end()) {
					continue;
				}
				//mOMGCounter++;
				
				uint32 j_usage_count = j_usgclass.GetUsageCount();
				if (j_usage_count < minUsage || j_usage_count < mMinSup)
					continue;

				if(i_usgclass.HeuristicAvailable(j_candidate.first, i, j)) {
					continue;
				}
				uint32* usage_j  = j_usgclass.GetItemSet()->GetUsages();

				size_t est_ij_usage  = ArrayUtils::IntersectionLength(usage_i, i_usage_count, usage_j, j_usage_count, mMinSup-1);

				if (est_ij_usage >= mMinSup) {
					//CandidateMJ *candidate = new CandidateMJ(i_candidate.first, j_candidate.first, est_ij_usage, 0, 0, i, j);
					CandidateCTGain *candidate = new CandidateCTGain(i_candidate.first, j_candidate.first, est_ij_usage, 0, 0, i, j);
					mTopCandidates.push_back(candidate);
					push_heap(mTopCandidates.begin(),mTopCandidates.end(), GreaterComparer());
					i_usgclass.AddTopCandidateI(j_candidate.first, candidate);
					j_usgclass.AddTopCandidateJ(i_candidate.first, candidate);
				} else {
					nci->second.insert(maxUniqueId);
				}
			}
		}
	}
	#pragma endregion

	#pragma region SLIM_GAIN

	else if (mEstStrategy == SlimMJ_GAIN) {
		// normalized gain in bits based
		uint64 countSum = mCT->GetCurStats().usgCountSum;
		double log_countsum = log2((double)countSum);
		double best_ij_gain = mMinGainEst;
		
		size_t a = mItemSetSorted->mCurrentCT->size();

		for(size_t i=0; i < a; i++) {
			std::pair<uint32,UsageClass> &i_candidate = mItemSetSorted->mCurrentCT->at(i);//MG
			UsageClass &i_usgclass = i_candidate.second;

			// first estimate
			uint32 i_max_usage = i_usgclass.GetUsageCount();
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

			if (ii_gain <= best_ij_gain)
				break;

			uint32* usage_i = i_usgclass.GetUsages();

			for(size_t j = i + 1; j < mCurNumCTElems; ++j) {

				std::pair<uint32,UsageClass> &j_candidate = mItemSetSorted->mCurrentCT->at(j);
				UsageClass &j_usgclass = j_candidate.second;

				uint32 minUniqueId = min(i_candidate.first,j_candidate.first);
				uint32 maxUniqueId = max(i_candidate.first,j_candidate.first);

				unordered_map<uint32,unordered_set<uint32>>::iterator nci = mNoCa->find(minUniqueId);
				unordered_set<uint32> &us = nci->second;
				if(us.find(maxUniqueId) != us.end()) {
					continue;
				}
				//mOMGCounter++;

				uint32 j_max_usage = j_usgclass.GetUsageCount();
				if(j_max_usage < mMinSup)
					continue;
				
				if(i_usgclass.HeuristicAvailable(j_candidate.first, i, j)){
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
					nci->second.insert(maxUniqueId);
					break;
				}

				//// essentially just calculate how many usages are necessary at minimum to improve over best_gain
				//
				//double l1 = pow(2,-best_gain);
				//double l2 = pow(2, z);
				//double l = l1/l2;
				//double r = (zxy-zzy-zzx+zzz) / (c-z)
				//// minimising over this is monotone, so we can do this with binary search
				//// min_z = 1
				//// max_z = ij_max_usage
				// 

				uint32* usage_j = j_usgclass.GetUsages();
				size_t est_ij_usage  = ArrayUtils::IntersectionLength(usage_i, i_max_usage, usage_j, j_max_usage, mMinSup-1);

				if(est_ij_usage == 0 || est_ij_usage < mMinSup) {
					nci->second.insert(maxUniqueId);
					continue;
				} 
				
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


				double gain_ct_ij = 0.0;
				gain_ct_ij -= log_new_usage_ij;
				if (new_usage_i != old_usage_i) {
					if (new_usage_i != 0.0 && old_usage_i != 0.0) {
						gain_ct_ij -= log_new_usage_i;
						gain_ct_ij += log_old_usage_i;
					} else if (old_usage_i == 0.0) {
						gain_ct_ij += i_usgclass.GetStandardLength();
						gain_ct_ij -= log_new_usage_i;
					} else if (new_usage_i == 0.0) {
						gain_ct_ij -= i_usgclass.GetStandardLength();
						gain_ct_ij += log_old_usage_i;
					}
				}
				if (new_usage_j != old_usage_j) {
					if (new_usage_j != 0.0 && old_usage_j != 0.0) {
						gain_ct_ij -= log_new_usage_j;
						gain_ct_ij += log_old_usage_j;
					} else if (old_usage_j == 0.0) {
						gain_ct_ij += j_usgclass.GetStandardLength();
						gain_ct_ij -= log_new_usage_j;
					} else if (new_usage_j == 0.0) {
						gain_ct_ij += j_usgclass.GetStandardLength();
						gain_ct_ij -= log_old_usage_j;
					}
				}
				gain_ct_ij += new_num_codes_with_non_zero_usage * log_new_countsum;
				gain_ct_ij -= old_num_codes_with_non_zero_usage * log_old_countsum;
				// note: gain_ct_ij still misses estimate for ij.standardlength ! (this is calculated when candidate is evaluated)
				//		 we'll use a lower bound on complete gain_ct_ij

				double gain_ij = gain_db_ij - gain_ct_ij - min(i_usgclass.GetStandardLength(),j_usgclass.GetStandardLength()); 
				if ( gain_ij > 0.0 ) {
					CandidateCTGain *candidate = new CandidateCTGain(i_candidate.first, j_candidate.first, gain_ij, (uint32) new_countsum, (uint32) new_usage_ij, (uint32) i, (uint32) j);
					mTopCandidates.push_back(candidate);
					push_heap(mTopCandidates.begin(),mTopCandidates.end(), GreaterComparer());
					i_usgclass.AddTopCandidateI(j_candidate.first, candidate);
					j_usgclass.AddTopCandidateJ(i_candidate.first, candidate);
				} else {
					nci->second.insert(maxUniqueId);
				}
			}
		}
	}
	#pragma endregion

	sort_heap(mTopCandidates.begin(), mTopCandidates.end(), GreaterComparer());
	reverse(mTopCandidates.begin(), mTopCandidates.end());
}

void SlimMJ::FillCodeTable(){
	// MG style
	uint32 abLen = mCT->GetAlphabetSize();
	for(uint16 i=0; i<abLen; ++i) {
		ItemSet *item = ItemSet::CreateSingleton(mDB->GetDataType(), i, (uint32) mDB->GetAlphabetSize());
		uint32* dbrows = new uint32[mDB->GetAlphabetNumRows()[i]];
		memcpy(dbrows, mDB->GetAlphabetDbOccs()[i], mDB->GetAlphabetNumRows()[i] * sizeof(uint32));
		uint32 abCnt = mCT->GetAlphabetCount(i);
		item->SetDBOccurrence(dbrows, mDB->GetAlphabetNumRows()[i]);
		item->SetSupport(mDB->GetAlphabetNumRows()[i]);
		item->SetUsageCount(abCnt);
		item->SetUsage(mDB->GetAlphabetDbOccs()[i], abCnt);
		item->SetStandardLength(mDB->GetStdLength(i));
		item->SetUniqueID(i);

		// gcc doesnt like
		//mItemSetSorted->Insert(i, UsageClass(item));
		// so we do
		UsageClass uc = UsageClass(item); //
		mItemSetSorted->Insert(i, uc);
	}
}

list<uint32> SlimMJ::PrunePostAccept(CodeTable *ct) {
	islist *pruneList;
	// Build PruneList
	if (mPruneStrategy == PostAcceptPruneStrategy){
		pruneList = ct->GetPostPruneList();
	}
	else /* if(mPruneStrategy == PostEstimationPruning)*/{
		pruneList = ct->GetEstimatedPostPruneList();
	}
	CoverStats &stats = ct->GetCurStats();
	std::list<uint32> deletedIDs;

	islist::iterator iter;
	for(iter = pruneList->begin(); iter != pruneList->end(); ++iter) {
		ItemSet *is = (ItemSet*)(*iter);

		if(ct->GetUsageCount(is) == 0) {
			deletedIDs.push_back(is->GetUniqueID());
			ct->Del(is, true, false); // zap immediately
		} else {
			ct->Del(is, false, false);
			ct->CoverDB(stats);
			if(ct->GetCurSize() <= ct->GetPrevSize()) {
				deletedIDs.push_back(is->GetUniqueID());
				ct->CommitLastDel(true); // zap when we're not keeping a list
				if (mPruneStrategy == PostAcceptPruneStrategy){
					ct->UpdatePostPruneList(pruneList, iter);
				}
				else/* if(mPruneStrategy == PostEstimationPruning)*/{
					ct->UpdateEstimatedPostPruneList(pruneList, iter);
				}
			} else {
				ct->RollbackLastDel();
			}
		}
	}
	delete pruneList;
	return deletedIDs;
}

#endif // BLOCK_COMPRESS_NG
