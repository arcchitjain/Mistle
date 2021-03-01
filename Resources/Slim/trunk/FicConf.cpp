
#include "global.h"

#include <Bass.h>
#include <Config.h>
#include <FileUtils.h>
#include <logger/Log.h>

#include "FicConf.h"

FicConf::FicConf() {
	mConfig = new Config();
}

FicConf::~FicConf() {
	delete mConfig;
}

Config* FicConf::GetConfig(uint32 inst/* =0 */) {
	return mConfig->GetInstance(inst);
}

uint32 FicConf::GetNumInstances() {
	return mConfig->GetNumInstances();
}

void FicConf::Read(string confName) {
	mConfig->Clear(); // clean start

	string configFile;

	// user-specific config
	configFile = Bass::GetExecDir() + "fic.user.conf";
	if(FileUtils::FileExists(configFile)) {
		mConfig->ReadFile(configFile);
	} else
		printf(" * User conf does not exist: %s\n", configFile.c_str());

	// experiment-specific config
	if(!FileUtils::FileExists(confName)) {
		string tmp =  confName + ".conf";
		if(!FileUtils::FileExists(tmp)) {
			throw string("Config file " + confName + " (or " + tmp + ") does not exist!");
		}
		confName = tmp;
	}
	mConfig->ReadFile(confName);

	// Load external config if specified
	if(mConfig->KeyExists("externalconf")) {
		confName = mConfig->Read<string>("externalconf");

		if(!FileUtils::FileExists(confName)) {
			confName += ".conf";
			if(!FileUtils::FileExists(confName)) {
				throw string("Specified external config file " + confName + " does not exist!");
			}
		}
		mConfig->ReadFile(confName);
		mConfig->Remove("externalconf");
	}

	// Read datadir from datadir.conf if nog specified
	if(!mConfig->KeyExists("datadir") || mConfig->Read<string>("datadir") == "") {
		configFile = Bass::GetExecDir() + "datadir.conf";
		if(FileUtils::FileExists(configFile))
			mConfig->ReadFile(configFile);
	}

	// Check command
	if(!mConfig->KeyExists("command"))
		throw string("No command specified.");
	else
		mConfig->DetermineNumInsts();
}
void FicConf::Write(const string &confName) {
	mConfig->WriteFile(confName);
}
