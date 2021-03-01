// fic.cpp : Defines the entry point for the console application.

#include "global.h"

#include <time.h>
#if defined (_WINDOWS)
	#include <direct.h>
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <unistd.h>
#endif

#include <SystemUtils.h>
#include <FileUtils.h>
#include <Config.h>
#include <logger/Logger.h>

#include <Bass.h>

#include "FicConf.h"
#include "FicMain.h"

//#include "ficmkii.vs03.ver"

void ProcessConf(const string &confName, uint32 fromInst=0, uint32 toInst=0);
void ProcessBatch(const string &batchName);
void ProcessSmartBatch(const string &batchName);
void DoSmartBatch(string &dir, uint32 depth, const string &execute, const string &skipIfExists);
void ProcessSmartCollect(const string &batchName);
void DoSmartCollect(string &dir, uint32 depth, const string &collect, FILE *out);

#define VERSION_MARK "III"
#define VERSION_REVISION "0"
#define VERSION_BUILD "n"

int main(int argc, char* argv[])
{
#ifdef _MEMLEAK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode ( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	// break on allocation of specific memory block
	 //_crtBreakAlloc = 107974;

	string arg;
	bool pressEnter = false;

	const string defaultConf = "fic.conf";
	{
		arg = argv[0];
		string::size_type loc = arg.find_last_of("\\/");
		if(loc != string::npos) {
			string ed = arg.substr(0, loc+1);
			Bass::SetExecDir(ed);
		} else {
#if defined (_MSC_VER)
			Bass::SetExecDir(FileUtils::GetCurDirectory());
#elif defined (__GNUC__)
			string ed(FileUtils::GetCurDirectory());
			Bass::SetExecDir(ed);
#endif
		}
		Bass::SetExecName(arg);
	}

	uint32 f,t;
	switch(argc) {
		case 1:
			if(arg.compare("-version") == 0) {
				printf("FIC mark %s revision %s build %s", VERSION_MARK, VERSION_REVISION, VERSION_BUILD);
			} else {
				ProcessConf(defaultConf);
			}
			break;
		case 2:
			arg = argv[1];
			if(arg.find("\\") != string::npos || arg.find("/") != string::npos) {
				string::size_type loc = arg.find_last_of("\\/");
				string cwd = arg.substr(0, loc);
#if defined (_WINDOWS)
				_chdir(cwd.c_str()); /* check success ? */
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
				chdir(cwd.c_str()); /* check success ? */
#endif
				printf("Changed working directory to: %s\n", cwd.c_str());
				pressEnter = true;
			}
			if(arg.find(".smartbatch") != string::npos)
				ProcessSmartBatch(arg);
			else if(arg.find(".smartcollect") != string::npos)
				ProcessSmartCollect(arg);
			else if(arg.find(".batch") != string::npos)
				ProcessBatch(arg);
			else
				ProcessConf(arg);
			break;
		case 3:
			arg = argv[1];
			if(arg.compare("batch") == 0) {
				arg = argv[2];
				ProcessBatch(arg);
			} else if(/*arg.find(".conf") != string::npos &&(*/ sscanf(argv[2],"%d",&f)) {
				ProcessConf(argv[1],f);				
			} else 
				printf("Unknown command: %s\n\n", argv[1]);
			break;
		case 4:
			arg = argv[1];
			if(arg.find(".conf") != string::npos && sscanf(argv[2],"%d",&f) && sscanf(argv[3],"%d",&t)) {
				ProcessConf(argv[1],f,t);
			} else 
				printf("Unknown command: %s\n\n", argv[1]);
			break;
		default:
			printf("Error. Wrong number of arguments.\n\n");
	}
#if defined (_WINDOWS)
	if(pressEnter) {
		printf("\n\nPress enter ...");
		getchar();
	} 
#endif
	return 0;
}

void ProcessBatch(const string &batchName) {
	FILE *fp = NULL;
	char *buffer = new char[100];
	fp = fopen(batchName.c_str(), "r");
	string confName;

	if(fp != NULL) {
		while(fgets(buffer, 100, fp)) {
			confName = buffer;
			confName.resize(confName.size() - 1);
			if(confName.size() > 1 && confName.at(0) != '#')
				ProcessConf(confName);
			else
				printf("Batch -- skipping '%s'\n", confName.c_str());
		}
		if(ferror(fp))
			printf("* Error while reading batch file.\n");
		fclose(fp);

	} else {	// File open error!
		printf("* Could not read batch file.\n");
	}
	delete[] buffer;
}

void ProcessConf(const string &confName, uint32 fromInst, uint32 toInst) {
	printf(" ---------------------------------- FIC mkIII ---------------------------------\n\n");
	printf("** Processing conf: '%s'\n", confName.c_str());

	// Main code.
	try { 
		FicConf ficConf;
		ficConf.Read(confName);

		uint32 numInsts = ficConf.GetNumInstances();
		if((fromInst > 0 && fromInst > numInsts) || (toInst > 0 && toInst > numInsts))
			throw string("Ho! ho! ho! There arent that many instances in the config file");

		if(numInsts > 1 && ((fromInst == 0 && toInst == 0) || (fromInst > 0 && toInst > 0 && fromInst < toInst) )) {
			if(fromInst > 0) fromInst--;
			if(toInst > 0) toInst--;
			else if(toInst == 0) toInst = numInsts-1;

			for(uint32 curInst=fromInst; curInst <= toInst; curInst++) {
				Config* instConf = ficConf.GetConfig(curInst);
				FicMain main(instConf);
				try {
					main.Execute();
					delete instConf;
				} catch(...) {
					delete instConf;
					throw;
				}
/*
				PROCESS_INFORMATION procInfo;
				STARTUPINFO startupInfo = {0}; 
				startupInfo.cb = sizeof(STARTUPINFO);
				char ts[255];
				sprintf(ts, "cmd.exe /C ficd.exe %s %d", confName.c_str(), curInst+1);
				CreateProcess("cmd.exe", ts, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &procInfo);
*/
				printf("\n\n");
#if defined (_WINDOWS)
				Sleep(1000);
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
				sleep(1);
#endif
			}
		} else {
			if(fromInst > 0) fromInst--;
			Config* instConf = ficConf.GetConfig(fromInst);
			FicMain main(instConf);
			try {
				main.Execute();
				delete instConf;
			} catch(string ex) {
				Logger::CleanUp();
				delete instConf;
				throw ex;
			} catch(...) {
				Logger::CleanUp();
				delete instConf;
				throw;
			}
		}
	} catch(char *msg) {
		printf("\n!! Run-time fatal exception:\n ! %s\n", msg);
	} catch(string msg) {
		printf("\n!! Run-time fatal exception:\n ! %s\n", msg.c_str());
	} catch(Config::key_not_found knf) {
		printf("\n\nFatal: Required key '%s' not found in config file.", knf.key.c_str());
	}
	// Endof main.
}
void ProcessSmartBatch(const string &batchName) {
	try { 
		Config *config = new Config();
		config->ReadFile(batchName);

		if(config->GetNumInstances() > 1)
			throw string("Bummer. Multiple config instances in smartbatch not supported.");

		string root			= config->Read<string>("rootdir");
		string execute		= config->Read<string>("execute");
		uint32 recurseDepth	= config->Read<uint32>("recursedepth", 0);
		string skipIfExists = config->Read<string>("skipifexists", "");

		if(!FileUtils::DirectoryExists(root))
			throw string("Specified rootdir does not seem to exist. Emergency exit.");

		DoSmartBatch(root, recurseDepth, execute, skipIfExists);

		delete config;

	} catch(char *msg) {
		printf("\n!! Run-time fatal exception:\n ! %s\n", msg);
	} catch(string msg) {
		printf("\n!! Run-time fatal exception:\n ! %s\n", msg.c_str());
	} catch(Config::key_not_found knf) {
		printf("\n\nFatal: Required key '%s' not found in config file.", knf.key.c_str());
	}
}
void DoSmartBatch(string &dir, uint32 depth, const string &execute, const string &skipIfExists) {
	if(skipIfExists.compare("") != 0) {
		directory_iterator dirit(dir, skipIfExists);
		if(dirit.next())
			return;	// skip this directory
	}

	directory_iterator dirit(dir, execute);
	while(dirit.next())
		ProcessConf(dirit.fullpath());

	if(depth > 0) {
		directory_iterator dirit(dir, "*");
		while(dirit.next())
			if(FileUtils::IsDirectory(dirit.fullpath())) {
#if defined (_MSC_VER)
				DoSmartBatch(dirit.fullpath(), depth-1, execute, skipIfExists);
#elif defined (__GNUC__)
				string nextdir(dirit.fullpath());
				DoSmartBatch(nextdir, depth-1, execute, skipIfExists);	
#endif
			}
	}
}

void ProcessSmartCollect(const string &batchName) {
	try { 
		Config *config = new Config();
		config->ReadFile(batchName);

		if(config->GetNumInstances() > 1)
			throw string("Bummer. Multiple config instances in smartbatch not supported.");

		string root			= config->Read<string>("rootdir");
		string collect		= config->Read<string>("collect");
		uint32 recurseDepth	= config->Read<uint32>("recursedepth", 0);
		string output = config->Read<string>("output", "collect.csv");

		if(!FileUtils::DirectoryExists(root))
			throw string("Specified rootdir does not seem to exist. Emergency exit.");

		FILE *fp;
		fopen_s(&fp, output.c_str(), "w");
		DoSmartCollect(root, recurseDepth, collect, fp);
		fclose(fp);

		delete config;

	} catch(char *msg) {
		printf("\n!! Run-time fatal exception:\n ! %s\n", msg);
	} catch(string msg) {
		printf("\n!! Run-time fatal exception:\n ! %s\n", msg.c_str());
	} catch(Config::key_not_found knf) {
		printf("\n\nFatal: Required key '%s' not found in config file.", knf.key.c_str());
	}
}
void DoSmartCollect(string &dir, uint32 depth, const string &collect, FILE *out) {
	{
		char buffer[2048];
		FILE *in;
		directory_iterator dirit(dir, collect);
		while(dirit.next()) {
			printf(" * Collecting: %s\n", dirit.fullpath().c_str());
			fopen_s(&in, dirit.fullpath().c_str(), "r");
			while(!feof(in)) {
				size_t res = fread(buffer, 1, 2048, in);
				fwrite(buffer, 1, res, out);
			}
			fclose(in);
		}
	}

	if(depth > 0) {
		directory_iterator dirit(dir, "*");
		while(dirit.next())
			if(FileUtils::IsDirectory(dirit.fullpath())) { 
#if defined (_MSC_VER)
				DoSmartCollect(dirit.fullpath(), depth-1, collect, out);
#elif (__GNUC__)
        string nextdir(dirit.fullpath());
				DoSmartCollect(nextdir, depth-1, collect, out);
#endif
			}
	}
}
