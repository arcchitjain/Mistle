#include "../global.h"

// system
#include <time.h>

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>
#include <logger/Log.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <db/ClassedDatabase.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <isc/BinaryFicIscFile.h>
#include <isc/ItemTranslator.h>

// fic
#include "../blocks/classifier/Classifier.h"

// leluk
#include "../FicMain.h"

#include "TaskHandler.h"
#include "DataTransformTH.h"

DataTransformTH::DataTransformTH(Config *conf) : TaskHandler(conf){
}

DataTransformTH::~DataTransformTH() {
	// not my Config*
}

void DataTransformTH::HandleTask() {
	Logger::SetLogToDisk(false); // log only to stdout by default

	string command = mConfig->Read<string>("command");
	if(command.compare("convertdb") == 0)				ConvertDB();
	else	if(command.compare("isc2bisc") == 0)		Isc2Bisc();
	//else	if(command.compare("bindb") == 0)			BinDB();
	//else	if(command.compare("unbindb") == 0)			UnBinDB();
	else	if(command.compare("convertisc") == 0)		ConvertISC();
	else	if(command.compare("swaprnd") == 0)			SwapRandomiseDB();
	else	if(command.compare("mergechunks") == 0)		MergeChunks();
	else	if(command.compare("split") == 0)			SplitDB();
	else	if(command.compare("sample") == 0)			SampleDB();

	else	throw string("DataTransformTH :: Unable to handle task `" + command + "`");
}

void DataTransformTH::ConvertDB() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType, DbFile::StringToType(mConfig->Read<string>("dbinencoding")));
	string path = db->GetPath(); // store in same dir as where the db was found

	/*	-- obsolete, pruned on compress
	uint32 minSup = mConfig->Read<uint32>("dboutprune");
	if(minSup != 0) {
	db->PruneBelowSupport(minSup);
	char s[10];
	itoa(minSup, s, 10);
	dbName += s;
	}*/

	ECHO(2, printf(" * Processing Data: ") );

	uint32 countMultiplier = mConfig->Read<uint32>("dbcntmultiplier", 1);
	if(countMultiplier > 1) {
		ECHO(2, printf("\r * Processing Data: Multiplying Counts") );
		db->MultiplyCounts(countMultiplier);
		char temp[50];
		_itoa(countMultiplier, temp, 10);
		dbName = dbName + "[multi," + temp + "]";
	}

	uint32 givenAlphSize = mConfig->Read<uint32>("usealphabetsize", 0);
	if(givenAlphSize != 0) {
		ECHO(2, printf("\r * Processing Data: Given Alphabet Size  ") );
		db->SetAlphabetSize(givenAlphSize);
	}

	if(mConfig->Read<bool>("dbouttranspose", false)) {
		ECHO(2, printf("\r * Processing Data: Transposing Data  ") );
		Database *newDb = db->Transpose();
		delete db;
		db = newDb;
		dbName = dbName + "[transposed]";
	}

	if(mConfig->Read<string>("dbOutSelectColumns", "all").compare("all") != 0) {
		ECHO(2, printf("\r * Processing Data: Selecting Columns ") );
		if(db->HasColumnDefinition() == false)
			throw string("You can only select columns from a database that has a column definition!");
		uint32 numSelectCols = 0;
		uint32 *selectCols = StringUtils::TokenizeUint32(mConfig->Read<string>("dbOutSelectColumns"), numSelectCols, ",");
		uint32 numCols = db->GetNumColumns();
		ItemSet **colDef = db->GetColumnDefinition();
		ItemSet **newColDef = new ItemSet*[numSelectCols];
		ItemSet *zapItems = ItemSet::CreateEmpty(db->GetDataType(),(uint32) db->GetAlphabetSize());
		for(uint32 i=0; i<numCols; i++) {
			bool select = false;
			for(uint32 j=0; j<numSelectCols; j++)
				if(selectCols[j]-1 == i) {
					select = true;
					newColDef[j] = colDef[i]->Clone();
					break;
				}
			if(select == false)
				zapItems->Unite(colDef[i]);
		}
		db->RemoveItems(zapItems);
		db->SetColumnDefinition(newColDef,numSelectCols);
		delete zapItems;
		delete[] selectCols;
		dbName = dbName + "[SC-" + mConfig->Read<string>("dbOutSelectColumns") + "]";
	}

	if(mConfig->Read<bool>("dbouttranslatefw", false)) {
		ECHO(2, printf("\r * Processing Data: Translating Forward") );
		if(mConfig->KeyExists("usealphabetfrom")) {
			string itDbFile = mConfig->Read<string>("usealphabetfrom");
			Database *itDb = Database::RetrieveDatabase(itDbFile, db->GetDataType());
			ItemTranslator *it = itDb->GetItemTranslator();
			if(it == NULL)
				throw string("convertdb -- Item translator required when using 'useAlphabetFrom' option.");

			ItemTranslator *itOld = db->GetItemTranslator();
			if(itOld != NULL)
				delete itOld;
			db->SetItemTranslator(new ItemTranslator(it));
			db->TranslateForward();
			db->UseAlphabetFrom(itDb);
			delete itDb;
		} else {
			db->TranslateForward();
		}
	}

	if(mConfig->Read<bool>("dboutorderintrans", false)) {
		ECHO(2, printf("\r * Processing Data: Sorting within Rows") );
		db->SortWithinRows();
	}

	if(mConfig->Read<bool>("dboutremovedoubleitems", false)) {
		ECHO(2, printf("\r * Processing Data: Removing double Items") );
		db->RemoveDoubleItems();
	}

	if(mConfig->KeyExists("dboutremovetranslongerthan")) {
		uint32 removeLongerThan = mConfig->Read<uint32>("dboutremovetranslongerthan");
		ECHO(2, printf("\r * Processing Data: Removing Transactions Longer than %d items", removeLongerThan) );
		db->RemoveTransactionsLongerThan(removeLongerThan);
		char temp[100];
		sprintf_s(temp, 100, "[maxlen%d]", removeLongerThan);
		dbName = dbName + temp;
	}

	if(mConfig->KeyExists("dboutremovefrequentitems")) {
		float freqThresh = mConfig->Read<float>("dboutremovefrequentitems");
		ECHO(2, printf("\r * Processing Data: Removing Items More Frequent than %.3f%%", freqThresh) );
		db->RemoveFrequentItems(freqThresh);
		char temp[100];
		sprintf_s(temp, 100, "[maxfreq%.3f]", freqThresh);
		dbName = dbName + temp;
	}

	if(mConfig->Read<bool>("dbOutBinned", false)) {
		ECHO(2, printf("\r * Processing Data: Binning Database     ") );
		db->Bin();
		dbName = dbName + "[binned]";
	}

	if(mConfig->Read<bool>("dbOutUnbinned", false)) {
		ECHO(2, printf("\r * Processing Data: Unbinning Database   ") );
		db->UnBin();
		dbName = dbName + "[unbinned]";
	}

	if(mConfig->Read<bool>("dboutclassed", false)) {
		ECHO(2, printf("\r * Processing Data: Classing Database    ") );
		Database *newDb = NULL;
		/* for ACMK -- Custom ClassDef
		uint16 classes[2];
		classes[0] = 0;
		classes[1] = 1;
		db->SetClassDefinition(2, classes);
		// */
		if(db->HasClassDefinition()) {
			newDb = new ClassedDatabase(db);
		} else
			throw string("DataTransformTH::ConvertDB() -- No class definition in DB!");
		delete db;
		db = newDb;
		dbName = dbName + "[classed]";
	}

	char temp[500];
	if(mConfig->KeyExists("dboutremoveitems")) {
		ECHO(2, printf("\r * Processing Data: Removing some items    ") );
		string s = mConfig->Read<string>("dboutremoveitems");
		uint32 numItems = 0;
		uint16 *items = StringUtils::TokenizeUint16(s, numItems);
		db->RemoveItems(items, numItems);
		if(numItems > 5) {
			sprintf_s(temp, 500, "[min%ditems]", numItems);
			dbName = dbName + temp;
		} else {
			dbName = dbName + "[min";
			for(uint32 i=0; i<numItems; i++) {
				sprintf_s(temp, 500, "%d", items[i]);
				dbName = dbName + temp;
			}
			dbName = dbName + "]";
		}
		delete items;
	}

	if(mConfig->Read<bool>("dboutremovedoubletransactions", false)) {
		ECHO(2, printf("\r * Processing Data: Removing double Transactions") );
		db->RemoveDuplicateTransactions();
		dbName = dbName + "[unique]";
	}

	ECHO(2, printf("\r * Processing Data: Writing Data              ") );
	string dbOutName = mConfig->Read<string>("dbOutName","");
	if(dbOutName.length() == 0)
		dbOutName = dbName;

	try {
		Database::WriteDatabase(db, dbOutName, path, DbFile::StringToType(mConfig->Read<string>("dboutencoding")));
		ECHO(2, printf("\r * Processing Data: Finished                  \n") );
	} catch(char*) {
		delete db;
		throw;
	}
	delete db;
}
void DataTransformTH::BinDB() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName, "", Uint16ItemSetType);
	db->Bin();
	string binDBName = dbName + "[binned]";
	try {
		Database::WriteDatabase(db, binDBName);
	} catch(char*) {
		delete db;
		throw;
	}
	delete db;
}
void DataTransformTH::UnBinDB() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName, "", Uint16ItemSetType);
	db->UnBin();
	string unbinDBName = dbName + "-unbinned";
	try {
		Database::WriteDatabase(db, unbinDBName);
	} catch(char*) {
		delete db;
		throw;
	}
	delete db;
}

void DataTransformTH::ConvertISC() {
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype", ItemSet::TypeToString(Uint16ItemSetType)));
	Database *db = Database::RetrieveDatabase(dbName, dataType);

	string inIscName = mConfig->Read<string>("inIscName");
	string inIscTypeStr = mConfig->Read<string>("inIscType");
	string inIscFileName = inIscName + "." + inIscTypeStr;
	string inIscPath = mConfig->Read<string>("inIscPath", Bass::GetIscReposDir());

	if(!FileUtils::FileExists(inIscPath + inIscFileName))
		THROW("Ey, whut?");

	string outIscName = mConfig->Read<string>("outIscName");
	string outIscTypeStr = mConfig->Read<string>("outIscType");
	string outIscFileName = outIscName + "." + outIscTypeStr;
	string outIscPath = mConfig->Read<string>("outIscPath", Bass::GetWorkingDir());
	string outIscOrderStr = mConfig->Read<string>("outiscorder");

	IscFileType outIscType = IscFile::StringToType(outIscTypeStr);
	IscOrderType outIscOrder = ItemSetCollection::StringToIscOrderType(outIscOrderStr);

	bool translate = mConfig->Read<bool>("translate");
	bool zapFromFile = mConfig->Read<bool>("zaporiginal");

	ItemSetCollection::ConvertIscFile(inIscPath, inIscFileName, outIscPath, outIscFileName, db, outIscType, outIscOrder, translate, zapFromFile);

	delete db;
}


void DataTransformTH::SwapRandomiseDB() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType);

	uint32 numSwaps = 0;
	string numSwapsStr = mConfig->Read<string>("numswaps", "1.0");
	if(numSwapsStr.find('.')!=string::npos) {
		numSwaps = (uint32) ((double) db->GetNumItems() * mConfig->Read<double>("numswaps"));
	} else
		numSwaps = mConfig->Read<uint32>("numswaps");

	uint32 num = mConfig->Read<uint32>("numdbs", 1);
	string swapName;
	char s[10];

	string outDir = Bass::GetExpDir() + "swprnd/";
	FileUtils::CreateDirIfNotExists(outDir);

	try {
		for(uint32 i=1; i<=num; i++) {
			ECHO(1,	printf(" * SwapRnd databases:\t\tdb_s%d...\n", i));
			Database *sdb = db->Clone();

			sdb->SwapRandomise();
			_itoa(i, s, 10);

			swapName = dbName + "_n" + numSwapsStr + "_s" + s;
			Database::WriteDatabase(db, swapName, outDir);
			delete sdb;
		}

	} catch(char*) {
		delete db;
		throw;
	}
	ECHO(1,	printf(" * SwapRnd databases:\t\tdone...\n"));
	delete db;
}

void DataTransformTH::Isc2Bisc() {
	// Determine ISC properties
	string iscName = mConfig->Read<string>("iscname");

	// Determine DB properties
	string dbName = mConfig->Read<string>("dbname", iscName.substr(0,iscName.find_first_of('-')));
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));

	// Read DB
	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);

	// Get ISC
	string iscFileName = iscName + ".isc";
	string iscFullPath = Bass::GetIscReposDir() + iscFileName;
	ItemSetCollection* isc = ItemSetCollection::RetrieveItemSetCollection(iscFileName, db, false);
	
	string biscFullPath = Bass::GetIscReposDir() + iscName + ".bisc";

	BinaryFicIscFile* iscFile = new BinaryFicIscFile();
	iscFile->WriteItemSetCollection(isc, biscFullPath);

	delete iscFile;
	delete isc;
	delete db;
}

void DataTransformTH::MergeChunks() {
	// Determine ISC properties
	string iscName = mConfig->Read<string>("iscname");
	string inPath = mConfig->Read<string>("inpath", Bass::GetExecDir());
	string outPath = mConfig->Read<string>("outpath", Bass::GetExecDir());
	string iscFileName = iscName + "." + mConfig->Read<string>("iscOutExt");

	uint32 numChunks = mConfig->Read<uint32>("numchunks");

	// Determine DB properties
	string dbName = mConfig->Read<string>("dbname", iscName.substr(0,iscName.find_first_of('-')));
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));

	// Read DB
	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);

	ItemSetCollection::MergeChunks(inPath, outPath, iscFileName, numChunks, FicIscFileType, db, dataType, FicIscFileType);

	printf("victory!\n");

	delete db;
}

void DataTransformTH::SplitDB() {
	string dbName = mConfig->Read<string>("dbname");
	uint32 numParts = mConfig->Read<uint32>("numParts");
	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType);
	db->RandomizeRowOrder();
	Database **dbs = db->Split(numParts);

	string outDir = Bass::GetExpDir() + "split/";
	FileUtils::CreateDirIfNotExists(outDir);

	char s[10];
	for (uint32 i = 0; i < numParts; ++i) {
		_itoa(i, s, 10);
		Database::WriteDatabase(dbs[i], dbName + '_' + s, outDir);
	}

	// Cleanup
	for (uint32 i = 0; i < numParts; ++i) {
		delete dbs[i];
	}
	delete[] dbs;
	delete db;
}

void DataTransformTH::SampleDB() {
	string dbName = mConfig->Read<string>("dbname");
	float sampleRate = mConfig->Read<float>("sampleRate");
	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType);
	db->RandomizeRowOrder();
	Database *sample = db->Subset(0, (uint32)(sampleRate * db->GetNumRows()));

	string outDir = Bass::GetExpDir() + "sample/";
	FileUtils::CreateDirIfNotExists(outDir);

	char s[10];
	_itoa(sample->GetNumRows(), s, 10);

	Database::WriteDatabase(sample, dbName + "_sample_" + s, outDir);

	delete sample;
	delete db;
}
