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

#include "clobal.h"

#include <Config.h>
#include <TimeUtils.h>

#include <Bass.h>
#include <db/Database.h>
#include <isc/ItemSetCollection.h>

#include "afopt/AfoptCroupier.h"
#include "apriori/AprioriCroupier.h"

#include "Croupier.h"

Croupier::Croupier(Config *config) { 
	mDbName = config->Read<string>("dbname","");
	mHistogram = NULL;

	mStoreIscFileType = config->KeyExists("iscStoreType") ? IscFile::StringToType(config->Read<string>("iscStoreType")) : BinaryFicIscFileType;
	mChunkIscFileType = config->KeyExists("iscChunkType") ? IscFile::StringToType(config->Read<string>("iscChunkType")) : mStoreIscFileType;
}
Croupier::Croupier() {
	mHistogram = NULL;
	mStoreIscFileType = BinaryFicIscFileType;
	mChunkIscFileType = BinaryFicIscFileType;
}
Croupier::~Croupier() {}

void Croupier::InitOnline(Database *db) {
	mDatabase = db;
}

string Croupier::BuildOutputFilename() {
	return mDbName + "-" + TimeUtils::GetTimeStampString() + ".pat";
}
ItemSetCollection* Croupier::MinePatternsToMemory(Database *db) {
	if(CanDealOnline() == true) {
		THROW("Mine Patterns to Memory not implemented.");
	} else
		THROW("Cannot Mine Patterns to Memory");
}

string Croupier::MinePatternsToFile(Database *db) {
	return MinePatternsToFile(Bass::GetWorkingDir(), db);
}

string Croupier::MinePatternsToFile(const string &workingDir, Database *db) {
	string dbFilename = mDbName;
	string outputFilename = workingDir + BuildOutputFilename();
	if(MinePatternsToFile(dbFilename, outputFilename, db))
		return outputFilename;
	else
		throw string("MineAllPatternsToFile failed."); // or: return "";, but then additional checks shouldn't be forgotten
}

Croupier *Croupier::Create(Config *config) {
	string name = config->Read<string>("croupier", "");
	if(name.size() > 0) {
		if(name.compare("fim") == 0)
			return new AfoptCroupier(config);
		THROW("Cannot create Croupier with that name.");
	} else {
		if(config->KeyExists("iscname") || config->KeyExists("isctype")) {
			// Extract type from iscname
			string iscName = config->Read<string>("iscname");
			return Create(iscName);
		}
		THROW("Don't recognize Croupier for this config.");
	}
}

Croupier *Croupier::Create(const string &iscName) {
	size_t firstDashPos = iscName.find('-');
	size_t secondDashPos = iscName.find_first_of('-',firstDashPos+1);
	if(firstDashPos == string::npos || secondDashPos == string::npos)
		THROW("Not an ItemSetCollection name: " + iscName);
	string type = iscName.substr(firstDashPos+1,secondDashPos-firstDashPos-1);

	// Choose croupier based on type
	if(type.length() > 8 && type.substr(0,7).compare("maxlen[")==0)
		return new AprioriCroupier(iscName);		// use apriori croupier if iscName of form <dbname>-maxlen[<maxlen>]-<minsup><order>
	else
		return new AfoptCroupier(iscName);		// otherwise, default to Afopt (for all and closed)
}
