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

#include "global.h"

#include <Bass.h>
#include <Config.h>
#include <FileUtils.h>
#include <VORegistry.h>
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

#if defined (_WINDOWS)
	// lookup and parse device-specific config
	CVORegistry registry(HKEY_LOCAL_MACHINE, "Software\\JaM\\fic", false);
	LPTSTR temp = registry.ReadString("deviceConfig");
	configFile = "";
	if(temp != NULL) {
		configFile = temp;
		delete[] temp;
	}
	if(configFile.size() > 0) {
		if(FileUtils::FileExists(configFile))
			mConfig->ReadFile(configFile);
		else
			printf(" * Device conf does not exist: %s\n", configFile.c_str());
	}
#endif

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
		throw("No command specified.");
	else
		mConfig->DetermineNumInsts();
}
void FicConf::Write(const string &confName) {
	mConfig->WriteFile(confName);
}
