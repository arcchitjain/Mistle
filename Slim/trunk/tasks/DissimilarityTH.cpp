#include "../global.h"

// system
#include <time.h>

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <RandomUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>

// bass
//#include <Bass.h>
#include <db/ClassedDatabase.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <isc/ItemTranslator.h>

// fic
//#include "../classify/Classifier.h"
#include "../blocks/krimp/codetable/CodeTable.h"

// leluk
#include "../FicMain.h"

#include "TaskHandler.h"
#include "DissimilarityTH.h"

DissimilarityTH::DissimilarityTH(Config *conf) : TaskHandler(conf){
}

DissimilarityTH::~DissimilarityTH() {
	// not my Config*
}

void DissimilarityTH::HandleTask() {
	string command = mConfig->Read<string>("command");

	if(command.compare("innerdbdists") == 0)				InnerDBDistances();
	else	if(command.compare("innerdbdiststofull") == 0)	InnerDBSampleToFullDBDistances();
	else	if(command.compare("classdistances") == 0)		ClassDistances();
	else	if(command.compare("gimmethedistance") == 0)	ComputeDSForGivenDBsAndCTs();

	else	throw string("DissimilarityTH :: Unable to handle task `" + command + "`");
}

void DissimilarityTH::ComputeDSForGivenDBsAndCTs() {
	string dsDir = mConfig->Read<string>("dsDir");
	Database *db1 = Database::ReadDatabase(mConfig->Read<string>("db1"), dsDir);
	Database *db2 = Database::ReadDatabase(mConfig->Read<string>("db2"), dsDir);
//	CodeTable *ct1 = CodeTable::LoadCodeTable(Bass::GetDataDir() + mConfig->Read<string>("db1") + ".ct", db1);
//	CodeTable *ct2 = CodeTable::LoadCodeTable(Bass::GetDataDir() + mConfig->Read<string>("db2") + ".ct", db2);
	CodeTable *ct1 = CodeTable::LoadCodeTable(dsDir + mConfig->Read<string>("ct1") + ".ct", db1);
	CodeTable *ct2 = CodeTable::LoadCodeTable(dsDir + mConfig->Read<string>("ct2") + ".ct", db2);
	ct1->AddOneToEachUsageCount();
	ct2->AddOneToEachUsageCount();
	double ds = DissimilarityTH::BerekenAfstandTussen(db1, db2, ct1, ct2);
	string fullpathoutput = dsDir + "dissimilarity.txt";
	FILE* fp = fopen(fullpathoutput.c_str(),"w");
	fprintf(fp, "%f\n", ds);
	fclose(fp);
	delete ct1;
	delete ct2;
	delete db1;
	delete db2;
}

void DissimilarityTH::ClassDistances() {
	char temp[200];

	string iscName = mConfig->Read<string>("iscname");
	string dbName = iscName.substr(0, iscName.find_first_of("-"));
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	Database *db = Database::RetrieveDatabase(dbName, dataType);

	ClassedDatabase *cdb = new ClassedDatabase(db);
	Database **cdbs = cdb->SplitOnClassLabels();
	uint32 numClasses = cdb->GetNumClasses();
	uint16 *classes = db->GetClassDefinition();
	string **strClasses = new string *[numClasses];
	for(uint16 i=0; i<numClasses; i++) {
		_itoa(classes[i], temp, 10);
		strClasses[i] = new string(temp);
	}

	CodeTable **cts = new CodeTable *[numClasses];

	uint32 lpcFactor = mConfig->Read<uint32>("lpcfactor");
	uint32 lpcOffset = mConfig->Read<uint32>("lpcoffset");

	for(uint32 i=0; i<numClasses; i++) {
		cts[i] = FicMain::CreateCodeTable(cdbs[i], mConfig, true);
		cts[i]->ApplyLaplaceCorrection(lpcFactor, lpcOffset);
	}

	double dsSum = 0.0f, ds = 0.0f;
	FILE *fp;
	_itoa(lpcFactor, temp, 10);
	string lpcFactorStr(temp);
	_itoa(lpcOffset, temp, 10);
	string lpcOffsetStr(temp);
	string s = Bass::GetWorkingDir() + iscName + "_" + lpcFactorStr + "_" + lpcOffsetStr + "_classdistance.txt";
	fopen_s(&fp, s.c_str(), "w");
	uint32 num = 0;

	for(uint32 i=0; i<numClasses; i++)
		for(uint32 j=0; j<i; j++) {
			ds = DissimilarityTH::BerekenAfstandTussen(cdbs[i], cdbs[j], cts[i], cts[j]);
			dsSum += ds;
			fprintf(fp, "%s <> %s :: %.02f\n", strClasses[j]->c_str(), strClasses[i]->c_str(), ds);
			num++;
		}

	dsSum /= num;
	fprintf(fp, "\nAvgDS = %.02f\n", dsSum);
	fclose(fp);

	for(uint32 i=0; i<numClasses; i++) {
		delete cdbs[i];
		delete cts[i];
		delete strClasses[i];
	}
	delete[] cdbs;
	delete[] cts;
	delete[] strClasses;
	delete cdb;
	delete db;
}
/* ------ 'DEPRECATED', OR: ANCIENT ----------------
void DissimilarityTH::ClassDistances() {
	bool useExistingResults = false;

	// Read DB
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);

	// Determine tag, output dir & backup full config
	string outDir, tag;
	{
		outDir = Bass::GetWorkingDir() + dbName;
		if(!FileUtils::CreateDir(outDir)) throw string("Unable to create output directory");
		outDir += "/distances";
		if(!FileUtils::CreateDir(outDir)) throw string("Unable to create output directory");
		tag = mConfig->Read<string>("isctype") + "-" + mConfig->Read<string>("iscminsup") + mConfig->Read<string>("iscoutorder");
		string timestamp = mConfig->Read<string>("timestamp", "");
		if(timestamp.length() > 0) // existing compresion results
			useExistingResults = true;
		else // new compression
			timestamp = TimeUtils::GetTimeStampString();
		tag += "-" + timestamp;
		outDir += "/" + tag;
		if(!useExistingResults)
			if(!FileUtils::CreateDir(outDir)) throw string("Unable to create output directory");
		outDir += "/";
		//if(!useExistingResults) {
		string confFile = outDir + mConfig->Read<string>("command") + ".conf";
		mConfig->WriteFile(confFile);
		//}
		ECHO(1,printf("** Experiment ::\n * Tag:\t\t\t%s\n", tag.c_str()));
	}

	// Build target array
	uint32 numTargets = 0;
	uint32 *targets = StringUtils::TokenizeUint32(mConfig->Read<string>("targets"), numTargets);
	ECHO(1,printf(" * Specified targets:\t"));
	for(uint32 i=0; i<numTargets; i++)
		ECHO(1,printf("%d ", targets[i]));
	ECHO(1,printf("\n"));

	// Prepare for splitting in classes
	char tmp[10];
	string target;
	Database **perClassDbs = new Database *[numTargets];
	for(uint32 i=0; i<numTargets; i++)
		perClassDbs[i] = NULL;

	uint8 outbak = Bass::GetOutputLevel(); Bass::SetOutputLevel(0);
	ECHO(2,printf("\r * Database per class:\tcompressing.\n"));

	// Split per class for compression
	db->SplitForClassification(numTargets, targets, perClassDbs);
	delete db;
	Bass::SetWorkingDir(outDir);
	double **sizes = new double *[numTargets];
	double **avgLen = new double *[numTargets];
	double **sdLen = new double *[numTargets];

	uint32 maxNumRows = 0;
	for(uint32 i=0; i<numTargets; i++)
		if(perClassDbs[i]->GetNumRows() > maxNumRows)
			maxNumRows = perClassDbs[i]->GetNumRows();
	double *codeLens = new double[maxNumRows];
	string filename = "", target2 = "";

	// Foreach class
	for(uint32 t=0; t<numTargets; t++) {
		ECHO(1,printf("*** Processing&compressing class %d ::\n",targets[t]));

		sizes[t] = new double[numTargets];
		avgLen[t] = new double[numTargets];
		sdLen[t] = new double[numTargets];

#if _MSC_VER >= 1400
		_itoa(targets[t], tmp, 10);
#elif _MSC_VER >= 1310
		itoa(targets[t], tmp, 10);
#endif		
		target = tmp;
		dbName = string("class") + target;
		db = new Database(perClassDbs[t]);
		Database::WriteDatabase(db, dbName, outDir); // ! write copy instead of real db because it fucks up the datatype
		delete db;
		db = perClassDbs[t];

		if(!useExistingResults) {
			// FIS generation
			string iscGenerateName = outDir + dbName + "-" + mConfig->Read<string>("isctype") 
				+ mConfig->Read<string>("iscminsup") + ".fi";
			string isctype = mConfig->Read<string>("isctype");
			if(isctype.compare("cls")==0)
				isctype = "closed";
			string exec = Bass::GetExecDir() + "fim_" + isctype + ".exe \"" + outDir + dbName + ".db\" " 
				+ mConfig->Read<string>("iscminsup") + " \"" + iscGenerateName + "\"";
			ECHO(1,printf("** Creating Frequent Item Set Collection ::\n"));
			ECHO(2,printf(" * Item Set Type:\t%s\n",mConfig->Read<string>("isctype").c_str()));
			ECHO(2,printf(" * Mining Item Sets:\tin progress..."));
			system(exec.c_str());	// !!! exit code checken
			if(!FileUtils::FileExists(iscGenerateName)) {
				delete db;
				throw string("FicMain::ClassDistances -- Frequent ItemSet generation failed.");
			}
			ECHO(2,printf("\r * Mining Item Sets:\tdone.         \n\n"));

			string iscFilename = dbName + "-" + mConfig->Read<string>("isctype") + mConfig->Read<string>("iscminsup");
			IscOrderType iscOrder = ItemSetCollection::StringToIscOrderType(mConfig->Read<string>("iscoutorder"));
			iscFilename += "." + mConfig->Read<string>("iscoutext");
			string iscFullname = outDir + iscFilename;

			IscFileType iscType = IscFile::StringToType(iscFullname);

			ItemSetCollection::ConvertIscFile(iscGenerateName, iscFullname, db, iscType, iscOrder, false);

			remove( iscGenerateName.c_str() ); // check success ? 
			if(!FileUtils::FileExists(iscFullname)) {
				delete db;
				throw string("FicMain::ClassDistances -- Frequent ItemSet conversion failed.");
			}
			ECHO(3,printf("** Finished Creating Frequent Item Set Collection\n"));
			ECHO(1,printf("\n"));

			// Read ItemSetCollection
			ItemSetCollection *isc = ItemSetCollection::OpenItemSetCollection(iscFullname, "", db);

			// Perform compression
			FicMain::DoCompress(mConfig, db, isc, dbName, TimeUtils::GetTimeStampString(),0, 0);

			// Delete Frequent Itemsets if not kept
			delete isc;
			if(mConfig->Read<uint32>("isckeep") == 0)
				remove( iscFullname.c_str() );
		}

		// Cover each class with resulting codetable
		CodeTable *ct = CodeTable::Create("coverfull"); //mConfig->Read<string>("algo"));
		ct->UseThisStuff(perClassDbs[t], perClassDbs[t]->GetDataType(), InitCTEmpty, 0, 1);
		ct->ReadFromDisk(outDir + dbName + "/ct-" + dbName + "-" + mConfig->Read<string>("iscminsup") + ".ct", true);
		ct->AddOneToEachCount();
		ct->CalcCodeTableSize();

		ItemSet **iss;
		uint32 numRows;
		double avg;
		double sd;

		for(uint32 d=0; d<numTargets; d++) {
			sizes[t][d] = 0;
			sd = 0;

			iss = perClassDbs[d]->GetRows();
			numRows = perClassDbs[d]->GetNumRows();
			for(uint32 r=0; r<numRows; r++) {
				codeLens[r] = ct->CalcTransactionCodeLength(iss[r]);
				sizes[t][d] += codeLens[r];
			}

			avg = sizes[t][d] / numRows;
			avgLen[t][d] = avg;

			// add code table sizes (or // not)
			//sizes[t][d] += ct->GetCurStats().ctSize;

#if _MSC_VER >= 1400
			_itoa(targets[d], tmp, 10);
#elif _MSC_VER >= 1310
			itoa(targets[d], tmp, 10);
#endif		
			target2 = tmp;
			filename = outDir + "codelens" + target2 + "-ct" + target + ".txt";
			FILE *fp = fopen(filename.c_str(), "w");
			for(uint32 r=0; r<numRows; r++) {
				fprintf(fp, "%.6f\n", codeLens[r]);
				sd += pow(codeLens[r] - avg, 2);
			}
			fclose(fp);
			sdLen[t][d] = sqrt(sd / numRows);
		}

		// Build covers and write counts
		for(uint32 d=0; d<numTargets; d++) {
#if _MSC_VER >= 1400
			_itoa(targets[d], tmp, 10);
#elif _MSC_VER >= 1310
			itoa(targets[d], tmp, 10);
#endif		
			target2 = tmp;
			ct->SetDatabase(perClassDbs[d]);
			filename = outDir + "cover" + target2 + "-ct" + target + ".txt";
			ct->WriteCover(ct->GetCurStats(), filename);
			filename = outDir + "covercounts" + target2 + "-ct" + target + ".txt";
			ct->WriteCountsToFile(filename);
		}

		delete ct;
	}

	// Compute and write to disk
	filename = outDir + "distances.txt"; 
	FILE *fp = fopen(filename.c_str(), "w");

	// Data
	fprintf(fp, "Database:\t%s\nFis:\t\t%s%s%s\n\n** Interclass compression **\n\n\t\t", mConfig->Read<string>("dbname").c_str(), mConfig->Read<string>("isctype").c_str(), mConfig->Read<string>("iscminsup").c_str(), mConfig->Read<string>("iscoutorder").c_str());
	for(uint32 t=0; t<numTargets; t++)
		fprintf(fp, "%d\t\t", targets[t]);
	fprintf(fp, "db's\n");
	for(uint32 t=0; t<numTargets; t++) {
		fprintf(fp, "%d\t", targets[t]);
		for(uint32 i=0; i<numTargets; i++) {
			fprintf(fp, "%.02f\t",
				sizes[t][i]
			);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "ct's\n");

	// Min
	fprintf(fp, "\n\n** Simple data distance MIN **\n\n\t\t");
	for(uint32 t=0; t<numTargets; t++)
		fprintf(fp, "%d\t\t", targets[t]);
	fprintf(fp, "db's\n");
	for(uint32 t=0; t<numTargets; t++) {
		fprintf(fp, "%d\t", targets[t]);
		for(uint32 i=0; i<t; i++) {
			fprintf(fp, "%.02f\t",
				min((sizes[t][i] - sizes[i][i]) / sizes[i][i], (sizes[i][t] - sizes[t][t]) / sizes[t][t])
				//				min(sizes[t][i] - sizes[i][i], sizes[i][t] - sizes[t][t]) / 
				//					max(sizes[t][t], sizes[i][i])
				);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "ct's\n");

	// Max
	fprintf(fp, "\n\n** Simple data distance MAX **\n\n\t\t");
	for(uint32 t=0; t<numTargets; t++)
		fprintf(fp, "%d\t\t", targets[t]);
	fprintf(fp, "db's\n");
	for(uint32 t=0; t<numTargets; t++) {
		fprintf(fp, "%d\t", targets[t]);
		for(uint32 i=0; i<t; i++) {
			fprintf(fp, "%.02f\t",
				max((sizes[t][i] - sizes[i][i]) / sizes[i][i], (sizes[i][t] - sizes[t][t]) / sizes[t][t])
				//				max(sizes[t][i] - sizes[i][i], sizes[i][t] - sizes[t][t]) / 
				//					max(sizes[t][t], sizes[i][i])
				);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "ct's\n");

	// Deviations
	fprintf(fp, "\n\n** Code lengths - averages and standard deviations **\n\n\t\t");
	for(uint32 t=0; t<numTargets; t++)
		fprintf(fp, "%d\t\t\t\t", targets[t]);
	fprintf(fp, "db's\n");
	for(uint32 t=0; t<numTargets; t++) {
		fprintf(fp, "%d\t\t", targets[t]);
		for(uint32 i=0; i<numTargets; i++) {
			fprintf(fp, "%.02f +- %.02f\t",
				avgLen[t][i], sdLen[t][i]
			);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "ct's\n");

	// Asymmetric Distances
	fprintf(fp, "\n\n** Asymmetric distances **\n\n\t\t");
	for(uint32 t=0; t<numTargets; t++)
		fprintf(fp, "%d\t\t", targets[t]);
	fprintf(fp, "db's\n");
	for(uint32 t=0; t<numTargets; t++) {
		fprintf(fp, "%d\t", targets[t]);
		for(uint32 i=0; i<numTargets; i++) {
			fprintf(fp, "%.02f\t",
				(sizes[t][i] - sizes[i][i]) / sizes[i][i]
			);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "ct's\n");

	fclose(fp);

	// Clean up
	for(uint32 t=0; t<numTargets; t++) {
		delete[] sizes[t];
		delete[] avgLen[t];
		delete[] sdLen[t];
		delete perClassDbs[t];
	}
	delete sizes;
	delete avgLen;
	delete sdLen;
	delete[] perClassDbs;
	delete[] codeLens;
	delete targets;
}
*/

void DissimilarityTH::InnerDBDistances() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName);

	string dir_path( Bass::GetWorkingDir() + mConfig->Read<string>("ctdir"));
	if (!FileUtils::DirectoryExists(dir_path))
		throw string("GenerateDBs :: Code table directory not found.");

	uint32 terugLeg = mConfig->Read<uint32>("putback",0);

	string ctPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/";
	string genTimestamp = TimeUtils::GetTimeStampString();
	string idbdPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/idbd-" +  genTimestamp + "/";
	if(!FileUtils::DirectoryExists(idbdPath)) {		// maak directory
		FileUtils::CreateDir(idbdPath);
	}


	string minSupCtFileName="", maxSupCtFileName="";

	uint32 numCTs = 0, minSupCtMinSupUint = 0, maxSupCtMinSupUint = 0; minSupCtMinSupUint--;
	directory_iterator itr(dir_path);
	while(itr.next()) {
		if(!FileUtils::IsDirectory(itr.fullpath()) && itr.filename().substr(itr.filename().length()-3,3) == ".ct") {
			uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
			if(ms < minSupCtMinSupUint)	{ minSupCtMinSupUint = ms; minSupCtFileName = itr.filename(); }
			if(ms > maxSupCtMinSupUint)	{ maxSupCtMinSupUint = ms; maxSupCtFileName = itr.filename(); }
		}
	}
	char tmp[10];
	_itoa(minSupCtMinSupUint,tmp,10);
	string minSupCtMinSup = tmp;
	_itoa(maxSupCtMinSupUint,tmp,10);
	string maxSupCtMinSup = tmp;

	CodeTable *ctO = CodeTable::Create("coverfull", db->GetDataType());
	ctO->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
	ctO->ReadFromDisk(ctPath + minSupCtFileName, true);
	ctO->AddOneToEachUsageCount();
	ctO->CalcCodeTableSize();

	uint32 numTimes = mConfig->Read<uint32>("numtimes");
	uint32 numFolds = 2;

	double ooSize = 0;
	double *s1oSizes = new double[numTimes];
	double *s2oSizes = new double[numTimes];
	double *os1Sizes = new double[numTimes];
	double *os2Sizes = new double[numTimes];
	double *s1s1Sizes = new double[numTimes];
	double *s2s2Sizes = new double[numTimes];
	double *s1s2Sizes = new double[numTimes];
	double *s2s1Sizes = new double[numTimes];

	uint32 numORows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	for(uint32 i=0; i<numORows; i++) {
		ooSize += ctO->CalcTransactionCodeLength(rows[i]);
	}

	RandomUtils::Init();
	uint32 pickNumRows = mConfig->Read<uint32>("picknumrows",(uint32) (ceil((double)numORows/2.0)));

	for(uint32 t=0; t<numTimes; t++) {
		// Deel-databases maken
		time_t seed = 0; time(&seed);
		db->RandomizeRowOrder((uint32)seed);

		uint32 *partitionSizes = new uint32[numFolds];
		ItemSet ***partitions;
		if(terugLeg == 0) {
			partitions = db->SplitForCrossValidation(numFolds, partitionSizes);
		} else {
			// terugLeg doen
			partitions = new ItemSet **[numFolds];
			for(uint32 i=0; i<numFolds; i++) {
				partitionSizes[i] = pickNumRows;
				partitions[i] = new ItemSet*[partitionSizes[i]];
				for(uint32 j=0; j<partitionSizes[i]; j++) {
					uint32 row = RandomUtils::UniformUint32(numORows);
					partitions[i][j] = rows[row]->Clone();
				}
			}
		} 
		string *sdbNames = new string[numFolds];
		//Database **sdbs = new Database*[numFolds];
		CodeTable **sdbcts = new CodeTable*[numFolds];

		for(uint32 i=0; i<numFolds; i++) {
			//  - Maak databases van itemset**'s
			uint32 size = partitionSizes[i];
			ItemSet ** collection = new ItemSet *[size];
			for(uint32 j=0; j<size; j++)
				collection[j] = partitions[i][j]->Clone();
			Database *sdb = new Database(collection, size, false);
			sdb->SetAlphabet(new alphabet(*db->GetAlphabet()));
			sdb->SetItemTranslator(new ItemTranslator(db->GetItemTranslator()));
			sdb->CountAlphabet();
			sdb->ComputeEssentials();

			//  - En save
			_itoa(i,tmp,10);
			string tmpi = tmp;
			Database::WriteDatabase(sdb, "sdb"+tmpi, idbdPath);
			sdbNames[i] = "sdb"+tmpi;
			delete sdb;

			//  - Krimp databases (?? tot 1/2 van minsup in compress.conf)
			string inTag = ctPath.substr(ctPath.find_last_of('-')+1,ctPath.length() - ctPath.find_last_of('-') - 2 );
			Config *config = new Config();
			config->ReadFile(ctPath + "compress-" + inTag + ".conf");
			config->Set("dataDir", idbdPath);
			string iscName = config->Read<string>("iscname");
			config->Set("iscName", sdbNames[i] + iscName.substr(iscName.find_first_of('-'), iscName.length()-iscName.find_first_of('-')));
			config->Set("iscZapIfMined", 1);
			config->WriteFile(idbdPath + "compress-" + sdbNames[i] + ".conf");

			Config *confbak = mConfig;
			mConfig = config;
			string datadirbak = Bass::GetWorkingDir();
			Bass::SetWorkingDir(idbdPath);

			string tag = sdbNames[i] + "-" + TimeUtils::GetTimeStampString();
			FicMain::Compress(mConfig, tag);

			Bass::SetWorkingDir(datadirbak);
			mConfig = confbak;

			// als alles goed is kunnen we nu de ct uitlezen
			string iscname = config->Read<string>("iscname");
			uint32 firstdash = (uint32) iscname.find_first_of('-');
			uint32 seconddash = (uint32) iscname.find_first_of('-',firstdash+1);
			string iscminsup = iscname.substr(seconddash+1,iscname.length()-seconddash-2);

			delete config;

			string crossCtFilename = "ct-" + tag + "-" + iscminsup + ".ct";
			CodeTable *ctX = CodeTable::Create("coverfull", db->GetDataType());
			ctX->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
			ctX->ReadFromDisk(idbdPath + tag + "/" + crossCtFilename, true);
			ctX->AddOneToEachUsageCount();
			ctX->CalcCodeTableSize();

			sdbcts[i] = ctX;
		}

		double os1Size = 0, os2Size = 0;
		double s1s1Size = 0, s1s2Size = 0, s1oSize = 0;
		double s2s2Size = 0, s2s1Size = 0, s2oSize = 0;

		uint32 numS1Rows = partitionSizes[0];
		uint32 numS2Rows = partitionSizes[1];

		// Compress en x-compress databases om afstand te bepalen
		for(uint32 i=0; i<numS1Rows; i++) {
			os1Size += ctO->CalcTransactionCodeLength(partitions[0][i]);
			s1s1Size += sdbcts[0]->CalcTransactionCodeLength(partitions[0][i]);
			s2s1Size += sdbcts[1]->CalcTransactionCodeLength(partitions[0][i]);
		}
		for(uint32 i=0; i<numS2Rows; i++) {
			os2Size += ctO->CalcTransactionCodeLength(partitions[1][i]);
			s1s2Size += sdbcts[0]->CalcTransactionCodeLength(partitions[1][i]);
			s2s2Size += sdbcts[1]->CalcTransactionCodeLength(partitions[1][i]);
		}
		ItemSet **rows = db->GetRows();
		for(uint32 i=0; i<numORows; i++) {
			s1oSize += sdbcts[0]->CalcTransactionCodeLength(rows[i]);
			s2oSize += sdbcts[1]->CalcTransactionCodeLength(rows[i]);
		}
		s1oSizes[t] = s1oSize;
		s2oSizes[t] = s2oSize;
		os1Sizes[t] = os1Size;
		os2Sizes[t] = os2Size;
		s1s1Sizes[t] = s1s1Size;
		s2s2Sizes[t] = s2s2Size;
		s1s2Sizes[t] = s1s2Size;
		s2s1Sizes[t] = s2s1Size;

		for(uint32 i=0; i<numFolds; i++) {
			delete sdbcts[i];
			for(uint32 j=0; j<partitionSizes[i]; j++) {
				delete partitions[i][j];
			}
			delete[] partitions[i];
		}
		delete[] partitionSizes;
		delete[] partitions;
		delete[] sdbcts;
		delete[] sdbNames;
	}

	double os1Size = 0, os2Size = 0;
	double s1s1Size = 0, s1s2Size = 0, s1oSize = 0;
	double s2s2Size = 0, s2s1Size = 0, s2oSize = 0;
	for(uint32 t=0; t<numTimes; t++) {
		os1Size += os1Sizes[t];
		os2Size += os2Sizes[t];
		s1oSize += s1oSizes[t];
		s2oSize += s2oSizes[t];
		s1s1Size += s1s1Sizes[t];
		s2s2Size += s2s2Sizes[t];
		s1s2Size += s1s2Sizes[t];
		s2s1Size += s2s1Sizes[t];
	}
	os1Size /= numTimes;
	os2Size /= numTimes;
	s1oSize /= numTimes;
	s2oSize /= numTimes;
	s1s1Size /= numTimes;
	s2s2Size /= numTimes;
	s1s2Size /= numTimes;
	s2s1Size /= numTimes;

	// Bereken afstand en schrijf naar synopsis
	string synposisFullpath = idbdPath + "synopsis - innerdatabasedistances.txt";
	FILE *f = fopen(synposisFullpath.c_str(), "w");

	//goSize - ooSize / ooSize == (encSizes[i][j] - encSizes[i][i]) / encSizes[i][i]
	//ogSize - ggSize / ggSize == (encSizes[j][i] - encSizes[j][j]) / encSizes[j][j]
	double os1MinDist = min((s1oSize - ooSize) / ooSize, (os1Size - s1s1Size) / s1s1Size);
	double os1MaxDist = max((s1oSize - ooSize) / ooSize, (os1Size - s1s1Size) / s1s1Size);

	double os2MinDist = min((s2oSize - ooSize) / ooSize, (os2Size - s2s2Size) / s2s2Size);
	double os2MaxDist = max((s2oSize - ooSize) / ooSize, (os2Size - s2s2Size) / s2s2Size);

	double s1s2MinDist = min((s1s2Size - s1s1Size) / s1s1Size, (s1s2Size - s2s2Size) / s2s2Size);
	double s1s2MaxDist = max((s1s2Size - s1s1Size) / s1s1Size, (s1s2Size - s2s2Size) / s2s2Size);

	fprintf(f,"%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n",s1s2MinDist,s1s2MaxDist,os1MinDist,os1MaxDist,os2MinDist,os2MaxDist);
	fprintf(f,"s1s2dist:s2s1,s1s1,s1s2,minD,maxD\n%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n",s2s1Size,s1s1Size,s1s2Size,s1s2MinDist,s1s2MaxDist);
	fprintf(f,"os1dist:s1o,oo,os1,minD,maxD\n%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n",s1oSize,ooSize,os1Size,os1MinDist,os1MaxDist);
	fprintf(f,"os2dist:s2o,oo,os2,minD,maxD\n%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n",s2oSize,ooSize,os2Size,os2MinDist,os2MaxDist);
	fclose(f);


	delete[] s1oSizes; delete[] s2oSizes; delete[] s1s1Sizes; delete[] s2s2Sizes; delete[] s1s2Sizes; delete[] s2s1Sizes;
	delete[] os1Sizes; delete[] os2Sizes;

	delete ctO;
	delete db;
}
void DissimilarityTH::InnerDBSampleToFullDBDistances() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName);

	string dir_path( Bass::GetWorkingDir() + mConfig->Read<string>("ctdir"));
	if (!FileUtils::DirectoryExists(dir_path))
		throw string("GenerateDBs :: Code table directory not found.");

	uint32 terugLeg = mConfig->Read<uint32>("putback",0);

	string ctPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/";
	string genTimestamp = TimeUtils::GetTimeStampString();
	string idbdPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "nidbdf-" +  genTimestamp + "/";
	if(!FileUtils::DirectoryExists(idbdPath)) {		// maak directory
		FileUtils::CreateDir(idbdPath);
	}


	string minSupCtFileName="", maxSupCtFileName="";

	uint32 numCTs = 0, minSupCtMinSupUint = 0, maxSupCtMinSupUint = 0; minSupCtMinSupUint--;
	directory_iterator itr(dir_path);
	while(itr.next()) {
		if(!FileUtils::IsDirectory(itr.fullpath()) && itr.filename().substr(itr.filename().length()-3,3) == ".ct") {
			uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
			if(ms < minSupCtMinSupUint)	{ minSupCtMinSupUint = ms; minSupCtFileName = itr.filename(); }
			if(ms > maxSupCtMinSupUint)	{ maxSupCtMinSupUint = ms; maxSupCtFileName = itr.filename(); }
		}
	}
	char tmp[10];
	_itoa(minSupCtMinSupUint,tmp,10);
	string minSupCtMinSup = tmp;
	_itoa(maxSupCtMinSupUint,tmp,10);
	string maxSupCtMinSup = tmp;

	CodeTable *ctO = CodeTable::Create("coverfull", db->GetDataType());
	ctO->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
	ctO->ReadFromDisk(ctPath + minSupCtFileName, true);
	ctO->AddOneToEachUsageCount();
	ctO->CalcCodeTableSize();

	uint32 numTimes = mConfig->Read<uint32>("numtimes");
	uint32 numFolds = 1;

	double ooSize = 0;
	double *goSizes = new double[numTimes];
	double *ogSizes = new double[numTimes];
	double *ggSizes = new double[numTimes];

	uint32 numORows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	for(uint32 i=0; i<numORows; i++) {
		ooSize += ctO->CalcTransactionCodeLength(rows[i]);
	}

	RandomUtils::Init();
	uint32 pickNumRows = mConfig->Read<uint32>("picknumrows",(uint32) (ceil((double)numORows/2.0)));

	for(uint32 t=0; t<numTimes; t++) {
		// Deel-databases maken
		time_t seed = 0; time(&seed);
		db->RandomizeRowOrder((uint32)seed);

		uint32 *partitionSizes = new uint32[numFolds];
		ItemSet ***partitions;
		if(terugLeg == 0) {
			partitions = db->SplitForCrossValidation(numFolds, partitionSizes);
		} else {
			// terugLeg doen
			partitions = new ItemSet **[numFolds];
			for(uint32 i=0; i<numFolds; i++) {
				partitionSizes[i] = pickNumRows;
				partitions[i] = new ItemSet*[partitionSizes[i]];
				for(uint32 j=0; j<partitionSizes[i]; j++) {
					uint32 row = RandomUtils::UniformUint32(numORows);
					partitions[i][j] = rows[row]->Clone();
				}
			}
		} 
		string *sdbNames = new string[numFolds];
		//Database **sdbs = new Database*[numFolds];
		CodeTable **sdbcts = new CodeTable*[numFolds];

		for(uint32 i=0; i<numFolds; i++) {
			//  - Maak databases van itemset**'s
			uint32 size = partitionSizes[i];
			ItemSet ** collection = new ItemSet *[size];
			for(uint32 j=0; j<size; j++)
				collection[j] = partitions[i][j]->Clone();
			Database *sdb = new Database(collection, size, false);
			sdb->SetAlphabet(new alphabet(*db->GetAlphabet()));
			sdb->SetItemTranslator(new ItemTranslator(db->GetItemTranslator()));
			sdb->CountAlphabet();
			sdb->ComputeEssentials();

			//  - En save
			_itoa(i,tmp,10);
			string tmpi = tmp;
			Database::WriteDatabase(sdb, "sdb"+tmpi, idbdPath);
			sdbNames[i] = "sdb"+tmpi;
			delete sdb;

			//  - Krimp databases (?? tot 1/2 van minsup in compress.conf)
			string inTag = ctPath.substr(ctPath.find_last_of('-')+1,ctPath.length() - ctPath.find_last_of('-') - 2 );
			Config *config = new Config();
			config->ReadFile(ctPath + "compress-" + inTag + ".conf");
			config->Set("dataDir", idbdPath);
			string iscName = config->Read<string>("iscname");
			config->Set("iscName", sdbNames[i] + iscName.substr(iscName.find_first_of('-'), iscName.length()-iscName.find_first_of('-')));
			config->Set("iscZapIfMined", 1);
			config->WriteFile(idbdPath + "compress-" + sdbNames[i] + ".conf");

			Config *confbak = mConfig;
			mConfig = config;
			string datadirbak = Bass::GetWorkingDir();
			Bass::SetWorkingDir(idbdPath);

			string tag = sdbNames[i] + "-" + TimeUtils::GetTimeStampString();
			FicMain::Compress(mConfig,tag);

			Bass::SetWorkingDir(datadirbak);
			mConfig = confbak;

			// als alles goed is kunnen we nu de ct uitlezen
			string iscname = config->Read<string>("iscname");
			size_t firstdash = iscname.find_first_of('-');
			size_t seconddash = iscname.find_first_of('-',firstdash+1);
			string iscminsup = iscname.substr(seconddash+1,iscname.length()-seconddash-2);

			delete config;

			string crossCtFilename = "ct-" + tag + "-" + iscminsup + ".ct";
			CodeTable *ctX = CodeTable::Create("coverfull", db->GetDataType());
			ctX->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
			ctX->ReadFromDisk(idbdPath + tag + "/" + crossCtFilename, true);
			ctX->AddOneToEachUsageCount();
			ctX->CalcCodeTableSize();

			sdbcts[i] = ctX;
		}

		double ogSize = 0;
		double ggSize = 0, goSize = 0;

		for(uint32 r=0; r<partitionSizes[0]; r++) {
			ogSize += ctO->CalcTransactionCodeLength(partitions[0][r]);
			ggSize += sdbcts[0]->CalcTransactionCodeLength(partitions[0][r]);
		}

		ItemSet **rows = db->GetRows();
		for(uint32 i=0; i<numORows; i++) {
			goSize += sdbcts[0]->CalcTransactionCodeLength(rows[i]);
		}
		goSizes[t] = goSize;
		ogSizes[t] = ogSize;
		ggSizes[t] = ggSize;

		for(uint32 i=0; i<numFolds; i++) {
			delete sdbcts[i];
			for(uint32 j=0; j<partitionSizes[i]; j++) {
				delete partitions[i][j];
			}
			delete[] partitions[i];
		}
		delete[] partitionSizes;
		delete[] partitions;
		delete[] sdbcts;
		delete[] sdbNames;
	}

	string synposisFullpath = idbdPath + "synopsis - innerdatabasedistances.txt";
	FILE *f = fopen(synposisFullpath.c_str(), "w");

	double ggSize = 0, ogSize = 0, goSize = 0;
	for(uint32 t=0; t<numTimes; t++) {
		double ognd = min((goSizes[t] - ooSize) / ooSize, (ogSizes[t] - ggSizes[t]) / ggSizes[t]);
		double ogxd = max((goSizes[t] - ooSize) / ooSize, (ogSizes[t] - ggSizes[t]) / ggSizes[t]);
		fprintf(f,"%.4f\t%.4f\n",ognd,ogxd);

		ogSize += ogSizes[t];
		goSize += goSizes[t];
		ggSize += ggSizes[t];
	}
	ogSize /= numTimes;
	goSize /= numTimes;
	ggSize /= numTimes;

	// Bereken afstand en schrijf naar synopsis

	//goSize - ooSize / ooSize == (encSizes[i][j] - encSizes[i][i]) / encSizes[i][i]
	//ogSize - ggSize / ggSize == (encSizes[j][i] - encSizes[j][j]) / encSizes[j][j]
	double ogMinDist = min((goSize - ooSize) / ooSize, (ogSize - ggSize) / ggSize);
	double ogMaxDist = max((goSize - ooSize) / ooSize, (ogSize - ggSize) / ggSize);

	fprintf(f,"avgs:\n");
	fprintf(f,"%.4f\t%.4f\n",ogMinDist,ogMaxDist);
	fclose(f);

	delete[] goSizes; 
	delete[] ogSizes; 
	delete[] ggSizes;

	delete ctO;
	delete db;
}


double DissimilarityTH::BerekenAfstandTussen(Database *db1, Database *db2, CodeTable *ct1, CodeTable *ct2) {

	// Stap 1 - Prepare the Codetables
	// gefopt! dat moet je zelf maar van te voren doen!
	//ct1->AddOneToEachCount();
	//ct2->AddOneToEachCount();

	// Stap 2 - DB1
	double db1ct1 = 0, db1ct2 = 0;
	ItemSet **db1rows = db1->GetRows(); 
	uint32 db1numRows = db1->GetNumRows();
	for(uint32 i=0; i<db1numRows; i++) {
		db1ct1 += ct1->CalcTransactionCodeLength(db1rows[i]) * db1rows[i]->GetSupport();
		db1ct2 += ct2->CalcTransactionCodeLength(db1rows[i]) * db1rows[i]->GetSupport();
	}

	// Stap 3 - DB2
	double db2ct1 = 0, db2ct2 = 0;	
	ItemSet **db2rows = db2->GetRows(); 
	uint32 db2numRows = db2->GetNumRows();
	for(uint32 i=0; i<db2numRows; i++) {
		db2ct1 += ct1->CalcTransactionCodeLength(db2rows[i]) * db2rows[i]->GetSupport();
		db2ct2 += ct2->CalcTransactionCodeLength(db2rows[i]) * db2rows[i]->GetSupport();
	}

	// Stap 4 - Uitrekenen die boel
	double dissimilarity = max((db1ct2-db1ct1)/db1ct1, (db2ct1-db2ct2)/db2ct2);

	return dissimilarity;
}
