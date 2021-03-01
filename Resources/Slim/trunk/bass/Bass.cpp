#include <omp.h>

#include "blobal.h"

#include <Config.h>
#include <SystemUtils.h>

#include "Bass.h"

bool Bass::sMineToMemory = false;

uint64 Bass::sMaxMemUse = (uint64) 512 * (uint64) MEGABYTE;		// 512MB should be enough, roughly
uint32 Bass::sNumThreads = 1;

uint8 Bass::sOutputLevel = 3;

string Bass::sExecDir = "";
string Bass::sExecName = "";

string Bass::sDbReposDir = "";
string Bass::sIscReposDir = "";
string Bass::sCtReposDir = "";

string Bass::sExpDir = "";
string Bass::sDataDir = "";
string Bass::sWorkingDir = "";
string Bass::sTempDir = "";

uint64 Bass::GetMaxMemUse() { 
	return sMaxMemUse==0 ? // if sMaxMemUse set to 0
		SystemUtils::GetProcessMemUsage() + SystemUtils::GetAvailableMemory() : // max usage is all that is already used + what is still available
		sMaxMemUse; // otherwise, use fixed max mem usage
}
uint64 Bass::GetRemainingMaxMemUse() {
	return sMaxMemUse==0 ? // if sMaxMemUse set to 0
		SystemUtils::GetAvailableMemory() : // use all available memory
		(SystemUtils::GetProcessMemUsage() < sMaxMemUse ? 
			sMaxMemUse - SystemUtils::GetProcessMemUsage() :
			0); // otherwise, compute allowed remaining
}
void Bass::ParseGlobalSettings(Config *config) {
	// Directories
	string bd = config->KeyExists("basedir") ? config->Read<string>("basedir") : GetExecDir() + "../";
	if(!config->KeyExists("basedir")) {
		if(GetExecDir().find_last_of("\\/",GetExecDir().length()-1) != string::npos)
			bd = GetExecDir().substr(0,GetExecDir().find_last_of("\\/",GetExecDir().length()-2)+1);
	}
	string datadir = config->KeyExists("datadir") ? config->Read<string>("datadir") : bd + "data/";
	SetDataDir(datadir);
	string expdir = config->KeyExists("expdir") ? config->Read<string>("expdir") : bd + "xps/";
	SetExpDir(expdir);

	SetWorkingDir(GetExpDir());
	string dbd = GetDataDir() + "datasets/";
	string iscd = GetDataDir() + "candidates/";
	string ctd = GetDataDir() + "codetables/";
	string td = config->KeyExists("tempdir") ? config->Read<string>("tempdir") : GetExecDir();
	SetDbReposDir(dbd);
	SetIscReposDir(iscd);
	SetCtReposDir(ctd);
	SetTempDir(td);

	// Croupier preferences
	SetMineToMemory(config->Read<string>("internalmineto","file").compare("memory") == 0);

	// Get max.number of threads
	string numThreadsStr = config->Read<string>("numthreads", "");
	uint32 numThreads = numThreadsStr.compare("auto") == 0 ? omp_get_num_procs() : config->Read<uint32>("numthreads",1);
	SetNumThreads(numThreads);

	// Set max memory usage
	if(config->KeyExists("maxmemusage") || config->KeyExists("maxmemoryusage")) {
		string mmu = config->KeyExists("maxmemusage") ? config->Read<string>("maxmemusage") : config->Read<string>("maxmemoryusage");
		if(mmu.compare("AVAILABLE") == 0) {				// AVAILABLE
			Bass::SetMaxMemUse(0);
		} else if(mmu.find('.') != string::npos) {		// Percentage of total
			float mmufloat = (float)atof(mmu.c_str());
			if(mmufloat > 1.0f)
				mmufloat = 1.0f;
			Bass::SetMaxMemUse((uint64)(mmufloat * SystemUtils::GetTotalMemory()));
		} else {										// Absolute
			uint64 mmuint = ((uint64) atoi(mmu.c_str())) * (uint64) MEGABYTE; // atoi, mb to b
			if(mmuint > SystemUtils::GetTotalMemory())
				mmuint = SystemUtils::GetTotalMemory();
			Bass::SetMaxMemUse(mmuint);
		}
	}

}

void Bass::SetNumThreads(uint32 nt) {
	 sNumThreads = nt; 
	 omp_set_num_threads(nt);
}
