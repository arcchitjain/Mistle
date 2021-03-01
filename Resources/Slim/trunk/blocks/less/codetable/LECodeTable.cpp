#ifdef BLOCK_LESS

#include "../../global.h"

#include <logger/Log.h>

#include <db/Database.h>
#include "LowEntropySetCollection.h"
#include "LowEntropySet.h"
#include <itemstructs/ItemSet.h>

// abstract ct
#include "LECodeTable.h"
// specific cts
#include "LESOCodeTable.h"
#include "LECFCodeTable.h"
#include "LECFOptCodeTable.h"


LECodeTable::LECodeTable() {
	mDB = NULL;

	mCurStats.setCountSum = 0;
	mCurStats.dbSize = 0;
	mCurStats.ct1Size = 0;
	mCurStats.ct2Size = 0;
	mCurStats.size = 0;
	mCurStats.numSetsUsed = 0;
	mCurStats.alphItemsUsed = 0;
	mCurStats.likelihood = 0;
	mPrunedStats = mPrevStats = mCurStats;

	mCurNumSets = 0;
	mMaxSetLen = 0;
}

LECodeTable::~LECodeTable() {
}

void LECodeTable::UseThisStuff(Database *db, LEBitmapStrategy bs, uint32 maxSetLen) { 
	mDB = db;
	mNumDBRows = db->GetNumRows();
	mCoverSetType = db->GetDataType();
	mBitmapStrategy = bs;
	mMaxSetLen = maxSetLen;
}
void LECodeTable::SetDatabase(Database *db) {
	mDB = db;
}
void LECodeTable::BackupStats() {
	mPrevStats = mCurStats;
}
void LECodeTable::RollbackStats() {
	mCurStats = mPrevStats;
}

LECodeTable* LECodeTable::Create(const string &name) {
	if(name.compare(LESOCodeTable::GetConfigBaseName()) == 0)
		return new LESOCodeTable();
	else if(name.compare(0,LECFCodeTable::GetConfigBaseName().length(),LECFCodeTable::GetConfigBaseName()) == 0)
		return new LECFCodeTable(name);
	else if(name.compare(0,LECFOptCodeTable::GetConfigBaseName().length(),LECFOptCodeTable::GetConfigBaseName()) == 0)
		return new LECFOptCodeTable(name);
	throw string("LECodeTable::Create -- Unable to create LECodeTable for type ") + name;
}

void LECodeTable::WriteToDisk(const string &filename) {
	throw string("not yet.");
}

#endif // BLOCK_LESS
