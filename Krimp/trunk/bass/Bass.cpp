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
#if defined (_WINDOWS)
	SetDbReposDir(GetDataDir() + "datasets/");
	SetIscReposDir(GetDataDir() + "candidates/");
	SetCtReposDir(GetDataDir() + "codetables/");
	SetTempDir(config->KeyExists("tempdir") ? config->Read<string>("tempdir") : GetExecDir());
#elif defined (__GNUC__)
	string dbd = GetDataDir() + "datasets/";
	string iscd = GetDataDir() + "candidates/";
	string ctd = GetDataDir() + "codetables/";
	string td = config->KeyExists("tempdir") ? config->Read<string>("tempdir") : GetExecDir();
	SetDbReposDir(dbd);
	SetIscReposDir(iscd);
	SetCtReposDir(ctd);
	SetTempDir(td);
#endif

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
