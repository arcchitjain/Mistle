#include "../global.h"
#include <Config.h>
#include <Bass.h>
#include <logger/Logger.h>

#include "TaskHandler.h"

TaskHandler::TaskHandler(Config *conf) {
	mConfig = conf;
}

TaskHandler::~TaskHandler() {
	// not my Config*
}

string TaskHandler::BuildWorkingDir() {
	//char s[100];
	//_i64toa_s(Logger::GetCommandStartTime(), s, 100, 10);
	return ""; //mConfig->Read<string>("command") + "/" + s + "/";
}

bool TaskHandler::ShouldWriteTaskLogs() {
	if(mConfig != NULL && mConfig->Read<uint32>("logtodisk",0) == 1)
		return true;
	return false;	
}
