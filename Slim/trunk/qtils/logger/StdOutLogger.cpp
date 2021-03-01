
#include "../qlobal.h"

#include "StdOutLogger.h"

#define PROGRESS_BAR_LEN		30

StdOutLogger::StdOutLogger(uint32 maxLevel) : Logger(maxLevel) {
	mProgress = false;
	mProgressMax = 0;
	mMessage = "";
}
StdOutLogger::~StdOutLogger() {

}
void StdOutLogger::LogMessage(const string &msg) {
	if(!mEnabled) return;
	if(mProgress)
		mMessage = msg;
	else
		puts(msg.c_str());
}
void StdOutLogger::LogMessage(const string &msg, uint32 level) {
	if(!mEnabled) return;
	if(mProgress)
		mMessage = msg;
	else if(level <= mMaxLevel) {
		for(uint32 i=0; i<level; i++)
			printf("\t");
		puts(msg.c_str());
	}
}
void StdOutLogger::InitProgress(uint32 max, string message) {
	if(!mEnabled) return;
	mProgressMax = max;
	mProgress = true;
	mMessage = message;
}
void StdOutLogger::ShowProgress(uint32 current) {
	if(!mEnabled) return;
	uint32 num = (PROGRESS_BAR_LEN * current) / mProgressMax;
	printf("[");
	for(uint32 i=0; i<num; i++)
		printf("%C",240);
	for(uint32 i=num; i<PROGRESS_BAR_LEN; i++)
		printf(" ");
	uint32 charsLeft = 78 - PROGRESS_BAR_LEN;
	charsLeft -= printf("] %d/%d : ", current, mProgressMax); 
	printf("%s\r", mMessage.substr(0,charsLeft).c_str());
}
void StdOutLogger::EndProgress() {
	if(!mEnabled) return;
	printf("\n");
	mProgressMax = 0;
	mProgress = false;
	mMessage = "";
}
