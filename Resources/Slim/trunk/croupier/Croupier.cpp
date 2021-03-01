
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
