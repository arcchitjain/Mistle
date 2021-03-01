
#include "../qlobal.h"
#include <time.h>
#if defined (_WINDOWS)
	#include <Windows.h>
#endif

#include "../SystemUtils.h"
#include "../TimeUtils.h"

#include "Log.h"

StdOutLogger *Logger::sStdOut = NULL;
FileLogger *Logger::sSummary = NULL;
FileLogger *Logger::sExtended = NULL;

uint32	Logger::sCurrentLevel = 0;
bool	Logger::sLogToDisk = false;

Logger	**Logger::sExtraLoggers	= NULL;
uint32	Logger::sNumExtraLoggers	= 0;

string	Logger::sCommandTask = "";
string	Logger::sCommand = "";
time_t	Logger::sCommandStart = 0;

string	Logger::sAppLogDir = "";
string	Logger::sCmdLogDir = "";

#include <ctime>

Logger::Logger(uint32 maxLevel) : mMaxLevel(maxLevel), mEnabled(true) {

}
Logger::~Logger() {

}

void Logger::CommandInit(const string &taskClass, const string &command) {
	char s[200];
	sCommandTask = taskClass;
	sCommand = command;
	sCommandStart = time(NULL);
	string timestring = TimeUtils::GetDateTimeString();
	sprintf_s(s, 200, "%s * Initiating (%s,%s)\n", timestring.c_str(), sCommandTask.c_str(), sCommand.c_str());
	string str = s;
	WriteToAppLog(str);
}
void Logger::CommandEnd() {
	char s[200];
	time_t seconds = time(NULL) - sCommandStart;
	string timestring = TimeUtils::GetDateTimeString();
	sprintf_s(s, 200, "%s * Finished (%s,%s)\n", timestring.c_str(), sCommandTask.c_str(), sCommand.c_str());
	string str = s;
	float peakMem = (float)SystemUtils::GetProcessPeakMemUsage() / (1024 * 1024);
	sprintf_s(s, 200, "\t\t\t\t\t\tThis took about %" I64d " seconds, peak mem usage: %.02f Mb\n", seconds, peakMem);
	str += s;
	WriteToAppLog(str);

	sprintf_s(s, 200, "\nTask `%s`, command `%s` finished. This took %d seconds.\n", sCommandTask.c_str(), sCommand.c_str(), seconds);
	str = s;
	LogMessageToAll(s);
}

void Logger::LogMessageToAll(string message) {
	sStdOut->LogMessage(message, sCurrentLevel);
	if(sLogToDisk) {
		sSummary->LogMessage(message, sCurrentLevel);
	//	sExtended->LogMessage(message, sCurrentLevel);
	}
	for(uint32 i=0; i<sNumExtraLoggers; i++)
		sExtraLoggers[i]->LogMessage(message, sCurrentLevel);
}
void Logger::LogMessageTo(string message, const uint32 logs) {
	if((logs & LOGS_STDOUT) && sStdOut != NULL)
		sStdOut->LogMessage(message);

	if(sLogToDisk) {
		if(logs & LOGS_SUMMARY)
			sSummary->LogMessage(message);

	//	if(logs & LOGS_EXTENDED)
	//		sExtended->LogMessage(message);
	}

	if(sNumExtraLoggers > 0 && (logs & LOGS_EXTRA)) {
		for(uint32 i=0; i<sNumExtraLoggers; i++)
			sExtraLoggers[i]->LogMessage(message);
	}
}
void Logger::LogError(string errorMessage, bool warningOnly) {
	if(warningOnly)
		errorMessage = " ** WARNING : " + errorMessage + " **\n";
	else {
		errorMessage = " ** ERROR : " + errorMessage + " **\n";
		WriteToAppLog(errorMessage);
	}
	LogMessageTo(errorMessage, LOGS_ALL);
}

void Logger::LogDebug(string debugMessage) {
	debugMessage = " * Debug : " + debugMessage + " *\n";
	LogMessageToAll(debugMessage);
}

void Logger::SetAllEnabled(bool e) {
	sStdOut->SetEnabled(e);
	if(sLogToDisk) {
		sSummary->SetEnabled(e);
	//	sExtended->SetEnabled(e);
	}
	for(uint32 i=0; i<sNumExtraLoggers; i++)
		sExtraLoggers[i]->SetEnabled(e);
}
void Logger::SetAllMaxLevels(uint32 maxLevel) {
	sStdOut->SetMaxLevel(maxLevel);
	if(sLogToDisk) {
		sSummary->SetMaxLevel(maxLevel);
	//	sExtended->SetMaxLevel(maxLevel);
	}
	for(uint32 i=0; i<sNumExtraLoggers; i++)
		sExtraLoggers[i]->SetMaxLevel(maxLevel);
}
void Logger::SetCommandLogDir(const string &dir) {
	sCmdLogDir = dir;
	if(sSummary)
		sSummary->SetDirectory(dir);
	//if(sExtended)
	//	sExtended->SetDirectory(dir);
}
void Logger::Init(bool ltd, uint32 stdOutMaxLevel, uint32 summaryMaxLevel, uint32 extendedMaxLevel) {
	sStdOut = new StdOutLogger(stdOutMaxLevel);
	sLogToDisk = ltd;
	if(sLogToDisk) {
		sSummary = new FileLogger(sCmdLogDir, "_summary.log", summaryMaxLevel);
		//sExtended = new FileLogger(sCmdLogDir, "_extended.log", extendedMaxLevel);
	}
}
void Logger::CleanUp() {
	delete sStdOut;		sStdOut = NULL;
	delete sSummary;		sSummary = NULL;
	//delete sExtended;	sExtended = NULL;
	for(uint32 i=0; i<sNumExtraLoggers; i++)
		delete sExtraLoggers[i];
	delete[] sExtraLoggers;
	sNumExtraLoggers = 0;
	sExtraLoggers = NULL;
}
void Logger::AddLogger(Logger *logger) {
	Logger **l = new Logger *[sNumExtraLoggers+1];
	for(uint32 i=0; i<sNumExtraLoggers; i++)
		l[i] = sExtraLoggers[i];
	l[sNumExtraLoggers] = logger;
	delete[] sExtraLoggers;
	sExtraLoggers = l;
	++sNumExtraLoggers;
}
void Logger::RemoveLogger(Logger *logger) {
	Logger **l = new Logger *[sNumExtraLoggers-1];
	uint32 idx = 0;
	for(uint32 i=0; i<sNumExtraLoggers; i++)
		if(sExtraLoggers[i] != logger)
			l[idx++] = sExtraLoggers[i];
	delete logger;
	delete[] sExtraLoggers;
	sExtraLoggers = l;
	--sNumExtraLoggers;
}
void Logger::WriteToAppLog(string line) {
	char s[200];
	sprintf_s(s, 200, "[%d] -- ", sCommandStart);
	line = s + line;
	FILE *fp;
	string file = sAppLogDir + "_fic.log";
	uint32 trial = 0;

	while(fopen_s(&fp, file.c_str(), "a") != 0 && trial++ < 1000)
		SystemUtils::Sleep(10); // if the file cannot be opened, it may already be opened, wait 10ms

	if(trial < 1000) { // succeeded!
		fputs(line.c_str(), fp);
		fclose(fp);
	}
}
