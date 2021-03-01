#include "../global.h"

// system
#include <time.h>
#if defined (_WINDOWS)
	#include <direct.h>
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <sys/stat.h>
	#include <glibc_s.h>	
#endif

// qtils
#include <Config.h>
#include <TimeUtils.h>
#include <logger/Log.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

// frequent itemsets (all, closed, maxlen)
#include <Croupier.h>

// tiles/tiling
#include <tiles/TileMiner.h>
#ifdef BLOCK_TILES
#include "../blocks/tile/TilingMiner.h"
#include "../blocks/tile/OverlappingTilingMiner.h"
#endif

// lesets
#include <lesets/LESetMiner.h>

#include "TaskHandler.h"
#include "MineTH.h"

MineTH::MineTH(Config *conf) : TaskHandler(conf){
	mTimestamp = TimeUtils::GetTimeStampString();
}

MineTH::~MineTH() {
	// not my Config*
}

void MineTH::HandleTask() {
	string command = mConfig->Read<string>("command");
	if(command.compare("frequent") == 0)			MineFrequentSets();			// handles different flavours of frequent itemsets: all, closed, maxlen
	else if(command.compare("le") == 0)				MineLowEntropySets();
#ifdef BLOCK_TILES
	else if(command.compare("tiles") == 0)			MineTiles();
	else if(command.compare("tiling") == 0)			MineTiling();
#endif
	//else	if(command.compare("max") == 0)			MineMaximalFrequentItemsets();
	//else	if(command.compare("ndi") == 0)			MineNonDerivableItemsets();
	//else	if(command.compare("tag") == 0)			MineByTag();
	else THROW("Unable to handle task `" + command + "`");
}

string MineTH::BuildWorkingDir() {
	string command = mConfig->Read<string>("command");
	return "mine/";// + command + "/";
}

void MineTH::MineFrequentSets() {
	string iscName = mConfig->Read<string>("iscname");
	string dbName, patternType, settings;
	IscOrderType orderType;
	ItemSetCollection::ParseTag(iscName, dbName, patternType, settings, orderType);
	
	Database *db = Database::RetrieveDatabase(dbName);
	Croupier *croupier = Croupier::Create(mConfig);
	croupier->MinePatternsToFile(db);
	delete croupier;
	delete db;
}

#ifdef BLOCK_TILES
void MineTH::MineTiles() {
	// get dbfilename and outputfilename from config
	string dbName = mConfig->Read<string>("dbname");

	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType);
	TileMiner *miner = new TileMiner(mConfig);
	string outputFileName = miner->GenerateOutputFileName(db);
	string outputFullPath = Bass::GetExpDir() + outputFileName;
	miner->MinePatternsToFile(dbName, outputFullPath, db);
	delete miner;
	delete db;
}

void MineTH::MineTiling() {
	// get dbfilename and outputfilename from config
	string dbName = mConfig->Read<string>("dbname");

	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType);
	bool overlap = mConfig->Read<bool>("overlap", true);
	TilingMiner *miner = NULL;
	if(overlap == true) {
		miner = new OverlappingTilingMiner(mConfig);
	} else {
		miner = new TilingMiner(mConfig);
	}
	string outputFileName = miner->GenerateOutputFileName(db);
	string outputFullPath = Bass::GetExpDir() + outputFileName;
	miner->MinePatternsToFile(dbName, outputFullPath, db);
	delete miner;
	delete db;
}
#endif //BLOCK_TILES

// needs to be tested...
void MineTH::MineLowEntropySets() {
	THROW_NOT_IMPLEMENTED();

	// get dbfilename and outputfilename from config
	string dbName = mConfig->Read<string>("dbname");

	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType);
	LESetMiner *miner = new LESetMiner(mConfig);
	string outputFileName = miner->GenerateOutputFileName(db);
	miner->MinePatternsToFile(db, outputFileName);
	delete miner;
	delete db;
}
