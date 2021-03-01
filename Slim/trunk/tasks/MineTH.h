#ifndef __MINE_H
#define __MINE_H

#include "TaskHandler.h"

class MineTH : public TaskHandler {
public:
	MineTH(Config *conf);
	virtual ~MineTH();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	virtual void MineFrequentSets();
	virtual void MineLowEntropySets();

#ifdef BLOCK_TILES
	virtual void MineTiles();
	virtual void MineTiling();
#endif

	string mTimestamp;
};

#endif // __MINE_H
