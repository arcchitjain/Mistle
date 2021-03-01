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
#include "../global.h"

// system
#include <time.h>
#if defined (_WINDOWS)
	#include <direct.h>
#elif defined (__unix__)
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
#ifdef BLOCK_TILES
#include <tiles/TileMiner.h>
#include "../blocks/tile/TilingMiner.h"
#include "../blocks/tile/OverlappingTilingMiner.h"
#endif

// lesets
#ifdef BLOCK_LESS
#include <lesets/LESetMiner.h>
#endif

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
#ifdef BLOCK_LESS
	else if(command.compare("le") == 0)				MineLowEntropySets();
#endif
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
	miner->MinePatternsToFile(db, outputFileName);
	delete miner;
	delete db;
}

void MineTH::MineTiling() {
	OverlappingTilingMiner *miner = new OverlappingTilingMiner(mConfig);
	miner->MineTiling();
	delete miner;
}
#endif //BLOCK_TILES

#ifdef BLOCK_LESS
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
#endif //BLOCK_LESS
