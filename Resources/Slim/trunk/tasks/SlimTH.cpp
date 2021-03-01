#ifdef ENABLE_SLIM

#if defined (_WINDOWS)
	#include <direct.h>
	#include <windows.h>
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <sys/stat.h>
#endif

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
#include <itemstructs/CoverSet.h>

#include "../blocks/slim/SlimAlgo.h"
#include "../blocks/krimp/codetable/CodeTable.h"

#include "../FicMain.h"

#include "SlimTH.h"

SlimTH::SlimTH(Config *conf) : TaskHandler(conf){

}

SlimTH::~SlimTH() {
	// not my Config*
}

void SlimTH::HandleTask() {
	string command = mConfig->Read<string>("command");

	if(command.compare("compress") == 0)	Compress(mConfig);
	else	THROW("Unable to handle task `" + command + "`");
}

string SlimTH::BuildWorkingDir() {
	return mConfig->Read<string>("taskclass") + "/";
}

// Originally, FicMain::Compress
void SlimTH::Compress(Config *config, const string tagIn) {
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
		tag = iscName;
	} else {
		tag = tagIn;
		timetag = tag.substr(tag.find_last_of('-')+1,tag.length()-1-tag.find_last_of('-'));
	}

	// Read DB
	Database *db = Database::RetrieveDatabase(dbName, dataType, dbType);

	// Get ISC
	string iscFullPath = Bass::GetWorkingDir() + iscName + ".isc";

	ItemSetCollection *isc = NULL;
	string iscIfMinedStr = config->Read<string>("iscifmined", "");
	bool loadAll = config->Read<bool>("loadall", 0);		// ??? sander-add
	bool iscBeenMined = false;
	if (config->Read<string>("algo").compare(0, 4, "slim") == 0) { // We don't really need a itemset collection...
		stringstream ss;
		ss << dbName << "-all-" << db->GetNumTransactions()+1 << "d";
		iscName = ss.str();
		iscIfMinedStr = "zap"; // Otherwise, we can crash... don't feel the urgent need to fix this properly ;-(
	} 
	try {
		uint8 ol = Bass::GetOutputLevel();
		if (config->Read<string>("algo").compare(0, 4, "slim") == 0) { // We don't really need a itemset collection...
			Bass::SetOutputLevel(0);
		}
		IscFileType storeIscFileType = config->KeyExists("iscStoreType") ? IscFile::ExtToType(config->Read<string>("iscStoreType")) : BinaryFicIscFileType;
		IscFileType chunkIscFileType = config->KeyExists("iscChunkType") ? IscFile::ExtToType(config->Read<string>("iscChunkType")) : BinaryFicIscFileType;
		isc = FicMain::ProvideItemSetCollection(iscName, db, iscBeenMined, iscIfMinedStr.compare("store") == 0, loadAll, storeIscFileType, chunkIscFileType);
		if (config->Read<string>("algo").compare(0, 4, "slim") == 0) { // We don't really need a itemset collection...
			Bass::SetOutputLevel(ol);
		}
	} catch(string msg) {
		delete db;
		throw msg;
	}

	DoCompress(config, db, isc, tag, timetag, 0, 0);
	delete db;
	delete isc;

	if(iscBeenMined == true && iscIfMinedStr.compare("zap") == 0) {	// if stored, it's already gone from workingdir
		FileUtils::RemoveFile(iscFullPath);
	}
}

// Originally, FicMain::DoCompress
void SlimTH::DoCompress(Config *config, Database *db, ItemSetCollection *isc, const string &tag, const string &timetag, const uint32 resumeSup, const uint64 resumeCand) {
	// ISC normalized?

	// Build KrimpBase
	string algoName = config->Read<string>("algo");
	KrimpAlgo *algo = SlimAlgo::CreateAlgo(algoName, db->GetDataType(), config);
	if(algo == NULL)
		THROW("Unknown algorithm.");
	ECHO(1, printf("** Compression :: \n"));
	ECHO(3, printf(" * Initiating...\n"));

	// Determine tag, output dir & backup full config
	{
		string outDir = Bass::GetWorkingDir() + tag + "-" + algoName.substr(0, algoName.find_first_of('-')) + "-" + algo->GetShortName() + "-" + config->Read<string>("prunestrategy","nop") + "-" + timetag + "/";
		FileUtils::CreateDir(outDir); /* check succes ? */

		//string confFile = outDir + "/" + config->Read<string>("command") + "-" + TimeUtils::GetTimeStampString() + ".conf";
		string confFile = outDir + "/" + config->Read<string>("command") + "-" + timetag + ".conf";
		config->WriteFile(confFile);
		algo->SetOutputDir(outDir);
		algo->SetTag(tag);

		ECHO(1, printf(" * Experiment tag\t%s\n", tag.c_str()));
	}

	// Further configure KrimpBase
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

		SlimAlgo* algo_ng = static_cast<SlimAlgo*>(algo);
		if (algo_ng) {
			bool reportIter = config->Read<bool>("reportIter", false);
			ReportType reportIterWhat = KrimpAlgo::StringToReportType(config->Read<string>("reportiterwhat", "all"));
			algo_ng->SetReportIter(reportIter);
			algo_ng->SetReportIterStyle(reportIterWhat);

			string ctFile = config->Read<string>("ctFile", "");
			if (!ctFile.empty())
				algo_ng->SetResumeCodeTable(ctFile);
		}

	}
	if(isc->GetNumItemSets() > 0)
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

		// Load appropriate codetable
		algo->LoadCodeTable(resumeSup, resumeCand);
	} else {

		// Write Run-Time Info file
		char rti[100];
		sprintf_s(rti, 100, "__%s %s %s %s", FicMain::ficVersion.c_str(), algo->GetShortName().c_str(), ItemSet::TypeToString(db->GetDataType()).c_str(), algo->GetShortPruneStrategyName().c_str());
		string rtiFile = algo->GetOutDir() + "/" + rti;
		FILE *rtifp = fopen(rtiFile.c_str(), "w");
		fclose(rtifp); /* check success ? */
	}

	// Put algo to work
	CodeTable *ct = algo->DoeJeDing(candOffset, startSup);

	ECHO(3, printf("** Finished compression.\n"));

	// Write transaction covers to disk
	if (config->Read<bool>("writeCover", false)) {
		string filename = algo->GetOutDir() + "/" + tag + ".cover";
		FILE *fp = fopen(filename.c_str(), "w");
		CoverSet* cs = CoverSet::Create(db->GetDataType(), db->GetAlphabetSize());
		for(uint32 r = 0; r < db->GetNumRows(); ++r) {
			ItemSet* t = db->GetRow(r);
			uint16* values = new uint16[t->GetLength()];
			double codeLen = 0.0;

			cs->InitSet(t);

			// item sets (length > 1)
			islist* isl = ct->GetItemSetList();
			for (islist::iterator cit = isl->begin(); cit != isl->end(); ++cit) {
				if(cs->Cover(*cit)) {
					double cl = CalcCodeLength((*cit)->GetUsageCount(), ct->GetCurStats().usgCountSum);
					if (fp) fprintf(fp, "(%s : %.2f) ", (*cit)->ToString(false, false).c_str(), cl);
					codeLen += cl;
				}
			}
			isl->clear();
			delete isl;

			// alphabet items
			islist* sl = ct->GetSingletonList();
			uint16 i = 0;
			for (islist::iterator cit = sl->begin(); cit != sl->end(); ++cit, ++i)
				if(cs->IsItemUncovered(i)) {
					double cl = CalcCodeLength((*cit)->GetUsageCount(), ct->GetCurStats().usgCountSum);
					if (fp) fprintf(fp, "(%s : %.2f) ", (*cit)->ToString(false, false).c_str(), cl);
					codeLen += cl;
				}
			sl->clear();
			delete sl;

			if (fp) fprintf(fp, ": %.2f\n", codeLen);

			delete values;
		}
		delete cs;
		fclose(fp);
	}

	delete algo;
	delete ct;
}

#endif // ENABLE_SLIM
