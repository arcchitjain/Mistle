#ifdef BLOCK_TILING
#ifdef BLOCK_SLIM

#include "../global.h"
#include <time.h>
#include <omp.h>
#include <unordered_set>
#include <cassert>
#include <sstream>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>
#include <SystemUtils.h>


// tiles/tiling
#include <tiles/TileMiner.h>

#include "SlimStar.h"

SlimStar::SlimStar(CodeTable* ct, HashPolicyType hashPolicy, string typeCT, Config* config)
: SlimAlgo(ct, hashPolicy, config), mTypeCT(typeCT)
{
	mWriteLogFile = true;

	// TODO: Currently only CFOCodeTable can encode DBs correctly
	if (mTypeCT == "coverfull")
		mTypeCT = "coverpartial-orderly-length";

	mMaxTileLength = mConfig->Read<uint32>("maxTileLength", UINT32_MAX_VALUE);
}

string SlimStar::GetShortName() {
	stringstream ss;
	if (mMaxTileLength != UINT32_MAX_VALUE)
		ss << mMaxTileLength << "-";
	ss << SlimAlgo::GetShortName();
	return ss.str();
}

CodeTable* SlimStar::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */)
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
	printf(" * Start:\t\t(%da,%du,%"",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);

	CoverStats &curStats = mCT->GetCurStats();

	// TODO: This if for debugging purposes only as it only works with CFCodeTable
	if (!mCTFile.empty())
		LoadCodeTable(mCTFile);

	ItemSet* minCandidate;
	double minSize = 0;
	uint64 numIterations = 0;
	uint64 numCandidates = 0;
	do {
		if(mWriteProgressToDisk) {
			if (numIterations == 0) {
				ProgressToDisk(mCT, 0 /* TODO: should read maximum support */, 0 /* TODO: curLength */, numIterations, true, true);
			} else if (mReportIteration) {
				ProgressToDisk(mCT, minCandidate->GetSupport(), minCandidate->GetLength(), numCandidates, mReportIterType == ReportCodeTable || mReportIterType == ReportAll, mReportIterType == ReportStats || mReportIterType == ReportAll);
			}
		}

		Database* encodedDB;
		string strTempCT = mOutDir + "ct-temporary.ct";
		mCT->WriteToDisk(strTempCT);
		CodeTable *tempCT = CodeTable::LoadCodeTable(strTempCT, mDB, mTypeCT);
		stringstream ss;
		ss << "encoded-db-" << numIterations;
		encodedDB = tempCT->EncodeDB(true, mOutDir + ss.str());
		encodedDB->SetName(ss.str());
		Database::WriteDatabase(encodedDB, ss.str(), mOutDir);
		delete tempCT;



		minCandidate = NULL;
		minSize = mCT->GetCurSize();
		uint64 topK = 1;
		while (!minCandidate) {
			TileMiner tm;
			tm.SetMinerVersion(1);
			tm.SetMinTileLength(2); // We definitely want to merge 2 codetable elements
			tm.SetMaxTileLength(mMaxTileLength);
			tm.SetTopK((uint32) topK);
			if (mWriteLogFile) fprintf(mLogFile, "TOP-K: %d\n", (uint32) topK);
			if (mWriteLogFile) fprintf(mLogFile, "Memory usage before mining: %d\n", SystemUtils::GetProcessMemUsage());
			ItemSetCollection* isc = tm.MinePatternsToMemory(encodedDB);
			if (mWriteLogFile) fprintf(mLogFile, "Memory usage after mining: %d\n", SystemUtils::GetProcessMemUsage());

			for (uint64 i = topK; i <= isc->GetNumItemSets(); ++i) {
				++numCandidates;

				m = isc->GetNextItemSet();

				if (mWriteLogFile) fprintf(mLogFile, "Maximum Tile (encoded): %s\n", m->ToCodeTableString().c_str());

				// Translate tile in encoded-space to decoded-space
				uint16* values = m->GetValues();
				ItemSet** encodedCD = encodedDB->GetColumnDefinition();
				ItemSet* decoded_m = ItemSet::CreateEmpty(mDB->GetDataType(), mDB->GetAlphabetSize());
				for (uint32 i = 0; i < m->GetLength(); ++i) {
					decoded_m->Unite(encodedCD[values[i]]);
				}
				delete[] values;
				delete m;
				m = decoded_m;

				if(mNeedsOccs)
					mDB->DetermineOccurrences(m);
				mDB->CalcSupport(m);

				if (m->GetSupport() < mMinSup) {
					delete m;
					continue;
				}

				mDB->CalcStdLength(m); // BUGFIX: itemset has no valid standard encoded size
				m->SetUsageCount(0);

				if (mWriteLogFile) fprintf(mLogFile, "Maximum Tile (candidate): %s\n", m->ToCodeTableString().c_str());

				mCT->Add(m);
				mCT->CoverDB(curStats);
				if(curStats.encDbSize < 0) {
					THROW("dbSize < 0. Dat is niet goed.");
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

			if(minCandidate) {
				mCT->Add(minCandidate);
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

				++numIterations;
				if (mWriteLogFile) fprintf(mLogFile, "Maximum Tile (accepted): %s\n\n", minCandidate->ToCodeTableString().c_str());
			} else {
				if (isc->GetNumItemSets() < topK) {
					delete isc;
					break; // Bail out we have already checked all this tiles...
				} else {
					topK = isc->GetNumItemSets() + 1;
				}
			}

			delete isc;
		}
		//FileUtils::RemoveFile(strTempCT);
		delete encodedDB;
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
#endif // BLOCK_TILES