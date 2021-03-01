#include "global.h"

#if defined (_WINDOWS)
	#include <direct.h>
	#include <windows.h>
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <sys/stat.h>
#endif
#include <string>
#include <iostream>
#include <time.h>		// om time stamps te maken, en wellicht RandomUtils te inniten
#include <omp.h>

// -- qtils
#include <RandomUtils.h>
#include <Config.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <SystemUtils.h>
#include <TimeUtils.h>
#include <logger/Log.h>

// -- bass
#include <Bass.h>
#include <db/ClassedDatabase.h>
#include <db/DbFile.h>
#include <isc/IscFile.h>
#include <isc/ItemSetCollection.h>

// -- croupier
#include <Croupier.h>

#include "FicConf.h"
#include "blocks/krimp/KrimpAlgo.h"
#include "blocks/krimp/codetable/CodeTable.h"
#include "tasks/TaskHandler.h"

								// -- Always include
#include "tasks/MainTH.h"
#include "tasks/MineTH.h"
#include "tasks/DataTransformTH.h"

								// -- Dependent on block config
#ifdef ENABLE_CLASSIFIER
#include "tasks/ClassifyTH.h"
#endif
#ifdef ENABLE_CLUSTER
#include "tasks/ClusterTH.h"
#endif
#ifdef ENABLE_SLIM
#include "tasks/SlimTH.h"
#include "../blocks/slim/SlimAlgo.h"
#endif
#ifdef ENABLE_DATAGEN
#include "tasks/DataGenTH.h"
#endif
#ifdef ENABLE_EMM
#include "tasks/ExceptionalTH.h"
#endif
#ifdef ENABLE_LESS
#include "tasks/LESetsTH.h"
#endif
#ifdef ENABLE_OCCLASSIFIER
#include "tasks/OCClassifyTH.h"
#endif
#ifdef ENABLE_STREAM
#include "tasks/StreamTH.h"
#endif
#ifdef ENABLE_TAGS
#include "tasks/TagGroupTH.h"
#include "tasks/TagRecomTH.h"
#endif

#ifndef _PUBLIC_RELEASE			// -- Always include for non-public releases
#include "tasks/DissimilarityTH.h"
#include "tasks/MissingValuesTH.h"
#include "tasks/RecommendTH.h"
#include "tasks/TestTH.h"
#endif

#include "FicMain.h"

const string FicMain::ficVersion = "mkIII beta";

FicMain::FicMain(Config *config) {
	mConfig = config;
	mTaskClass = mConfig->Read<string>("taskclass");

	Bass::ParseGlobalSettings(mConfig);
}
FicMain::~FicMain() {
	
}

/* ------------------------- Main execution -------------------------- */

void FicMain::Execute() {
	// Init TaskHandler
	TaskHandler *th = NULL;
			if(mTaskClass.compare("main") == 0)				th = new MainTH(mConfig);
	else	if(mTaskClass.compare("mine") == 0)				th = new MineTH(mConfig);
	else	if(mTaskClass.compare("datatrans") == 0)		th = new DataTransformTH(mConfig);

#ifdef ENABLE_CLASSIFIER
	else	if(mTaskClass.compare("classify") == 0)			th = new ClassifyTH(mConfig);
#endif
#ifdef ENABLE_CLUSTER
	else	if(mTaskClass.compare("cluster") == 0)			th = new ClusterTH(mConfig);
#endif
#ifdef ENABLE_SLIM
	else	if(mTaskClass.compare("compress_ng") == 0)		th = new SlimTH(mConfig);
#endif
#ifdef ENABLE_DATAGEN
	else	if(mTaskClass.compare("dgen") == 0)				th = new DataGenTH(mConfig);
#endif
#ifdef ENABLE_EMM
	else	if(mTaskClass.compare("exceptional") == 0)		th = new ExceptionalTH(mConfig);
#endif
#ifdef ENABLE_LESS
	else	if(mTaskClass.compare("less") == 0)				th = new LESetsTH(mConfig);
#endif
#ifdef ENABLE_OCCLASSIFIER
	else	if(mTaskClass.compare("occlassify") == 0)		th = new OCClassifyTH(mConfig);
#endif
#ifdef ENABLE_STREAM
	else	if(mTaskClass.compare("stream") == 0)			th = new StreamTH(mConfig);
#endif
#ifdef ENABLE_TAGS
	else	if(mTaskClass.compare("taggroup") == 0)			th = new TagGroupTH(mConfig);
	else	if(mTaskClass.compare("tagrecom") == 0)			th = new TagRecomTH(mConfig);
#endif

#ifndef _PUBLIC_RELEASE
	else	if(mTaskClass.compare("dissim") == 0)			th = new DissimilarityTH(mConfig);
	else	if(mTaskClass.compare("miss") == 0)				th = new MissingValuesTH(mConfig);
	else	if(mTaskClass.compare("recommend") == 0)		th = new RecommendTH(mConfig);
	else	if(mTaskClass.compare("test") == 0)				th = new TestTH(mConfig);
#endif

	else	throw string("No TaskHandler available for `" + mTaskClass + "`");

	// Init command in AppLog
	if(mConfig->Read<uint32>("logstuff",1) == 1)
		Logger::CommandInit(mTaskClass, mConfig->Read<string>("command"));

	// Set working directory, based on TaskHandler
	// JJJ ? th->BuildWorkingDir...
	string wd = Bass::GetExpDir() + th->BuildWorkingDir();
	Bass::SetWorkingDir(wd);

	// Configure Logger
	// JJJ Prepare Logger, no more
	Logger::SetAppLogDir(Bass::GetExecDir());
	Logger::SetCommandLogDir(Bass::GetWorkingDir());
	Logger::Init(th->ShouldWriteTaskLogs());

	// Some old stuff that needs to be changed
	Bass::SetOutputLevel(mConfig->Read<uint32>("verbosity", 2));
	ECHO(1,printf(" * Verbosity:\t\t%d\n", Bass::GetOutputLevel()));
	ECHO(1, printf(" * Max Mem Usage:\t%dmb\n",(uint32)(Bass::GetMaxMemUse()/(uint64)MEGABYTE)));

	if(mConfig->KeyExists("takeiteasy") && mConfig->Read<uint32>("takeiteasy") != 0) {
		ECHO(1, printf(Bass::GetOutputLevel() > 1 ? " * Priority:\t\tYa man, i and i gonna take it easy, man...\n" : " * Priority:\t\tLow\n"))
			SystemUtils::TakeItEasy();
	} else
		ECHO(1, printf(Bass::GetOutputLevel() > 1 ? " * Priority:\t\tOpzij, opzij, opzij!\n\t\t\tMaak plaats, maak plaats, maak plaats!\n\t\t\tWij hebben ongelovelijke haast!\n" : " * Priority:\t\tHigh\n"))
	if(Bass::GetNumThreads() > 1)
		ECHO(1,printf(" * Threaded:\t\tusing %d threads\n", Bass::GetNumThreads()));

	ECHO(1,printf("\n"));


	// Execute task at hand
	th->HandleTask();	// throws error if it can't handle it, baby.

	// Finished
	ECHO(1,printf("\n * Check results in:\t%s\n", Bass::GetWorkingDir().c_str()));

	// Round up command.log
	if(mConfig->Read<uint32>("logstuff",1) == 1)
		Logger::CommandEnd();
	Logger::CleanUp();

	// Cleanup
	delete th;
}

/* ------------------------- Helpers -------------------------- */
// Select? 
// BuildCodeTable? 
// CandidateSelect?
void FicMain::Compress(Config *config, const string tagIn) {
	// selName	`mushroom-cls-20d-nop`		SelectionName
	// iscName	`mushroom-cls-20d`			ItemSetCollectionName
	// dbName	`mushroom`					DatabaseName

	// Determine ISC properties
	string iscName = config->Read<string>("iscname");

	// Determine DB properties
	string dbName = config->Read<string>("dbname", iscName.substr(0,iscName.find_first_of('-')));
	ItemSetType dataType = ItemSet::StringToType(config->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(config->Read<string>("dbinencoding",""));

	// Determine Pruning Strategy
	string pruneStrategyStr = config->Read<string>("prunestrategy","nop");

	string tag, timetag;
	if(tagIn.length() == 0) {
		timetag = TimeUtils::GetTimeStampString();
		tag = iscName + "-" + pruneStrategyStr + "-" + timetag;
	} else {
		tag = tagIn;
		timetag = tag.substr(tag.find_last_of('-')+1,tag.length()-1-tag.find_last_of('-'));
	}

	// Read DB
	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);

	// Get ISC
	string iscFullPath = Bass::GetWorkingDir() + iscName + ".isc";

	double mTotalStartTime = omp_get_wtime();

	ItemSetCollection *isc = NULL;
	string iscIfMinedStr = config->Read<string>("iscifmined", "");
	bool loadAll = config->Read<int>("loadall", 0) == 1;		// ??? sander-add
	bool iscBeenMined = false;
	try {
		IscFileType storeIscFileType = config->KeyExists("iscStoreType") ? IscFile::ExtToType(config->Read<string>("iscStoreType")) : BinaryFicIscFileType;
		IscFileType chunkIscFileType = config->KeyExists("iscChunkType") ? IscFile::ExtToType(config->Read<string>("iscChunkType")) : BinaryFicIscFileType;
		isc = FicMain::ProvideItemSetCollection(iscName, db, iscBeenMined, iscIfMinedStr.compare("store") == 0, loadAll, storeIscFileType, chunkIscFileType);
	} catch(string msg) {
		delete db;
		throw msg;
	}

	FicMain::DoCompress(config, db, isc, tag, timetag, 0, 0);
	delete db;
	delete isc;

	double timeTotal = omp_get_wtime() - mTotalStartTime;


	FILE* fp = fopen((Bass::GetWorkingDir() + tag + "/" + "time.log").c_str(), "w");
	fprintf(fp, " * Total time:\t\tMining & compressing the database took %f seconds.\t\t\n", timeTotal);
	fclose(fp);

	if(iscBeenMined == true && iscIfMinedStr.compare("zap") == 0) {	// if stored, it's already gone from workingdir
		FileUtils::RemoveFile(iscFullPath);
	}
}
void FicMain::DoCompress(Config *config, Database *db, ItemSetCollection *isc, const string &tag, const string &timetag, const uint32 resumeSup, const uint64 resumeCand) {
	// ISC normalized?
	/*if(!isc->IsNormalized()) {
		delete isc;
		delete db;
		throw string("ItemSetCollection must be normalized before use!");
	}*/

	// Build Algo
	string algoName = config->Read<string>("algo");
	KrimpAlgo *algo = KrimpAlgo::CreateAlgo(algoName, db->GetDataType());
	if(algo == NULL)
		throw string("FicMain::DoCompress - Unknown algorithm.");
	ECHO(1, printf("** Compression :: \n"));
	ECHO(3, printf(" * Initiating...\n"));

	// Determine tag, output dir & backup full config
	{
		string outDir = Bass::GetWorkingDir() + tag + "/";
		FileUtils::CreateDirIfNotExists(outDir); /* check success ? */

		string confFile = outDir + "/" + config->Read<string>("command") + "-" + timetag + ".conf";
		config->WriteFile(confFile);
		algo->SetOutputDir(outDir);
		algo->SetTag(tag);

		ECHO(1, printf(" * Experiment tag\t%s\n", tag.c_str()));
	}

	// Further configure Algo
	ECHO(1, printf(" * Algorithm:\t\t%s", algoName.c_str()));
	{
		string		pruneStrategyStr = config->Read<string>("prunestrategy");
		PruneStrategy	pruneStrategy = KrimpAlgo::StringToPruneStrategy(pruneStrategyStr);
		string pruneStrategyLongName = KrimpAlgo::PruneStrategyToName(pruneStrategy);
		ECHO(1, printf(" %s\n", pruneStrategyLongName.c_str()))

		uint32		reportSup = config->Read<uint32>("reportsup", 0);
		uint32		reportCnd = config->Read<uint32>("reportcnd", 0);
		bool		reportAcc = config->Read<bool>("reportAcc", false);

		ReportType		reportSupWhat = KrimpAlgo::StringToReportType(config->Read<string>("reportsupwhat", "all"));
		ReportType		reportCndWhat = KrimpAlgo::StringToReportType(config->Read<string>("reportcndwhat", "all"));
		ReportType		reportAccWhat = KrimpAlgo::StringToReportType(config->Read<string>("reportaccwhat", "all"));

		CTInitType	initCT = config->Read<bool>("initct", true) ? InitCTAlpha : InitCTEmpty;

		algo->UseThisStuff(db, isc, db->GetDataType(), initCT, true);
		algo->SetPruneStrategy(pruneStrategy);
		algo->SetReporting(reportSup, reportCnd, reportAcc);
		algo->SetReportStyle(reportSupWhat,reportCndWhat,reportAccWhat);
	}
	ECHO(1, printf(" * Candidates:\t\t%" I64d "p\n", isc->GetNumItemSets()));

	// Resume from specified support or candidate
	uint64 candOffset = 0;
	uint32 startSup = 0;
	if(resumeSup != 0) {
		startSup = resumeSup;
		ECHO(1, printf("** Resuming Compression ::\n * Experiment:\t%s\n", tag.c_str()));
		if(resumeCand == 0) {
			candOffset = isc->ScanPastSupport(resumeSup);
		} else {
			ECHO(1, printf(" * Candidate:\t%" I64d "\n", resumeCand+1));
			isc->ScanPastCandidate(resumeCand);
			candOffset = resumeCand+1;		// +1 want ctCnd geeft laatste candidate die aan codetable heeft bijgedragen weer, dus die skippen we (deze +1 is voor stats)
		}							

		// Load appropiate codetable
		algo->LoadCodeTable(resumeSup, resumeCand);
	} else {

		// Write Run-Time Info file
		char rti[100];
		sprintf_s(rti, 100, "__%s %s %s %s", ficVersion.c_str(), algo->GetShortName().c_str(), ItemSet::TypeToString(db->GetDataType()).c_str(), algo->GetShortPruneStrategyName().c_str());
		string rtiFile = algo->GetOutDir() + "/" + rti;
		FILE *rtifp = fopen(rtiFile.c_str(), "w");
		fclose(rtifp); /* check success ? */
	}

	// Put algo to work
	CodeTable *ct = algo->DoeJeDing(candOffset, startSup);

	ECHO(3, printf("** Finished compression.\n"));

	delete algo;
	delete ct;
}
// Tries to retrieve the requested CodeTable, creates it through  
// compression if not available. Doing so, it attempts to retrieve
// the ISC, mines it if not available. Nothing is logged nor are 
// .ct files written to disk.
// config-reads:
// - iscifmined, iscname, prunestrategy
CodeTable* FicMain::ProvideCodeTable(Database *db, Config *config, bool zapIsc /* = false */) {
	// Try to retrieve
	// -- not yet available


	// -- iscMayRetrieve = ... ?
	// -- determine iscIfMined = ...
	//dummyDbName + "-" + iscTypeStr + "-" + iscMinSupStr + iscOrderStr;
	string iscTag = config->Read<string>("iscname"); 
	string iscIfMinedStr = config->Read<string>("iscifmined", "");
	IscFileType storeIscFileType = config->KeyExists("iscStoreType") ? IscFile::ExtToType(config->Read<string>("iscStoreType")) : BinaryFicIscFileType;
	IscFileType chunkIscFileType = config->KeyExists("iscChunkType") ? IscFile::ExtToType(config->Read<string>("iscChunkType")) : BinaryFicIscFileType;
	bool iscBeenMined = false;
	ItemSetCollection *isc = FicMain::ProvideItemSetCollection(iscTag, db, iscBeenMined, iscIfMinedStr.compare("store") == 0, false, storeIscFileType, chunkIscFileType);

	// Build Algo
	string algoName = config->Read<string>("algo");
	KrimpAlgo *algo = KrimpAlgo::CreateAlgo(algoName, db->GetDataType());
	if(algo == NULL) {
		delete isc;
		throw string("FicMain::CreateCodeTable - Unknown algorithm: ") + algoName;
	}
	algo->SetOutputDir(Bass::GetWorkingDir());

	CTInitType ctInit = InitCTAlpha;
	
	string		pruneStrategyStr = config->Read<string>("prunestrategy", "nop");
	PruneStrategy	pruneStrategy = KrimpAlgo::StringToPruneStrategy(pruneStrategyStr);

	algo->UseThisStuff(db, isc, db->GetDataType(), ctInit, false);
	algo->SetPruneStrategy(pruneStrategy);
	algo->SetReporting(0,0,false);
	algo->SetReportStyle(ReportNone, ReportNone, ReportNone);
	algo->SetWriteFiles(false);

	// Put algo to work
	CodeTable *ct = algo->DoeJeDing();

	if(iscBeenMined && zapIsc) {
		IscFile *iscFile = isc->GetIscFile();
		if(iscFile != NULL)
			iscFile->Delete();
	}

	delete algo;
	delete isc;

	return ct;
}

// Creates the requested CodeTable through a fresh compression run (ISC is mined, not retrieved)
CodeTable* FicMain::CreateCodeTable(Database *db, Config *config, bool zapIsc /* = false */, ItemSetCollection *candidates /* = NULL */) {
	// botweg een Krimp-run doen volgens config, niets loggen, gewoon de codetable opleveren. 

	ItemSetCollection *isc = NULL;
	if(candidates != NULL) {
		isc = candidates;
	} else {
		// Build ISC
		//string s = SystemUtils::GetProcessIdString() + TimeUtils::GetTimeStampString();
		string iscTag = config->Read<string>("iscname", "");
		if(iscTag.size() == 0) {
			string iscTypeStr = config->Read<string>("isctype");
			string iscMinSupStr = config->Read<string>("iscminsup");
			string iscOrderStr = config->Read<string>("iscorder");
			iscTag = "dummy-" + iscTypeStr + "-" + iscMinSupStr + iscOrderStr;
		}
		// J! Zou mooi zijn als isc on-delete zichzelf op zou kunnen ruimen?
		IscFileType storeIscFileType = config->KeyExists("iscStoreType") ? IscFile::ExtToType(config->Read<string>("iscStoreType")) : BinaryFicIscFileType;
		IscFileType chunkIscFileType = config->KeyExists("iscChunkType") ? IscFile::ExtToType(config->Read<string>("iscChunkType")) : BinaryFicIscFileType;
		isc = FicMain::MineItemSetCollection(iscTag, db, false, true, storeIscFileType, chunkIscFileType);
	}

	// Build Algo
	string algoName = config->Read<string>("algo");
	KrimpAlgo *algo = KrimpAlgo::CreateAlgo(algoName, db->GetDataType());
	if(algo == NULL) {
		delete isc;
		throw string("FicMain::CreateCodeTable - Unknown algorithm: ") + algoName;
	}
	algo->SetOutputDir(Bass::GetWorkingDir());

	CTInitType ctInit = InitCTAlpha;
	string		pruneStrategyStr = config->Read<string>("prunestrategy", "nop");
	PruneStrategy	pruneStrategy = KrimpAlgo::StringToPruneStrategy(pruneStrategyStr);		//	Default

	algo->UseThisStuff(db, isc, db->GetDataType(), ctInit, false);
	algo->SetPruneStrategy(pruneStrategy);
	algo->SetReporting(0,0,false);
	algo->SetReportStyle(ReportNone,ReportNone,ReportNone);
	algo->SetWriteFiles(false);

	// Put algo to work
	CodeTable *ct = algo->DoeJeDing();

	delete algo;

	if(candidates == NULL) {
		if(zapIsc && isc->GetIscFile() != NULL) {
			isc->GetIscFile()->Delete();	
		}
		delete isc;
	}

	return ct;
}



// Write the requested CodeTables through a fresh compression run (ISC is mined, not retrieved)
void FicMain::MineAndWriteCodeTables(const string &path, Database *db, Config *config, bool zapIsc /* = false */) {
	// Build ISC
	//string s = SystemUtils::GetProcessIdString() + TimeUtils::GetTimeStampString();
	string iscTag = config->Read<string>("iscname", "");
	if(iscTag.size() == 0) {
		string iscTypeStr = config->Read<string>("isctype");
		string iscMinSupStr = config->Read<string>("iscminsup");
		string iscOrderStr = config->Read<string>("iscorder");
		iscTag = "dummy-" + iscTypeStr + "-" + iscMinSupStr + iscOrderStr;
	}

	string tmpIscTag = iscTag;
	if (config->Read<string>("algo").compare(0, 4, "slim") == 0) { // We don't really need a itemset collection...
		stringstream ss;
		ss << db->GetName() << "-all-" << db->GetNumTransactions()+1 << "d";
		iscTag = ss.str();
		zapIsc = true; // Otherwise, we can crash... don't feel the urgent need to fix this properly ;-(
	}

	// J! Zou mooi zijn als isc on-delete zichzelf op zou kunnen ruimen?
	IscFileType storeIscFileType = config->KeyExists("iscStoreType") ? IscFile::ExtToType(config->Read<string>("iscStoreType")) : BinaryFicIscFileType;
	IscFileType chunkIscFileType = config->KeyExists("iscChunkType") ? IscFile::ExtToType(config->Read<string>("iscChunkType")) : BinaryFicIscFileType;
	ItemSetCollection *isc = FicMain::MineItemSetCollection(iscTag, db, false, true, storeIscFileType, chunkIscFileType); // TODO: If slim we don't need to mine isc!!!

	if (config->Read<string>("algo").compare(0, 4, "slim") == 0) { // We don't really need a itemset collection...
		iscTag = tmpIscTag;
	}

	// Build Algo
	string algoName = config->Read<string>("algo");
	KrimpAlgo *algo = SlimAlgo::CreateAlgo(algoName, db->GetDataType(), config);
	if(algo == NULL) {
		delete isc;
		throw string(__FUNCTION__) + ": Unknown algorithm: " + algoName;
	}
	algo->SetOutputDir(path);
	algo->SetTag(iscTag/*db->GetName()*/); // TODO: check still ok...

	CTInitType		ctInit = InitCTAlpha;
	string			pruneStrategyStr = config->Read<string>("prunestrategy", "nop");
	PruneStrategy	pruneStrategy = KrimpAlgo::StringToPruneStrategy(pruneStrategyStr);
	uint32			reportSup = config->Read<uint32>("reportsup", 0);
	uint32			reportCnd = config->Read<uint32>("reportcnd", 0);
	bool			reportAcc = config->Read<bool>("reportacc", false);
	ReportType		reportSupWhat = KrimpAlgo::StringToReportType(config->Read<string>("reportsupwhat", "all"));
	ReportType		reportCndWhat = KrimpAlgo::StringToReportType(config->Read<string>("reportcndwhat", "all"));
	ReportType		reportAccWhat = KrimpAlgo::StringToReportType(config->Read<string>("reportaccwhat", "all"));

	// niet schuldig:
	algo->UseThisStuff(db, isc, db->GetDataType(), ctInit, false);
	algo->SetPruneStrategy(pruneStrategy);
	algo->SetReporting(reportSup, reportCnd, reportAcc);
	algo->SetReportStyle(reportSupWhat,reportCndWhat,reportAccWhat);

	// Put algo to work
	CodeTable *ct = algo->DoeJeDing();

	if(zapIsc) {
		IscFile *iscFile = isc->GetIscFile();
		if(iscFile != NULL)
			iscFile->Delete();
	}

	delete algo;
	delete isc;
	delete ct;
}


//////////////////////////////////////////////////////////////////////////
// The New World
//////////////////////////////////////////////////////////////////////////

// Retrieves the requested ItemSetCollection, or mines, saves and opens in the working directory
ItemSetCollection* FicMain::ProvideItemSetCollection(const string &iscTag, Database * const db, bool &beenMined, const bool storeMined /* = false */, const bool loadAll /* = false */, const IscFileType storeIscFileType /* = FicIscFileType */, const IscFileType chunkIscFileType /* FicIscFileType */) {
	beenMined = false;

	// Try to retrieve it. 
	if(ItemSetCollection::CanRetrieveItemSetCollection(iscTag)) {
		ItemSetCollection *isc = ItemSetCollection::RetrieveItemSetCollection(iscTag, db, loadAll);
		return isc;
	}

	// Try to mine it.
	ItemSetCollection *isc = FicMain::MineItemSetCollection(iscTag, db, storeMined, loadAll, storeIscFileType, chunkIscFileType);
	beenMined = true;

	return isc;
}
ItemSetCollection* FicMain::MineItemSetCollection(const string &iscTag, Database * const db, const bool store /* = false */, const bool loadAll /* = false */, const IscFileType storeIscFileType /* = FicIscFileType */, const IscFileType chunkIscFileType /* = FicIscFileType */) {
	Croupier *croupier = Croupier::Create(iscTag);
	croupier->SetChunkIscFileType(chunkIscFileType);
	croupier->SetStoreIscFileType(storeIscFileType);
	ItemSetCollection *isc = NULL;

	ECHO(1, printf("** ItemSetCollection ::\n"));
	//printf(" * FileType\t\t:%s\n", IscFile::TypeToExt(isc->GetIscFile()->GetType()));

	if(Bass::GetMineToMemory() && croupier->CanDealOnline()) {			// -- Attempt in-memory mining
		isc = croupier->MinePatternsToMemory(db);
		if(store == true) {
			ECHO(1, printf(" * Storing:\t\tIn progress...\r"));
			ItemSetCollection::StoreItemSetCollection(isc, iscTag, storeIscFileType);
			string filename = isc->GetIscFile()->GetFileName();
			string path = "";
			FileUtils::SplitPathFromFilename(filename,path);
			ECHO(1, printf(" * Stored:\t\t%s\n", filename.c_str()));
		}
		string setsWhere = "";
		ECHO(1, printf(" * # Sets:\t\t%" I64d " (%s)\n", isc->GetNumItemSets(), (loadAll || (isc->GetNumItemSets() == isc->GetNumLoadedItemSets())) ? "resident" : "hd"));
	
		ECHO(1, printf("\n"));

	} else {															// -- Mine to file
		string iscFullPath = Bass::GetWorkingDir() + iscTag;
		IscFileType fileType = IscFile::StringToType(iscFullPath);
		if(fileType == NoneIscFileType) {
			fileType = storeIscFileType;
			iscFullPath += "." + IscFile::TypeToExt(fileType);
		}
		if(!croupier->MinePatternsToFile(iscTag.substr(0,iscTag.find_first_of('-')), iscFullPath, db)) {
			delete croupier;
			string err = string(__FUNCTION__) + " -- Error mining the shizzle.";
			LOG_ERROR(err);
			throw err;
		}

		string iscPath = Bass::GetWorkingDir();
		if(store == true) {
			iscPath = Bass::GetIscReposDir();
			if(!FileUtils::DirectoryExists(iscPath))
				if(!FileUtils::CreateDir(iscPath))
					THROW("Error: trouble creating the IscReposDir: " + iscPath + "\n");
			string iscFilename = iscTag + "." + IscFile::TypeToExt(fileType);
			FileUtils::FileMove(Bass::GetWorkingDir() + iscFilename, Bass::GetIscReposDir() + iscFilename);
		}	
		isc = ItemSetCollection::OpenItemSetCollection(iscTag, iscPath, db, loadAll, fileType);
	}
	delete croupier;
	ECHO(1,printf("\n"));
	return isc;
}

CodeTable* FicMain::LoadCodeTableForMinSup(const Config *config, Database *db, const uint32 minsup, string ctDir) {
	uint32 numCTs;
	CTFileInfo **ctFiles = FicMain::LoadCodeTableFileList(config, db, numCTs, ctDir);

	if(numCTs == 0)
		throw string("FicMain::LoadCodeTableForMinSup() -- No code tables found.");

	CodeTable *ct = NULL;
	for(uint32 i=0; i<numCTs; i++)
		if(ctFiles[i]->minsup == minsup) {
			ct = CodeTable::LoadCodeTable(ctDir + ctFiles[i]->filename, db);
			break;
		}

	if(ct == NULL)
		THROW("Code table for specified minsup not found.");

	for(uint32 i=0; i<numCTs; i++)
		delete ctFiles[i];
	delete[] ctFiles;

	return ct;
}
CodeTable* FicMain::LoadFirstCodeTable(const Config *config, Database *db, string ctDir) {
	uint32 numCTs;
	CTFileInfo **ctFiles = FicMain::LoadCodeTableFileList(config, db, numCTs, ctDir);

	if(numCTs == 0)
		THROW("No code tables found.");

	CodeTable *ct = CodeTable::LoadCodeTable(ctDir + ctFiles[0]->filename, db);

	for(uint32 i=0; i<numCTs; i++)
		delete ctFiles[i];
	delete[] ctFiles;

	return ct;
}
CodeTable* FicMain::LoadFirstCodeTable(Database *db, string ctDir) {
	uint32 numCTs;
	CTFileInfo **ctFiles = FicMain::LoadCodeTableFileList("min-max", db, numCTs, ctDir);

	if(numCTs == 0)
		THROW("No code tables found.");

	CodeTable *ct = CodeTable::LoadCodeTable(ctDir + ctFiles[0]->filename, db);

	for(uint32 i=0; i<numCTs; i++)
		delete ctFiles[i];
	delete[] ctFiles;

	return ct;
}
CodeTable* FicMain::LoadLastCodeTable(const Config *config, Database *db, string ctDir) {
	uint32 numCTs;
	CTFileInfo **ctFiles = FicMain::LoadCodeTableFileList(config, db, numCTs, ctDir);

	if(numCTs == 0)
		THROW("No code tables found.");

	CodeTable *ct = CodeTable::LoadCodeTable(ctDir + ctFiles[numCTs-1]->filename, db);

	for(uint32 i=0; i<numCTs; i++)
		delete ctFiles[i];
	delete[] ctFiles;

	return ct;
}
CodeTable* FicMain::LoadLastCodeTable(Database *db, string ctDir) {
	uint32 numCTs;
	CTFileInfo **ctFiles = FicMain::LoadCodeTableFileList("min-max", db, numCTs, ctDir);

	if(numCTs == 0)
		THROW("No code tables found.");

	CodeTable *ct = CodeTable::LoadCodeTable(ctDir + ctFiles[numCTs-1]->filename, db);

	for(uint32 i=0; i<numCTs; i++)
		delete ctFiles[i];
	delete[] ctFiles;

	return ct;
}

string FicMain::FindCodeTableFullDir(string ctDir) {
	// Make sure ctDir ends with a (back)slash
	if(ctDir.find_last_of("\\/") != ctDir.length()-1)
		ctDir += "/";

	// Try to find existing directory
	string dir_path = ctDir;											// 1) First see if given path is full path
	if(!FileUtils::DirectoryExists(dir_path)) {
		dir_path = Bass::GetWorkingDir() + ctDir;						// 2) Working directory
		if(!FileUtils::DirectoryExists(dir_path)) {
			dir_path = Bass::GetExpDir() + "compress/" + ctDir;		// 3) Compression experiments directory
			if(!FileUtils::DirectoryExists(dir_path)) {
				dir_path = Bass::GetDataDir() + "compress/" + ctDir;		// 3) legacy data directory
				if(!FileUtils::DirectoryExists(dir_path)) {
					dir_path = Bass::GetCtReposDir() + ctDir;				// 4) CodeTable repository
					if(!FileUtils::DirectoryExists(dir_path)) {
						dir_path = Bass::GetDataDir() + ctDir;				// 5) Base data dir
						if(!FileUtils::DirectoryExists(dir_path))
							throw string("FicMain::FindCodeTableFullDir() :: Cannot find code table directory.");
					}
				}
			}
		}
	}
	return dir_path;
}

CodeTable* FicMain::RetrieveCodeTable(const Config *config, Database *db) {
	// tag opbouwen
	string tag = config->Read<string>("dbname") + "-" + config->Read<string>("isctype") + "-" + config->Read<string>("iscminsup") + config->Read<string>("iscorder") + "-" + config->Read<string>("prunestrategy");
	return FicMain::RetrieveCodeTable(tag, db);
}
CodeTable* FicMain::RetrieveCodeTable(const string &tag, Database *db) {
	directory_iterator itr = directory_iterator(Bass::GetCtReposDir(), tag + "*");
	if(!itr.next())
		return NULL;

	string dir_path = itr.fullpath();
	if(!FileUtils::DirectoryExists(dir_path))
		return NULL;

	return LoadLastCodeTable(db, dir_path + "/");
}


/* Usage:
	uint32 numCTs;
	CTFileInfo **ctFiles = FicMain::LoadCodeTableFileList(config, db, numCTs[, ctDir]);

	// use ctFiles[i]->filename and ctFiles[i]->minsup

	for(uint32 i=0; i<numCTs; i++) {
		delete ctFiles[i];
	}
	delete[] ctFiles;
*/
CTFileInfo** FicMain::LoadCodeTableFileList(const Config *config, Database *db, uint32 &numCTs, string ctDir) {
	string dir_path = (ctDir.length() > 0) ? ctDir : FicMain::FindCodeTableFullDir(config->Read<string>("ctdir"));
	return LoadCodeTableFileList(config->Read<string>("minsups"), db, numCTs, dir_path);
}
CTFileInfo** FicMain::LoadCodeTableFileList(const string &minsups, Database *db, uint32 &numCTs, string dir_path) {
	// NOTE:	This code is only based on minsups, but with the new .ct naming convention
	//			we could also add extraction (and usage) of candidate # information.

	// check 'minsups'-parameter
	bool minsupRange = false, minLiteral = false, maxLiteral = false;
	uint32 *minsupAr = NULL;
	uint32 minsupFrom = 0, minsupTo = 0, numMinsup = 0, minLiteralPos = 0, maxLiteralPos = 0;

	if(minsups.find_first_of('-')!=string::npos) {
		minsupRange = true;
		minsupFrom = (minsups.substr(0,3).compare("min")==0) ? 0 : atoi(minsups.c_str());
		minsupTo = (minsups.substr(minsups.find_first_of('-')+1,3).compare("max")==0 || minsups.substr(minsups.find_first_of('-')+2,3).compare("max")==0) ? UINT32_MAX_VALUE : atoi(minsups.substr(minsups.find_first_of('-')+1,minsups.length()-minsups.find_first_of('-')-1).c_str());
	} else {
		const char *minsupstr = minsups.c_str();
		uint32 pos = 0, curMinsup = 0;
		while((pos = (uint32) minsups.find_first_of(',',pos+1))!=string::npos) {
			numMinsup++;
		} numMinsup++;
		minsupAr = new uint32[numMinsup];
		pos = 0;
		for(uint32 i=0; i<numMinsup; i++) {
			while (minsupstr[pos] == ',' || minsupstr[pos] == ' ')	pos++;
			if(minsups.substr(pos,3).compare("min") == 0) {
				minLiteral = true;
				minLiteralPos = i;
				minsupAr[i] = 0;
			} else if(minsups.substr(pos,3).compare("max") == 0) {
				maxLiteral = true;
				maxLiteralPos = i;
				minsupAr[i] = 0;
			} else 
				minsupAr[i] = atoi(minsupstr + pos);
			pos = (uint32) minsups.find_first_of(',',pos+1)+1;
		}
	}

	numCTs = 0;
	uint32 minminsup = UINT32_MAX_VALUE, maxminsup = 0;

	directory_iterator itr(dir_path, "*.ct");
	string minsupStr;
	while(itr.next()) {
		string filename = itr.filename();
		// old .ct names (ends with -[minsup].ct)
		//uint32 ms = atoi(filename.substr(filename.find_last_of('-')+1,filename.length()-filename.find_last_of('-')-4).c_str());
		// new .ct names (ends with -[minsup]-[cand].ct)
		minsupStr = filename.substr(0, filename.find_last_of('-'));
		minsupStr = minsupStr.substr(minsupStr.find_last_of('-')+1);
		uint32 ms = atoi(minsupStr.c_str());
		if(minsupRange) {
			if(ms >= minsupFrom && ms <= minsupTo)
				numCTs++;
		} else {
			for(uint32 i=0; i<numMinsup; i++) {
				if(ms < minminsup)	minminsup = ms;
				if(ms > maxminsup)	maxminsup = ms;
				if(minsupAr[i] == ms) {
					numCTs++;
					break;
				}
			}
		}
	}
	if(minLiteral == true) {			// moet nog gearmoured worden, met name ivm later ophalen van xcompressed ct's
		bool addMinMinSup = true;
		for(uint32 i=0; i<numMinsup; i++) {
			if(i != minLiteralPos && minsupAr[i] == minminsup) {
				addMinMinSup = false;
				break;
			}
		}
		if(addMinMinSup == true) {
			minsupAr[minLiteralPos] = minminsup;
			numCTs++;
		}
	}
	if(maxLiteral == true) {			// moet nog gearmoured worden, met name ivm later ophalen van xcompressed ct's
		bool addMaxMinSup = true;
		for(uint32 i=0; i<numMinsup; i++)
			if(i != maxLiteralPos && minsupAr[i] == maxminsup) {
				addMaxMinSup = false;
				break;
			}
			if(addMaxMinSup == true) {
				minsupAr[maxLiteralPos] = maxminsup;
				numCTs++;
			}
	}
	if(numCTs == 0) {
		delete minsupAr;
		THROW("Geen specifiek genoemde codetables gevonden...");
	}

	CTFileInfo **ctFiles = new CTFileInfo *[numCTs];
	uint32 curCT = 0;
	{
		directory_iterator itr(dir_path, "*.ct");
		while(itr.next()) {
			// new .ct names (ends with -[minsup]-[cand].ct)
			minsupStr = itr.filename().substr(0, itr.filename().find_last_of('-'));
			minsupStr = minsupStr.substr(minsupStr.find_last_of('-')+1);
			uint32 ms = atoi(minsupStr.c_str());
			if(minsupRange) {
				if(ms >= minsupFrom && ms <= minsupTo) {
					ctFiles[curCT] = new CTFileInfo();
					ctFiles[curCT]->filename = itr.filename();
					ctFiles[curCT]->minsup = ms;
					curCT++;
				}
			} else {
				for(uint32 i=0; i<numMinsup; i++) {
					if(minsupAr[i] == ms) {
						ctFiles[curCT] = new CTFileInfo();
						ctFiles[curCT]->filename = itr.filename();
						ctFiles[curCT]->minsup = ms;
						curCT++;
						break;
					}
				}
			}
		}
	}

	uint32 tmp;
	string tmpstr;
	for(uint32 i=0; i<numCTs-1; i++) {					// sort ct-arrays op minsup
		for(uint32 j=i+1; j<numCTs; j++) {
			if(ctFiles[i]->minsup > ctFiles[j]->minsup) {
				tmp = ctFiles[i]->minsup;
				ctFiles[i]->minsup = ctFiles[j]->minsup;
				ctFiles[j]->minsup = tmp;
				tmpstr = ctFiles[i]->filename;
				ctFiles[i]->filename = ctFiles[j]->filename;
				ctFiles[j]->filename = tmpstr;
			}
		}
	}

	delete minsupAr;

	return ctFiles;
}

void FicMain::ParseCompressTag(const string &compressTag, string &iscName, string &pruneStrategy, string &timestamp) {
	string tag = compressTag;
	if(tag.find_last_of("\\/") == tag.length()-1)	// remove trailing slash or backslash
		tag = tag.substr(0, tag.length()-1);
	tag = tag.substr(tag.find_last_of("\\/")+1);	// remove path before tag
	size_t idx = 0, idx2;
	for(uint32 i=0; i<3; i++)
		idx = tag.find_first_of("-", idx+1);
	idx2 = tag.find_first_of("-", idx+1);
	iscName = tag.substr(0, idx);
	pruneStrategy = tag.substr(idx+1, idx2-idx-1);
	timestamp = tag.substr(idx2+1, tag.length() - idx2 - 1);
}
