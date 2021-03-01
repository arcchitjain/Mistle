
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

#include "AprioriMiner.h"

#include "AprioriCroupier.h"

AprioriCroupier::AprioriCroupier(Config *config) : Croupier(config) {
	if(config->KeyExists("iscname")) {
		string iscname = config->Read<string>("iscname");
		Configure( iscname );

	} else {
		mDbName = config->Read<string>("dbname");
		mType = config->Read<string>("isctype");
		mMaxLen = config->Read<uint32>("iscmaxlen");
		mMinSup = config->Read<float>("iscminsup");
		mMinSupMode = config->Read<string>("iscminsupmode");
		mOrder = ItemSetCollection::StringToIscOrderType( config->Read<string>("iscorder") );
	}
}
AprioriCroupier::AprioriCroupier(const string &iscname) : Croupier() {
	Configure( iscname );
}
AprioriCroupier::AprioriCroupier() : Croupier() {
	mDbName = "";
	mType = "all";
	mMaxLen = UINT32_MAX_VALUE;
	mMinSup = 1.0f;
	mMinSupMode = "absolute";
	mOrder = NoneIscOrder;
}

AprioriCroupier::~AprioriCroupier() {
	delete mMiner;
}

void AprioriCroupier::InitOnline(Database *db) {
	Croupier::InitOnline(db);

	// Determine minsup for this db
	uint32 minSup;
	if(mMinSupMode.compare("percentage") == 0)
		minSup = (uint32)(ceil(db->GetNumTransactions() * mMinSup));
	else
		minSup = (uint32)(ceil(mMinSup));

	if(minSup == 0)
		minSup++;

	mMiner = new AprioriMiner(db, minSup, mMaxLen);
}
ItemSetCollection* AprioriCroupier::MinePatternsToMemory(Database *db) {
	// prep da db shizzle
	printf(" * Mining:\t\tin progress...\r");
	bool deleteDb = false;
	if(db == NULL) {
		db = Database::RetrieveDatabase(mDbName);
		deleteDb = true;
	}
	mOnlineNumChunks = 0;
	mOnlineNumTotalSets = 0;

	if(db->HasBinSizes())
		THROW("This croupier don't like no binning.");

	// prep da mining shizzle
	InitOnline(db);

	// Build tree
	mMiner->BuildTree();

	// Init buffer
	uint32 bufferSize = (uint32)(Bass::GetRemainingMaxMemUse() / (uint64)(2.5 * sizeof(uint32) + ItemSet::MemUsage(db->GetDataType(), (uint32) db->GetAlphabetSize(), mMaxLen)));
	if(bufferSize == 0) bufferSize = 1;
	ItemSet **buffer = new ItemSet *[bufferSize];

	// Gather itemsets
	mMiner->MineOnline(this, buffer, bufferSize);
	// mine has been putted out

	// Cleanup tree, no longer needed
	mMiner->ChopDownTree();

	// Build ISC
	ItemSetCollection *isc = NULL;
	if(mOnlineNumChunks == 0) {
		// All ItemSets aboard!
		isc = mOnlineIsc;
		mOnlineIsc = NULL;

		// Compute stdLens for loaded itemsets
		double *stdLens = mDatabase->GetStdLengths();
		ItemSet **iss = isc->GetLoadedItemSets();
		uint64 numSets = isc->GetNumItemSets();
		uint16 *values = new uint16[isc->GetMaxSetLength()];
		for(uint64 i=0; i<numSets; i++) {
			iss[i]->GetValuesIn(values);
			double len = 0.0;
			for(uint32 j=0; j<iss[i]->GetLength(); j++)
				len += stdLens[values[j]];
			iss[i]->SetStandardLength(len);
		}
		delete[] values;
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

	printf("\r * Mining:\t\tFinished mining %" I64d " ItemSets.\t\n", isc->GetNumItemSets());

	delete[] buffer;
	return isc;
}
void AprioriCroupier::MinerIsErVolVanCBFunc(ItemSet **buf, uint32 numSets) {
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
	ECHO(2, printf(" * Mining:\t\tStored chunk #%d (gathering more itemsets)\r", mOnlineNumChunks+1));
	delete isc;
	mOnlineNumChunks++;
	mOnlineNumTotalSets += numSets;
}
void AprioriCroupier::MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets) {
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
void AprioriCroupier::SetIscProperties(ItemSetCollection *isc) {
	isc->SetDatabaseName(mDbName);
	isc->SetPatternType(mType);
	isc->SetMinSupport(mMiner->GetMinSup());
	isc->SetMaxSetLength(mMiner->GetActualMaxLen());
	isc->SetHasSepNumRows(false); // not (yet) supported
	uint8 ol = Bass::GetOutputLevel();
	Bass::SetOutputLevel(1);
	isc->SortLoaded(mOrder);
	Bass::SetOutputLevel(ol);
}

void AprioriCroupier::Configure(const string &iscname) {
	// example: tictactoe-closed-20d
	// tictactoe-cls-20d
	string settings;
	ItemSetCollection::ParseTag(iscname, mDbName, mType, settings, mOrder);

	if(mType.compare("cls")==0)
		mType = "closed";
	if(mType.compare("closed")==0)
		THROW("Mining closed itemsets not (yet) supported by this miner.");

	if(mType.length() > 7 && mType.substr(0,7).compare("maxlen[") == 0) {		// maxlen[x] -- maxlen specified
		size_t pos = mType.find('[');
		mMaxLen = atoi(mType.c_str()+pos+1);
	} else {
		mMaxLen = mDatabase->GetMaxSetLength();
	}

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
bool AprioriCroupier::MinePatternsToFile(const string &dbFilename, const string &outputFile, Database *db) {
	bool zapDB = false;
	if(db != NULL) {
		if(dbFilename.length() == 0) { THROW("No database specified."); }
		db = Database::RetrieveDatabase(dbFilename);
		if(db == NULL) { THROW("Cannot retrieve database!"); }
		zapDB = true;
	}
	string iscName = BuildOutputFilename();
	iscName = iscName.substr(0, iscName.find_last_of("."));

	ItemSetCollection *isc = MinePatternsToMemory(db);
	ItemSetCollection::WriteItemSetCollection(isc, iscName, Bass::GetWorkingDir(), FicIscFileType);
	delete isc;

	if(zapDB)
		delete db;
	return true;
}
string AprioriCroupier::BuildOutputFilename() {
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
string AprioriCroupier::GetPatternTypeStr() {
	return mType;
}
