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

#if defined (_WINDOWS)
	#include <direct.h>
#endif
#include <time.h>

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <db/ClassedDatabase.h>
#include <db/DbAnalyser.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <isc/ItemTranslator.h>

#include "../FicMain.h"

#include "../algo/CodeTable.h"
//#include "../blocks/dgen/DGen.h"
//#include "../blocks/dgen/cover/CTree.h"
#include "../FicConf.h"

#include <Croupier.h>
#include <afopt/AfoptCroupier.h>

#include "DissimilarityTH.h"
#include "MainTH.h"

MainTH::MainTH(Config *conf) : TaskHandler(conf){
}

MainTH::~MainTH() {
	// not my Config*
}

void MainTH::HandleTask() {
	string command = mConfig->Read<string>("command");

			if(command.compare("analysedb") == 0)		AnalyseDB();
//	else	if(command.compare("analysect") == 0)		AnalyseCT();
	else	if(command.compare("histogram") == 0)		CreateFISHistogram();

	else	if(command.compare("createdepdb") == 0)		CreateDependentDB();

														// not very nice like this, should be reorganised a bit.
														// class FicGlobal / FicTools / FicBase with 'default' methods like:
														//		- SelectCandidates()
														//		- Compress()
														// ? 
														// (collection of static helper functions that can be used by THs)
	else	if(command.compare("compress") == 0)		FicMain::Compress(mConfig);

	else	if(command.compare("getlastct") == 0)		GetLastCTAndWriteToDisk(); // if you only need a single CT for use in another app (yes, this sometimes happens..)

	else	if(command.compare("swprnd") == 0)			SwpRndKrmpXps();
	//else	if(command.compare("compresswith") == 0)	FicMain::CompressWith(mConfig);
	//else	if(command.compare("resume") == 0)			FicMain::Resume(mConfig);

	//else	if(command.compare("ontwardatagenbline") == 0) NormaleDGen();

	else	throw string("MainTH :: Unable to handle task `" + command + "`");
}

string MainTH::BuildWorkingDir() {
	//char s[100];
	//_i64toa_s(Logger::GetCommandStartTime(), s, 100, 10);
	return mConfig->Read<string>("command") + "/";
}


/* ------------------------- Commands -------------------------- */

void MainTH::CreateDependentDB() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName, "", Uint16ItemSetType);

	// Build target array
	uint32 numItems = 0;
	uint16 *elems = StringUtils::TokenizeUint16(mConfig->Read<string>("itemset"), numItems);
	ItemSet *is = ItemSet::Create(Uint16ItemSetType, elems, numItems, (uint32) db->GetAlphabetSize(), 1);
	db->RemoveFromDatabase(is);
	delete is;

	string depDBName = dbName + "-dep";

	try {
		Database::WriteDatabase(db, depDBName);
	} catch(char*) {
		delete db;
		throw;
	}
	delete db;

}


void MainTH::AnalyseDB() {
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype","bm128"));
	
	Database *db = Database::RetrieveDatabase(dbName, dataType);

	string analysisFile = db->GetFullPath() + ".analysis.txt";
	DbAnalyser *dba = new DbAnalyser();
	dba->Analyse(db, analysisFile);

	delete dba;
	delete db;
}
#if 0
void MainTH::AnalyseCT() {
	// Load db
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("dbindatatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));

	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);

	// Load ct
	//CodeTable *ct = CodeTable::Create(mConfig->Read<string>("cttype"));
	//ct->ReadFromDisk(mConfig->Read<string>("ctfile"),true);
	// ... 

	// Build CTree
	CTree *ctree = new CTree();

	string analysisFile = Bass::GetWorkingDir() + dbName + ".treeanalysis.txt";
	// Generate a database from the PTree
	string dbGenName = dbName + "_gen";
	Database *generateDb;

	// Cover db, add covering sets to tree
	//pt->BuildDistributionTree(db, mDataDir + mConfig->Read<string>("ctfile"));
	generateDb = ctree->BuildHMMMatrix(db, Bass::GetWorkingDir() + mConfig->Read<string>("ctfile"));
	//pt->BuildCoverTree(db, mDataDir + mConfig->Read<string>("ctfile"));
	//pt->AddOneToCounts();
	//pt->CalcProbabilities();
	//pt->WriteAnalysisFile(analysisFile);

	// Write a file that lists the probability for each covered row in a database
	/*delete db;
	dbName += "2";
	db = Database::ReadDatabase(dbName);*/
	//string coverFile = mDataDir + dbName + ".coverprobs.txt";
	//pt->CoverProbabilities(db, coverFile);

	try {
		//generateDb = pt->GenerateDatabase(3196);
		Database::WriteDatabase(generateDb, dbGenName);
		delete generateDb;
	} catch(char *la) {
		delete ctree;
		delete db;
		throw la;
	}

	// Save tree for joyfull activities
	// ...

	// Clean stuff up
	delete ctree;
	ctree = NULL;
	delete db;

	db = Database::ReadDatabase(dbGenName);

	analysisFile = Bass::GetWorkingDir() + dbGenName + ".analysis.txt";
	DbAnalyser *dba = new DbAnalyser();
	dba->Analyse(db, analysisFile);

	delete dba;
	delete db;
}

void MainTH::NormaleDGen() {
	// Stap 0 - Load db
	string dbName = mConfig->Read<string>("dbname");
	ClassedDatabase *origDB = (ClassedDatabase*) Database::RetrieveDatabase(dbName);

	// Stap 1 - Comprimeer origDB
	CodeTable *origDBCT = FicMain::RetrieveCodeTable(mConfig, origDB);
	if(origDBCT == NULL) {
		origDBCT = FicMain::CreateCodeTable(origDB, mConfig);
		string tag = dbName + "-" + mConfig->Read<string>("isctype") + "-" + mConfig->Read<string>("iscminsup") + mConfig->Read<string>("iscorder") + "-" + mConfig->Read<string>("prunestrategy");
		string ctPath = Bass::GetCtReposDir() + tag + "/";
		FileUtils::CreateDir(ctPath);
		origDBCT->WriteToDisk(ctPath + "ct-" + tag + "-" + TimeUtils::GetTimeStampString() + "-" + mConfig->Read<string>("iscminsup") + "-1337.ct");
	}

	origDBCT->AddOneToEachCount();

	string fname = Bass::GetWorkingDir() + "_dgen-" + TimeUtils::GetTimeStampString() + ".txt";
	FILE *fp = fopen(fname.c_str(),"w");

	uint32 numGen = mConfig->Read<uint32>("numgen");
	double avgdissim = 0;
	for(uint32 j=0; j<numGen; j++) {
		DGen *dgen = DGen::Create(mConfig->Read<string>("dgentype"));
		dgen->SetInputDatabase(origDB);
		dgen->SetColumnDefinition(mConfig->Read<string>("coldef"));
		dgen->BuildModel(origDB, mConfig, origDBCT);
		Database *genDB = dgen->GenerateDatabase(origDB->GetNumTransactions());

		// Stap 5 - Comprimeer genDB
		CodeTable *genDBCT = FicMain::CreateCodeTable(genDB, mConfig, true);

		// Stap 6 - Bereken afstand tussen clusterGenDB en origDB
		genDBCT->AddOneToEachCount();
		double dissim = DissimilarityTH::BerekenAfstandTussen(origDB, genDB, origDBCT, genDBCT);
		avgdissim += dissim;

		printf("dissim: %.2f\n", dissim);
		fprintf(fp, "%.2f\n", dissim);
		delete genDBCT;
		delete genDB;
		delete dgen;
	}
	printf("%.2f\n", avgdissim / numGen);
	fprintf(fp, "%.2f\n", avgdissim / numGen);
	fclose(fp);

	delete origDBCT;
	delete origDB;
}
#endif
void MainTH::CreateFISHistogram() {
	// dbName
	// iscName
	// ?dataType
	// whereHistogram = [xpsdir|store]

	Bass::SetMineToMemory(true);

	string dbName = mConfig->Read<string>("dbname");
	string dbBaseName = dbName.substr(0,dbName.find_first_of('['));
	string iscTag = mConfig->Read<string>("iscname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	Database *db = Database::RetrieveDatabase(dbName, dataType);
	AfoptCroupier *croup = new AfoptCroupier(iscTag);

	bool setLengthMode = mConfig->Read<bool>("minesetlengths", false);
	bool storeHistogram = mConfig->Read<string>("whereHistogram", "").compare("store") == 0;

	uint64* histo = croup->MineHistogram(db, setLengthMode);
	string pathstr = (storeHistogram == true ? Bass::GetDbReposDir() + dbBaseName + "/" : Bass::GetExpDir() + "histogram/");
	if(storeHistogram && !FileUtils::DirectoryExists(pathstr))
		pathstr = Bass::GetDbReposDir();

	string filestr = pathstr + dbName + "." + (setLengthMode?"setlen":"supp") + "_histogram" + iscTag.substr(iscTag.find_first_of('-')) + ".txt";
	FILE *fp = fopen(filestr.c_str(), "w");
	uint64 sum = 0;
	if(setLengthMode) {
		for(uint32 i=1; i<=db->GetMaxSetLength(); i++) {
			sum += histo[i];
			if(histo[i] > 0)
#if defined (_MSC_VER) && defined (_WINDOWS)
				fprintf(fp, "%d\t%I64d\t%I64d\n", i, histo[i], sum);
#elif defined (__GNUC__) && defined (__unix__)
				fprintf(fp, "%d\t%lu\t%lu\n", i, histo[i], sum);
#endif
		}
	} else {
		for(uint32 i=db->GetNumTransactions(); i>0; i--) {
			sum += histo[i];
			if(histo[i] > 0)
#if defined (_MSC_VER) && defined (_WINDOWS)
				fprintf(fp, "%d\t%I64d\t%I64d\n", i, histo[i], sum);
#elif defined (__GNUC__) && defined (__unix__)
				fprintf(fp, "%d\t%lu\t%lu\n", i, histo[i], sum);
#endif
		}
	}
	delete[] histo;
	fclose(fp);

	delete croup;

	delete db;
}

void MainTH::GetLastCTAndWriteToDisk() {
	Database *db = Database::ReadDatabase(mConfig->Read<string>("dbname"), Bass::GetDataDir());
	CodeTable* ct = FicMain::ProvideCodeTable(db, mConfig, true);
	ct->WriteToDisk(Bass::GetDataDir() + db->GetName() + ".ct");
	delete ct;
	delete db;
}

void MainTH::SwpRndKrmpXps() {
	string numExpsStr = mConfig->Read<string>("numexps");
	string prcSwapsStr = mConfig->Read<string>("prcSwaps");
	uint32 numExps = mConfig->Read<uint32>("numexps", 1);
	float prcSwaps = mConfig->Read<float>("prcSwaps", 100);

	string iscName = mConfig->Read<string>("iscname");
	string dbName = mConfig->Read<string>("dbname", iscName.substr(0,iscName.find_first_of('-')));
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));

	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);
	uint32 numSwaps = (uint32) ceil((float) db->GetNumItems() * (prcSwaps/100));

	string outputFilename = Bass::GetWorkingDir() + "swprnd-" + numExpsStr + "x-" + prcSwapsStr + "s-" + iscName + "-" + TimeUtils::GetTimeStampString() + ".csv";

	FILE* fp = fopen(outputFilename.c_str(), "w");
	for(uint32 i=0; i<numExps; i++) {
		Database *sdb = db->Clone();
		sdb->SwapRandomise(numSwaps);

		time_t timeStart = time(NULL);
		CodeTable *ct = FicMain::CreateCodeTable(sdb, mConfig, true);

		islist* ctels = ct->GetItemSetList();
		uint32 ctElLenCnt = 0;
		islist::iterator ctcur, ctend = ctels->end();
		for(ctcur = ctels->begin(); ctcur !=ctend; ++ctcur) {
			if((*ctcur)->GetLength() > 1 && (*ctcur)->GetUsageCount() > 0)
				ctElLenCnt += (*ctcur)->GetLength();
		}
		delete ctels;

		// write stats
		CoverStats stats = ct->GetCurStats();
#if defined (_MSC_VER) && defined (_WINDOWS)
		fprintf(fp, "%I64d;%.2f;%d;%d;%d;%I64d;%.0f;%.0f;%.0f;%d\n", 
			stats.numCandidates, (float)ctElLenCnt/(float)stats.numSetsUsed, stats.alphItemsUsed, stats.numSetsUsed, ct->GetCurNumSets(), stats.usgCountSum, stats.encDbSize, stats.encCTSize, stats.encSize, (uint32) time(NULL) - timeStart);
#elif defined (__GNUC__) && defined (__unix__)
		fprintf(fp, "%lu;%.2f;%d;%d;%d;%lu;%.0f;%.0f;%.0f;%d\n", 
			stats.numCandidates, (float)ctElLenCnt/(float)stats.numSetsUsed, stats.alphItemsUsed, stats.numSetsUsed, ct->GetCurNumSets(), stats.usgCountSum, stats.encDbSize, stats.encCTSize, stats.encSize, (uint32) time(NULL) - timeStart);
#endif

		delete ct;
		delete sdb;
	}
	fclose(fp);

	delete db;
}
