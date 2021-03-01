
#include "../qlobal.h"

#include "FileLogger.h"

FileLogger::FileLogger(const string &dir, const string &filename, uint32 maxLevel) : Logger(maxLevel), mDir(dir), mFilename(filename), mFileOpen(false), mFile(NULL) {
}
FileLogger::~FileLogger() {
	if(mFileOpen)
		fclose(mFile);
}
void FileLogger::SetDirectory(const string &dir) {
	if(!mFileOpen)
		mDir = dir;
}
void FileLogger::OpenFile() {
	if(mFileOpen == true)
		throw string("FileLogger::FileLogger -- Logfile already opened.");
	string fullPath = mDir + mFilename;
	if(fopen_s(&mFile, fullPath.c_str(), "a+") != 0)
		throw string("FileLogger::FileLogger -- Cannot open logfile for writing.");
	mFileOpen = true;
}
void FileLogger::LogMessage(const string &msg) {
	if(!mEnabled) return;
	if(!mFileOpen) OpenFile();
	fprintf(mFile, "%s\n", msg.c_str());
	fflush(mFile);
}
void FileLogger::LogMessage(const string &msg, uint32 level) {
	if(!mEnabled) return;
	if(!mFileOpen) OpenFile();
	if(level <= mMaxLevel) {
		for(uint32 i=0; i<level; i++)
			fputs("\t", mFile);
		fprintf(mFile, "%s\n", msg.c_str());
		fflush(mFile);
	}
}
