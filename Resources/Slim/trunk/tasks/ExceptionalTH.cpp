#ifdef ENABLE_EMM

#include "../global.h"

#include <time.h>

// qtils
#include <Config.h>
#include <logger/Log.h>

// bass
#include <Bass.h>

// croupier
#include <Croupier.h>

#include "ExceptionalTH.h"

ExceptionalTH::ExceptionalTH(Config *conf) : TaskHandler(conf){

}
ExceptionalTH::~ExceptionalTH() {
	// not my Config*
}

void ExceptionalTH::HandleTask() {
	string command = mConfig->Read<string>("command");

//	if(command.compare("ctelemlinking") == 0)				CTElemLinking();
//	else	
			throw string("ExceptionalTH :: Unable to handle task ") + command;
}

string ExceptionalTH::BuildWorkingDir() {
	return mConfig->Read<string>("command") + "/";
}

#endif // ENABLE_EMM
