#ifdef BLOCK_SLIM

#include <time.h>
#include <omp.h>

#include "SlimAlgo.h"
#include "../krimp/Kramp.h"
#include "../krimp/Kromp.h"
#include "../krimp/Krump.h"
#include "Slim.h"		// true gain based Slim
#include "SlimStar.h"
#include "SlimPlus.h"
#include "SlimCS.h"		// the 'real' Slim-in-CoverSpace, by Koen Smets
#include "SlimMJ.h"


SlimAlgo::SlimAlgo(CodeTable* ct, HashPolicyType hashPolicy, Config* config) {
	mConfig = config;
	mHashPolicy = hashPolicy;
	mCT = ct;

	mReportIteration = false;
	mReportIterType = ReportAll;

	mLogFile = NULL;

	mWriteCTLogFile = config->Read<bool>("writeCTLogFile", true);
	mWriteReportFile = config->Read<bool>("writeReportFile", true);
	mWriteProgressToDisk = config->Read<bool>("writeProgressToDisk", true);
	mWriteLogFile = config->Read<bool>("writeLogFile", true);
}

string SlimAlgo::GetShortName() {
	return HashPolicyTypeToString(mHashPolicy) + "-" + KrimpAlgo::GetShortName();
}

KrimpAlgo* SlimAlgo::CreateAlgo(const string &algoname, ItemSetType type, Config* config) {
	if(algoname.compare(0, 6, "krimp-") == 0) {
		return KrimpAlgo::CreateAlgo(algoname.substr(6), type);
	}
	if(algoname.compare(0, 8, "krimp^2-") == 0 || algoname.compare(0, 6, "kramp-") == 0) {
		string strippedAlgoname = algoname.substr(8);
		HashPolicyType hashPolicy;
		strippedAlgoname = StringToHashPolicyType(strippedAlgoname, hashPolicy);
		return new Kramp(CodeTable::Create(strippedAlgoname, type), hashPolicy, config);
	}
	if(algoname.compare(0, 7, "krimp*-") == 0 || algoname.compare(0, 6, "kromp-") == 0) {
		string strippedAlgoname = algoname.substr(7);
		HashPolicyType hashPolicy;
		strippedAlgoname = StringToHashPolicyType(strippedAlgoname, hashPolicy);
		return new Kromp(CodeTable::Create(strippedAlgoname, type), hashPolicy, config);
	}
	if(algoname.compare(0, 7, "krimp'-") == 0 || algoname.compare(0, 6, "krump-") == 0) {
		string strippedAlgoname = algoname.substr(7);
		HashPolicyType hashPolicy;
		strippedAlgoname = StringToHashPolicyType(strippedAlgoname, hashPolicy);
		return new Krump(CodeTable::Create(strippedAlgoname, type), hashPolicy, config);
	}
	if(algoname.compare(0, 5, "slim-") == 0) {
		string strippedAlgoname = algoname.substr(5);
		HashPolicyType hashPolicy;
		strippedAlgoname = StringToHashPolicyType(strippedAlgoname, hashPolicy);
		return new Slim(CodeTable::Create(strippedAlgoname, type), hashPolicy, config);
	}
#ifdef BLOCK_TILING
	if(algoname.compare(0, 6, "slim*-") == 0) {
		string strippedAlgoname = algoname.substr(6);
		HashPolicyType hashPolicy;
		strippedAlgoname = StringToHashPolicyType(strippedAlgoname, hashPolicy);
		return new SlimStar(CodeTable::Create(strippedAlgoname, type), hashPolicy, strippedAlgoname, config);
	}
#endif
	if(algoname.compare(0, 6, "slim+-") == 0) {
		string strippedAlgoname = algoname.substr(6);
		HashPolicyType hashPolicy;
		strippedAlgoname = StringToHashPolicyType(strippedAlgoname, hashPolicy);
		return new SlimPlus(CodeTable::Create(strippedAlgoname, type), hashPolicy, config);
	}
	if(algoname.compare(0, 7, "slimCS-") == 0 || algoname.compare(0,7, "slimKS-") == 0) { // for Koen Smets
		string strippedAlgoname = algoname.substr(7);
		HashPolicyType hashPolicy = hashNoCandidates;
		strippedAlgoname = StringToHashPolicyType(strippedAlgoname, hashPolicy);
		return new SlimCS(CodeTable::Create(strippedAlgoname, type), hashPolicy, config);
	}
	if(algoname.compare(0, 7, "slimMJ-") == 0) { // for Manan Gandhi
		string strippedAlgoname = algoname.substr(7);
		HashPolicyType hashPolicy = hashNoCandidates;
		strippedAlgoname = StringToHashPolicyType(strippedAlgoname, hashPolicy);
		return new SlimMJ(CodeTable::Create(strippedAlgoname, type), hashPolicy, config);
	}
	return KrimpAlgo::CreateAlgo(algoname, type);
}

string SlimAlgo::HashPolicyTypeToString(HashPolicyType type) {
	switch (type) {
	case hashNoCandidates:
		return "n";
	case hashAllCandidates:
		return "a";
	case hashCtCandidates:
		return "c";
	}
	THROW("Say whut?");
}

string SlimAlgo::StringToHashPolicyType(string algoname, HashPolicyType& hashPolicy) {
	hashPolicy = hashNoCandidates;
	if (algoname.compare(0, 2, "n-") == 0) {
		hashPolicy = hashNoCandidates;
		algoname = algoname.substr(2);
	} else if (algoname.compare(0, 2, "a-") == 0) {
		hashPolicy = hashAllCandidates;
		algoname = algoname.substr(2);
	} else if (algoname.compare(0, 2, "c-") == 0) {
		hashPolicy = hashCtCandidates;
		algoname = algoname.substr(2);
	}
	return algoname;
}

void SlimAlgo::LoadCodeTable(const string& ctFile) {
	mCT->ReadFromDisk(ctFile, false);
	mCT->CoverDB(mCT->GetCurStats());
	mCT->CommitAdd(); // mAdded = NULL; prevStats = curStats;
}

void SlimAlgo::OpenLogFile() {
	string logFilename = mOutDir;
	logFilename.append(mTag);
	logFilename.append(".log");
//	printf("open LogFile: %s\n", logFilename.c_str());
	mLogFile = fopen(logFilename.c_str(), "w");
}

void SlimAlgo::CloseLogFile() {
	if(mLogFile) {
//		printf("close LogFile\n");
		fclose(mLogFile);
		mLogFile = NULL;
	}
}

void SlimAlgo::ProgressToScreen(const uint64 curCandidate, CodeTable *ct) {
	if(Bass::GetOutputLevel() > 0) {
		uint64 canDif = curCandidate - mScreenReportCandidateIdx;

		if(true) {
			// Calculate Candidates per Second, update per 30s
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

			// Calculate Percentage
			int d = curCandidate % 4;
			char c = '*';
			switch(d) {
			case 0 : c = '-'; break;
			case 1 : c = '\\'; break;
			case 2 : c = '|'; break;
			default : c = '/';
			}
			uint32 numprinted = 0;
			numprinted = printf_s(" %c Progress:\t\t%" I64d " (%d/s), %.00fb (-%.01fb) (%ds)    ", c, curCandidate, mScreenReportCandPerSecond, numBits, difBits, curTime - mStartTime);
			if(Bass::GetOutputLevel() == 3)
				printf("\n");
			else {
				if(numprinted < 45)
					for(uint32 i=0; i< (45-numprinted); i++)
						printf(" ");
				printf("\r");
			}
			fflush(stdout);
		}
	}
}

#endif // BLOCK_SLIM
