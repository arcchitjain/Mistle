#ifdef BLOCK_LESS

#include "../../global.h"
#include <time.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>

#include "LowEntropySet.h"
#include "LowEntropySetCollection.h"

#include "LECodeTable.h"
//#include "LECTStrat1.h"

#include "LEAlgo.h"

LEAlgo::LEAlgo() {
	mDB = NULL;
	mCT = NULL;
	mLESC = NULL;
	mReportFile = NULL;
	mOutDir = Bass::GetExpDir();
	mCTLogFile = NULL;
	mPruneStrategy = LENoPruneStrategy;
	mBitmapStrategy = LEBitmapStrategyConstantCT;

	mScreenReportTime = 0;
	mScreenReportCandidateIdx = 0;
	mScreenReportCandPerSecond = 0;
	mScreenReportCandidateDelta = 0;

	mWriteReportFile = true;
	mWriteProgressToDisk = true;
}

LEAlgo::~LEAlgo() {
	mDB = NULL;		// je delete de Database maar in dezelfde class als dat je 'm aangemaakt hebt!
	mLESC = NULL;	// je delete de ItemSetCollection maar in dezelfde class als dat je 'm aangemaakt hebt!
	mCT = NULL;		// je delete de CodeTable maar in dezelfde class als dat je 'm aangemaakt hebt!
	if(mReportFile)
		CloseReportFile();
}

/* LEAlgo claims ownership of db. */
void LEAlgo::UseThisStuff(Database *db, LowEntropySetCollection *lesc, LECodeTable *ct, bool fastGenerateDBOccs, LEBitmapStrategy bs) {
	mDB = db;
	mCT = ct;
	mLESC = lesc;
	mBitmapStrategy = bs;
	if(mCT != NULL) {
		mCT->UseThisStuff(db, mBitmapStrategy, lesc->GetMaxSetLength());
		//if(mLESC != NULL)
		//	mLESC->SetProvideDBOccurrences(mCT->NeedsDBOccurrences(),fastGenerateDBOccs);
	}
}

LECodeTable* LEAlgo::DoeJeDing(const uint64 candidateOffset /* = 0 */, const uint32 startSup /* = 0 */) {
	uint64 numM = mLESC->GetNumLowEntropySets(), nextReportCnd;
	float curEntropy = 0;
	LowEntropySet *m;
	mProgress = -1;

	if(mWriteReportFile == true) 
		OpenReportFile(true);

	mStartTime = time(NULL);
	mScreenReportTime = time(NULL);
	mScreenReportCandidateIdx = 0;
	mScreenReportCandPerSecond = 0;
	mScreenReportCandidateDelta = 5000;

	LECoverStats &curStats = mCT->GetCurStats();

	nextReportCnd = (mReportCndDelta == 0) ? numM : mReportCndDelta;

	for(uint64 curM=candidateOffset; curM<numM; curM++) {
		ProgressToScreen(curM, numM);

		m = mLESC->GetNextPattern();
		curEntropy = m->GetEntropy();

		if(mWriteProgressToDisk == true) {
			if(curM == 0) {
				ProgressToDisk(curEntropy, curM, true, true, false, "-first");
				// !!! nextReportSup moet niet 0-based, maar minSup-based zijn (dus (curSup - minSup) % mRepSupD == 0, ofzo
			} else if(curM == nextReportCnd) {
				ProgressToDisk(curEntropy, curM, true, false, true);
				nextReportCnd += mReportCndDelta;
			}
		}

		if(m->GetLength() <= 1)	{	// Skip singleton LowEntropySets: already in alphabet
			delete m;
			continue;
		}

		//printf("\nConsidering m : %s\n",m->ToString().c_str());
		mCT->Add(m, curM);
		m->SetCount(0);
		mCT->CoverDB(curStats);
		if(curStats.dbSize < 0) {
			throw string("LEAlgo::Compress : dbSize < 0. Dit is niet goed.");
		}

		if(mPruneStrategy == LEPostAcceptPruneStrategy) {				// Post-decide On-the-fly pruning
			if(mCT->GetCurSize() < mCT->GetPrevSize()) {
				mCT->CommitAdd();
				PrunePost();
				mCT->BackupStats();
				ProgressToDisk(curEntropy, curM, true, true, false);
			} else {
				mCT->RollbackAdd();
				mCT->RollbackCounts();
			}
		} else {												// No On-the-fly pruning
			if(mCT->GetCurSize() < mCT->GetPrevSize()) {
				mCT->CommitAdd();
				ProgressToDisk(curEntropy, curM, true, true, false);
			} else {
//				ProgressToDisk(curEntropy, curM, true, true, false);
				mCT->RollbackAdd();
			}
		}
	}
	LECoverStats stats = mCT->GetCurStats();
	printf(" * Result:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.setCountSum,stats.dbSize,stats.bmSize,stats.ct1Size,stats.ct2Size,stats.size);
	if(mWriteProgressToDisk == true) {
		mCT->CoverDB(curStats);
		ProgressToDisk(curEntropy, numM, true, true, true);
	}
	CloseReportFile();

	return mCT;
}

void LEAlgo::WriteEncodedRowLengthsToFile() {
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
		//enclen = mCT->CalcTransactionCodeLength(rows[r]);
		throw string("hier niet af");

		fprintf(fp, "%d\t%d\t%d\t%.2f\n", r, curClass, count, enclen);
	}

	fclose(fp);
}


void LEAlgo::PrunePost() {
	// Build PruneList
	leslist *pruneList = mCT->GetPostPruneList();
	LECoverStats &stats = mCT->GetPrunedStats();

	leslist::iterator iter;
	for(iter = pruneList->begin(); iter != pruneList->end(); ++iter) {
		// !!! kan efficienter voor die sets met count 0, maar dat doen we nu even niet, want dat is count-backup-hell
		LowEntropySet *les = (LowEntropySet*)(*iter);

		if(les->GetCount() == 0) {
			mCT->Del(les, false); // zap as immediately as possible...
			mCT->CoverDB(stats);
			mCT->CommitLastDel(true);
		} else {
			mCT->Del(les, false);
			mCT->CoverDB(stats);
			if(mCT->GetPrunedSize() <= mCT->GetCurSize()) {
				mCT->CommitLastDel(true);
				mCT->UpdatePostPruneList(pruneList, iter);
			} else {
				mCT->RollbackLastDel();
				mCT->RollbackCounts();
			}
		}
	}
	delete pruneList;
}

LEAlgo* LEAlgo::Create(const string &algoname) {
	return new LEAlgo();
}

void LEAlgo::ProgressToScreen(const uint64 cur, const uint64 total) {
	if(Bass::GetOutputLevel() > 0) {
		uint16 p = (uint16) (100 * cur / total);
		uint64 canDif = cur - mScreenReportCandidateIdx;

		if(mProgress < p || canDif >= mScreenReportCandidateDelta) {
			// Candidates per Second uitrekenen, update per 30s
			if(canDif >= mScreenReportCandidateDelta) {
				time_t curTime = time(NULL);
				uint32 timeDif = (uint32) (curTime - mScreenReportTime);
				mScreenReportCandPerSecond = (uint32) (cur - mScreenReportCandidateIdx) / (timeDif > 0 ? timeDif : 1);
				mScreenReportCandidateDelta = (mScreenReportCandPerSecond + 1) * 30;
				mScreenReportTime = curTime;
				mScreenReportCandidateIdx = cur;
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
				printf(" %c Progress:\t\t%d%% (%" I64d ") %" I64d " c/s\n", c,p,cur);
			else {
				if(mScreenReportCandPerSecond == 0)
					printf(" %c Progress:\t\t%d%% (%" I64d ") ? c/s\t\t\r", c,p,cur);
				else
					printf(" %c Progress:\t\t%d%% (%" I64d ") %d c/s\t\t\r", c,p,cur,mScreenReportCandPerSecond);
			}
		}
	}
}

void LEAlgo::ProgressToDisk(const float curEntropy, const uint64 numCands, const bool writeCodetable, const bool writeStats, const bool writeCover, const string postfix) {
	if(writeStats) {
		LECoverStats stats = mCT->GetCurStats();
		fprintf(mReportFile, "%.2f;%" I64d "" I64d ";%d;%d;%d;%" I64d ";%.0f;%.0f;%.0f;%.0f;%.0f;%.2f;%d\n", curEntropy, numCands, stats.alphItemsUsed, stats.numSetsUsed, mCT->GetCurNumSets(), stats.setCountSum, stats.dbSize, stats.bmSize, stats.ct1Size, stats.ct2Size, stats.size, stats.likelihood, time(NULL) - mStartTime);
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
		mCT->WriteToDisk(ctFilename);*/

		char buf[255];
		sprintf_s(buf, 255, "%s/ct-%s-%" I64d "%s.ct", mOutDir.c_str(),mTag.c_str(),numCands, (postfix == "" ? "" : ("-"+postfix)).c_str());

		string ctFilename = buf;
		mCT->WriteToDisk(ctFilename);
	}
	if(writeCover) {
		char buf[255];
		sprintf_s(buf, 255, "%s/ct-%s-%" I64d "%s.cv", mOutDir.c_str(),mTag.c_str(),numCands, (postfix == "" ? "" : ("-"+postfix)).c_str());
		string ctFilename = buf;
		mCT->WriteCoverToDisk(ctFilename);
	}
}
bool LEAlgo::OpenReportFile(const bool header) {
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
		fprintf(mReportFile, "entropy;numCands;numAlphUsed;numSetsUsed;numSets;countSum;dbSize;bmSize;ct1Size;ct2Size;totalSize;seconds\n");
		fflush(mReportFile);
	}
	return true;
}
void LEAlgo::CloseReportFile() {
	if(mReportFile) {
		fclose(mReportFile);
		mReportFile = NULL;
	}
}

string LEAlgo::GetShortName() {
	 return mCT->GetShortName();
}

string LEAlgo::GetShortPruneName() {
	if(mPruneStrategy == LEPreAcceptPruneStrategy) {
		return "pre";
	} else if(mPruneStrategy == LEPostAcceptPruneStrategy) {
		return "pop";
	} 
	return "nop";
}

void LEAlgo::SetWriteFiles(const bool write) {
	mWriteProgressToDisk = write;
	mWriteReportFile = write;
}

#endif // BLOCK_LESS
