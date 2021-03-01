#ifdef ENABLE_DATAGEN

#include "../global.h"

// -- qtils
#include <Config.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>

#include <db/Database.h>
#include <db/DbAnalyser.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>

#include "../algo/CodeTable.h"
#include "../blocks/dgen/DGen.h"
#include "../blocks/dgen/DGSet.h"

// leluk
#include "../FicMain.h"

#include "TaskHandler.h"
#include "DataGenTH.h"

DataGenTH::DataGenTH(Config *conf) : TaskHandler(conf) {
}

DataGenTH::~DataGenTH() {
	// not my Config*
}

void DataGenTH::HandleTask() {
	string command = mConfig->Read<string>("command");
			if(command.compare("checkgdbfor") == 0)			CheckGenDbForOrigRows();
	else	if(command.compare("checkgdbpri") == 0)			CheckGenDbForPrivacy();
	else	if(command.compare("generatedbs") == 0)			GenerateDBs();
	else	if(command.compare("generatedbsffct") == 0)		GenerateDBsFromFirstCT();
	else	if(command.compare("generatedbscb") == 0)		GenerateDBsClassBased();
	else	throw string("DataGenTH :: Unable to handle task `" + command + "`");

}

void DataGenTH::GenerateDBs() {
	// Load db
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);

	// Get list with code tables
	uint32 numCTs;
	CTFileInfo **ctFiles = FicMain::LoadCodeTableFileList(mConfig, db, numCTs);

	string ctPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/";
	string genType = mConfig->Read<string>("gentype");

	string genPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/" + genType + "-" + TimeUtils::GetTimeStampString() + "/";
	if(!FileUtils::DirectoryExists(genPath)) {		// maak directory
		FileUtils::CreateDir(genPath);
	}

	CodeTable *ctO = CodeTable::Create("coverfull", db->GetDataType());
	ctO->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
	ctO->ReadFromDisk(ctPath + ctFiles[0]->filename, true);
	ctO->AddOneToEachUsageCount();
	ctO->CalcCodeTableSize();

	double sizeOO=0;
	ItemSet **issO = db->GetRows();
	uint32 numRows = db->GetNumRows();
	for(uint32 r=0; r<numRows; r++) {
		sizeOO += ctO->CalcTransactionCodeLength(issO[r]);
	}

	uint8 outputLevelBak = Bass::GetOutputLevel();
	Bass::SetOutputLevel(Bass::GetOutputLevel() > 0 ? Bass::GetOutputLevel()-1 : 0);

	Database *generateDb;
	string **dbGenNames = new string*[numCTs];
	uint32 genNumRows = mConfig->Read<uint32>("gennumrows", db->GetNumRows());

	// hoeveel databases moet ik genereren per ct?
	uint32 numGen = mConfig->Read<uint32>("numgen");

	double **ogSizes = new double*[numCTs];
	double **sgSizes = new double*[numCTs];
	double **ggSizes = new double*[numCTs];
	double **goSizes = new double*[numCTs];
	uint32 **cntSums = new uint32*[numGen];

	bool xCompress = mConfig->Read<uint32>("xcompress",0)==1;

	char tmp[10];
	for(uint32 i=0; i<numCTs; i++) {
		printf("\n\r * Generating Database %d:\tbuilding model   (1/6)", i+1);
		DGen *dg = DGen::Create(genType);
		dg->SetInputDatabase(db);
		dg->SetColumnDefinition(mConfig->Read<string>("coldef"));
		dg->BuildModel(db, mConfig, ctPath + ctFiles[i]->filename);

		ogSizes[i] = new double[numGen];
		sgSizes[i] = new double[numGen];
		ggSizes[i] = new double[numGen];
		goSizes[i] = new double[numGen];
		cntSums[i] = new uint32[numGen];

		dbGenNames[i] = new string[numGen];

		mConfig->WriteFile(genPath + "gendbs.conf");

		for(uint32 g=0; g<numGen; g++) {

			printf("\n\r * Generating Database %d:\tgenerating       (2/6)", i+1);
			generateDb = dg->GenerateDatabase(genNumRows);

			try { 
				_itoa(ctFiles[i]->minsup, tmp, 10);
				string dbGenName = dbName + "_ct"; dbGenName.append(tmp); dbGenName.append("_g"); _itoa(g, tmp, 10); dbGenName.append(tmp);
				printf("\r * Generating Database %d:\twriting          (3/6)", i+1);
				Database::WriteDatabase(generateDb, dbGenName, genPath);
				dbGenNames[i][g] = dbGenName;
				delete generateDb;

				string analysisFile = genPath + dbGenName + ".analysis.txt";
				printf("\r * Generating Database %d:\treading          (4/6)", i+1);
				Database *dbG = Database::ReadDatabase(dbGenName, genPath, db->GetDataType());
				DbAnalyser *dba = new DbAnalyser();
				printf("\r * Generating Database %d:\tanalysing        (5/6)", i+1);
				dba->Analyse(dbG, analysisFile);
				delete dba;

				// analyse ct-elem usage
				uint32 numce = dg->GetNumCodingSets();
				DGSet** ce = dg->GetCodingSets();
				uint32 cntsum = 0;
				for(uint32 e=0; e<numce; e++) {
					cntsum += ce[e]->GetItemSet()->GetUsageCount();
					ce[e]->GetItemSet()->ResetUsage();

				}
				cntSums[i][g] = cntsum;

				if(xCompress) {
					printf("\r * Generating Database %d:\tcompressing      (6/6)\n", i+1);

					string inTag = ctPath.substr(ctPath.find_last_of('-')+1,ctPath.length() - ctPath.find_last_of('-') - 2 );
					Config *config = new Config();
					config->ReadFile(ctPath + "compress-" + inTag + ".conf");
					config->Set("dataDir", genPath);
					string iscName = config->Read<string>("iscname");
					config->Set("iscName", dbGenNames[i][g] + iscName.substr(iscName.find_first_of('-'), iscName.length()-iscName.find_first_of('-')));
					config->Set("iscZapIfMined", 1);
					config->WriteFile(genPath + "compress-"+dbGenNames[i][g]+".conf");

					Config *confbak = mConfig;
					mConfig = config;
					string datadirbak = Bass::GetWorkingDir();
					Bass::SetWorkingDir(genPath);

					string tag = dbGenNames[i][g] + "-" + TimeUtils::GetTimeStampString();
					FicMain::Compress(mConfig, tag);

					Bass::SetWorkingDir(datadirbak);
					mConfig = confbak;
					delete config;

					// als alles goed is kunnen we nu de ct uitlezen
					char tmp[10];
					_itoa(ctFiles[0]->minsup, tmp, 10);
					string crossCtFilename = "ct-" + tag + "-" + tmp + ".ct";
					CodeTable *ctX = CodeTable::Create("coverfull", db->GetDataType());
					ctX->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
					ctX->ReadFromDisk(genPath + tag + "/" + crossCtFilename, true);
					ctX->AddOneToEachUsageCount();
					ctX->CalcCodeTableSize();

					double sizeGO=0, sizeGG=0;
					ItemSet **issG = dbG->GetRows();
					for(uint32 r=0; r<numRows; r++) {
						sizeGO += ctX->CalcTransactionCodeLength(issO[r]);
					}
					for(uint32 r=0; r<genNumRows; r++) {
						sizeGG += ctX->CalcTransactionCodeLength(issG[r]);
					}
					goSizes[i][g] = sizeGO;
					ggSizes[i][g] = sizeGG;

					delete ctX;
				}

				CodeTable *ctS = CodeTable::Create("coverfull", db->GetDataType());
				ctS->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
				ctS->ReadFromDisk(ctPath + ctFiles[i]->filename, true);
				ctS->AddOneToEachUsageCount();
				ctS->CalcCodeTableSize();

				printf("\r * Generating Database %d:\tencoding         (6/6)", i+1);

				double sizeOG=0, sizeSG=0;
				ItemSet **issG = dbG->GetRows();
				for(uint32 r=0; r<genNumRows; r++) {
					sizeOG += ctO->CalcTransactionCodeLength(issG[r]);
					sizeSG += ctS->CalcTransactionCodeLength(issG[r]);
				}
				ogSizes[i][g] = sizeOG;
				sgSizes[i][g] = sizeSG;

				delete dbG;
				delete ctS;

			} catch(char *la) {
				delete dg;
				delete db;
				throw la;
			}
		}
		delete dg;
	}

	ECHO(2, printf(" * Writing Synopsis :\tin progress..."));

	string synposisFullpath = genPath + "synopsis - " + mConfig->Read<string>("ctdir").substr(0,mConfig->Read<string>("ctdir").find_last_of('-')) + ".txt";
	FILE *synopsis = fopen(synposisFullpath.c_str(), "w");
	double ogL=0, sgL = 0;

	fprintf(synopsis, "sup\t\tCTo_%d(CTo_x->DBg)\tCTo_x(CTo_x->DBg)\tavgCTE/row\n", ctFiles[0]->minsup);
	for(uint32 i=numCTs; i>0; i--) {
		double og = 0, sg = 0, ce = 0;
		for(uint32 g=0; g<numGen; g++) {
			og += ogSizes[i-1][g];
			sg += sgSizes[i-1][g];
			ce += cntSums[i-1][g];
		}
		//fprintf(synopsis, "%5d\t%13.2f\t%13.2f\n", ctMinSupAr[i-1], ogSizes[i-1], sgSizes[i-1]);
		og /= numGen;
		sg /= numGen;
		ce /= numGen;
		ce /= genNumRows;
		if(i==1) {
			sgL = sg;
		}
		fprintf(synopsis, "%5d\t%13.2f\t%13.2f\t%13.2f\n", ctFiles[i-1]->minsup, og, sg,ce);
	}
	fprintf(synopsis, "\n");
	fprintf(synopsis, "CTo_%d(DBo) = %.2f\n", ctFiles[0]->minsup, sizeOO);
	ogL = sizeOO;

	/* conf voor compress van gegenereerde db at minsup */

	if(xCompress) {
		fprintf(synopsis, "\ndistances : (go-oo/oo), (og-gg/gg), (min), (max)\n");

		for(uint32 i=numCTs; i>0; i--) {
			double gg = 0, go = 0, og = 0;
			for(uint32 g=0; g<numGen; g++) {
				og += ogSizes[i-1][g];
				gg += ggSizes[i-1][g];
				go += goSizes[i-1][g];
			}
			gg /= numGen;
			go /= numGen;
			og /= numGen;
			fprintf(synopsis, "\navgs (ct %d):\n", i);

			double distMax = max((go - sizeOO) / sizeOO, (og - gg) / gg);
			double distMin = min((go - sizeOO) / sizeOO, (og - gg) / gg);

			double distA = (go - sizeOO) / sizeOO;
			double distB = (og - gg) / gg;

			fprintf(synopsis, "\n");
			fprintf(synopsis, "%.2f\t%.2f\t\t%.4f\t%.4f\n", sgL, ogL, distMin, distMax);
			fprintf(synopsis, "distances : %.4f , %.4f , %.4f , %.4f\n", distA, distB, distMin, distMax);
		}

		for(uint32 i=numCTs; i>0; i--) {
			double gg = 0, go = 0, og = 0;
			for(uint32 g=0; g<numGen; g++) {
				double distMax = max((goSizes[i-1][g] - sizeOO) / sizeOO, (ogSizes[i-1][g] - ggSizes[i-1][g]) / ggSizes[i-1][g]);
				double distMin = min((goSizes[i-1][g] - sizeOO) / sizeOO, (ogSizes[i-1][g] - ggSizes[i-1][g]) / ggSizes[i-1][g]);
				double distA = (goSizes[i-1][g] - sizeOO) / sizeOO;
				double distB = (ogSizes[i-1][g] - ggSizes[i-1][g]) / ggSizes[i-1][g];

				fprintf(synopsis, "\ndist ct %d, g %d\n", i, g);
				fprintf(synopsis, "distances : %.4f , %.4f , %.4f , %.4f\n", distA, distB, distMin, distMax);
			}
		}
	}

	fclose(synopsis);

	for(uint32 i=0; i<numCTs; i++) {
		delete[] ogSizes[i];
		delete[] sgSizes[i];
		delete[] goSizes[i];
		delete[] ggSizes[i];
		delete[] dbGenNames[i];
		delete ctFiles[i];
	}
	delete[] ogSizes;
	delete[] sgSizes;
	delete[] goSizes;
	delete[] ggSizes;
	delete[] dbGenNames;
	delete[] ctFiles;

	delete[] cntSums;
	delete ctO;
	delete db;
	Bass::SetOutputLevel(outputLevelBak);
}
void DataGenTH::GenerateDBsFromFirstCT() {
	// Load db
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName);

	// for each ct file in directory
	string dir_path = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir");
	if (!FileUtils::DirectoryExists(dir_path))
		throw string("GenerateDBs :: Code table directory not found.");

	// check 'minsups'-parameter
	string minSupCtFileName="", maxSupCtFileName="";

	uint32 numCTs = 0, minSupCtMinSupUint = 0, maxSupCtMinSupUint = 0; minSupCtMinSupUint--;
	directory_iterator itr(dir_path);
	while(itr.next(), "*.ct") {
		uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
		if(ms < minSupCtMinSupUint)	{ minSupCtMinSupUint = ms; minSupCtFileName = itr.filename(); }
		if(ms > maxSupCtMinSupUint)	{ maxSupCtMinSupUint = ms; maxSupCtFileName = itr.filename(); }
	}
	char tmp[10];
	_itoa(minSupCtMinSupUint,tmp,10);
	string minSupCtMinSup = tmp;
	_itoa(maxSupCtMinSupUint,tmp,10);
	string maxSupCtMinSup = tmp;

	string ctPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/";
	string genType = mConfig->Read<string>("gentype");
	//DGen *tmpdg = DGen::Create(genType); 
	//delete tmpdg;

	string genPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/" + genType + "-" + TimeUtils::GetTimeStampString() + "/";
	if(!FileUtils::DirectoryExists(genPath)) {		// maak directory
		FileUtils::CreateDir(genPath);
	}

	CodeTable *ctO = CodeTable::Create("coverfull", db->GetDataType());
	ctO->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
	ctO->ReadFromDisk(ctPath + minSupCtFileName, true);
	ctO->AddOneToEachUsageCount();
	ctO->CalcCodeTableSize();

	double sizeOO=0;
	ItemSet **issO = db->GetRows();
	uint32 numRows = db->GetNumRows();
	for(uint32 r=0; r<numRows; r++) {
		sizeOO += ctO->CalcTransactionCodeLength(issO[r]);
	}

	uint8 outputLevelBak = Bass::GetOutputLevel();
	Bass::SetOutputLevel(Bass::GetOutputLevel() > 0 ? Bass::GetOutputLevel()-1 : 0);

	Database *generateDb;
	uint32 genNumRows = (mConfig->KeyExists("gennumrows")) ? mConfig->Read<uint32>("gennumrows") : db->GetNumRows();

	// hoeveel databases moet ik genereren per ct?
	uint32 numGen = mConfig->Read<uint32>("numgen");

	bool xCompress = mConfig->KeyExists("xcompress") && mConfig->Read<uint32>("xcompress")==1;

	DGen *dg = DGen::Create(genType);
	dg->SetInputDatabase(db);
	dg->SetColumnDefinition(mConfig->Read<string>("coldef"));
	dg->BuildModel(db, mConfig, ctPath + maxSupCtFileName);

	double *ogSizes = new double[numGen];
	double *sgSizes = new double[numGen];
	double *ggSizes = new double[numGen];
	double *goSizes = new double[numGen];
	string *dbGenNames = new string[numGen];

	mConfig->WriteFile(genPath + "gendbs.conf");

	for(uint32 g=0; g<numGen; g++) {

		generateDb = dg->GenerateDatabase(genNumRows);

		try { 
			string dbGenName = dbName + "_ct" + maxSupCtMinSup + "_g"; _itoa(g, tmp, 10); dbGenName.append(tmp);
			Database::WriteDatabase(generateDb, dbGenName, genPath);
			dbGenNames[g] = dbGenName;
			delete generateDb;

			string analysisFile = genPath + dbGenName + ".analysis.txt";
			Database *dbG = Database::ReadDatabase(dbGenName, genPath, db->GetDataType());
			DbAnalyser *dba = new DbAnalyser();
			dba->Analyse(dbG, analysisFile);
			delete dba;

			if(xCompress) {
				string inTag = ctPath.substr(ctPath.find_last_of('-')+1,ctPath.length() - ctPath.find_last_of('-') - 2 );
				Config *config = new Config();
				config->ReadFile(ctPath + "compress-" + inTag + ".conf");
				config->Set("dataDir", genPath);
				string iscName = config->Read<string>("iscname");
				config->Set("iscName", dbGenNames[g] + iscName.substr(iscName.find_first_of('-'), iscName.length()-iscName.find_first_of('-')));
				config->Set("iscZapIfMined", 1);
				config->WriteFile(genPath + "compress-"+dbGenNames[g]+".conf");

				Config *confbak = mConfig;
				mConfig = config;
				string datadirbak = Bass::GetWorkingDir();
				Bass::SetWorkingDir(genPath);

				string tag = dbGenNames[g] + "-" + TimeUtils::GetTimeStampString();
				FicMain::Compress(mConfig, tag);

				Bass::SetWorkingDir(datadirbak);
				mConfig = confbak;

				// als alles goed is kunnen we nu de ct uitlezen
				string iscname = config->Read<string>("iscname");
				uint32 firstdash = iscname.find_first_of('-');
				uint32 seconddash = iscname.find_first_of('-',firstdash+1);
				string iscminsup = iscname.substr(seconddash+1,iscname.length()-seconddash-2);

				delete config;

				string crossCtFilename = "ct-" + tag + "-" + iscminsup + ".ct";
				CodeTable *ctX = CodeTable::Create("coverfull", db->GetDataType());
				ctX->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
				ctX->ReadFromDisk(genPath + tag + "/" + crossCtFilename, true);
				ctX->AddOneToEachUsageCount();
				ctX->CalcCodeTableSize();

				double sizeGO=0, sizeGG=0;
				ItemSet **issG = dbG->GetRows();
				for(uint32 r=0; r<numRows; r++) {
					sizeGO += ctX->CalcTransactionCodeLength(issO[r]);
					sizeGG += ctX->CalcTransactionCodeLength(issG[r]);
				}
				goSizes[g] = sizeGO;
				ggSizes[g] = sizeGG;

				delete ctX;
			}

			CodeTable *ctS = CodeTable::Create("coverfull", db->GetDataType());
			ctS->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
			ctS->ReadFromDisk(ctPath + maxSupCtFileName, true);
			ctS->AddOneToEachUsageCount();
			ctS->CalcCodeTableSize();

			double sizeOG=0, sizeSG=0;
			ItemSet **issG = dbG->GetRows();
			for(uint32 r=0; r<numRows; r++) {
				sizeOG += ctO->CalcTransactionCodeLength(issG[r]);
				sizeSG += ctS->CalcTransactionCodeLength(issG[r]);
			}
			ogSizes[g] = sizeOG;
			sgSizes[g] = sizeSG;

			delete dbG;
			delete ctS;

		} catch(char *la) {
			delete dg;
			delete db;
			throw la;
		}
	}
	delete dg;

	ECHO(2, printf(" * Writing Synopsis :\tin progress..."));

	string synposisFullpath = genPath + "synopsis - " + mConfig->Read<string>("ctdir").substr(0,mConfig->Read<string>("ctdir").find_last_of('-')) + ".txt";
	FILE *synopsis = fopen(synposisFullpath.c_str(), "w");
	double ogL=0, sgL = 0;

	fprintf(synopsis, "sup\t\tCTo_%d(CTo_x->DBg)\tCTo_x(CTo_x->DBg)\n", minSupCtMinSupUint);
	//	for(uint32 i=numCTs; i>0; i--) {
	double og = 0, sg = 0;
	for(uint32 g=0; g<numGen; g++) {
		og += ogSizes[g];
		sg += sgSizes[g];
	}
	//fprintf(synopsis, "%5d\t%13.2f\t%13.2f\n", ctMinSupAr[i-1], ogSizes[i-1], sgSizes[i-1]);
	og /= numGen;
	sg /= numGen;
	sgL = sg;

	fprintf(synopsis, "%5d\t%13.2f\t%13.2f\n", maxSupCtMinSupUint, og, sg);
	//	}
	fprintf(synopsis, "\n");
	fprintf(synopsis, "CTo_%d(DBo) = %.2f\n", maxSupCtMinSupUint, sizeOO);
	ogL = sizeOO;

	/* conf voor compress van gegenereerde db at minsup */


	if(xCompress) {


		fprintf(synopsis, "\ndistances : (go-oo/oo), (og-gg/gg), (min), (max)\n");

		//		for(uint32 i=numCTs; i>0; i--) {
		double gg = 0, go = 0, og = 0;
		for(uint32 g=0; g<numGen; g++) {
			og += ogSizes[g];
			gg += ggSizes[g];
			go += goSizes[g];
		}
		gg /= numGen;
		go /= numGen;
		og /= numGen;
		//fprintf(synopsis, "\navgs (ct %d):\n", i);

		double distMax = max((go - sizeOO) / sizeOO, (og - gg) / gg);
		double distMin = min((go - sizeOO) / sizeOO, (og - gg) / gg);

		double distA = (go - sizeOO) / sizeOO;
		double distB = (og - gg) / gg;

		fprintf(synopsis, "\n");
		fprintf(synopsis, "%.2f\t%.2f\t\t%.4f\t%.4f\n", sgL, ogL, distMin, distMax);
		fprintf(synopsis, "distances : %.4f , %.4f , %.4f , %.4f\n", distA, distB, distMin, distMax);
		//		}

		//		for(uint32 i=numCTs; i>0; i--) {
		gg = 0, go = 0, og = 0;
		for(uint32 g=0; g<numGen; g++) {
			double distMax = max((goSizes[g] - sizeOO) / sizeOO, (ogSizes[g] - ggSizes[g]) / ggSizes[g]);
			double distMin = min((goSizes[g] - sizeOO) / sizeOO, (ogSizes[g] - ggSizes[g]) / ggSizes[g]);
			double distA = (goSizes[g] - sizeOO) / sizeOO;
			double distB = (ogSizes[g] - ggSizes[g]) / ggSizes[g];

			//				fprintf(synopsis, "\ndist ct %d, g %d\n", i, g);
			fprintf(synopsis, "distances : %.4f , %.4f , %.4f , %.4f\n", distA, distB, distMin, distMax);
		}
		//		}
	}

	fclose(synopsis);

	delete[] ogSizes;
	delete[] sgSizes;
	delete[] ggSizes;
	delete[] goSizes;
	delete ctO;

	delete[] dbGenNames;
	delete db;
	Bass::SetOutputLevel(outputLevelBak);
}
void DataGenTH::GenerateDBsClassBased() {
	// Load db
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName);

	// for each ct file in directory
	string dir_path( Bass::GetWorkingDir() + mConfig->Read<string>("ctdir"));
	if (!FileUtils::DirectoryExists(dir_path))
		throw string("GenerateDBs :: Code table directory not found.");

	// Splits db -> db_c
	// - Build target array
	char tmp[10];
	uint32 numClasses = 0;
	string classs;

	//uint16 *classes = FicMain::ReadTargets(mConfig, numClasses);
	uint32 *classes = StringUtils::TokenizeUint32(mConfig->Read<string>("targets"), numClasses);
	

	string ctPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/";			// om config uit te halen
	string genType = mConfig->Read<string>("gentype");
	string dbcPath = ctPath + "dbc-" + genType + "-" + TimeUtils::GetTimeStampString() + "/";
	if(!FileUtils::DirectoryExists(dbcPath))		// maak directory
		FileUtils::CreateDir(dbcPath);



	// - Prepare for splitting 
	Database ** classDatabases = new Database*[numClasses];
	string *classIdString = new string[numClasses];
	for(uint32 c=0; c<numClasses; c++)
		classDatabases[c] = NULL;

	db->SplitForClassification(numClasses, classes, classDatabases);
	for(uint32 c=0; c<numClasses; c++) {
		_itoa(classes[c], tmp, 10);
		classIdString[c] = tmp;
		Database *dbClone = new Database(classDatabases[c]);
		Database::WriteDatabase(dbClone, dbName + "_class" + classIdString[c], dbcPath);
		delete dbClone;
	}

	string inTag = ctPath.substr(ctPath.find_last_of('-')+1,ctPath.length() - ctPath.find_last_of('-') - 2 );

	// orig minsup ct inladen
	uint32 minsupuint = 999999999;
	string ctFileName;
	directory_iterator itr(dir_path, "*.ct");
	while(itr.next()) {
		uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
		if(ms < minsupuint) {
			minsupuint = ms;
			ctFileName = itr.filename();
		}
	}
	_itoa(minsupuint,tmp,10);
	string minsup = tmp;

	ItemSet **dbGIsAr = new ItemSet*[db->GetNumRows()];
	uint32 dbGIsArIdx = 0;

	CodeTable **classCodeTables = new CodeTable*[numClasses];
	CodeTable **classGenCodeTables = new CodeTable*[numClasses];
	Database **classGenDatabases = new Database*[numClasses];

	// Compress elk db_c
	for(uint32 c=0; c<numClasses; c++) {
		ItemSet *classItemSet = ItemSet::CreateSingleton(db->GetDataType(), classes[c], db->GetAlphabetSize());

		/*
		{
		string analysisFile = dbcPath + dbName + "_class" + classIdString[c] + ".analysis.txt";
		Database *dbG = ReadDatabase(dbName + "_class" + classIdString[c], db->GetDataType(), dbcPath);
		DbAnalyser *dba = new DbAnalyser();
		dba->Analyse(dbG, analysisFile);
		delete dba;
		}
		*/

		Config *config = new Config();
		config->ReadFile(ctPath + "compress-" + inTag + ".conf");
		config->Set("dataDir", dbcPath);
		string iscName = config->Read<string>("iscname");
		config->Set("iscname", dbName + "_class" + classIdString[c] + iscName.substr(iscName.find_first_of('-'), iscName.length()-iscName.find_first_of('-')));
		config->Set("iscZapIfMined", 1);
		config->WriteFile(dbcPath + "compress-"+classIdString[c]+".conf");

		Config *confbak = mConfig;
		mConfig = config;
		string datadirbak = Bass::GetWorkingDir();
		Bass::SetWorkingDir(dbcPath);

		string tag = dbName + "_class" + classIdString[c] + "-" + TimeUtils::GetTimeStampString();
		FicMain::Compress(mConfig, tag);

		Bass::SetWorkingDir(datadirbak);
		mConfig = confbak;

		// als alles goed is kunnen we nu de ct uitlezen
		uint32 ctminsupuint = 999999999;
		string msCtFileName;
		string compress_dir_path( dbcPath + tag + "/");
		directory_iterator itr(dir_path);
		while(itr.next()) {
			if(!FileUtils::IsDirectory(itr.fullpath()) && itr.filename().substr(itr.filename().length()-3,3) == ".ct") {
				uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
				if(ms < ctminsupuint) {
					ctminsupuint = ms;
					msCtFileName = itr.filename();
				}
			}
		}
		CodeTable *ctX = CodeTable::Create("coverfull", db->GetDataType());
		ctX->UseThisStuff(classDatabases[c], classDatabases[c]->GetDataType(), InitCTEmpty, 0, 1);
		ctX->ReadFromDisk(dbcPath + tag + "/" + msCtFileName, true);
		ctX->AddOneToEachUsageCount();
		ctX->CalcCodeTableSize();

		classCodeTables[c] = ctX;

		// en hier gaan we dan genereren

		DGen *dg = DGen::Create(genType);
		dg->SetInputDatabase(classDatabases[c]);
		dg->SetColumnDefinition(mConfig->Read<string>("coldef"));
		dg->BuildModel(classDatabases[c], mConfig, dbcPath + tag + "/" + msCtFileName);

		Database *generateDb = dg->GenerateDatabase(classDatabases[c]->GetNumRows());
		string dbGenName = dbName + "_class" + classIdString[c] + "g";
		Database *gDbClone = new Database(generateDb);
		Database::WriteDatabase(gDbClone, dbGenName, dbcPath);
		delete gDbClone;

		{
			string analysisFile = dbcPath + dbName + "_class" + classIdString[c] + "g.analysis.txt";
			Database *dbG = Database::ReadDatabase(dbGenName, dbcPath, db->GetDataType());
			DbAnalyser *dba = new DbAnalyser();
			dba->Analyse(dbG, analysisFile);
			delete dba;
		}

		ItemSet **genDbIs = generateDb->GetRows();
		uint32 numGenDbIs = generateDb->GetNumRows();
		for(uint32 i=0; i<numGenDbIs; i++) {
			dbGIsAr[dbGIsArIdx] = genDbIs[i]->Union(classItemSet);
			dbGIsAr[dbGIsArIdx]->SetUsageCount(1);
			dbGIsArIdx++;
		}

		iscName = config->Read<string>("iscname");
		config->Set("iscname", dbName + "_class" + classIdString[c] + "g" + iscName.substr(iscName.find_first_of('-'), iscName.length()-iscName.find_first_of('-')));
		config->WriteFile(dbcPath + "compress-" + classIdString[c]+"g.conf");

		confbak = mConfig;
		mConfig = config;
		datadirbak = Bass::GetWorkingDir();
		Bass::SetWorkingDir(dbcPath);

		tag = dbName + "_class" + classIdString[c] + "g-" + TimeUtils::GetTimeStampString();
		FicMain::Compress(mConfig, tag);

		Bass::SetWorkingDir(datadirbak);
		mConfig = confbak;

		string dbcGCtFilename = "ct-" + tag + "-" + minsup + ".ct";
		CodeTable *ctG = CodeTable::Create("coverfull", db->GetDataType());
		ctG->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
		ctG->ReadFromDisk(dbcPath + tag + "/" + dbcGCtFilename, true);
		ctG->AddOneToEachUsageCount();
		ctG->CalcCodeTableSize();

		classGenDatabases[c] = generateDb;
		classGenCodeTables[c] = ctG;

		delete dg;
		delete config;
		delete classItemSet;
	}

	Database *genDatabase = new Database(dbGIsAr, db->GetNumRows(), false);
	alphabet *a = new alphabet(*db->GetAlphabet());
	genDatabase->SetAlphabet(a);
	genDatabase->CountAlphabet();
	genDatabase->ComputeStdLengths();
	genDatabase->ComputeEssentials();

	string genDbName = dbName + "g";
	Database *gDbClone = new Database(genDatabase);
	Database::WriteDatabase(gDbClone, genDbName, dbcPath);
	delete gDbClone;

	Config *config = new Config();
	config->ReadFile(ctPath + "compress-" + inTag + ".conf");
	config->Set("dataDir", dbcPath);
	string iscName = config->Read<string>("iscname");
	config->Set("iscname", dbName + "g" + iscName.substr(iscName.find_first_of('-'), iscName.length()-iscName.find_first_of('-')));
	config->Set("iscZapIfMined", 1);
	config->WriteFile(dbcPath + "compress-g.conf");

	Config *confbak = mConfig;
	mConfig = config;
	string datadirbak = Bass::GetWorkingDir();
	Bass::SetWorkingDir(dbcPath);

	string tag = dbName + "g-" + TimeUtils::GetTimeStampString();
	FicMain::Compress(mConfig, tag);

	Bass::SetWorkingDir(datadirbak);
	mConfig = confbak;
	delete config;

	string dbgCtFilename = "ct-" + tag + "-" + minsup + ".ct";
	CodeTable *genCodeTable = CodeTable::Create("coverfull", db->GetDataType());
	genCodeTable->UseThisStuff(genDatabase, genDatabase->GetDataType(), InitCTEmpty, 0, 1);
	genCodeTable->ReadFromDisk(dbcPath + tag + "/" + dbgCtFilename, true);
	genCodeTable->AddOneToEachUsageCount();
	genCodeTable->CalcCodeTableSize();

	CodeTable *origCodeTable = CodeTable::Create("coverfull", db->GetDataType());
	origCodeTable->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
	origCodeTable->ReadFromDisk(ctPath + ctFileName, true);
	origCodeTable->AddOneToEachUsageCount();
	origCodeTable->CalcCodeTableSize();

	// Bereken afstanden
	// db <-> dbg
	double ogSize = 0, ooSize = 0, ggSize = 0, goSize = 0;
	ItemSet **oRows = db->GetRows();
	ItemSet **gRows = genDatabase->GetRows();
	uint32 numRows = db->GetNumRows();
	for(uint32 i=0; i<numRows; i++) {
		goSize += genCodeTable->CalcTransactionCodeLength(oRows[i]);
		ooSize += origCodeTable->CalcTransactionCodeLength(oRows[i]);
		ggSize += genCodeTable->CalcTransactionCodeLength(gRows[i]);
		ogSize += origCodeTable->CalcTransactionCodeLength(gRows[i]);
	}
	double dbDbGMaxDist = (goSize - ooSize) / ooSize;
	double dbDbGMinDist = (ogSize - ggSize) / ggSize;

	// dbg_c <-> dbg_c
	double **encSizes = new double*[numClasses];
	for(uint32 i=0; i<numClasses; i++) {
		encSizes[i] = new double[numClasses];
		uint32 numRows = classGenDatabases[i]->GetNumRows();
		ItemSet ** rows = classGenDatabases[i]->GetRows();

		for(uint32 j=0; j<numClasses; j++) {
			double encSize = 0;
			for(uint32 k=0; k<numRows; k++) {
				encSize += classGenCodeTables[j]->CalcTransactionCodeLength(rows[k]);
			}
			encSizes[i][j] = encSize;
		}
	}

	// dists uitrekenen
	double **minDists = new double*[numClasses];
	double **maxDists = new double*[numClasses];
	for(uint32 i=0; i<numClasses; i++) {
		minDists[i] = new double[numClasses];
		maxDists[i] = new double[numClasses];

		for(uint32 j=0; j<=i; j++) {
			//goSize - ooSize / ooSize == (encSizes[i][j] - encSizes[i][i]) / encSizes[i][i]
			//ogSize - ggSize / ggSize == (encSizes[j][i] - encSizes[j][j]) / encSizes[j][j]

			minDists[i][j] = min((encSizes[i][j] - encSizes[i][i]) / encSizes[i][i],(encSizes[j][i] - encSizes[j][j]) / encSizes[j][j]);
			maxDists[i][j] = max((encSizes[i][j] - encSizes[i][i]) / encSizes[i][i],(encSizes[j][i] - encSizes[j][j]) / encSizes[j][j]);
		}
	}

	FILE *cdistf = fopen((dbcPath + "synopsis - cdistf.txt").c_str(), "w");
	fprintf(cdistf, "%.4f\t%.4f\n", min(dbDbGMaxDist,dbDbGMinDist), max(dbDbGMinDist,dbDbGMaxDist));
	fprintf(cdistf,"EncSizes:\n");
	for(uint32 i=0; i<numClasses; i++) {
		for(uint32 j=0; j<numClasses; j++) {
			fprintf(cdistf,"%.2f\t",encSizes[i][j]);
		}
		fprintf(cdistf,"\n");
	}
	fprintf(cdistf,"Min:\n");
	for(uint32 i=1; i<numClasses; i++) {
		for(uint32 j=0; j<i; j++) {
			fprintf(cdistf,"%.2f\t",minDists[i][j]);
		}
		fprintf(cdistf,"\n");
	}
	fprintf(cdistf,"\nMax:\n");
	for(uint32 i=1; i<numClasses; i++) {
		for(uint32 j=0; j<i; j++) {
			fprintf(cdistf,"%.2f\t",maxDists[i][j]);
		}
		fprintf(cdistf,"\n");
	}
	fclose(cdistf);


	// -- Ruim Op
	for(uint32 c=0; c<numClasses; c++) {
		delete[] minDists[c];
		delete[] maxDists[c];
		delete[] encSizes[c];
		delete classDatabases[c];
		delete classCodeTables[c];
		delete classGenCodeTables[c];
		delete classGenDatabases[c];
	}
	delete[] encSizes;
	delete[] minDists;
	delete[] maxDists;
	delete[] classDatabases;
	delete[] classCodeTables;
	delete[] classGenCodeTables;
	delete[] classGenDatabases;
	delete[] classIdString;
	delete[] classes;
	delete genDatabase;
	delete genCodeTable;
	delete origCodeTable;
	delete db;
}
void DataGenTH::CheckGenDbForPrivacy() {
	// Load db
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName);

	// for each ct file in directory
	string dir_path( Bass::GetWorkingDir() + mConfig->Read<string>("ctdir"));
	if (!FileUtils::DirectoryExists(dir_path))
		throw string("GenerateDBs :: Code table directory not found.");

	// check 'minsups'-parameter
	string minsups = mConfig->Read<string>("minsups");
	bool minsupRange = false, minLiteral = false, maxLiteral = false;
	uint32 *minsupAr = NULL;
	uint32 minsupFrom = 0, minsupTo = 0, numMinsup = 0, minLiteralPos = 0, maxLiteralPos = 0;
	if(minsups.find_first_of('-')!=string::npos) {
		minsupRange = true;
		minsupFrom = (minsups.substr(0,3).compare("min")==0) ? 0 : atoi(minsups.c_str());
		minsupTo = (minsups.substr(minsups.find_first_of('-')+1,3).compare("max")==0 || minsups.substr(minsups.find_first_of('-')+2,3).compare("max")==0) ? db->GetNumRows() : atoi(minsups.substr(minsups.find_first_of('-')+1,minsups.length()-minsups.find_first_of('-')-1).c_str());
	} else {
		const char *minsupstr = minsups.c_str();
		uint32 pos = 0, curMinsup = 0;
		while((pos = minsups.find_first_of(',',pos+1))!=string::npos) {
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
			pos = minsups.find_first_of(',',pos+1)+1;
		}
	}

	uint32 numCTs = 0, minminsup = 0, maxminsup = 0; minminsup--;
	directory_iterator itr(dir_path);
	while(itr.next(), "*.ct") {
		uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
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
		delete db;
		delete minsupAr;
		throw string("Geen specifiek genoemde codetables gevonden...");
	}

	string *ctFileNameAr = new string[numCTs];
	uint32 *ctMinSupAr = new uint32[numCTs];
	uint32 curCT = 0;
	itr = directory_iterator(dir_path, "*.ct");
	while(itr.next()) {
		uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
		if(minsupRange) {
			if(ms >= minsupFrom && ms <= minsupTo) {
				ctFileNameAr[curCT] = itr.filename();
				ctMinSupAr[curCT] = atoi(ctFileNameAr[curCT].substr(ctFileNameAr[curCT].find_last_of('-')+1,ctFileNameAr[curCT].length()-ctFileNameAr[curCT].find_last_of('-')-4).c_str());
				curCT++;
			}
		} else {
			for(uint32 i=0; i<numMinsup; i++) {
				if(minsupAr[i] == ms) {
					ctFileNameAr[curCT] = itr.filename();
					ctMinSupAr[curCT] = atoi(ctFileNameAr[curCT].substr(ctFileNameAr[curCT].find_last_of('-')+1,ctFileNameAr[curCT].length()-ctFileNameAr[curCT].find_last_of('-')-4).c_str());
					curCT++;
					break;
				}
			}
		}
	}

	for(uint32 i=0; i<numCTs-1; i++) {					// sort ct-arrays op minsup
		for(uint32 j=i+1; j<numCTs; j++) {
			if(ctMinSupAr[i] > ctMinSupAr[j]) {
				uint32 tmp = ctMinSupAr[i];
				ctMinSupAr[i] = ctMinSupAr[j];
				ctMinSupAr[j] = tmp;
				string tmpstr = ctFileNameAr[i];
				ctFileNameAr[i] = ctFileNameAr[j];
				ctFileNameAr[j] = tmpstr;
			}
		}
	}

	string ctPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/";
	string genType = mConfig->Read<string>("gentype");

	string genPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/dr-" + genType + "-" + TimeUtils::GetTimeStampString() + "/";
	if(!FileUtils::DirectoryExists(genPath)) {		// maak directory
		FileUtils::CreateDir(genPath);
	}

	uint8 outputLevelBak = Bass::GetOutputLevel();
	Bass::SetOutputLevel(Bass::GetOutputLevel() > 0 ? Bass::GetOutputLevel()-1 : 0);

	uint32 genNumRows = (mConfig->KeyExists("gennumrows")) ? mConfig->Read<uint32>("gennumrows") : db->GetNumRows();

	// hoeveel databases moet ik genereren per ct?
	uint32 numGen = mConfig->Read<uint32>("numgen");

	uint32 numOrigRows = db->GetNumRows();
	uint32 numOrigRowsU = 0;
	ItemSet **origDbRows = db->GetRows();

	ItemSet ** origDbRowsU = new ItemSet*[numOrigRows];
	for(uint32 i=0; i<numOrigRows; i++) {
		if(origDbRows[i]->GetUsageCount() > 0) {
			origDbRowsU[numOrigRowsU] = origDbRows[i];
			numOrigRowsU++;
		}
		for(uint32 j=i+1; j<numOrigRows; j++) {
			if(origDbRows[j]->GetUsageCount()>0 && origDbRows[i]->Equals(origDbRows[j])) {
				origDbRows[i]->AddOneToUsageCount();
				origDbRows[j]->SetUsageCount(0);
			}
		}	
	}

	// supp-stratum bases
	ItemSet ** origDbRowsUSupp = new ItemSet*[numOrigRows];
	memcpy(origDbRowsUSupp, origDbRowsU, numOrigRows * sizeof(ItemSet*));

	uint32 *levels = new uint32[numOrigRows];
	uint32 *levelNumRows = new uint32[numOrigRows];
	ItemSet ***levelOrigSets = new ItemSet**[numOrigRows];

	uint32 numlevels = 0;
	for(uint32 i=0; i<numOrigRowsU; i++) {
		if(origDbRowsUSupp[i] != NULL) {
			uint32 levelcount = 0;
			uint32 level = origDbRowsUSupp[i]->GetUsageCount();
			levelOrigSets[numlevels] = new ItemSet*[numOrigRows];
			levelOrigSets[numlevels][levelcount] = origDbRowsUSupp[i];
			levelcount++;

			for(uint32 j=i+1; j<numOrigRowsU; j++) {
				if(origDbRowsUSupp[j] != NULL && origDbRowsUSupp[j]->GetUsageCount() == level) {
					levelOrigSets[numlevels][levelcount] = origDbRowsUSupp[j];
					origDbRowsUSupp[j] = NULL;
					levelcount++;
				}
			}
			origDbRowsUSupp[i] = NULL;
			levels[numlevels] = level;
			levelNumRows[numlevels] = levelcount;
			numlevels++;
		}
	}

	double insense = 0;
	for(uint32 i=0; i<numOrigRowsU; i++) {
		insense += 1 / (double) origDbRowsU[i]->GetUsageCount();
		//insense += 1;
	}
	printf("%f\n", insense);

	string synposisFullpath = genPath + "synopsis - " + mConfig->Read<string>("ctdir").substr(0,mConfig->Read<string>("ctdir").find_last_of('-')) + ".txt";
	FILE *synopsis = fopen(synposisFullpath.c_str(), "w");
	fprintf(synopsis, "%d\t%d\n", numOrigRowsU, numOrigRows);

	for(uint32 i=0; i<numlevels; i++) {
		printf("%d\t%d\t%d\n",i+1,levels[i],levelNumRows[i]);
		fprintf(synopsis, "%d\t%d\t%d\n",i+1,levels[i],levelNumRows[i]);
	}

	double selfSensitivity = 0;
	double selfSensitivityCS = 0;
	for(uint32 sl=0; sl<numlevels; sl++) {
		selfSensitivity += (1.0/levels[sl]);
		double countSum = 0;
		for(uint32 slo=0; slo<levelNumRows[sl]; slo++) {
			countSum += levelOrigSets[sl][slo]->GetUsageCount();
		}
		selfSensitivityCS += (1.0/levels[sl]) * (countSum / numOrigRows);
	}
	printf("%f\t%f\n",selfSensitivity, selfSensitivityCS);

	double asenseavg = 0;
	double jmsenseavg = 0;

	// 1/sup based

	Database *generateDb;
	ItemSet ** genDbRows;
	for(uint32 i=0; i<numCTs; i++) {
		DGen *dg = DGen::Create(genType);
		dg->SetInputDatabase(db);
		dg->SetColumnDefinition(mConfig->Read<string>("coldef"));
		dg->BuildModel(db, mConfig, ctPath + ctFileNameAr[i]);

		mConfig->WriteFile(genPath + "gendbs.conf");

		for(uint32 g=0; g<numGen; g++) {
			printf("Generating and checking %dth database of %dth codetable\r",g+1,i+1);
			generateDb = dg->GenerateDatabase(genNumRows);

			// G binnen
			genDbRows = generateDb->GetRows();
			ItemSet ** genDbRowsU = new ItemSet*[genNumRows];
			uint32 numGenRowsU = 0;
			for(uint32 i=0; i<genNumRows; i++) {
				if(genDbRows[i]->GetUsageCount() > 0) {
					genDbRowsU[numGenRowsU] = genDbRows[i];
					numGenRowsU++;
				}
				for(uint32 j=i+1; j<genNumRows; j++) {
					if(genDbRows[j]->GetUsageCount()>0 && genDbRows[i]->Equals(genDbRows[j])) {
						genDbRows[i]->AddOneToUsageCount();
						genDbRows[j]->SetUsageCount(0);
					}
				}	
			}

			// sensitivity in G uitrekenen

			// 1/sup aanpak
			double jmsense = 0;
			for(uint32 ro=0; ro<numOrigRowsU; ro++) {
				for(uint32 rg=0; rg<numGenRowsU; rg++) {
					if(origDbRowsU[ro]->Equals(genDbRowsU[rg])) {
						jmsense += 1 / (double) origDbRowsU[ro]->GetUsageCount();
						//jmsense += genDbRowsU[rg]->GetCount() / (double) origDbRowsU[ro]->GetCount();
						break;
					}					
				}
			}
			printf("%f\n",jmsense);
			jmsenseavg += jmsense;

			fprintf(synopsis, "%d\t%d\n", numGenRowsU, genNumRows);

			bool found = false;
			for(uint32 ro=0; ro<numOrigRowsU; ro++) {
				found = false;
				for(uint32 rg=0; rg<numGenRowsU; rg++) {
					if(genDbRowsU[rg]->Equals(origDbRowsU[ro])) {
						// output shizzle;
						found = true;
						fprintf(synopsis, "%d\t%d\n", origDbRowsU[ro]->GetUsageCount(), genDbRowsU[rg]->GetUsageCount());
						break;
					}
				}
				if(!found) {
					fprintf(synopsis, "%d\t%d\n", origDbRowsU[ro]->GetUsageCount(), 0);
				}
			}

			// strata-based
			double genSensitivity = 0;
			double osum = 0, onum = 0;
			double gsum = 0, gnum = 0;
			double genSensitivityCS = 0;
			for(uint32 sl=0; sl<numlevels; sl++) {
				double gevaar = (1.0/levels[sl]);

				uint32 slgRowCount = 0;
				uint32 slgRowCountSum = 0;
				for(uint32 slo=0; slo<levelNumRows[sl]; slo++) {
					for(uint32 rg=0; rg<numGenRowsU; rg++) {
						if(genDbRowsU[rg]->Equals(levelOrigSets[sl][slo])) {
							slgRowCount++;
							slgRowCountSum += genDbRowsU[rg]->GetUsageCount();
							break;
						}
					}
				}

				genSensitivity += gevaar * (slgRowCount / (double) levelNumRows[sl]);
				genSensitivityCS += gevaar * (slgRowCountSum / (double) genNumRows);
				if(levels[sl] <= 50) {
					for(uint32 slo=0; slo<levelNumRows[sl]; slo++) {
						for(uint32 rg=0; rg<numGenRowsU; rg++) {
							if(genDbRowsU[rg]->Equals(levelOrigSets[sl][slo])) {
								gnum++;
								gsum+=genDbRowsU[rg]->GetUsageCount();								
								break;
							}
						}
						osum += levelOrigSets[sl][slo]->GetUsageCount();
						onum ++;
					}
				}

			}
			printf("%f\t%f\n",genSensitivity, genSensitivityCS);
			asenseavg += genSensitivity;

			printf("and now! sum and num\n%f\t%f\n",gsum/osum, gnum/onum);

			// foutput shizzle
			delete generateDb;
			delete[] genDbRowsU;
		}
		delete dg;
	}

	jmsenseavg /= numGen;
	asenseavg /= numGen;
	jmsenseavg /= selfSensitivity;
	asenseavg /= selfSensitivity;
	fprintf(synopsis, "njms\tnas\n",jmsenseavg,asenseavg);
	fprintf(synopsis, "%.2f\t%.2f\n",jmsenseavg,asenseavg);

	fclose(synopsis);

	delete[] minsupAr;

	for(uint32 i=0; i<numlevels; i++) {
		delete[] levelOrigSets[i];
	}
	delete[] levels;
	delete[] levelNumRows;
	delete[] levelOrigSets;
	delete[] origDbRowsUSupp;

	delete[] ctMinSupAr;
	delete[] ctFileNameAr;
	delete[] origDbRowsU;
	delete db;
	Bass::SetOutputLevel(outputLevelBak);
}
void DataGenTH::CheckGenDbForOrigRows() {
	// Load db
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName);

	// for each ct file in directory
	string dir_path( Bass::GetWorkingDir() + mConfig->Read<string>("ctdir"));
	if(!FileUtils::DirectoryExists(dir_path))
		throw string("GenerateDBs :: Code table directory not found.");

	// check 'minsups'-parameter
	string minsups = mConfig->Read<string>("minsups");
	bool minsupRange = false, minLiteral = false, maxLiteral = false;
	uint32 *minsupAr = NULL;
	uint32 minsupFrom = 0, minsupTo = 0, numMinsup = 0, minLiteralPos = 0, maxLiteralPos = 0;
	if(minsups.find_first_of('-')!=string::npos) {
		minsupRange = true;
		minsupFrom = (minsups.substr(0,3).compare("min")==0) ? 0 : atoi(minsups.c_str());
		minsupTo = (minsups.substr(minsups.find_first_of('-')+1,3).compare("max")==0 || minsups.substr(minsups.find_first_of('-')+2,3).compare("max")==0) ? db->GetNumRows() : atoi(minsups.substr(minsups.find_first_of('-')+1,minsups.length()-minsups.find_first_of('-')-1).c_str());
	} else {
		const char *minsupstr = minsups.c_str();
		uint32 pos = 0, curMinsup = 0;
		while((pos = minsups.find_first_of(',',pos+1))!=string::npos) {
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
			pos = minsups.find_first_of(',',pos+1)+1;
		}
	}

	uint32 numCTs = 0, minminsup = 0, maxminsup = 0; minminsup--;
	directory_iterator itr(dir_path, "*.ct");
	while(itr.next()) {
		uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
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
		delete db;
		delete minsupAr;
		throw string("Geen specifiek genoemde codetables gevonden...");
	}

	string *ctFileNameAr = new string[numCTs];
	uint32 *ctMinSupAr = new uint32[numCTs];
	uint32 curCT = 0;
	itr = directory_iterator(dir_path, "*.ct");
	while(itr.next()) {
		uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
		if(minsupRange) {
			if(ms >= minsupFrom && ms <= minsupTo) {
				ctFileNameAr[curCT] = itr.filename();
				ctMinSupAr[curCT] = atoi(ctFileNameAr[curCT].substr(ctFileNameAr[curCT].find_last_of('-')+1,ctFileNameAr[curCT].length()-ctFileNameAr[curCT].find_last_of('-')-4).c_str());
				curCT++;
			}
		} else {
			for(uint32 i=0; i<numMinsup; i++) {
				if(minsupAr[i] == ms) {
					ctFileNameAr[curCT] = itr.filename();
					ctMinSupAr[curCT] = atoi(ctFileNameAr[curCT].substr(ctFileNameAr[curCT].find_last_of('-')+1,ctFileNameAr[curCT].length()-ctFileNameAr[curCT].find_last_of('-')-4).c_str());
					curCT++;
					break;
				}
			}
		}
	}

	for(uint32 i=0; i<numCTs-1; i++) {					// sort ct-arrays op minsup
		for(uint32 j=i+1; j<numCTs; j++) {
			if(ctMinSupAr[i] > ctMinSupAr[j]) {
				uint32 tmp = ctMinSupAr[i];
				ctMinSupAr[i] = ctMinSupAr[j];
				ctMinSupAr[j] = tmp;
				string tmpstr = ctFileNameAr[i];
				ctFileNameAr[i] = ctFileNameAr[j];
				ctFileNameAr[j] = tmpstr;
			}
		}
	}

	string ctPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/";
	string genType = mConfig->Read<string>("gentype");

	string genPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/dr-" + genType + "-" + TimeUtils::GetTimeStampString() + "/";
	if(!FileUtils::DirectoryExists(genPath)) {		// maak directory
		FileUtils::CreateDir(genPath);
	}

	uint8 outputLevelBak = Bass::GetOutputLevel();
	Bass::SetOutputLevel(Bass::GetOutputLevel() > 0 ? Bass::GetOutputLevel()-1 : 0);

	Database *generateDb;
	uint32 genNumRows = (mConfig->KeyExists("gennumrows")) ? mConfig->Read<uint32>("gennumrows") : db->GetNumRows();

	uint32 numOrigRows = db->GetNumRows();
	uint32 numOrigRowsU = 0;
	ItemSet **origDbRows = db->GetRows();

	ItemSet ** origDbRowsU = new ItemSet*[numOrigRows];
	for(uint32 i=0; i<numOrigRows; i++) {
		if(origDbRows[i]->GetUsageCount() > 0) {
			origDbRowsU[numOrigRowsU] = origDbRows[i];
			numOrigRowsU++;
		}
		for(uint32 j=i+1; j<numOrigRows; j++) {
			if(origDbRows[j]->GetUsageCount()>0 && origDbRows[i]->Equals(origDbRows[j])) {
				origDbRows[i]->AddOneToUsageCount();
				origDbRows[j]->SetUsageCount(0);
			}
		}	
	}

	// supp-stratum bases
	ItemSet ** origDbRowsUSupp = new ItemSet*[numOrigRows];
	memcpy(origDbRowsUSupp, origDbRowsU, numOrigRows * sizeof(ItemSet*));

	uint32 *levels = new uint32[numOrigRows];
	uint32 *levelNumRows = new uint32[numOrigRows];
	ItemSet ***levelOrigSets = new ItemSet**[numOrigRows];

	uint32 numlevels = 0;
	for(uint32 i=0; i<numOrigRowsU; i++) {
		if(origDbRowsUSupp[i] != NULL) {
			uint32 levelcount = 0;
			uint32 level = origDbRowsUSupp[i]->GetUsageCount();
			levelOrigSets[numlevels] = new ItemSet*[numOrigRows];
			levelOrigSets[numlevels][levelcount] = origDbRowsUSupp[i];
			levelcount++;

			for(uint32 j=i+1; j<numOrigRowsU; j++) {
				if(origDbRowsUSupp[j] != NULL && origDbRowsUSupp[j]->GetUsageCount() == level) {
					levelOrigSets[numlevels][levelcount] = origDbRowsUSupp[j];
					origDbRowsUSupp[j] = NULL;
					levelcount++;
				}
			}
			origDbRowsUSupp[i] = NULL;
			levels[numlevels] = level;
			levelNumRows[numlevels] = levelcount;
			numlevels++;
		}
	}


	// hoeveel databases moet ik genereren per ct?
	uint32 numGen = mConfig->Read<uint32>("numgen");


	uint32 **numN = new uint32*[numCTs];
	uint32 **numD = new uint32*[numCTs];

	ItemSet ** genDbRows;
	for(uint32 i=0; i<numCTs; i++) {
		DGen *dg = DGen::Create(genType);
		dg->SetInputDatabase(db);
		dg->SetColumnDefinition(mConfig->Read<string>("coldef"));
		dg->BuildModel(db, mConfig, ctPath + ctFileNameAr[i]);

		numN[i] = new uint32[numGen];
		numD[i] = new uint32[numGen];

		mConfig->WriteFile(genPath + "gendbs.conf");

		for(uint32 g=0; g<numGen; g++) {
			printf("Generating and checking %dth database of %dth codetable\r",g+1,i+1);
			generateDb = dg->GenerateDatabase(genNumRows);

			// G binnen
			genDbRows = generateDb->GetRows();
			ItemSet ** genDbRowsU = new ItemSet*[genNumRows];
			uint32 numGenRowsU = 0;
			for(uint32 rg=0; rg<genNumRows; rg++) {
				if(genDbRows[rg]->GetUsageCount() > 0) {
					genDbRowsU[numGenRowsU] = genDbRows[rg];
					numGenRowsU++;
				}
				for(uint32 rgg=rg+1; rgg<genNumRows; rgg++) {
					if(genDbRows[rgg]->GetUsageCount()>0 && genDbRows[rg]->Equals(genDbRows[rgg])) {
						genDbRows[rg]->AddOneToUsageCount();
						genDbRows[rgg]->SetUsageCount(0);
					}
				}	
			}

			numN[i][g] = 0;
			numD[i][g] = 0;
			for(uint32 rg=0; rg<numGenRowsU; rg++) {
				if(genDbRowsU[rg]->GetUsageCount() < 50) {
					numD[i][g] += genDbRowsU[rg]->GetUsageCount();

					for(uint32 ro=0; ro<numOrigRowsU; ro++) {
						if(origDbRowsU[ro]->GetUsageCount() < 50 && origDbRowsU[ro]->Equals(genDbRowsU[rg])) {
							numN[i][g] += genDbRowsU[rg]->GetUsageCount();
						}
					}
				}
			}
			printf("%d\t%d\n",numN[i][g],numD[i][g]);
			delete generateDb;
			delete[] genDbRowsU;
		}
		delete dg;
	}


	string synposisFullpath = genPath + "synopsis - " + mConfig->Read<string>("ctdir").substr(0,mConfig->Read<string>("ctdir").find_last_of('-')) + ".txt";
	FILE *synopsis = fopen(synposisFullpath.c_str(), "w");

	for(uint32 i=numCTs; i>0; i--) {
		double numdeler = 0, numnoemer = 0;
		for(uint32 g=0; g<numGen; g++) {
			numdeler += numD[i-1][g];
			numnoemer += numN[i-1][g];
		}
		numdeler /= numGen;
		numnoemer /= numGen;
		fprintf(synopsis, "avg: %.2f\t%.2f\t%.2f\n", numnoemer, numdeler, numnoemer/numdeler);
		for(uint32 g=0; g<numGen; g++) {
			fprintf(synopsis, "%d\t%d\n", numN[i-1][g], numD[i-1][g]);
		}
	}

	fclose(synopsis);

	for(uint32 i=0; i<numCTs; i++) {
		delete[] numN[i];
		delete[] numD[i];
	}
	delete[] numN;
	delete[] numD;
	delete[] minsupAr;

	delete[] ctMinSupAr;
	delete[] ctFileNameAr;
	delete[] origDbRowsU;
	delete db;
	Bass::SetOutputLevel(outputLevelBak);
}

void DataGenTH::CompareISCs() {
	// Load db
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName);

	// for each ct file in directory
	string dir_path( Bass::GetWorkingDir() + mConfig->Read<string>("ctdir"));
	if (!FileUtils::DirectoryExists(dir_path))
		throw string("GenerateDBs :: Code table directory not found.");

	// check 'minsups'-parameter
	string minsups = mConfig->Read<string>("minsups");
	bool minsupRange = false, minLiteral = false, maxLiteral = false;
	uint32 *minsupAr = NULL;
	uint32 minsupFrom = 0, minsupTo = 0, numMinsup = 0, minLiteralPos = 0, maxLiteralPos = 0;
	if(minsups.find_first_of('-')!=string::npos) {
		minsupRange = true;
		minsupFrom = (minsups.substr(0,3).compare("min")==0) ? 0 : atoi(minsups.c_str());
		minsupTo = (minsups.substr(minsups.find_first_of('-')+1,3).compare("max")==0 || minsups.substr(minsups.find_first_of('-')+2,3).compare("max")==0) ? db->GetNumRows() : atoi(minsups.substr(minsups.find_first_of('-')+1,minsups.length()-minsups.find_first_of('-')-1).c_str());
	} else {
		const char *minsupstr = minsups.c_str();
		uint32 pos = 0, curMinsup = 0;
		while((pos = minsups.find_first_of(',',pos+1))!=string::npos) {
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

	uint32 numCTs = 0, minminsup = 0, maxminsup = 0; minminsup--;

	directory_iterator itr(dir_path, "*.ct");
	while(itr.next()) {
		uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
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
		delete db;
		delete minsupAr;
		throw string("Geen specifiek genoemde codetables gevonden...");
	}

	string *ctFileNameAr = new string[numCTs];
	uint32 *ctMinSupAr = new uint32[numCTs];
	uint32 curCT = 0;

	itr = directory_iterator(dir_path, "*.ct");
	while(itr.next()) {
		uint32 ms = atoi(itr.filename().substr(itr.filename().find_last_of('-')+1,itr.filename().length()-itr.filename().find_last_of('-')-4).c_str());
		if(minsupRange) {
			if(ms >= minsupFrom && ms <= minsupTo) {
				ctFileNameAr[curCT] = itr.filename();
				ctMinSupAr[curCT] = atoi(ctFileNameAr[curCT].substr(ctFileNameAr[curCT].find_last_of('-')+1,ctFileNameAr[curCT].length()-ctFileNameAr[curCT].find_last_of('-')-4).c_str());
				curCT++;
			}
		} else {
			for(uint32 i=0; i<numMinsup; i++) {
				if(minsupAr[i] == ms) {
					ctFileNameAr[curCT] = itr.filename();
					ctMinSupAr[curCT] = atoi(ctFileNameAr[curCT].substr(ctFileNameAr[curCT].find_last_of('-')+1,ctFileNameAr[curCT].length()-ctFileNameAr[curCT].find_last_of('-')-4).c_str());
					curCT++;
					break;
				}
			}
		}
	}

	for(uint32 i=0; i<numCTs-1; i++) {					// sort ct-arrays op minsup
		for(uint32 j=i+1; j<numCTs; j++) {
			if(ctMinSupAr[i] > ctMinSupAr[j]) {
				uint32 tmp = ctMinSupAr[i];
				ctMinSupAr[i] = ctMinSupAr[j];
				ctMinSupAr[j] = tmp;
				string tmpstr = ctFileNameAr[i];
				ctFileNameAr[i] = ctFileNameAr[j];
				ctFileNameAr[j] = tmpstr;
			}
		}
	}

	string ctPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/";
	string genType = mConfig->Read<string>("gentype");

	string genPath = Bass::GetWorkingDir() + mConfig->Read<string>("ctdir") + "/isc-" + genType + "-" + TimeUtils::GetTimeStampString() + "/";
	if(!FileUtils::DirectoryExists(genPath)) {		// maak directory
		FileUtils::CreateDir(genPath);
	}

	uint8 outputLevelBak = Bass::GetOutputLevel();
	Bass::SetOutputLevel(Bass::GetOutputLevel() > 0 ? Bass::GetOutputLevel()-1 : 0);

	Database *generateDb;
	uint32 genNumRows = (mConfig->KeyExists("gennumrows")) ? mConfig->Read<uint32>("gennumrows") : db->GetNumRows();

	// hoeveel databases moet ik genereren per ct?
	uint32 numGen = mConfig->Read<uint32>("numgen");

	uint32 **numDup = new uint32*[numCTs];
	uint64 **numFPG = new uint64*[numCTs];
	double **numWF = new double*[numCTs];
	double **numNF = new double*[numCTs];

	uint64 numFPO =0; // uitlezen

	string inTag = ctPath.substr(ctPath.find_last_of('-')+1,ctPath.length() - ctPath.find_last_of('-') - 2 );

	Config *config = new Config();
	config->ReadFile(ctPath + "compress-" + inTag + ".conf");

	string iscname = config->Read<string>("iscname");
	string iscGenerateFileName = iscname + ".fi";
	string iscGeneratePath = genPath;
	string iscGenerateFullPath = iscGeneratePath + iscGenerateFileName;

	//iscName = tictactoe-closed-20d
	uint32 firstdash = (uint32) iscname.find_first_of('-');
	uint32 seconddash = (uint32) iscname.find_first_of('-',firstdash+1);

	string isctype = iscname.substr(firstdash+1,seconddash-firstdash-1);
	if(isctype.compare("cls")==0)
		isctype = "closed";

	string iscminsup = iscname.substr(seconddash+1,iscname.length()-seconddash-2);
	uint32 iscminsupuint = atoi(iscminsup.c_str());

	string dbFileName = Bass::GetWorkingDir() + "datasets/" + dbName + ".db";

	string exec = Bass::GetExecDir() + "fim_" + isctype + ".exe \"" + dbFileName + "\" " 
		+ iscminsup + " \"" + iscGenerateFullPath + "\"";

	system(exec.c_str());	// !!! exit code checken
	if(!FileUtils::FileExists(iscGenerateFullPath)) {
		delete db;
		throw string("FicMain::ClassifyCompress -- Frequent ItemSet generation failed.");
	}

	IscOrderType iscOrder = ItemSetCollection::StringToIscOrderType("z");			

	string outIscPath = genPath;
	string outIscFileName = iscname + "." + config->Read<string>("iscinext");
	IscFileType iscType = IscFile::ExtToType(config->Read<string>("iscinext"));

	ItemSetCollection::ConvertIscFile(iscGeneratePath, iscGenerateFileName, outIscPath, outIscFileName, db, iscType, iscOrder, false, true, iscminsupuint);

	string iscOFilename = iscname + "." + config->Read<string>("iscinext");
	ItemSetCollection *iscO = ItemSetCollection::OpenItemSetCollection(iscOFilename, genPath, db, true);

	numFPO = iscO->GetNumItemSets();
	ItemSet **iscOiss = iscO->GetLoadedItemSets();

	uint32 numOrigRows = db->GetNumRows();
	ItemSet **origDbRows = db->GetRows();
	char tmp[20];
	for(uint32 i=0; i<numCTs; i++) {
		DGen *dg = DGen::Create(genType);
		dg->SetInputDatabase(db);
		dg->SetColumnDefinition(mConfig->Read<string>("coldef"));
		dg->BuildModel(db, mConfig, ctPath + ctFileNameAr[i]);

		numDup[i] = new uint32[numGen];
		numFPG[i] = new uint64[numGen];
		numWF[i] = new double[numGen];
		numNF[i] = new double[numGen];

		mConfig->WriteFile(genPath + "gendbs.conf");

		for(uint32 g=0; g<numGen; g++) {
			printf("Generating and checking %dth database of %dth codetable\r",g+1,i+1);

			// save db
			generateDb = dg->GenerateDatabase(genNumRows);
			_itoa(ctMinSupAr[i], tmp, 10);
			string dbGenName = dbName + "_ct"; dbGenName.append(tmp); dbGenName.append("_g"); _itoa(g, tmp, 10); dbGenName.append(tmp);
			Database::WriteDatabase(generateDb, dbGenName, genPath);

			// mine fps
			string genDbFileName = genPath + dbGenName + "." + mConfig->Read<string>("dbinext");
			iscGenerateFullPath = genPath + dbGenName + iscname.substr(firstdash+1,iscname.length()) + ".fi";

			
/*
			string exec = Bass::GetExecDir() + "fim_" + isctype + ".exe \"" + genDbFileName + "\" " 
				+ iscminsup + " \"" + iscGenerateFullPath + "\"";

			ECHO(2,printf(" * Mining Item Sets:\tin progress..."));
			system(exec.c_str());	// !!! exit code checken
			if(!FileUtils::FileExists(iscGenerateFullPath)) {
				delete generateDb;
				throw string("FicMain::ClassifyCompress -- Frequent ItemSet generation failed.");
			}
			ECHO(2,printf("\r * Mining Item Sets:\tdone.         \n\n"));

			IscOrderType iscOrder = ItemSetCollection::StringToIscOrderType("z");			

			ItemSetCollection::ConvertIscFile(generateDb, iscGenerateFullPath, genPath + dbGenName + iscname.substr(firstdash+1,iscname.length()) + "." + config->Read<string>("iscinext"), IscFile::ExtToType(config->Read<string>("iscinext")), false, iscOrder, iscminsupuint);

			remove( iscGenerateFullPath.c_str() ); // check success ?
			if(!FileUtils::FileExists(genPath + dbGenName + iscname.substr(firstdash+1,iscname.length()) + "." + config->Read<string>("iscinext"))) {
				delete generateDb;
				throw string("FicMain::ClassifyCompress -- Frequent ItemSet conversion failed.");
			}
*/
			string iscFilename = dbGenName + iscname.substr(firstdash+1,iscname.length()) + "." + config->Read<string>("iscinext");
			ItemSetCollection *isc = NULL;
/*			try {
				isc = ItemSetCollection::OpenItemSetCollection(iscFilename, genPath, db, true);
			} catch(string msg) {
				delete generateDb;
				throw msg;
			}
*/			
			// /\ OLD

			// \/ NEW
			isc = FicMain::MineItemSetCollection(iscname, generateDb, false, true);

			uint64 numisc = isc->GetNumItemSets();
			numFPG[i][g] = numisc;
			ItemSet ** isccol = isc->GetLoadedItemSets();

			string iscafwFullpath = genPath + "synopsis - isafw" + mConfig->Read<string>("ctdir").substr(0,mConfig->Read<string>("ctdir").find_last_of('-')) + ".txt";
			FILE *iscafw = fopen(iscafwFullpath.c_str(), "w");

			uint32 ocstart = 0;
			uint64 welfound = 0, nietfound = 0;
			numDup[i][g] = 0;
			for(uint32 gc=0; gc<numisc; gc++) {
				bool found = false;
				for(uint32 oc=ocstart; oc<numFPO; oc++) {
					if(isccol[gc]->Equals(iscOiss[oc])) {
						found = true;
						numDup[i][g]++;
						uint32 diff = max(iscOiss[oc]->GetSupport(), isccol[gc]->GetSupport()) - min(iscOiss[oc]->GetSupport(), isccol[gc]->GetSupport());
						welfound += diff;
						fprintf(iscafw, "%d\t%d\t%d\n", iscOiss[oc]->GetSupport(), isccol[gc]->GetSupport(), diff);
						break;
					} else if(iscOiss[oc]->GetLength() < isccol[gc]->GetLength()) {
						ocstart = oc;
					} else if(iscOiss[oc]->GetLength() > isccol[gc]->GetLength()) {
						found = false;
						nietfound += isccol[gc]->GetSupport();
						break;
					}
				}
			}
			fclose(iscafw);
			numNF[i][g] = (double) nietfound / (double) (numFPG[i][g] - numDup[i][g]);
			numWF[i][g] = (double) welfound / (double) numDup[i][g];

			delete isc;
			delete generateDb;

		}
		delete dg;
	}


	string synposisFullpath = genPath + "synopsis - " + mConfig->Read<string>("ctdir").substr(0,mConfig->Read<string>("ctdir").find_last_of('-')) + ".txt";
	FILE *synopsis = fopen(synposisFullpath.c_str(), "w");

	for(uint32 i=numCTs; i>0; i--) {
		uint32 numd = 0;
		uint64 numg = 0;
		double numw = 0;
		double numn = 0;
		for(uint32 g=0; g<numGen; g++) {
			numd += numDup[i-1][g];
			numg += numFPG[i-1][g];
			numw += numWF[i-1][g];
			numn += numNF[i-1][g];
		}
		numd /= numGen;
		numg /= numGen;
		numw /= numGen;
		numn /= numGen;

		fprintf(synopsis, "%d\t%d\t%d\t%.3f\t%.3f\n", numd, numg, numFPO, numw, numn);
		for(uint32 g=0; g<numGen; g++) {
			fprintf(synopsis, "%d\t%d\t%.3f\t%.3f\n", numDup[i-1][g], numFPG[i-1][g], numWF[i-1][g], numNF[i-1][g]);
		}
	}

	fclose(synopsis);

	for(uint32 i=0; i<numCTs; i++) {
		delete[] numDup[i];
		delete[] numFPG[i];
		delete[] numWF[i];
		delete[] numNF[i];
	}
	delete[] numDup;
	delete[] numFPG;
	delete[] numWF;
	delete[] numNF;
	delete[] minsupAr;
	delete config;

	delete[] ctMinSupAr;
	delete[] ctFileNameAr;
	delete iscO;
	delete db;
	Bass::SetOutputLevel(outputLevelBak);

}

#endif // ENABLE_DATAGEN
