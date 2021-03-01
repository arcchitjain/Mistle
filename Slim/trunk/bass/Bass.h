#ifndef __BASS_H
#define __BASS_H

#include "blobal.h"
#include "BassApi.h"

struct ltalphabet
{
	bool operator()(const uint16 a1, const uint16 a2) const
	{
		return a1 < a2;
	}
};

typedef std::map<uint16,uint32,ltalphabet> alphabet;
typedef std::pair<uint16,uint32> alphabetEntry;

typedef std::map<uint16,uint16> valBitMap;
typedef std::pair<uint16,uint16> valBitMapEntry;

typedef std::map<uint32,uint32> supScoreMap;
typedef std::pair<uint32,uint32> supScoreMapEntry;

typedef std::map<uint32,double> marginSumMap;
typedef std::pair<uint32,double> marginSumMapEntry;

// inline functions
__inline double CalcCodeLength(uint32 count, uint64 countSum) {
	//return -log( (double) count / countSum ) / (log(2.0));
	return -log( (double) count / countSum ) / 0.69314718055994530941723212145818;
}

#define log2(a) log(a)/0.69314718055994530941723212145818

__inline double Log2(double x) {
	static const double invLog2 = 1.0 / 0.69314718055994530941723212145818;
	return invLog2 * log(x);
}

class Config;

#include <FileUtils.h>
#include <logger/Logger.h>

/*
Printf Screen Output
0 - no output (only for nesting)
1 - bare minimum (normal runtime, batch)
2 - more verbose (normal runtime, single)
3 - very verbose (debug)
*/

class BASS_API Bass {
public:
	static void SetExecDir(string &dir) { sExecDir = dir; }
	static string& GetExecDir() { return sExecDir; }
	static void SetExecName(string &name) { sExecName = name; }
	static string& GetExecName() { return sExecName; }

	static void SetDbReposDir(string &dir) { sDbReposDir = dir; FileUtils::CreateDirIfNotExists(dir); }
	static void SetIscReposDir(string &dir){ sIscReposDir = dir; FileUtils::CreateDirIfNotExists(dir); }
	static void SetCtReposDir(string &dir)	{ sCtReposDir = dir; FileUtils::CreateDirIfNotExists(dir); }
	static string& GetDbReposDir()			{ return sDbReposDir; }
	static string& GetIscReposDir()		{ return sIscReposDir; }
	static string& GetCtReposDir()			{ return sCtReposDir; }

	static void SetDataDir(string &dir)	{ sDataDir = dir; FileUtils::CreateDirIfNotExists(dir); }
	static string& GetDataDir()			{ return sDataDir; }

	static void SetExpDir(string &dir)	{ sExpDir = dir; FileUtils::CreateDirIfNotExists(dir); }
	static string& GetExpDir()			{ return sExpDir; }

	static void SetWorkingDir(string &dir) { sWorkingDir = dir; FileUtils::CreateDirIfNotExists(dir); Logger::SetCommandLogDir(dir); }
	static string& GetWorkingDir()			{ return sWorkingDir; }

	static void SetTempDir(string &dir) { sTempDir = dir; FileUtils::CreateDirIfNotExists(dir); }
	static string& GetTempDir()			{ return sTempDir; }

	static void SetOutputLevel(uint8 ol)	{ sOutputLevel = ol; }
	static uint8 GetOutputLevel()		{ return sOutputLevel; }

	static void SetMineToMemory(bool b) { sMineToMemory = b; }
	static bool GetMineToMemory()		{ return sMineToMemory; }

	// set to 0 if you want to use all memory that is available
	static void SetMaxMemUse(uint64 mmu)	{ sMaxMemUse = mmu; }
	static uint64 GetMaxMemUse();
	static uint64 GetRemainingMaxMemUse();

	static void SetNumThreads(uint32 nt);
	static uint32 GetNumThreads() { return sNumThreads; }

	static void ParseGlobalSettings(Config *config);

protected: 
	static bool sMineToMemory;	// mine patterns in memory if possible

	static uint64 sMaxMemUse;	// maximum number of bytes in use by fic
	static uint32 sNumThreads;	// number of threads to run da stuff on

	static uint8 sOutputLevel;

	static string sExecDir;		// executable dir (fic.exe, fim_all.exe, ...)
	static string sExecName;	// executable name (fic.exe, krimp.exe, ...)

	static string sDbReposDir;	// Database repository
	static string sIscReposDir;	// Candidate Collection repository
	static string sCtReposDir;	// Codetable repository

	static string sDataDir;		// base experimental result output directory
	static string sExpDir;		// new base experimental result output directory
	static string sWorkingDir;	// current working directory (default read/write dir)
	static string sTempDir;		// directory for temporary storage
};

#define ECHO(level, pf) if(Bass::GetOutputLevel() >= level) { pf; }
#define THROW_NOT_IMPLEMENTED() throw string(__FUNCTION__) + " - Not implemented.";
#define THROW_DROP_SHIZZLE() throw string(__FUNCTION__) + " - We drop this shizzle like it's hot.";
#define THROW(x) throw string(__FUNCTION__) + " - " + x;

#endif // __BASS_H
