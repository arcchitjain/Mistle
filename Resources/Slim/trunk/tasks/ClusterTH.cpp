#ifdef ENABLE_CLUSTER

#include "../global.h"

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <db/ClassedDatabase.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <isc/ItemTranslator.h>

// croupier
#include <Croupier.h>

// clusterer
#include "../blocks/cluster/DBOntwarreraar.h"
#include "../blocks/cluster/Clusterer1G.h"
#include "../blocks/cluster/Ontwar2Clusterer.h"
#include "../blocks/cluster/OntwarKClusterer.h"
#include "../blocks/dgen/DGen.h"
#include "../blocks/ptree/PTree.h"

#include "../algo/CodeTable.h"
#include "../FicConf.h"
#include "../FicMain.h"

#include "DissimilarityTH.h"
#include "ClusterTH.h"

ClusterTH::ClusterTH(Config *conf) : TaskHandler(conf){

}
ClusterTH::~ClusterTH() {
	// not my Config*
}

void ClusterTH::HandleTask() {
	string command = mConfig->Read<string>("command");

	if(command.compare("ontwardb") == 0)				OntwarDB();
	else	if(command.compare("ontwardbds") == 0)		OntwarDBDissimilarity();
	else	if(command.compare("ontwarct") == 0)		OntwarCT();
	else	if(command.compare("ontwarctsplitdb") == 0)	OntwarCTSplitDb();
	else	if(command.compare("ontwarctclassify") == 0)OntwarCTClassify();
	else	if(command.compare("ontwarctds") == 0)		OntwarCTDissimilarity();
	else	if(command.compare("ontwarctanalyse") == 0)	OntwarCTAnalyse();
	else	if(command.compare("ontwardatagen") == 0)	OntwarCTGenerateDB();
	else	if(command.compare("whichrowwhere") == 0)	WhichRowsWhatCluster();

	else	throw string("ClusterTH :: Unable to handle task `" + command + "`");
}

string ClusterTH::BuildWorkingDir() {
	//char s[100];
	//_i64toa_s(Logger::GetCommandStartTime(), s, 100, 10);
	return mConfig->Read<string>("command") + "/";
}

void ClusterTH::OntwarDB() {
	string dbName = mConfig->Read<string>("dbname");

	string clusterType = "owdb";

	// Determine tag, output base dir & backup full config
	string clusterBasePath = "";
	string tag;
	{
		// dbname-all-1d-pop-3-random-200712121212
		// example tag: mushroom-ontwar-all-1d-pop-2007123123123123
		tag = dbName + "-" + mConfig->Read<string>("isctype") + "-" + mConfig->Read<string>("iscminsup") + mConfig->Read<string>("iscorder") + "-" + mConfig->Read<string>("prunestrategy");
		tag += "-" + mConfig->Read<string>("numparts");
		tag += "-" + mConfig->Read<string>("initstylee");
		bool dir = mConfig->Read<uint32>("directional",0) == 0 ? false : true;
		bool best = mConfig->Read<uint32>("bestszdif",0) == 0 ? false : true; 
		string dirbst = dir == true ? (best == true ? "d1" : "da") : (best == true ? "b1" : "ba");
		tag += "-" + dirbst;
		tag += "-" + TimeUtils::GetTimeStampString();

		clusterBasePath = Bass::GetWorkingDir();
		if(!FileUtils::DirectoryExists(clusterBasePath))
			FileUtils::CreateDir(clusterBasePath);

		clusterBasePath += tag + "/";
		if(!FileUtils::DirectoryExists(clusterBasePath))
			FileUtils::CreateDir(clusterBasePath);

		Bass::SetWorkingDir(clusterBasePath);

		string confFile = clusterBasePath + mConfig->Read<string>("command") + ".conf";
		mConfig->WriteFile(confFile);
		ECHO(1,printf("** Experiment :: Cluster DB\n * Tag:\t\t\t%s\n", tag.c_str()));
	}

	// Run Clusterer
	Clusterer *clusterer = Clusterer::CreateClusterer(mConfig);
	clusterer->Cluster();
	delete clusterer;
}
void ClusterTH::OntwarDBDissimilarity() {
	string ontwarDir = mConfig->Read<string>("ontwardir");

	string ontwarPath = Bass::GetExpDir() + "ontwardb/" + ontwarDir + "/";
	printf("%s\n", ontwarPath.c_str());
	if(!FileUtils::DirectoryExists(ontwarPath))
		throw string("ClusterTH::OntwarDBDissimilarity could not find the given OntwarDir.");
	Bass::SetWorkingDir(ontwarPath);

	DBOntwarreraar *ontwarreraar = new DBOntwarreraar(mConfig);
	ontwarreraar->Dissimilarity(ontwarPath, mConfig->Read<bool>("minCTs", false));
	delete ontwarreraar;
}

void ClusterTH::OntwarCT() {
	string ctDir = FicMain::FindCodeTableFullDir(mConfig->Read<string>("ctdir"));
	string clusterPath;
	string clusterTag = "";

	// Get variant (2 or k)
	bool kclusters = mConfig->Read<string>("ontwarct").compare("ontwark") == 0;

	{
		// example tag: ontwar-2007123123123123
		if(kclusters)
			clusterTag = "ontwarK" + mConfig->Read<string>("k") + "ct-" + TimeUtils::GetTimeStampString();
		else
			clusterTag = "ontwar2ct-" + TimeUtils::GetTimeStampString();

		clusterPath = ctDir + clusterTag + "/";
		if(!FileUtils::DirectoryExists(clusterPath))
			FileUtils::CreateDir(clusterPath);

		Bass::SetWorkingDir(clusterPath);

		string confFile = clusterPath + mConfig->Read<string>("command") + ".conf";
		mConfig->WriteFile(confFile);
		ECHO(1,printf("** Experiment :: Ontwar CT\n * Tag:\t\t\t%s\n * Path:\t%s\n", clusterTag.c_str(), clusterPath.c_str()));
	}

	// Run Clusterer
	Clusterer *clusterer = NULL;
	if(kclusters)
		clusterer = new OntwarKClusterer(mConfig, ctDir);
	else
		clusterer = new Ontwar2Clusterer(mConfig, ctDir);
	clusterer->Cluster();
	delete clusterer;
}
void ClusterTH::OntwarCTSplitDb() {
	string ctDir = FicMain::FindCodeTableFullDir(mConfig->Read<string>("ctdir"));
	string ontwarDir = mConfig->Read<string>("ontwardir");

	string path = ctDir + ontwarDir + "/";
	Bass::SetWorkingDir(path);

	string confFile = path + mConfig->Read<string>("command") + ".conf";
	mConfig->WriteFile(confFile);
	ECHO(1,printf("** Experiment :: Ontwar CT split DB\n * Path:\t%s\n", path.c_str()));

	// Run Clusterer
	OntwarKClusterer *clusterer = new OntwarKClusterer(mConfig, ctDir);
	clusterer->SplitDB(path);
	delete clusterer;
}
void ClusterTH::OntwarCTClassify() {
	string ctDir = FicMain::FindCodeTableFullDir(mConfig->Read<string>("ctdir"));
	string ontwarDir = mConfig->Read<string>("ontwardir");

	string path = ctDir + ontwarDir + "/";
	Bass::SetWorkingDir(path);

	string confFile = path + mConfig->Read<string>("command") + ".conf";
	mConfig->WriteFile(confFile);
	ECHO(1,printf("** Experiment :: Ontwar CT Classify\n * Path:\t%s\n", path.c_str()));

	// Run Clusterer
	OntwarKClusterer *clusterer = new OntwarKClusterer(mConfig, ctDir);
	clusterer->Classify(path);
	delete clusterer;
}
void ClusterTH::OntwarCTDissimilarity() {
	string ctDir = FicMain::FindCodeTableFullDir(mConfig->Read<string>("ctdir"));
	string ontwarDir = mConfig->Read<string>("ontwardir");

	string path = ctDir + ontwarDir + "/";
	Bass::SetWorkingDir(path);

	string confFile = path + mConfig->Read<string>("command") + ".conf";
	mConfig->WriteFile(confFile);
	ECHO(1,printf("** Experiment :: Ontwar CT Dissimilarity\n * Path:\t%s\n", path.c_str()));

	// Run Clusterer
	OntwarKClusterer *clusterer = new OntwarKClusterer(mConfig, ctDir);
	clusterer->Dissimilarity(path);
	delete clusterer;
}
void ClusterTH::OntwarCTAnalyse() {
	string ctDir = FicMain::FindCodeTableFullDir(mConfig->Read<string>("ctdir"));
	string ontwarDir = mConfig->Read<string>("ontwardir");

	string path = ctDir + ontwarDir + "/";
	Bass::SetWorkingDir(path);

	string confFile = path + mConfig->Read<string>("command") + ".conf";
	mConfig->WriteFile(confFile);
	ECHO(1,printf("** Experiment :: Ontwar CT Analyser\n * Path:\t%s\n", path.c_str()));

	// Run Clusterer
	OntwarKClusterer *clusterer = new OntwarKClusterer(mConfig, ctDir);
	clusterer->ComponentsAnalyser(path);
	delete clusterer;
}

void ClusterTH::OntwarCTGenerateDB() {
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
	origDBCT->AddOneToEachUsageCount();

	// Stap 2 - Cluster de Database, dan wel ophalen die handel
	OntwarKClusterer *cluster = new OntwarKClusterer(mConfig);
	CodeTable **clusterCTs; uint32 *clusterNumTrans;
	uint32 numClusters = cluster->ProvideDataGenInfo(&clusterCTs, &clusterNumTrans);

	uint32 numGen = mConfig->Read<uint32>("numgen");
	double avgdissim = 0;
	string fname = Bass::GetWorkingDir() + "_ontwardgen.txt";
	FILE *fp = fopen(fname.c_str(),"w");
	Database **clusterGenParts = new Database*[numClusters];
	for(uint32 j=0; j<numGen; j++) {
		// Stap 3 - Genereer clusterGenParts
		for(uint32 i=0; i<numClusters; i++) {
			DGen *dgen = DGen::Create(mConfig->Read<string>("dgentype"));
			dgen->SetInputDatabase(origDB);
			dgen->SetColumnDefinition(mConfig->Read<string>("coldef"));
			dgen->BuildModel(origDB, mConfig, clusterCTs[i]);
			clusterGenParts[i] = dgen->GenerateDatabase(clusterNumTrans[i]);
			delete dgen;
		}

		// Stap 4 - Combineer clusterGenParts tot clusterGenDB
		Database *clusterGenDB = Database::Merge(clusterGenParts, numClusters);

		// Stap 5 - Comprimeer clusterGenDB
		CodeTable *clusterGenDBCT = FicMain::CreateCodeTable(clusterGenDB, mConfig, true);

		// Stap 6 - Bereken afstand tussen clusterGenDB en origDB
		clusterGenDBCT->AddOneToEachUsageCount();
		double dissim = 1;
		dissim = DissimilarityTH::BerekenAfstandTussen(origDB, clusterGenDB, origDBCT, clusterGenDBCT);
		avgdissim += dissim;

		printf("dissim: %.2f\n", dissim);
		fprintf(fp, "%.2f\n", dissim);

		delete clusterGenDBCT;
		delete clusterGenDB;
		for(uint32 i=0; i<numClusters; i++) {
			delete clusterGenParts[i];
		}
	}
	printf("%.2f\n", avgdissim / numGen);
	fprintf(fp, "%.2f\n", avgdissim / numGen);
	fclose(fp);

	for(uint32 i=0; i<numClusters; i++) {
		delete clusterCTs[i];
	}
	delete[] clusterGenParts;
	delete[] clusterCTs;
	delete cluster;
	delete origDBCT;
	delete origDB;
}

void ClusterTH::WhichRowsWhatCluster() {
	// 1. Open DB
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);
	uint32 numRows = db->GetNumRows();
	ItemSet** rows = db->GetRows();

	string expPath = mConfig->Read<string>("expPath", "");
	string expPrefix = mConfig->Read<string>("expPrefix", "");

	// 2. per Cluster-exp
	directory_iterator dit = directory_iterator(expPath + "/" + expPrefix, "*");
	char tmp[2048];
	while(dit.next()) {
		if(dit.filename().length() <= 3)
			continue;

		string expTag = dit.filename();

		string expRunPath = dit.fullpath() + "/";
		string minFilePath = expRunPath + "minClusterIdPerRow.txt";
		string endFilePath = expRunPath + "clusterIdPerRow.txt";

		string minFilePathXps = Bass::GetWorkingDir() + "m" + dit.filename() + ".txt";
		string endFilePathXps = Bass::GetWorkingDir() + "e" + dit.filename() + ".txt";

		FILE* minFileXps = fopen(minFilePathXps.c_str(), "w");
		FILE* endFileXps = fopen(endFilePathXps.c_str(), "w");
		FILE* minFile = fopen(minFilePath.c_str(), "w");
		FILE* endFile = fopen(endFilePath.c_str(), "w");

		string numClustersStr = dit.filename().substr(expPrefix.length()+1,dit.filename().find_first_of('-',expPrefix.length()+1)-expPrefix.length()-1);
		uint32 numClusters = atoi(numClustersStr.c_str());
		CodeTable **minCTs = new CodeTable*[numClusters];
		CodeTable **endCTs = new CodeTable*[numClusters];
		for(uint32 i=0; i<numClusters; i++) {
			sprintf_s(tmp, 2048, "%sminCT%d.ct", expRunPath.c_str(), i);
			string minCtFilename = tmp;
			minCTs[i] = CodeTable::LoadCodeTable(minCtFilename, db);
			minCTs[i]->AddOneToEachUsageCount();
			sprintf_s(tmp, 2048, "%sct%d.ct", expRunPath.c_str(), i);
			string endCTFilename = tmp;
			endCTs[i] = CodeTable::LoadCodeTable(minCtFilename, db);
			endCTs[i]->AddOneToEachUsageCount();
		}

		for(uint32 i=0; i<numRows; i++) {
			double bstMinCTEncSz = minCTs[0]->CalcTransactionCodeLength(rows[i]);
			uint32 bstMinCTIdx = 0;
			double bstEndCTEncSz = endCTs[0]->CalcTransactionCodeLength(rows[i]);
			uint32 bstEndCTIdx = 0;

			for(uint32 j=1; j<numClusters; j++) {
				double curMinCTEncSz = minCTs[j]->CalcTransactionCodeLength(rows[i]);
				double curEndCTEncSz = endCTs[j]->CalcTransactionCodeLength(rows[i]);
				if(curMinCTEncSz < bstMinCTEncSz) {
					bstMinCTIdx = j;
					bstMinCTEncSz = curMinCTEncSz;
				}
				if(curEndCTEncSz < bstEndCTEncSz) {
					bstEndCTIdx = j;
					bstEndCTEncSz = curEndCTEncSz;
				}
			}
			fprintf(minFile, "%d\n", bstMinCTIdx);
			fprintf(endFile, "%d\n", bstEndCTIdx);
			fprintf(minFileXps, "%d\n", bstMinCTIdx);
			fprintf(endFileXps, "%d\n", bstEndCTIdx);
		}

		for(uint32 i=0; i<numClusters; i++) {
			delete minCTs[i];
			delete endCTs[i];
		}
		delete[] minCTs;
		delete[] endCTs;
		fclose(minFile);
		fclose(endFile);
		fclose(minFileXps);
		fclose(endFileXps);
	}
	delete db;
}

#endif // ENABLE_CLUSTER
