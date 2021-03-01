#ifdef ENABLE_LESS

#include "../global.h"

// qtils
#include <Config.h>
#include <TimeUtils.h>

// bass
#include <Bass.h>
#include <db/Database.h>

// less
#include "../blocks/less/LowEntropySetCollection.h"
#include "../blocks/less/LowEntropySet.h"
#include "../blocks/less/LescFile.h"
#include "../blocks/less/LEAlgo.h"
#include "../blocks/less/LECodeTable.h"

#include "LESetsTH.h"

LESetsTH::LESetsTH(Config *conf) : TaskHandler(conf){

}
LESetsTH::~LESetsTH() {
	// not my Config*
}

void LESetsTH::HandleTask() {
	string command = mConfig->Read<string>("command");

	if(command.compare("less") == 0)					Less();
	else	if(command.compare("massless") == 0)		MassLess();
	else	if(command.compare("convertlesc") == 0)		ConvertLESC();
	else	if(command.compare("massconvertlesc") == 0) MassConvertLESC();
	else	if(command.compare("massaddheader") == 0)	MassAddLESCHeader();

	else	throw string("LessTH :: Unable to handle task `" + command + "`");
}

void LESetsTH::Less() {
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype","bm128"));
	Database *db = Database::RetrieveDatabase(dbName, dataType);

#if defined (_MSC_VER)
	Bass::SetWorkingDir(Bass::GetExpDir() + "less/");
#elif defined (__GNUC__)
	string wd(Bass::GetExpDir() + "less/");
	Bass::SetWorkingDir(wd);
#endif

	string lescName = mConfig->Read<string>("lescname");
	string lescFileName = lescName + ".lesc";
	string lescFullPath = Bass::GetWorkingDir() + "candidates/" + lescFileName;

	LescFile *lf = new LescFile();
	LowEntropySetCollection *lesc = lf->Open(lescFullPath, db);


	// Build Algo
	string algoName = mConfig->Read<string>("algo");
	LEAlgo *algo = LEAlgo::Create(algoName);
	if(algo == NULL)
		throw string("LESetsTH::Less - Unknown algorithm.");

	LECodeTable *ct = LECodeTable::Create(algoName);

	// Determine Pruning Strategy
	string timetag = TimeUtils::GetTimeStampString();
	string pruneStrategyStr = mConfig->Read<string>("prunestrategy","nop");
	string bitmapStrategyStr = mConfig->Read<string>("bitmapstrategy","const");
	string tag = lescName + "-" + pruneStrategyStr + "-" + bitmapStrategyStr + "-" + timetag;

	// Determine tag, output dir & backup full config
	{
		string outDir = Bass::GetWorkingDir() + tag;
		FileUtils::CreateDir(outDir);
		string confFile = outDir + "/" + mConfig->Read<string>("command") + "-" + timetag + ".conf";
		mConfig->WriteFile(confFile);
		algo->SetOutputDir(outDir);
		algo->SetTag(tag);

		ECHO(1, printf(" * Experiment tag\t%s\n", tag.c_str()));
	}

	// Further configure Algo
	ECHO(1, printf(" * Algorithm:\t\t%s", algoName.c_str()));
	{
		LEPruneStrategy	pruneStrategy = LENoPruneStrategy;		//	Default
		// set `pruneStrategy` and output prune strategy to screen
		if(pruneStrategyStr.compare("nop") == 0 ) {
			ECHO(1, printf(" no-prune "));
			pruneStrategy = LENoPruneStrategy;
		} else if(pruneStrategyStr.compare("pop") == 0) {
			ECHO(1, printf(" post-prune "));
			pruneStrategy = LEPostAcceptPruneStrategy;
		} else if(pruneStrategyStr.compare("pre") == 0) {
			ECHO(1, printf(" pre-prune "));
			pruneStrategy = LEPreAcceptPruneStrategy;
		} else 
			throw string("LESetsTH::Less - Unknown Pruning Strategy: ") + pruneStrategyStr;

		LEBitmapStrategy bitmapStrategy = LEBitmapStrategyConstantCT;		//	Default
		// set `bitmapStrategy` and output bitmap strategy to screen
		if(bitmapStrategyStr.compare("const") == 0 ) {
			ECHO(1, printf("& constant bitmap\n"));
			bitmapStrategy = LEBitmapStrategyConstantCT;
		} else if(bitmapStrategyStr.compare("full") == 0) {
			ECHO(1, printf("& complete bitmap ct\n"));
			bitmapStrategy = LEBitmapStrategyFullCT;
		} else if(bitmapStrategyStr.compare("part") == 0) {
			ECHO(1, printf("& optimal bitmap ct\n"));
			bitmapStrategy = LEBitmapStrategyOptimalCT;
		} else if(bitmapStrategyStr.compare("lenct") == 0) {
			ECHO(1, printf("& complete bitmap ct per length \n"));
			bitmapStrategy = LEBitmapStrategyFullCTPerLength;
		} else if(bitmapStrategyStr.compare("inst") == 0) {
			ECHO(1, printf("& individual inst encodings \n"));
			bitmapStrategy = LEBitmapStrategyIndivInstEncoding;
		} else 
			throw string("LESets::Less - Unknown Bitmap Strategy: ") + bitmapStrategyStr;

		//uint32		reportSup = config->Read<uint32>("reportsup");
		//uint32		reportCnd = config->Read<uint32>("reportcnd");

		algo->UseThisStuff(db, lesc, ct, false, bitmapStrategy);
		algo->SetPruneStrategy(pruneStrategy);
		//algo->SetBitmapStrategy(bitmapStrategy);
		//algo->SetReportSupDelta(reportSup);
		//algo->SetReportCndDelta(reportCnd);
	}
	ECHO(1, printf(" * Candidates:\t\t%" I64d "p\n", lesc->GetNumLowEntropySets()));

	ECHO(1, printf("** Compression :: \n"));
	ECHO(3, printf(" * Initiating...\n"));

	ct = algo->DoeJeDing();

	delete lesc;
	delete ct;
	delete algo;
	delete db;
}

void LESetsTH::MassLess() {
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype","bm128"));
	uint32 numMass = mConfig->Read<uint32>("nummass",0);

#if defined (_MSC_VER)
	Bass::SetWorkingDir(Bass::GetExpDir() + "less/swaprnd/");
#elif defined (__GNUC__)
	string wd(Bass::GetExpDir() + "less/swaprnd/");
	Bass::SetWorkingDir(wd);
#endif

	char buf[20];
	string repFilePath = Bass::GetWorkingDir() + "repFile.csv";
	FILE *repFile = fopen(repFilePath.c_str(), "w");
	for(uint32 m=1; m<numMass+1; m++) {
		_itoa_s(m,buf,20,10);

		string dbName = mConfig->Read<string>("dbname") + buf;
		Database *db = Database::ReadDatabase(dbName,Bass::GetWorkingDir() + "dbs/", dataType);

		string lescName = mConfig->Read<string>("lescname");
		string lescFileName = lescName + buf + ".lesc";
		string lescFullPath = Bass::GetWorkingDir() + "cands/" + lescFileName;

		LescFile *lf = new LescFile();
		LowEntropySetCollection *lesc = lf->Open(lescFullPath, db);


		// Build Algo
		string algoName = mConfig->Read<string>("algo");
		LEAlgo *algo = LEAlgo::Create(algoName);
		if(algo == NULL)
			throw string("LESets::MassLess - Unknown algorithm.");

		LECodeTable *ct = LECodeTable::Create(algoName);

		// Determine Pruning Strategy
		string timetag = TimeUtils::GetTimeStampString();
		string pruneStrategyStr = mConfig->Read<string>("prunestrategy","nop");
		string bitmapStrategyStr = mConfig->Read<string>("bitmapstrategy","const");
		string tag = lescName + "-" + pruneStrategyStr + "-" + bitmapStrategyStr + "-" + timetag;

		// Determine tag, output dir & backup full config
		{
			string outDir = Bass::GetWorkingDir() + tag;
			FileUtils::CreateDir(outDir);
			string confFile = outDir + "/" + mConfig->Read<string>("command") + "-" + timetag + ".conf";
			mConfig->WriteFile(confFile);
			algo->SetOutputDir(outDir);
			algo->SetTag(tag);

			ECHO(1, printf(" * Experiment tag\t%s\n", tag.c_str()));
		}

		// Further configure Algo
		ECHO(1, printf(" * Algorithm:\t\t%s", algoName.c_str()));
		{
			LEPruneStrategy	pruneStrategy = LENoPruneStrategy;		//	Default
			// set `pruneStrategy` and output prune strategy to screen
			if(pruneStrategyStr.compare("nop") == 0 ) {
				ECHO(1, printf(" no-prune "));
				pruneStrategy = LENoPruneStrategy;
			} else if(pruneStrategyStr.compare("pop") == 0) {
				ECHO(1, printf(" post-prune "));
				pruneStrategy = LEPostAcceptPruneStrategy;
			} else if(pruneStrategyStr.compare("pre") == 0) {
				ECHO(1, printf(" pre-prune "));
				pruneStrategy = LEPreAcceptPruneStrategy;
			} else 
				throw string("LESets::Less - Unknown Pruning Strategy: ") + pruneStrategyStr;

			LEBitmapStrategy bitmapStrategy = LEBitmapStrategyConstantCT;		//	Default
			// set `bitmapStrategy` and output bitmap strategy to screen
			if(bitmapStrategyStr.compare("const") == 0 ) {
				ECHO(1, printf("& constant bitmap\n"));
				bitmapStrategy = LEBitmapStrategyConstantCT;
			} else if(bitmapStrategyStr.compare("full") == 0) {
				ECHO(1, printf("& complete bitmap ct\n"));
				bitmapStrategy = LEBitmapStrategyFullCT;
			} else if(bitmapStrategyStr.compare("part") == 0) {
				ECHO(1, printf("& optimal bitmap ct\n"));
				bitmapStrategy = LEBitmapStrategyOptimalCT;
			} else if(bitmapStrategyStr.compare("lenct") == 0) {
				ECHO(1, printf("& complete bitmap ct per length \n"));
				bitmapStrategy = LEBitmapStrategyFullCTPerLength;
			} else if(bitmapStrategyStr.compare("inst") == 0) {
				ECHO(1, printf("& individual inst encodings \n"));
				bitmapStrategy = LEBitmapStrategyIndivInstEncoding;
			} else 
				throw string("LESets::Less - Unknown Bitmap Strategy: ") + bitmapStrategyStr;

			//uint32		reportSup = config->Read<uint32>("reportsup");
			//uint32		reportCnd = config->Read<uint32>("reportcnd");

			algo->UseThisStuff(db, lesc, ct, false, bitmapStrategy);
			algo->SetPruneStrategy(pruneStrategy);
			//algo->SetBitmapStrategy(bitmapStrategy);
			//algo->SetReportSupDelta(reportSup);
			//algo->SetReportCndDelta(reportCnd);
		}
		ECHO(1, printf(" * Candidates:\t\t%" I64d "p\n", lesc->GetNumLowEntropySets()));

		ECHO(1, printf("** Compression :: \n"));
		ECHO(3, printf(" * Initiating...\n"));

		ct = algo->DoeJeDing();

		// write to reportfile
		LECoverStats stats = ct->GetCurStats();
		fprintf(repFile, "%" I64d ";%d;%d;%d;%" I64d ";%.0f;%.0f;%.0f;%.0f;%.0f;%.2f\n", lesc->GetNumLowEntropySets(), stats.alphItemsUsed, stats.numSetsUsed, ct->GetCurNumSets(), stats.setCountSum, stats.dbSize, stats.bmSize, stats.ct1Size, stats.ct2Size, stats.size, stats.likelihood);
		fflush(repFile);

		delete lesc;
		delete ct;
		delete algo;
		delete db;
	}
	fclose(repFile);
}


void LESetsTH::ConvertLESC() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType);

	LescOrderType outOrder = LowEntropySetCollection::StringToLescOrderType(mConfig->Read<string>("lescoutorder"));

	string outName = "";
	if(mConfig->KeyExists("lescoutname"))	// Custom filename
		outName = mConfig->Read<string>("lescoutname");
	if(outName.length() == 0) {				// Default filename
		outName = mConfig->Read<string>("lescname") + mConfig->Read<string>("lescoutorder");
	}

	outName = Bass::GetWorkingDir() + outName + ".lesc";
	string fullName = Bass::GetExpDir() + "less/candidates/" + mConfig->Read<string>("lescname") + ".lesc";
	LowEntropySetCollection::ConvertLescFile(fullName, outName, db, outOrder);

	delete db;
}

// Adds correct fic-lesc headers to n .txt (helsinki-mined :-) LESC files
void LESetsTH::MassAddLESCHeader() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName, Bass::GetDbReposDir(), Uint16ItemSetType);

	LescOrderType outOrder = LowEntropySetCollection::StringToLescOrderType(mConfig->Read<string>("lescoutorder"));

	string outName = "";
	if(mConfig->KeyExists("lescoutname"))	// Custom filename
		outName = mConfig->Read<string>("lescoutname");
	if(outName.length() == 0) {				// Default filename
		outName = mConfig->Read<string>("lescname") + mConfig->Read<string>("lescoutorder");
	}

	uint32 numLESCs = mConfig->Read<uint32>("numlescs",0);
	char buf[20];
	char rbuf[500000];
	char*bpN;
	for(uint32 i=1; i<numLESCs+1; i++) {
		_itoa_s(i,buf,20,10);
		outName = Bass::GetWorkingDir() + "less/massconvert/" + outName + buf + ".lesc";
		string fullName = Bass::GetExpDir() + "less/massconvert/" + mConfig->Read<string>("lescname") + buf + ".txt";
		// quick dirty scan .txt file
		FILE *f = fopen(fullName.c_str(),"r");
		uint64 numRows = 0;
		uint32 maxLen = 0;

		while(!feof(f)) {
			fgets(rbuf,500000,f);
			numRows++;
			uint32 numAttributes = strtoul(rbuf,&bpN,10);
			if(numAttributes > maxLen)
				maxLen = numAttributes;
		}
		numRows--;
		rewind(f);
		// quick & dirty write header
		string fullName2 = Bass::GetExpDir() + "less/massconvert/" + mConfig->Read<string>("lescname") + buf + ".lesc";
		FILE *f2 = fopen(fullName2.c_str(), "w");

		fprintf(f2, "ficlesc-1.0\n");
		fprintf(f2, "mi: numSets=%" I64d " maxEnt=%lf maxLen=%d sepRows=0 lescOrder=lAeA patType=lesc dbName=unknown\n", numRows, 1.0, maxLen);
		for(uint64 j=0; j<numRows; j++) {
			fgets(rbuf,500000,f);
			fputs(rbuf,f2);
		}
		fclose(f);
		fclose(f2);
	}
	delete db;
}

// Converts n LESC files, primarily used for reordering
void LESetsTH::MassConvertLESC() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName, Bass::GetDbReposDir(), Uint16ItemSetType);

	LescOrderType outOrder = LowEntropySetCollection::StringToLescOrderType(mConfig->Read<string>("lescoutorder"));

	string outName = "";
	if(mConfig->KeyExists("lescoutname"))	// Custom filename
		outName = mConfig->Read<string>("lescoutname");
	if(outName.length() == 0) {				// Default filename
		outName = mConfig->Read<string>("lescname") + mConfig->Read<string>("lescoutorder");
	}

	uint32 numLESCs = mConfig->Read<uint32>("numlescs",0);
	char buf[20];
	for(uint32 i=1; i<numLESCs+1; i++) {
		_itoa_s(i,buf,20,10);
		string fullOutName = Bass::GetWorkingDir() + "less/massconvert/" + outName + buf + ".lesc";
		string fullName = Bass::GetExpDir() + "less/massconvert/" + mConfig->Read<string>("lescname") + buf + ".lesc";

		LowEntropySetCollection::ConvertLescFile(fullName, fullOutName, db, outOrder);
	}

	delete db;
}

#endif // ENABLE_LESS
