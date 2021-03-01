//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#include "../global.h"
#include <time.h>
#include <omp.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "CodeTable.h"

#include "Algo.h"

#include "coverfull/CoverFull.h"
#include "coverfull/ParallelCoverFull.h"

#include "coverpartial/CoverPartial.h"
#include "coverpartial/ParallelCoverPartial.h"

//#include "ParallelCoverAlgo.h"

Algo::Algo() {
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

Algo::~Algo() {
	mDB = NULL;		// je delete de Database maar in dezelfde class als dat je 'm aangemaakt hebt!
	mISC = NULL;	// je delete de ItemSetCollection maar in dezelfde class als dat je 'm aangemaakt hebt!
	mCT = NULL;		// je delete de CodeTable maar in dezelfde class als dat je 'm aangemaakt hebt!
	if(mReportFile)
		CloseReportFile();
	if(mCTLogFile)
		CloseCTLogFile();
}

/* Algo claims ownership of db. */
void Algo::UseThisStuff(Database *db, ItemSetCollection *isc, ItemSetType type, CTInitType ctinit, bool fastGenerateDBOccs) {
	mDB = db;
	mISC = isc;
	if(mCT != NULL) {
		mCT->UseThisStuff(db, type, ctinit, isc->GetMaxSetLength(), isc->GetMinSupport());
		if(mCT->NeedsDBOccurrences()) {
			mNeedsOccs = true;
			if(fastGenerateDBOccs)
				mDB->EnableFastDBOccurences();
		}
		//if(mISC != NULL)
		//	mISC->SetProvideDBOccurrences(mCT->NeedsDBOccurrences(),fastGenerateDBOccs);
	}
	if(mISC != NULL) {
		mNumCandidates = mISC->GetNumItemSets();
	}
}

CodeTable* Algo::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */) {
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets(), nextReportCnd;
	mNumCandidates = numM;
	int32 curSup=-1, nextReportSup, prevSup=-1;
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
#if defined (_MSC_VER) && defined (_WINDOWS)
	printf(" * Start:\t\t(%da,%du,%I64d,%.0Lf,%.0Lf,%.0Lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
#elif defined (__GNUC__) && defined (__unix__)
	printf(" * Start:\t\t(%da,%du,%lu,%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
#endif

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
				ProgressToDisk(mCT, curSup, curM, true, true);
				// !!! nextReportSup moet niet 0-based, maar minSup-based zijn (dus (curSup - minSup) % mRepSupD == 0, ofzo
				nextReportSup = (mReportSupDelta == 0) ? 0 : curSup - (((curSup-mISC->GetMinSupport()) % mReportSupDelta) == 0 ? mReportSupDelta : (curSup-mISC->GetMinSupport()) % mReportSupDelta);
			} else if(mReportOnAccept == true && candidateAccepted == true) {
				// we moeten alle accepts rapporteren
				ProgressToDisk(mCT, curSup, curM, mReportAccType == ReportCodeTable || mReportAccType == ReportAll, mReportAccType == ReportStats || mReportAccType == ReportAll);
			} else if(curSup < nextReportSup) {		// curSup < nRS, dus we moeten volledig reporten!
				ProgressToDisk(mCT, nextReportSup, curM, mReportSupType == ReportCodeTable || mReportSupType == ReportAll, mReportSupType == ReportStats || mReportSupType == ReportAll);
				nextReportSup -= mReportSupDelta;
				if(mReportCndDelta != 0)
					nextReportCnd = curM + mReportCndDelta;
				if(nextReportSup < 1)
					nextReportSup = 1;
				while(nextReportSup > curSup) {
					ProgressToDisk(mCT, nextReportSup, curM, mReportSupType == ReportCodeTable || mReportSupType == ReportAll, mReportSupType == ReportStats || mReportSupType == ReportAll);
					nextReportSup -= mReportSupDelta;
				}
			} else if(curM == nextReportCnd) {
				ProgressToDisk(mCT, curSup, curM, mReportCndType == ReportCodeTable || mReportCndType == ReportAll, mReportCndType == ReportStats || mReportCndType == ReportAll);
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

		if(mPruneStrategy == PostAcceptPruneStrategy) {						// Post-decide On-the-fly pruning
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
	}
	if(mPruneStrategy == SanitizeOfflinePruneStrategy) {				// Sanitize post-pruning
		PruneSanitize(mCT);
	}

	double timeCompression = omp_get_wtime() - mCompressionStartTime;
#if (_WINDOWS)
	printf(" * Time:\t\tCompressing the database took %f seconds.\t\t\n", timeCompression);
#elif (__unix__)
	printf(" * Time:    \t\tCompressing the database took %f seconds.\t\t\n", timeCompression);
#endif

	stats = mCT->GetCurStats();
#if defined (_MSC_VER) && defined (_WINDOWS)
	printf(" * Result:\t\t(%da,%du,%I64d,%.0Lf,%.0Lf,%.0Lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
#elif defined (__GNUC__) && defined (__unix__)
	printf(" * Result:\t\t(%da,%du,%lu,%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
#endif
	if(mWriteProgressToDisk == true) {
		ProgressToDisk(mCT, mISC->GetMinSupport(), numM, true, true);
	}
	CloseCTLogFile();
	CloseReportFile();

	mCT->EndOfKrimp();

	return mCT;
}

void Algo::WriteEncodedRowLengthsToFile() {
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


void Algo::PrunePreAccept(CodeTable *ct, ItemSet *m) {
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

void Algo::PrunePostAccept(CodeTable *ct) {
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

void Algo::PruneSanitize(CodeTable *ct) {
	// Build PruneList
	islist *pruneList = ct->GetSanitizePruneList();
	islist::iterator iter;
	for(iter = pruneList->begin(); iter != pruneList->end(); ++iter) {
		ItemSet *is = (ItemSet*)(*iter);
		ct->Del(is, true, false); // zap immediately
	}
	delete pruneList;
}


Algo* Algo::CreateAlgo(const string &algoname, ItemSetType type) {
	// format is like
	// [parallel-[cands-|cover-]][algo]

	// parallel shizzle
	if(algoname.compare(0, 9, "parallel-") == 0) {
		if(algoname.compare(0, 18, "parallel-coverfull") == 0)
			return new ParallelCoverFull(CodeTable::Create(algoname.substr(9), type));
		if(algoname.compare(0, 26, "parallel-coverpartial") == 0)
			return new ParallelCoverPartial(CodeTable::Create(algoname.substr(9), type));
	} // also parallel shizzle
	else if(Bass::GetNumThreads() > 1 && algoname.compare(0, 7, "single-") != 0) {
		if(algoname.compare(0, 9, "coverfull") == 0)
			return new ParallelCoverFull(CodeTable::Create(algoname, type));
		if(algoname.compare(0, 12, "coverpartial") == 0 ||
		   algoname.compare(0, 14, "cccoverpartial") == 0)
				return new ParallelCoverPartial(CodeTable::Create(algoname, type));
	} // single threaded

	string stAlgoname = algoname;
	if(algoname.compare(0,7,"single-") == 0) {
		stAlgoname = algoname.substr(7);
	} 
	
	if(stAlgoname.compare(0,9,"coverfull") == 0)
		return new CoverFull(CodeTable::Create(stAlgoname, type));
	if(stAlgoname.compare(0,12,"coverpartial") == 0 ||
		stAlgoname.compare(0,13,"ccoverpartial") == 0 ||
		stAlgoname.compare(0,14,"cccoverpartial") == 0)
		return new CoverPartial(CodeTable::Create(stAlgoname, type));

	return NULL; // !!! throw 'ha! die krijg je mooi niet...'
}

void Algo::ProgressToScreen(const uint64 curCandidate, const uint64 total) {
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
#if defined (_MSC_VER) && defined (_WINDOWS)
				printf(" %c Progress:\t\t%d%% (%I64d) %I64d c/s\n", c, p, curCandidate);
#elif defined (__GNUC__) && defined (__unix__)
				printf(" %c Progress:\t\t%d%% (%lu) %lu c/s\n", c, p, curCandidate);
#endif
			else {
				if(mScreenReportCandPerSecond == 0)
#if defined (_MSC_VER) && defined (_WINDOWS)
					printf(" %c Progress:\t\t%d%% (%I64d) ? c/s  \t\t\r", c, p, curCandidate);
#elif defined (__GNUC__) && defined (__unix__)
					printf(" %c Progress:\t\t%d%% (%lu) ? c/s  \t\t\r", c, p, curCandidate);
#endif
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
#if defined (_MSC_VER) && defined (_WINDOWS)
					printLen = printf_s(" %c Progress:\t\t%d%% (%I64u) %u c/s (%s +/- %s)", c, p, 
						curCandidate,mScreenReportCandPerSecond, elapsed, remaining);
#elif defined (__GNUC__) && defined (__unix__)
					printLen = printf_s(" %c Progress:\t\t%d%% (%lu) %u c/s (%s +/- %s)", c, p, 
						curCandidate,mScreenReportCandPerSecond, elapsed, remaining);
#endif
					printf_s("%s\r", spaces.substr(0,max(0,68-printLen)).c_str());
				}
			}
		}
	}
}

void Algo::ProgressToDisk(CodeTable *ct, const uint32 curSup, const uint64 numCands, const bool writeCodetable, const bool writeStats) {
	if(writeStats) {
		CoverStats stats = ct->GetCurStats();
#if defined (_MSC_VER) && defined (_WINDOWS)
		fprintf(mReportFile, "%d;%I64d;%d;%d;%d;%I64d;%.0f;%.0f;%.0f;%d\n", curSup, numCands, stats.alphItemsUsed, stats.numSetsUsed, mCT->GetCurNumSets(), stats.usgCountSum, stats.encDbSize, stats.encCTSize, stats.encSize, time(NULL) - mStartTime);
#elif defined (__GNUC__) && defined (__unix__)
		fprintf(mReportFile, "%d;%lu;%d;%d;%d;%lu;%.0f;%.0f;%.0f;%lu\n", curSup, numCands, stats.alphItemsUsed, stats.numSetsUsed, mCT->GetCurNumSets(), stats.usgCountSum, stats.encDbSize, stats.encCTSize, stats.encSize, time(NULL) - mStartTime);
#endif
		fflush(mReportFile);
	}

	if(writeCodetable) {
		/*string ctFilename = mOutDir;
		ctFilename.append("/ct-");
		ctFilename.append(mTag);
		ctFilename.append("-");
		char buf[20];
		_itoa(curSup, buf, 10);
		ctFilename.append(buf);
		if(!writeStats) {
			_itoa(numCands, buf, 10);
			ctFilename.append("-");
			ctFilename.append(buf);
		}
		ctFilename.append(".ct");
		ct->WriteToDisk(ctFilename);*/
		char buf[255];
		if(!writeStats) {
			sprintf_s(buf, 255, "%sct-%s-%d.ct", mOutDir.c_str(),mTag.c_str(),curSup);
		} else
#if defined (_MSC_VER) && defined (_WINDOWS)
			sprintf_s(buf, 255, "%sct-%s-%d-%I64d.ct", mOutDir.c_str(),mTag.c_str(),curSup,numCands);
#elif defined (__GNUC__) && defined (__unix__)
			sprintf_s(buf, 255, "%sct-%s-%d-%lu.ct", mOutDir.c_str(),mTag.c_str(),curSup,numCands);
#endif

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
void Algo::OpenCTLogFile() {
	string ctLogFilename = mOutDir;
	ctLogFilename.append("/ct-");
	ctLogFilename.append(mTag);
	ctLogFilename.append(".ctl");
	mCTLogFile = fopen(ctLogFilename.c_str(), "w");
}
void Algo::CloseCTLogFile() {
	if(mCTLogFile) {
		fclose(mCTLogFile);
		mCTLogFile = NULL;
	}
}
bool Algo::OpenReportFile(const bool header) {
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
		fprintf(mReportFile, "minSup;numCands;numAlphUsed;numSetsUsed;numSets;countSum;dbSize;ctSize;totalSize;seconds\n");
		fflush(mReportFile);
	}
	return true;
}
void Algo::CloseReportFile() {
	if(mReportFile) {
		fclose(mReportFile);
		mReportFile = NULL;
	}
}

void Algo::LoadCodeTable(const uint32 resumeSup, const uint64 resumeCand) {
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

string Algo::GetShortName() {
	 return mCT->GetShortName();
}

string Algo::GetShortPruneStrategyName() {
	return PruneStrategyToString(mPruneStrategy);
}

string Algo::PruneStrategyToString(PruneStrategy pruneStrategy) {
	if(pruneStrategy == PreAcceptPruneStrategy) {
		return "pre";
	} else if(pruneStrategy == PostAcceptPruneStrategy) {
		return "pop";
	} else if(pruneStrategy == SanitizeOfflinePruneStrategy) {
		return "post";
	} else /*if(pruneStrategy == NoPruneStrategy)*/ {
		return "nop";
	}
}
string Algo::PruneStrategyToName(PruneStrategy pruneStrategy) {
	if(pruneStrategy == PreAcceptPruneStrategy) {
		return "pre-accept-prune";
	} else if(pruneStrategy == PostAcceptPruneStrategy) {
		return "post-accept-prune";
	} else if(pruneStrategy == SanitizeOfflinePruneStrategy) {
		return "offline sanitize";
	} else /*if(pruneStrategy == NoPruneStrategy)*/ {
		return "no-prune";
	}
}

PruneStrategy Algo::StringToPruneStrategy(string pruneStrategyStr) {
	// set `pruneStrategy` and output prune strategy to screen
	if(pruneStrategyStr.compare("nop") == 0 ) {
		return NoPruneStrategy;
	} else if(pruneStrategyStr.compare("pop") == 0) {
		return PostAcceptPruneStrategy;
	} else if(pruneStrategyStr.compare("pre") == 0) {
		return PreAcceptPruneStrategy;
	} else if(pruneStrategyStr.compare("post") == 0) {
		return SanitizeOfflinePruneStrategy;
	} else 
		THROW("Unknown Pruning Strategy: " + pruneStrategyStr);
}

ReportType Algo::StringToReportType(string reportTypeStr) {
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

void Algo::SetWriteFiles(const bool write) {
	mWriteCTLogFile = write;
	mWriteProgressToDisk = write;
	mWriteReportFile = write;
}
