#ifdef ENABLE_TILES

#include "../global.h"

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <ArrayUtils.h>
#include <StringUtils.h>
#include <RandomUtils.h>
#include <TimeUtils.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <itemstructs/CoverSet.h>

#include "../blocks/tile/TileMiner.h"
#include "../blocks/tile/TilingMiner.h"
#include "../blocks/tile/OverlappingTilingMiner.h"

#include "TileTH.h"

TileTH::TileTH(Config *conf) : TaskHandler(conf){

}
TileTH::~TileTH() {
	// not my Config*
}

void TileTH::HandleTask() {
	string command = mConfig->Read<string>("command");

	if(command.compare("minetiles") == 0)					MineTiles();
	else	if(command.compare("minetiling") == 0)			MineTiling();

	else	throw string(__FUNCTION__) + ": Unable to handle task `" + command + "`";
}

string TileTH::BuildWorkingDir() {
	//char s[100];
	//_i64toa_s(Logger::GetCommandStartTime(), s, 100, 10);
	return mConfig->Read<string>("command") + "/";
}

void TileTH::MineTiles() {
	TileMiner *miner = new TileMiner(mConfig);
	miner->MineTiles();
	delete miner;
}

void TileTH::MineTiling() {
	OverlappingTilingMiner *miner = new OverlappingTilingMiner(mConfig);
	miner->MineTiling();
	delete miner;
}

#endif // ENABLE_TILES
