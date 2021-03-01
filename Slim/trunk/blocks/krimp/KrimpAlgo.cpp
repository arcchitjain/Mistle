#include "../global.h"
#include <time.h>
#include <omp.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "codetable/CodeTable.h"

#include "KrimpAlgo.h"

#include "CoverFullAlgo.h"
//#include "ParallelCoverFullAlgo.h"

#include "codetable/coverpartial/CoverPartial.h"
#include "codetable/coverpartial/ParallelCoverPartial.h"

#include <SystemUtils.h>

KrimpAlgo::KrimpAlgo() {
	mDB = NULL;
	mCT = NULL;
	mISC = NULL;
	mNumCandidates = 0;
	mReportFile = NULL;
	mCTLogFile = NULL;
	mPruneStrategy = NoPruneStrategy;
	mNeedsOccs = false;

	mScreenReportTime = 0;
	mScreenReportCandidateIdx = 0;
	mScreenReportCandPerSecond = 0;
	mScreenReportCandidateDelta = 0;

	mWriteCTLogFile = true;
	mWriteReportFile = true;
	mWriteProgressToDisk = true;

	mReportSupDelta = 0;
	mReportCndDelta = 0;
	mReportOnAccept = false;
	mReportSupType = ReportAll;
	mReportCndType = ReportAll;
	mReportAccType = ReportAll;

	mCompressionStartTime = omp_get_wtime();
	mStartTime = time(NULL);
}

KrimpAlgo::~KrimpAlgo() {
	mDB = NULL;		// je delete de Database maar in dezelfde class als dat je 'm aangemaakt hebt!
	mISC = NULL;	// je delete de ItemSetCollection maar in dezelfde class als dat je 'm aangemaakt hebt!
	mCT = NULL;		// je delete de CodeTable maar in dezelfde class als dat je 'm aangemaakt hebt!
	if(mReportFile)
		CloseReportFile();
	if(mCTLogFile)
		CloseCTLogFile();
}

/* KrimpBase claims ownership of db. */
void KrimpAlgo::UseThisStuff(Database *db, ItemSetCollection *isc, ItemSetType type, CTInitType ctinit, bool fastGenerateDBOccs) {
	mDB = db;
	mISC = isc;
	if(mCT != NULL) {
		mCT->UseThisStuff(db, type, ctinit, isc->GetMaxSetLength(), isc->GetMinSupport());
		if(mCT->NeedsDBOccurrences()) {
			mNeedsOccs = true;
			if(fastGenerateDBOccs)
				mDB->EnableFastDBOccurences();
		}
	}
	if(mISC != NULL) {
		mNumCandidates = mISC->GetNumItemSets();
	}
}

CodeTable* KrimpAlgo::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */) {
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
		ProgressToScreen(curM, numM);

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

void KrimpAlgo::WriteEncodedRowLengthsToFile() {
	string encLenFileName = mOutDir + "/el-" + mTag + ".txt";
	FILE *fp = fopen(encLenFileName.c_str(), "w");

	uint32 numRows = mDB->GetNumRows();
	ItemSet **rows = mDB->GetRows();
	uint32 count;
	double enclen;

	uint32 numClasses = mDB->GetNumClasses();
	uint16* classes = mDB->GetClassDefinition();
	uint16 curClass;
	for(uint32 r=0; r<numRows; r++) {
		curClass = 0;
		for(uint32 c=0; c<numClasses; c++) {
			if(rows[r]->IsItemInSet(classes[c])) {
				curClass = classes[c];
				break;
			}
		}

		count = rows[r]->GetUsageCount();
		enclen = mCT->CalcTransactionCodeLength(rows[r]);

		fprintf(fp, "%d\t%d\t%d\t%.2f\n", r, curClass, count, enclen);
	}

	fclose(fp);
}


void KrimpAlgo::PrunePreAccept(CodeTable *ct, ItemSet *m) {
	// Build PruneList
	islist *pruneList = ct->GetPrePruneList();
	CoverStats &pruneStats = ct->GetPrunedStats();

	islist::iterator iter, lend=pruneList->end();
	for(iter = pruneList->begin(); iter != lend; ++iter) {
		CoverStats &curStats = ct->GetCurStats();
		pruneStats = curStats;
		pruneStats.encSize += 1.0;

		ItemSet *is = (ItemSet*)(*iter);
		if(is == m)
			continue;

		ct->Del(is, false, true);
		ct->CoverDB(pruneStats);
		if(ct->GetPrunedSize() <= ct->GetCurSize()) {
			ct->CommitLastDel(false); // zap when we're not keeping a list
		} else {
			ct->RollbackLastDel();
		}
	}
	delete pruneList;
}

void KrimpAlgo::PrunePostAccept(CodeTable *ct) {
	// Build PruneList
	islist *pruneList = ct->GetPostPruneList();
	CoverStats &stats = ct->GetCurStats();

	islist::iterator iter;
	for(iter = pruneList->begin(); iter != pruneList->end(); ++iter) {
		ItemSet *is = (ItemSet*)(*iter);

		if(ct->GetUsageCount(is) == 0) {
			ct->Del(is, true, false); // zap immediately
		} else {
			ct->Del(is, false, false);
			ct->CoverDB(stats);
			if(ct->GetCurSize() <= ct->GetPrevSize()) {
				ct->CommitLastDel(true); // zap when we're not keeping a list
				ct->UpdatePostPruneList(pruneList, iter);
			} else {
				ct->RollbackLastDel();
			}
		}
	}
	delete pruneList;
}

void KrimpAlgo::PruneSanitize(CodeTable *ct) {
	// Build PruneList
	islist *pruneList = ct->GetSanitizePruneList();
	islist::iterator iter;
	for(iter = pruneList->begin(); iter != pruneList->end(); ++iter) {
		ItemSet *is = (ItemSet*)(*iter);
		ct->Del(is, true, false); // zap immediately
	}
	delete pruneList;
}


KrimpAlgo* KrimpAlgo::CreateAlgo(const string &KrimpBasename, ItemSetType type) {
	// format is like
	// [parallel-[cands-|cover-]][KrimpBase]

	// parallel shizzle
	if(KrimpBasename.compare(0, 9, "parallel-") == 0) {
	//	if(KrimpBasename.compare(0, 18, "parallel-coverfull") == 0)
	//		return new ParallelCoverFull(CodeTable::Create(KrimpBasename.substr(9), type));
		if(KrimpBasename.compare(0, 26, "parallel-coverpartial") == 0)
			return new ParallelCoverPartial(CodeTable::Create(KrimpBasename.substr(9), type));
	} // also parallel shizzle
	else if(Bass::GetNumThreads() > 1 && KrimpBasename.compare(0, 7, "single-") != 0) {
	//	if(KrimpBasename.compare(0, 9, "coverfull") == 0)
	//		return new ParallelCoverFull(CodeTable::Create(KrimpBasename, type));
		if(KrimpBasename.compare(0, 12, "coverpartial") == 0 ||
		   KrimpBasename.compare(0, 14, "cccoverpartial") == 0)
				return new ParallelCoverPartial(CodeTable::Create(KrimpBasename, type));
	} // single threaded

	string stKrimpBasename = KrimpBasename;
	if(KrimpBasename.compare(0,7,"single-") == 0) {
		stKrimpBasename = KrimpBasename.substr(7);
	} 
	
	if(stKrimpBasename.compare(0,9,"coverfull") == 0)
		return new CoverFullAlgo(CodeTable::Create(stKrimpBasename, type));
	if(stKrimpBasename.compare(0,12,"coverpartial") == 0 ||
		stKrimpBasename.compare(0,13,"ccoverpartial") == 0 ||
		stKrimpBasename.compare(0,14,"cccoverpartial") == 0)
		return new CoverPartial(CodeTable::Create(stKrimpBasename, type));

	return NULL; // !!! throw 'ha! die krijg je mooi niet...'
}

void KrimpAlgo::ProgressToScreen(const uint64 curCandidate, const uint64 total) {
	if(Bass::GetOutputLevel() > 0) {
		uint16 p = (uint16) (100 * curCandidate / total);
		uint64 canDif = curCandidate - mScreenReportCandidateIdx;

		if(mProgress < p || canDif >= mScreenReportCandidateDelta) {
			// Candidates per Second uitrekenen, update per 30s
			time_t curTime = time(NULL);
			if(canDif >= mScreenReportCandidateDelta) {
				uint32 timeDif = (uint32) (curTime - mScreenReportTime);
				mScreenReportCandPerSecond = (uint32) (curCandidate - mScreenReportCandidateIdx) / (timeDif > 0 ? timeDif : 1);
				mScreenReportCandidateDelta = (mScreenReportCandPerSecond + 1) * 30;
				mScreenReportTime = curTime;
				mScreenReportCandidateIdx = curCandidate;
			}

			mProgress = p;

			// Percentage uitrekenen
			int d = p % 4;
			char c = '*';
			switch(d) {
				case 0 : c = '-'; break;
				case 1 : c = '\\'; break;
				case 2 : c = '|'; break;
				default : c = '/';
			}
			if(Bass::GetOutputLevel() == 3)
				printf(" %c Progress:\t\t%d%% (%" I64d ") %" I64d " c/s\n", c, p, curCandidate);
			else {
				if(mScreenReportCandPerSecond == 0)
					printf(" %c Progress:\t\t%d%% (%" I64d ") ? c/s  \t\t\r", c, p, curCandidate);
				else {
					uint32 numElS = (uint32) (curTime - mStartTime);
					uint32 numSEl = numElS % 60;
					uint32 numMEl = ((numElS - numSEl) / 60) % 60;
					uint32 numHEl = (numElS - numSEl - numMEl*60) / 3600;

					char elapsed[21];
					uint32 lenEl = 0;
					if(numHEl > 0)
						lenEl = sprintf_s(elapsed, 20, "%dh%02d", numHEl, numMEl);
					else if(numMEl > 0)
						lenEl = sprintf_s(elapsed, 20, "%dm%02d", numMEl, numSEl);
					else
						lenEl = sprintf_s(elapsed, 20, "%ds", numSEl);

					uint32 numRmS = (uint32)((mNumCandidates - curCandidate) / mScreenReportCandPerSecond);
					uint32 numSRm = numRmS % 60;
					uint32 numMRm = ((numRmS - numSRm) / 60) % 60;
					uint32 numHRm = (numRmS - numSRm - (numMRm * 60)) / 3600;

					char remaining[21];
					uint32 lenRm = 0;
					if(numHRm > 0) 
						lenRm = sprintf_s(remaining, 20, "%dh%02d", numHRm, numMRm);
					else if(numMRm > 0)
						lenRm = sprintf_s(remaining, 20, "%dm%02d", numMRm, numSRm);
					else if(numSRm > 0)
						lenRm = sprintf_s(remaining, 20, "%ds", numSRm);
					else {
						lenRm = 3;
						remaining[0] = '<'; remaining[1] = '1'; remaining[2] = 's'; remaining[3] = 0;
					}

					string spaces = "                                                                      ";

					int32 printLen = 0;
					printLen = printf_s(" %c Progress:\t\t%d%% (%" I64d ") %u c/s (%s +/- %s)", c, p, 
						curCandidate,mScreenReportCandPerSecond, elapsed, remaining);
					printf_s("%s\r", spaces.substr(0,max(0,68-printLen)).c_str());
				}
			}
		}
	}
}

void KrimpAlgo::ProgressToDisk(CodeTable *ct, const uint32 curSup, const uint32 curLength, const uint64 numCands, const bool writeCodetable, const bool writeStats) {
	if(writeStats && mReportFile) {
		CoverStats stats = ct->GetCurStats();
		fprintf(mReportFile, "%d;%d;%" I64d ";%d;%d;%d;%" I64d ";%.0f;%.0f;%.0f;%d;%d\n", curSup, curLength, numCands, stats.alphItemsUsed, stats.numSetsUsed, mCT->GetCurNumSets(), stats.usgCountSum, stats.encDbSize, stats.encCTSize, stats.encSize, time(NULL) - mStartTime, SystemUtils::GetProcessMemUsage());
		fflush(mReportFile);
	}

	if(writeCodetable) {
		char buf[255];
		if(!writeStats) {
			sprintf_s(buf, 255, "%sct-%s-%d.ct", mOutDir.c_str(),mTag.c_str(),curSup);
		} else
			sprintf_s(buf, 255, "%sct-%s-%d-%" I64d ".ct", mOutDir.c_str(),mTag.c_str(),curSup,numCands);

		string ctFilename = buf;
		ct->WriteToDisk(ctFilename);

		if(numCands == 0) { // write max.sup
			string f = mOutDir + "/max.sup";
			FILE *fp = fopen(f.c_str(), "w");
			f = string(buf) + "\n";
			fputs(f.c_str(), fp);
			fclose(fp);
		}
	}
}
void KrimpAlgo::OpenCTLogFile() {
	string ctLogFilename = mOutDir;
	ctLogFilename.append("/ct-");
	ctLogFilename.append(mTag);
	ctLogFilename.append(".ctl");
	mCTLogFile = fopen(ctLogFilename.c_str(), "w");
}
void KrimpAlgo::CloseCTLogFile() {
	if(mCTLogFile) {
		fclose(mCTLogFile);
		mCTLogFile = NULL;
	}
}
bool KrimpAlgo::OpenReportFile(const bool header) {
	string s = mOutDir;
	s.append("/report-");
	s.append(mTag);
	s.append(".csv");
	mReportFile = fopen(s.c_str(), "a");
	if(!mReportFile) {
		printf("Cannot open report file for writing!\nFilename: %s\n", s.c_str());
		return false;
	}
	if(header) {
		fprintf(mReportFile, "minSup;length;numCands;numAlphUsed;numSetsUsed;numSets;countSum;dbSize;ctSize;totalSize;seconds;memUsage\n");
		fflush(mReportFile);
	}
	return true;
}
void KrimpAlgo::CloseReportFile() {
	if(mReportFile) {
		fclose(mReportFile);
		mReportFile = NULL;
	}
}

void KrimpAlgo::LoadCodeTable(const uint32 resumeSup, const uint64 resumeCand) {
	string ctFile = mOutDir + "/ct-" + mTag + "-";
	char tmp[10];
	_itoa(resumeSup, tmp, 10);
	ctFile.append(tmp);
	if(resumeCand != 0) {
		ctFile.append("-");
		_i64toa(resumeCand, tmp, 10);
		ctFile.append(tmp);
	}
	ctFile += ".ct";
	mCT->ReadFromDisk(ctFile, false);
	mCT->CoverDB(mCT->GetCurStats());
	mCT->CommitAdd(); // mAdded = NULL; prevStats = curStats;
}

string KrimpAlgo::GetShortName() {
	 return mCT->GetShortName();
}

string KrimpAlgo::GetShortPruneStrategyName() {
	return PruneStrategyToString(mPruneStrategy);
}

string KrimpAlgo::PruneStrategyToString(PruneStrategy pruneStrategy) {
	if(pruneStrategy == PreAcceptPruneStrategy) {
		return "pre";
	} else if(pruneStrategy == PostAcceptPruneStrategy) {
		return "pop";
	} else if(pruneStrategy == SanitizeOfflinePruneStrategy) {
		return "post";
	} else if (pruneStrategy == PostEstimationPruning){
		return "pep";
	} else /*if(pruneStrategy == NoPruneStrategy)*/ {
		return "nop";
	}
}
string KrimpAlgo::PruneStrategyToName(PruneStrategy pruneStrategy) {
	if(pruneStrategy == PreAcceptPruneStrategy) {
		return "pre-accept-prune";
	} else if(pruneStrategy == PostAcceptPruneStrategy) {
		return "post-accept-prune";
	} else if(pruneStrategy == SanitizeOfflinePruneStrategy) {
		return "offline sanitize";
	} else if (pruneStrategy == PostEstimationPruning){
		return "post-estimation-prune";
	} else /*if(pruneStrategy == NoPruneStrategy)*/ {
		return "no-prune";
	}
}

PruneStrategy KrimpAlgo::StringToPruneStrategy(string pruneStrategyStr) {
	// set `pruneStrategy` and output prune strategy to screen
	if(pruneStrategyStr.compare("nop") == 0 ) {
		return NoPruneStrategy;
	} else if(pruneStrategyStr.compare("pop") == 0) {
		return PostAcceptPruneStrategy;
	} else if(pruneStrategyStr.compare("pre") == 0) {
		return PreAcceptPruneStrategy;
	} else if (pruneStrategyStr.compare("post") == 0) {
		return SanitizeOfflinePruneStrategy;
	} else if (pruneStrategyStr.compare("pep") == 0) {
		return PostEstimationPruning;
	} else 
		THROW("Unknown Pruning Strategy: " + pruneStrategyStr);
}

ReportType KrimpAlgo::StringToReportType(string reportTypeStr) {
	if(reportTypeStr.compare("all") == 0)
		return ReportAll;
	else if(reportTypeStr.compare("ct") == 0)
		return ReportCodeTable;
	else if(reportTypeStr.compare("stats") == 0)
		return ReportStats;
	else if(reportTypeStr.compare("none") == 0)
		return ReportNone;
	THROW("SAY WHUT??");
}

void KrimpAlgo::SetWriteFiles(const bool write) {
	mWriteCTLogFile = write;
	mWriteProgressToDisk = write;
	mWriteReportFile = write;
}
