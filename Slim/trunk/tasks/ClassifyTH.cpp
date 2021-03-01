#ifdef ENABLE_CLASSIFIER

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
#include <FileUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>
#include <logger/Log.h>

// bass
#include <Bass.h>
#include <db/ClassedDatabase.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <isc/ItemTranslator.h>

// fic
#include "../blocks/krimp/codetable/CodeTable.h"
#include "../blocks/classifier/Classifier.h"
#include "../blocks/dgen/cover/CTree.h"

// leluk
#include "../FicMain.h"

#include "TaskHandler.h"
#include "ClassifyTH.h"

ClassifyTH::ClassifyTH(Config *conf) : TaskHandler(conf){
	mTimestamp = TimeUtils::GetTimeStampString();
	mTag = "";
}

ClassifyTH::~ClassifyTH() {
	// not my Config*
}

void ClassifyTH::HandleTask() {
	string command = mConfig->Read<string>("command");
	if(command.compare("classifycompress") == 0)			ClassifyCompress();
	else	if(command.compare("classifyanalyse") == 0)		ClassifyAnalyse();
	else	if(command.compare("computecodelengths") == 0)	ComputeCodeLengths();
	else	if(command.compare("computecovertree") == 0)	ComputeCoverTree();
	else	throw string("ClassifyTH :: Unable to handle task `" + command + "`");
}

string ClassifyTH::BuildWorkingDir() {
	string command = mConfig->Read<string>("command");

	if(command.compare("classifycompress") == 0) {
		mTag = mConfig->Read<string>("iscname") + "-" + mConfig->Read<string>("prunestrategy");
		string algo = mConfig->Read<string>("algo");
		{ // get CT type/name
			if(algo.compare(0, 6, "krimp-") == 0)
				algo = algo.substr(6);
			else if (algo.compare(0, 8, "krimp^2-") == 0)
				algo = algo.substr(8);
			else if(algo.compare(0, 7, "krimp*-") == 0)
				algo = algo.substr(7);
			else if(algo.compare(0, 7, "krimp'-") == 0)
				algo = algo.substr(7);
			else if(algo.compare(0, 5, "slim-") == 0)
				algo = algo.substr(5);
			else if(algo.compare(0, 6, "slim*-") == 0)
				algo = algo.substr(6);
			else if(algo.compare(0, 6, "slim+-") == 0)
				algo = algo.substr(6);
			else if(algo.compare(0, 7, "slimCS-") == 0)
				algo = algo.substr(7);
			else if(algo.compare(0, 7, "slimMJ-") == 0)
				algo = algo.substr(7);
			CodeTable *ct = CodeTable::Create(algo, Uint16ItemSetType); // ??? whoa, ugly
			mTag += "-" + ct->GetShortName();
			delete ct;
		}
		mTag += "-" + mTimestamp;
		return "classify/" + mTag + "/";
	} else	if(command.compare("classifyanalyse") == 0)
		return "classify/" + mConfig->Read<string>("exptag") + "/";
	return "";
}


/* ----------------- Phase 1: prepare & execute compression ---------------- */

void ClassifyTH::ClassifyCompress() {
	char temp[200]; 
	string s;

	// Determine ISC properties
	string iscName = mConfig->Read<string>("iscname");

	// Determine DB properties
	string dbName = mConfig->Read<string>("dbname", iscName.substr(0,iscName.find_first_of('-')));
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));

	// Read DB
	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);

	// Read config values
	string pruneStrategy = mConfig->Read<string>("prunestrategy","nop");
	uint32 numFolds		= mConfig->Read<uint32>("numfolds");
	time_t seed			= mConfig->Read<uint32>("seed");
	if(seed == 0)		{ time(&seed); mConfig->Set("seed", seed); }
	string sClassDef	= mConfig->Read<string>("classdefinition", "");
	
	string expDir = Bass::GetWorkingDir();
	if (!FileUtils::CreatePathIfNotExists(expDir)) throw string("An error occurred while creating directory: ") + expDir;
	
	//if (!FileUtils::DirectoryExists(expDir)) 
/*#if defined (_WINDOWS)
		if (FileUtils::CreateDir(expDir)) throw string("An error occurred while creating directory: ") + expDir;
#elif defined (__unix__)
		if (mkpath(expDir.c_str(), 0777)) throw string("An error occurred while creating directory: ") + expDir;
#endif*/

	// Store some settings & backup config
	{
		s = pruneStrategy;
		sprintf_s(temp, 200, " s%d", seed); s += temp;
		sprintf_s(temp, 200, " %df", numFolds); s += temp;
		FILE *fp;
		fopen_s(&fp, (expDir + "_" + s).c_str(), "w");
		fputs(s.c_str(), fp);
		fclose(fp);

		string confFile = expDir + mConfig->Read<string>("command") + "-" + mTimestamp + ".conf";
		mConfig->WriteFile(confFile);
		sprintf_s(temp, 200, "** Classification ::\n * Seed:\t\t\t\t%d", seed); s = temp; LOG_MSG(s);
		sprintf_s(temp, 200, " * numFolds:\t\t\t%d", numFolds); s = temp; LOG_MSG(s);
	}

	// Build target classes array
	if(sClassDef.size() > 0) {
		uint32 num = 0;
		uint16 *classes = StringUtils::TokenizeUint16(sClassDef, num);
		db->SetClassDefinition(num, classes);
	}
	uint32 numClasses = db->GetNumClasses();
	uint16 *classes = db->GetClassDefinition();

	// Test classlabel consistency
	{
		ClassedDatabase *cdb = new ClassedDatabase(db);
		uint32 count = 0;
		uint32 *freqs = cdb->GetClassSupports();
		for(uint32 i=0; i<numClasses; i++)
			count += freqs[i];
		if(count != db->GetNumRows())
			throw string("ClassifyTH::ClassifyCompress -- Classlabels or class definition not good.");
		delete cdb;
	}

	// Prepare database and partitioning
	db->RandomizeRowOrder((uint32)seed);

	// Split for cross-validation
	sprintf_s(temp, 200, " * Split database:\t\tcreating %d partitions", numFolds); s = temp; LOG_MSG(s);
	uint32 *partitionSizes = new uint32[numFolds];
	ItemSet ***partitions = db->SplitForCrossValidation(numFolds, partitionSizes);

	// For each fold
	for(uint32 f=1; f<=numFolds; f++) {
		sprintf_s(temp, 200, " * Creating:\t\t\tfold %d", f); s = temp; LOG_MSG(s);

		// Fold dir
		sprintf_s(temp, 200, "f%d/", f);
		string dir = expDir + temp;
		if(!FileUtils::CreateDir(dir)) throw string("Unable to create fold output directory");

		// Build test database
		uint32 size = partitionSizes[f-1];
		ItemSet **iss = new ItemSet *[size];
		for(uint32 i=0; i<size; i++)
			iss[i] = partitions[f-1][i]->Clone();
		ClassedDatabase *testDb = new ClassedDatabase(iss, size, db->HasBinSizes());
		testDb->SetAlphabet(new alphabet(*db->GetAlphabet()));
		testDb->SetMaxSetLength(db->GetMaxSetLength());
		if(db->GetItemTranslator() != NULL)
			testDb->SetItemTranslator(new ItemTranslator(db->GetItemTranslator()));
		uint16 *classdef = new uint16[numClasses];
		memcpy(classdef, classes, numClasses * sizeof(uint16));
		testDb->SetClassDefinition(numClasses, classdef);
		testDb->ExtractClassLabels();
		Database::WriteDatabase(testDb, string("test"), dir);
		delete testDb;

		// Build training database
		if(numFolds == 1) {		// Special case: 1 fold
			size = partitionSizes[0];
			iss = new ItemSet *[size];
			for(uint32 i=0; i<size; i++)
				iss[i] = partitions[0][i]->Clone();

		} else {				// > 1 fold.
			size = 0;
			for(uint32 c=0; c<numFolds; c++)
				if(c != f-1)
					size += partitionSizes[c];
			iss = new ItemSet *[size];
			size = 0;
			if(db->HasBinSizes()) {				// --> Possible optimisation: use optimised Database::Bin() <--
				ItemSet *is;
				uint32 j;
				for(uint32 c=0; c<numFolds; c++)
					if(c != f-1)
						for(uint32 i=0; i<partitionSizes[c]; i++) {
							is = partitions[c][i];
							for(j=0; j<size; j++) { // try to merge with existing collection
								if(iss[j]->Equals(is)) {
									iss[j]->AddUsageCount(is->GetUsageCount());
									break;
								}
							}
							if(j == size) { // not found, so add
								iss[size++] = is->Clone();
							}
						}

			} else {
				for(uint32 c=0; c<numFolds; c++)
					if(c != f-1)
						for(uint32 i=0; i<partitionSizes[c]; i++)
							iss[size++] = partitions[c][i]->Clone();
			}
		}
		ClassedDatabase *trainingDb = new ClassedDatabase(iss, size, db->HasBinSizes());
		trainingDb->SetAlphabet(new alphabet(*db->GetAlphabet()));
		trainingDb->SetMaxSetLength(db->GetMaxSetLength());
		if(db->GetItemTranslator() != NULL)
			trainingDb->SetItemTranslator(new ItemTranslator(db->GetItemTranslator()));
		classdef = new uint16[numClasses];
		memcpy(classdef, classes, numClasses * sizeof(uint16));
		trainingDb->SetClassDefinition(numClasses, classdef);
		trainingDb->ExtractClassLabels();
		Database::WriteDatabase(trainingDb, string("train"), dir);

		// Split per class for compression
		sprintf_s(temp, 200, " * Compressing:\t\t\tfold %d", f); s = temp; LOG_MSG(s);
		Database **classDbs = trainingDb->SplitOnClassLabels();
	
		for(uint32 i=0; i<numClasses; i++) {
			_itoa(classes[i], temp, 10);
			string classDbName = string("train") + temp;
			Database::WriteDatabase(classDbs[i], classDbName, dir);
			classDbs[i]->SetName(classDbName);

			// Compress
			string classDbDir = dir + classDbName + "/";
#if defined (_WINDOWS)
			_mkdir(classDbDir.c_str());
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
			mkdir(classDbDir.c_str(), 0777);
#endif
			//_fcloseall();
			FicMain::MineAndWriteCodeTables(classDbDir, classDbs[i], mConfig, true);
		}

		// Cleanup
		for(uint32 i=0; i<numClasses; i++)
			delete classDbs[i]; // Postpone until here; otherwise static mess is already wiped away...
		delete[] classDbs;
		delete trainingDb;
	}

	// Configure and write classify config
	Config *config = new Config();
	config->Set("taskclass", "classify");
	config->Set("command", "classifyanalyse");
	config->Set("takeiteasy", mConfig->Read<string>("takeiteasy"));
	config->Set("exptag", mTag);
	config->Set("numfolds", mConfig->Read<string>("numfolds"));
	config->Set("classifypercentage", mConfig->Read<string>("classifypercentage"));
	config->Set("allMatchings", mConfig->Read<string>("allMatchings"));
	config->Set("datatype", ItemSet::TypeToString(db->GetDataType()));
	config->Set("reportsup", mConfig->Read<string>("reportsup"));
	config->Set("algo", mConfig->Read<string>("algo"));
	config->Set("thresholdBits", mConfig->Read<string>("thresholdBits"));
	s = expDir + "classify.conf";
	config->WriteFile(s);
	delete config;

	// Cleanup
	for(uint32 p=0; p<numFolds; p++) {
		for(uint32 i=0; i<partitionSizes[p]; i++)
			delete partitions[p][i];
		delete[] partitions[p];
	}
	delete[] partitions;
	delete[] partitionSizes;
	delete db;
}

/* ------------------- Phase 2: perform actual classification & analyse ------------------- */

void ClassifyTH::ClassifyAnalyse() {
	ECHO(1, printf("** Classification ::\n"));

	if(mConfig->KeyExists("allmatchings") && mConfig->Read<uint32>("allmatchings")==1) {
		Classifier *cl;
		mConfig->Set<uint32>("classifypercentage",0);
		cl = new Classifier(mConfig);
		cl->Classify();
		delete cl;
		mConfig->Set<uint32>("classifypercentage",1);
		cl = new Classifier(mConfig);
		cl->Classify();
		delete cl;
	} else {
		Classifier *cl = new Classifier(mConfig);
		cl->Classify();
		delete cl;
	}

	ECHO(3, printf("** Finished classification.\n"));
	ECHO(1, printf("\n"));
}

void ClassifyTH::ComputeCodeLengths() {
	string expDir = mConfig->Read<string>("expdir");
	Bass::SetWorkingDir(expDir);

	// Determine DB properties
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));

	string ctName = mConfig->Read<string>("ctname");
	bool laplace = mConfig->Read<bool>("laplace");

	// Read DB
	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);

	// Read CodeTable
	CodeTable *ct = CodeTable::LoadCodeTable(expDir + ctName + ".ct", db, "coverfull");
	if(ct == NULL)
		throw string("Codetable not found.");
	if(laplace)
		ct->AddOneToEachUsageCount();

	// Output some lengths
	string filename = expDir + dbName + "_" + ctName + "_lengths.txt";
	FILE *fp;
	if(fopen_s(&fp, filename.c_str(), "w"))
		throw string("Cannot write code lengths to " + filename);
	double len = 0.0;
	ItemSet **iss = db->GetRows();
	for(uint32 i=0; i<db->GetNumRows(); i++) {
		len = ct->CalcTransactionCodeLength(iss[i]);
		fprintf_s(fp, "%f\n", len);
	}
	fclose(fp);

	delete ct;
	delete db;
}

void ClassifyTH::ComputeCoverTree() {
	string expDir = mConfig->Read<string>("expdir");
	Bass::SetWorkingDir(expDir);

	// Determine DB properties
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));

	string ctName = mConfig->Read<string>("ctname");

	// Read DB
	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);

	// Output some lengths
	string filename = expDir + dbName + "_" + ctName + "_ctree.txt";
	CTree* ctree = new CTree();
	ctree->BuildCoverTree(db, expDir + ctName + ".ct");
	ctree->WriteAnalysisFile(filename);

	delete ctree;
	delete db;
}

#endif // ENABLE_CLASSIFIER
