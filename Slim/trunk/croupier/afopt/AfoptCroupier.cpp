
#include "../clobal.h"

#include <Bass.h>
#include <Config.h>
#include <TimeUtils.h>
#include <SystemUtils.h>
#include <FileUtils.h>
#include <logger/Log.h>

#include <db/Database.h>
#include <isc/IscFile.h>
#include <isc/ItemSetCollection.h>
#include <itemstructs/ItemSet.h>

#include "AfoptMiner.h"
#include "fsout.h"

#include "AfoptCroupier.h"

AfoptCroupier::AfoptCroupier(Config *config) : Croupier(config) {
	if(config->KeyExists("iscname")) {
		string iscname = config->Read<string>("iscname");
		Configure( iscname );

	} else {
		mDbName = config->Read<string>("dbname");
		mType = config->Read<string>("isctype");
		mMinSup = config->Read<float>("iscminsup");
		mMinSupMode = config->Read<string>("iscminsupmode");
		mOrder = ItemSetCollection::StringToIscOrderType( config->Read<string>("iscorder") );
	}
	mMiner = NULL;
	mMayHaveMore = false;
}
AfoptCroupier::AfoptCroupier(const string &iscname) : Croupier() {
	Configure( iscname );
	mMiner = NULL;
	mMayHaveMore = false;
}
AfoptCroupier::AfoptCroupier() : Croupier() {
	mDbName = "";
	mType = "all";
	mMinSup = 1.0f;
	mMinSupMode = "absolute";
	mOrder = NoneIscOrder;
	mMiner = NULL;
	mMayHaveMore = false;
}

AfoptCroupier::~AfoptCroupier() {
	delete mMiner;
}

void AfoptCroupier::InitOnline(Database *db) {
	Croupier::InitOnline(db);

	// Determine minsup
	uint32 minSup;
	if(mMinSupMode.compare("percentage") == 0)
		minSup = (uint32)(ceil(db->GetNumTransactions() * mMinSup));
	else
		minSup = (uint32)(ceil(mMinSup));

	if(minSup == 0)
		minSup++;

	mMiner = new Afopt::AfoptMiner(db, this, minSup, mType);
	mMinerOut = (Afopt::MemoryOut *)Afopt::gpout;

	uint32 bufferSize = (uint32)(Bass::GetRemainingMaxMemUse() / (uint64)(2.5 * sizeof(uint32) + ItemSet::MemUsage(db)));
	if(bufferSize == 0) bufferSize = 1;
	mMinerOut->InitBuffer(bufferSize);
	mMayHaveMore = true;
}

void AfoptCroupier::MinerIsErVolVanCBFunc(ItemSet **buf, uint32 numSets) {
	// Save Chunk
	ItemSet **iss = new ItemSet *[numSets]; // iss may be much smaller than buffer
	memcpy_s(iss, numSets*sizeof(ItemSet*), buf, numSets*sizeof(ItemSet*));

	ItemSetCollection *isc = new ItemSetCollection(mDatabase);
	isc->SetLoadedItemSets(iss, numSets);
	ECHO(2, printf(" * Mining:\t\tSorting chunk #%d             \r", mOnlineNumChunks+1));
	SetIscProperties(isc);

	ECHO(2, printf(" * Mining:\t\tStoring chunk #%d             \r", mOnlineNumChunks+1));
	string iscTempPath = Bass::GetWorkingDir() + isc->GetTag() + "." + IscFile::TypeToExt(mChunkIscFileType);
	ItemSetCollection::SaveChunk(isc, iscTempPath, mOnlineNumChunks, mChunkIscFileType);
	ECHO(2, printf(" * Mining:\t\tStored chunk #%d             \r", mOnlineNumChunks+1));
	delete isc;
	mOnlineNumChunks++;
	mOnlineNumTotalSets += numSets;

	uint32 bufferSize = (uint32)((Bass::GetRemainingMaxMemUse()+numSets*sizeof(uint32)) / (uint64)(2.5 * sizeof(uint32) + ItemSet::MemUsage(mDatabase)));
	if(bufferSize == 0) bufferSize = 1;
	mMinerOut->ResizeBuffer(bufferSize);
}
void AfoptCroupier::MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets) {
	ItemSet **iss = new ItemSet *[numSets]; // iss may be much smaller than buffer
	memcpy_s(iss, numSets*sizeof(ItemSet*), buf, numSets*sizeof(ItemSet*));

	if(mOnlineNumChunks > 0) {
		ECHO(2, printf(" * Mining:\t\tSorting chunk #%d             \r", mOnlineNumChunks+1));
	} else
		ECHO(2, printf(" * Mining:\t\tSorting sets...            \r"));
	ItemSetCollection *isc = new ItemSetCollection(mDatabase);
	isc->SetNumItemSets(numSets);
	isc->SetLoadedItemSets(iss, numSets);
	SetIscProperties(isc);

	mOnlineIsc = isc;
	mOnlineNumTotalSets += numSets;
	if(mOnlineNumChunks > 0) {
		;//ECHO(2, printf(" * Mining:\t\tFinished mining %d chunks.         \r", mOnlineNumChunks+1));
	} else
		ECHO(2, printf(" * Mining:\t\tFinished.                          \r"));
}
void AfoptCroupier::HistogramMiningIsHistoryCBFunc(uint64 *histogram, uint32 histolen) {
	delete[] mHistogram;
	mHistogram = new uint64[histolen];
	memcpy_s(mHistogram, histolen * sizeof(uint64), histogram, histolen * sizeof(uint64));
}

ItemSetCollection* AfoptCroupier::MinePatternsToMemory(Database *db) {
	// prep da db shizzle
	ECHO(2, printf(" * Mining:\t\tin progress...\r"));
	bool deleteDb = false;
	if(db == NULL) {
		db = Database::RetrieveDatabase(mDbName);
		deleteDb = true;
	}
	mOnlineNumChunks = 0;
	mOnlineNumTotalSets = 0;

	// prep da mining shizzle
	InitOnline(db);

	// do da mining dance
	mMiner->MineOnline();
	// mine has been putted out

	ItemSetCollection *isc = NULL;
	if(mOnlineNumChunks == 0) {
		// All ItemSets aboard!
		isc = mOnlineIsc;
		mOnlineIsc = NULL;
	} else {
		// All Chunks Verzamelen!
		// All Chunks Verzamelen!
		string iscTempPath = Bass::GetWorkingDir();
		string iscTempFileName = mOnlineIsc->GetTag() + "." + IscFile::TypeToExt(mChunkIscFileType);
		string iscTempFullPath = iscTempPath + iscTempFileName;

		ECHO(2, printf(" * Mining:\t\tStoring chunk #%d\t\t\r", mOnlineNumChunks+1));
		ItemSetCollection::SaveChunk(mOnlineIsc, iscTempFullPath, mOnlineNumChunks, mChunkIscFileType);
		delete mOnlineIsc;
		mOnlineIsc = NULL;
		mOnlineNumChunks++;

		ECHO(2, printf(" * Mining:\t\tFinished mining %d chunks\t\t\r", mOnlineNumChunks));

		isc = new ItemSetCollection(db);
		SetIscProperties(isc);
		isc->SetNumItemSets(mOnlineNumTotalSets);
		ItemSetCollection::MergeChunks(iscTempPath, iscTempPath, iscTempFileName, mOnlineNumChunks, mChunkIscFileType, db, isc->GetDataType(), mChunkIscFileType);
		delete isc;
		isc = ItemSetCollection::OpenItemSetCollection(iscTempFileName, iscTempPath, db);
	}
	ECHO(2, printf("\r * Mining:\t\tFinished mining %" I64d " ItemSets.\t\n", isc->GetNumItemSets()));
	return isc;
}
void AfoptCroupier::SetIscProperties(ItemSetCollection *isc) {
	isc->SetDatabaseName(mDbName);
	isc->SetPatternType(mType);
	isc->SetMinSupport(mMiner->GetMinSup());
	isc->SetMaxSetLength(Afopt::gnmax_pattern_len);
	isc->SetHasSepNumRows(mMinerOut->HasBinSizes());
	uint8 ol = Bass::GetOutputLevel();
	Bass::SetOutputLevel(1);
	isc->SortLoaded(mOrder);
	Bass::SetOutputLevel(ol);
}

void AfoptCroupier::Configure(const string &iscname) {
	// example: tictactoe-closed-20d
	// tictactoe-cls-20d
	string settings;
	ItemSetCollection::ParseTag(iscname, mDbName, mType, settings, mOrder);

	if(mType.compare("cls")==0)
		mType = "closed";

	float minsupfloat;
	if(settings.find('.') != string::npos && (minsupfloat = (float)atof(settings.c_str())) <= 1.0f) {
		mMinSup = minsupfloat;
		mMinSupMode = "percentage";
	} else {
		uint32 minsupuint = atoi(settings.c_str());
		mMinSup = (float) minsupuint;
		mMinSupMode = "absolute";
	}
}

string AfoptCroupier::BuildOutputFilename() {
	string filename = mDbName + "-" + mType + "-";
	char sMinSup[40];
	if(mMinSupMode.compare("percentage") == 0) {
		sprintf_s(sMinSup, 40, "%.02f", mMinSup);
	} else { // absolute
		sprintf_s(sMinSup, 40, "%.0f", mMinSup);
	}
	filename += sMinSup + ItemSetCollection::IscOrderTypeToString(mOrder) + ".isc";
	return filename;
}
bool AfoptCroupier::MinePatternsToFile(const string &dbFilename, const string &outputFile, Database *db) {
	if(db != NULL) {
		return AfoptCroupier::MyMinePatternsToFile(db, outputFile);
	} else 
		return AfoptCroupier::MyMinePatternsToFile(dbFilename, outputFile);
}
bool AfoptCroupier::MyMinePatternsToFile(Database *db, const string &outputFile) {
	uint32 minSup;

	// Determine minsup
	if(mMinSupMode.compare("percentage") == 0)
		minSup = (uint32)(ceil(db->GetNumTransactions() * mMinSup));
	else
		minSup = (uint32)(ceil(mMinSup));

	if(minSup == 0)
		minSup++;

	string outFileName = outputFile;
	string outPath = "";
	FileUtils::SplitPathFromFilename(outFileName, outPath);
	string outFullPath = outPath + outFileName;

	string fiPath = outPath;
	string fiTmpFilename = SystemUtils::GetProcessIdString() + "t" + TimeUtils::GetTimeStampString() + ".fi-tmp";
	string fiTmpFullPath = fiPath + fiTmpFilename;
	string fiFileName = outFileName + ".fi";
	string fiFullPath = fiPath + fiFileName;

	//if(!FileUtils::FileExists(fiName)) {
	ECHO(1,printf("** Creating Frequent Item Set Collection.\n"));
	if(mType.compare("all") == 0)
		Afopt::AfoptMiner::MineAllPatternsToFile(db, minSup, fiTmpFullPath);
	else if(mType.compare("closed") == 0)
		Afopt::AfoptMiner::MineClosedPatternsToFile(db, minSup, fiTmpFullPath);
	else
		throw string("AfoptCroupier::MineAllPatternsToFile() -- Currently only FIS types 'all' and 'closed' supported.");
	//}
	if(!FileUtils::FileExists(fiTmpFullPath))
		throw string("AfoptCroupier::MineAllPatternsToFile() -- Temporary output file should be there, but isn't.");
	else {
		if(FileUtils::FileExists(fiFullPath) && !FileUtils::RemoveFile( fiFullPath )) {
			FileUtils::RemoveFile( fiTmpFullPath );
			throw string("AfoptCroupier::MineAllPatternsToFile() -- Target filename already exists, and could not be removed.");
		}
		if(!FileUtils::FileMove(fiTmpFullPath, fiFullPath)) {
			FileUtils::RemoveFile( fiTmpFullPath );
			throw string("AfoptCroupier::MineAllPatternsToFile() -- Error renaming ItemSetCollection file.");
		}
	}

	ItemSetCollection::ConvertIscFile(fiPath, fiFileName, outPath, outFileName, db, mStoreIscFileType, mOrder, false, true, minSup, db->GetDataType());
	FileUtils::RemoveFile( fiFullPath );


	//if(!FileUtils::FileExists(outputFile)) {
	//	if(deleteDb)
	//		delete db;
	//	throw string("AfoptCroupier::MineAllPatternsToFile() -- Frequent ItemSet generation failed.");
	//}

	ECHO(1,printf("** Finished Creating Frequent Item Set Collection\n"));
	ECHO(1,printf("\n"));

	return true;
}
bool AfoptCroupier::MyMinePatternsToFile(const string &dbFilename, const string &outputFile) {
	Database *db;
	if(dbFilename.length() == 0) // no filename given, use dbName for default db
		db = Database::RetrieveDatabase(mDbName);
	else
		db = Database::RetrieveDatabase(dbFilename);
	if(db == NULL)
		throw string("AfoptCroupier::::MineAllPatternsToFile() -- Cannot read database!");

	bool res = MyMinePatternsToFile(db, outputFile);
	delete db;
	return res;
}

void AfoptCroupier::InitHistogramMining(Database *db, const bool setLengthMode) {
	Croupier::InitOnline(db);

	// Determine minsup
	uint32 minSup;
	if(mMinSupMode.compare("percentage") == 0)
		minSup = (uint32)(ceil(db->GetNumTransactions() * mMinSup));
	else
		minSup = (uint32)(ceil(mMinSup));

	if(minSup == 0)
		minSup++;

	mMiner = new Afopt::AfoptMiner(db, this, minSup, mType, setLengthMode ? Afopt::MINE_GOAL_HISTOGRAM_SETLEN : Afopt::MINE_GOAL_HISTOGRAM_SUPPORT);
	mHistoOut = (Afopt::HistogramOut *)Afopt::gpout;

	mMayHaveMore = true;
}
uint64* AfoptCroupier::MineHistogram(Database *db, const bool setLengthMode) {
	// prep da db shizzle
	bool deleteDb = false;
	if(db == NULL) {
		db = Database::RetrieveDatabase(mDbName);
		deleteDb = true;
	}

	// prep da mining shizzle
	InitHistogramMining(db, setLengthMode);

	// do da mining dance
	mMiner->MineOnline();
	// mine has been putted out

	return mHistogram;// ((Afopt::HistogramOut *)mMinerOut)->GetHistogram();
}


string AfoptCroupier::GetPatternTypeStr() {
	return mMiner->GetPatternTypeStr();
}
