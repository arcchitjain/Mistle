#ifdef ENABLE_OCCLASSIFIER

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
#include "../algo/CodeTable.h"
#include "../algo/StandardCodeTable.h"
#include "../blocks/classifier/Classifier.h"
#include "../blocks/occlassifier/OneClassClassifier.h"

// leluk
#include "../FicMain.h"

#include "TaskHandler.h"
#include "OCClassifyTH.h"

OCClassifyTH::OCClassifyTH(Config *conf) : TaskHandler(conf){
	mTimestamp = TimeUtils::GetTimeStampString();
	mTag = "";
}

OCClassifyTH::~OCClassifyTH() {
	// not my Config*
}

void OCClassifyTH::HandleTask() {
	string command = mConfig->Read<string>("command");
	if(command.compare("classifycompress") == 0)			ClassifyCompress();
	else	if(command.compare("classifyanalyse") == 0)		ClassifyAnalyse();
	else	throw string("ClassifyTH :: Unable to handle task `" + command + "`");
}

string OCClassifyTH::BuildWorkingDir() {
	string command = mConfig->Read<string>("command");

	if(command.compare("classifycompress") == 0 || command.compare("prepare") == 0) {
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
			CodeTable *ct = CodeTable::Create(algo, Uint16ItemSetType); // ??? whoa, ugly
			mTag += "-" + ct->GetShortName();
			delete ct;
		}
		mTag += "-" + mTimestamp;
		return "occlassify/" + mTag + "/";
	} else	if(command.compare("classifyanalyse") == 0)
		return "occlassify/" + mConfig->Read<string>("exptag") + "/";
	return "";
}


/* ----------------- Phase 1: prepare & execute compression ---------------- */

void OCClassifyTH::ClassifyCompress() {
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
#if defined (__unix__)
	if (!FileUtils::DirectoryExists(expDir)) 
		if (mkpath(expDir.c_str(), 0777)) throw string("An error occurred while creating directory: ") + expDir;
#else
	if (!FileUtils::CreateDirIfNotExists(Bass::GetExpDir() + "occlassify/")) 
		THROW("An error occurred while creating directory: " + expDir);
	if (!FileUtils::CreateDirIfNotExists(expDir)) 
		THROW("An error occurred while creating directory: " + expDir);
#endif

	// Store some settings & backup config
	{
		s = pruneStrategy;
		sprintf_s(temp, 200, " s%d", seed); s += temp;
		sprintf_s(temp, 200, " %df", numFolds); s += temp;
		FILE *fp;
		string huh = (expDir + "_" + s);
		fopen_s(&fp, huh.c_str(), "w");
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
		Database **targetDbs = trainingDb->SplitOnClassLabels();
		Database **outlierDbs = trainingDb->SplitOnClassLabelsComplement();
	
		for(uint32 i=0; i<numClasses; i++) {
			_itoa(classes[i], temp, 10);
			string targetDbName = string("target") + temp;
			string outlierDbName = string("outlier") + temp;
			Database::WriteDatabase(targetDbs[i], targetDbName, dir);
			Database::WriteDatabase(outlierDbs[i], outlierDbName, dir);
			targetDbs[i]->SetName(targetDbName);
			outlierDbs[i]->SetName(outlierDbName);

			// Compress

			string targetDbDir = dir + targetDbName + "/";
			string outlierDbDir = dir + outlierDbName + "/";
#if defined (_WINDOWS)
			_mkdir(targetDbDir.c_str());
			_mkdir(outlierDbDir.c_str());
#elif defined (__unix__)
			mkdir(targetDbDir.c_str(), 0777);
			mkdir(outlierDbDir.c_str(), 0777);
#endif

			//_fcloseall();
			FicMain::MineAndWriteCodeTables(targetDbDir, targetDbs[i], mConfig, true);
			FicMain::MineAndWriteCodeTables(outlierDbDir, outlierDbs[i], mConfig, true);
		}

		// Cleanup
		for(uint32 i=0; i<numClasses; i++) {
			delete targetDbs[i]; // Postpone until here; otherwise static mess is already wiped away...
			delete outlierDbs[i];
		}
		delete[] targetDbs;
		delete[] outlierDbs;
		delete trainingDb;
		delete testDb;
	}

	// Configure and write one-class classify config
	Config *config = new Config();
	config->Set("taskclass", "occlassify");
	config->Set("command", "classifyanalyse");
	config->Set("inequality", "cantelli");
	config->Set("takeiteasy", mConfig->Read<string>("takeiteasy"));
	config->Set("exptag", mTag);
	config->Set("numfolds", mConfig->Read<string>("numfolds"));
	config->Set("datatype", ItemSet::TypeToString(db->GetDataType()));
	config->Set("reportsup", mConfig->Read<string>("reportsup"));
	config->Set("algo", mConfig->Read<string>("algo"));
	s = expDir + "occlassify.conf";
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

void OCClassifyTH::ClassifyAnalyse() {
	ECHO(1, printf("** One-Class Classification ::\n"));

	OneClassClassifier *cl = new OneClassClassifier(mConfig);
	cl->Analyse();
	//cl->Classify();
	delete cl;

	ECHO(3, printf("** Finished one-class classification.\n"));
	ECHO(1, printf("\n"));
}

#endif // ENABLE_OCCLASSIFIER
