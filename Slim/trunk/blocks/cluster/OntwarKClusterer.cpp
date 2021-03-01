#ifdef BLOCK_CLUSTER

#if defined (_WINDOWS)
	#include <direct.h>
#endif

#include "../../global.h"

// -- qtils
#include <Config.h>
#include <FileUtils.h>
#include <logger/Log.h>

// -- bass
#include <Bass.h>
#include <db/ClassedDatabase.h>
#include <isc/ItemSetCollection.h>
#include <itemstructs/ItemSet.h>

#include "../../FicConf.h"
#include "../../FicMain.h"
#include "../../algo/Algo.h"
#include "../../tasks/DissimilarityTH.h"
#include "../../tasks/MainTH.h"

#include "OntwarKClusterer.h"

OntwarKClusterer::OntwarKClusterer(Config *config, const string &ctDir) : Clusterer(config) {
	if(ctDir.length() > 0)
		mCtDir = ctDir;
	else
		mCtDir = FicMain::FindCodeTableFullDir(config->Read<string>("ctdir"));

	string typeStr = config->Read<string>("type");
	if(typeStr.compare("minsize") == 0)
		mType = CTOntwarMinSize;
	else
		throw string("OntwarKClusterer -- Only minsize type is supported by ontwark.");

	mK = config->Read<uint32>("k",2);
	if(mK < 2)
		throw string("OntwarKClusterer -- k < 2, doesn't make any sense.");
	if(mK > 100)
		throw string("OntwarKClusterer -- k > 100, are you sure about this?");

	mPreviousRowToCluster = NULL;
}
OntwarKClusterer::~OntwarKClusterer() {
	
}

void OntwarKClusterer::Cluster() {
	{
		char temp[20];
		sprintf_s(temp,20,"%d",mK);
		LOG_MSG(string("Initiating OntwarK")+temp+" experiment with directory:");
		LOG_MSG("\t" + mCtDir);
	}

	// Determine the compression tag of the original compression run
	string compressTag = mCtDir;
	if(compressTag.find_last_of("\\/") == compressTag.length()-1)			// remove trailing slash or backslash
		compressTag = compressTag.substr(0, compressTag.length()-1);
	compressTag = compressTag.substr(compressTag.find_last_of("\\/")+1);	// remove path before tag

	// Find and read the appropiate database
	mDbName = compressTag.substr(0, compressTag.find_first_of("-"));
	Database *db = Database::RetrieveDatabase(mDbName, Uint16ItemSetType);

	// Find and read the last codetable in the specified ctDir (influenced by minsups config directive)
	CodeTable **CTs = new CodeTable *[mK];
	for(uint32 i=0; i<mK; i++)
		CTs[i] = FicMain::LoadLastCodeTable(mConfig, db, mCtDir); // hmm, ct->Clone() doesn't exist (yet)

	// Basic check
	if(CTs[0]->GetCurNumSets() < 2)
		throw string("OntwarKClusterer::Cluster() -- Bummer, loaded codetable doesn't have enough elements to do anything useful.");

	// Let's init some variables
	uint32 **clusters = new uint32 *[mK];
	for(uint32 i=0; i<mK; i++)
		clusters[i] = new uint32[db->GetNumRows()];
	mPreviousRowToCluster = new uint32[db->GetNumRows()];
	mPreviousRowToCluster[0] = mK; // make sure the clustering is marked 'changed' in the first run.
	double originalSize = CTs[0]->CalcEncodedDbSize(db) + CTs[0]->CalcCodeTableSize();

	bool useLaplace = mConfig->Read<bool>("uselaplace",true);
	if(useLaplace)
		for(uint32 i=0; i<mK; i++)
			CTs[i]->AddOneToEachUsageCount();
	else
		for(uint32 i=0; i<mK; i++)
			CTs[i]->AddSTSpecialCode();
	double startSize = CTs[0]->CalcEncodedDbSize(db) + mK * CTs[0]->CalcCodeTableSize();
	uint32 originalNumSets = CTs[0]->GetCurNumSets();
	RemovalInfoK removal;
	double prevResult;

	prevResult = DOUBLE_MAX_VALUE;

	removal = FindMinSizeRemoval(db, CTs, clusters);
	while(removal.result < prevResult) {
		ApplyRemoval(removal, db, CTs, clusters); // removes CT elem and adjusts counts
		prevResult = removal.result;
		removal = FindMinSizeRemoval(db, CTs, clusters);
	}

	uint32 *numRows = new uint32[mK];
	ReassignRows(db, CTs, clusters, numRows);
	double finalSize = 0.0;
	for(uint32 i=0; i<mK; i++) {
		CTs[i]->CoverMaskedDB(CTs[i]->GetCurStats(), clusters[i], numRows[i]);
		finalSize += CTs[i]->GetCurSize();
	}

	// Output some stuff
	char temp[500];
	sprintf(temp, "Clustered <> original:\t%.0f %s %.0f", finalSize, finalSize<originalSize?"<":">", originalSize);
	LOG_MSG(temp);
	sprintf(temp, "Clustered <> start:\t\t%.0f %s %.0f", finalSize, finalSize<startSize?"<":">", startSize);
	LOG_MSG(temp);
	sprintf(temp, "Original:  %d ctElems, %d rows, %.0f bits", originalNumSets, db->GetNumRows(), originalSize);
	LOG_MSG(temp);
	for(uint32 i=0; i<mK; i++) {
		sprintf(temp, "Cluster %d: %d ctElems, %d rows, %.0f bits", i+1, CTs[i]->GetCurNumSets(), numRows[i], CTs[i]->GetCurSize());
		LOG_MSG(temp);
	}

	// Write resulting CTs to disk
	string ctFilename;
	for(uint32 i=0; i<mK; i++) {
		sprintf(temp, "cluster%d.ct", i+1);
		ctFilename = Bass::GetWorkingDir() + temp;
		CTs[i]->WriteToDisk(ctFilename);
	}

	// Compute 'confusion matrix' if we have some class info
	float accuracy = 0;
	if(db->GetType() == DatabaseClassed) {
		LOG_MSG("Class distribution");

		ClassedDatabase *cdb = (ClassedDatabase*)db;
		uint16 *classDef = cdb->GetClassDefinition();
		uint32 numClasses = cdb->GetNumClasses();
		uint32 *classDist = new uint32[numClasses];
		ItemSet **rows = cdb->GetRows();

		// Classes
		string s = "\t\t";
		for(uint32 j=0; j<numClasses; j++) {
			sprintf(temp, "%d\t\t", classDef[j]);
			s += temp;
		}
		LOG_MSG(s);

		// Cluster k
		uint32 largestVal;
		for(uint32 k=0; k<mK; k++) {
			for(uint32 j=0; j<numClasses; j++)
				classDist[j] = 0;
			for(uint32 i=0; i<numRows[k]; i++) {
				for(uint32 j=0; j<numClasses; j++)
					if(classDef[j] == cdb->GetClassLabel(clusters[k][i])) {
						classDist[j] += rows[clusters[k][i]]->GetUsageCount();
						break;
					}
			}
			sprintf(temp, "c%d\t\t", k+1);
			s = temp;
			largestVal = 0;
			for(uint32 j=0; j<numClasses; j++) {
				sprintf(temp, "%d\t\t", classDist[j]);
				s += temp;
				if(classDist[j] > largestVal)
					largestVal = classDist[j];
			}
			accuracy += largestVal;
			LOG_MSG(s);
		}
		accuracy /= db->GetNumTransactions();
		sprintf(temp, "Clustering accuracy = %.03f", accuracy);
		LOG_MSG(string(temp));
		delete[] classDist;
	}

	// Output summary
	{
		string resultLine = compressTag;
		resultLine += "\t" + mConfig->Read<string>("k") + "\t" + mConfig->Read<string>("cntmultiplier","1");
		sprintf_s(temp,100,"\t%.02f",finalSize); resultLine += temp;
		if(db->GetType() == DatabaseClassed) {
			sprintf_s(temp,100,"\t%.03f",accuracy);
			resultLine += temp;
		}
		resultLine += "\n";
		string summaryFile = Bass::GetWorkingDir() + "summary.txt";
		FILE *file;
		if(fopen_s(&file, summaryFile.c_str(), "w+") != 0)
			throw string("Cannot write OntwarK summary file.");
		fputs(resultLine.c_str(), file);
		fclose(file);
	}

	// Cleanup
	for(uint32 i=0; i<mK; i++) {
		delete clusters[i];
		delete CTs[i];
	}
	delete[] clusters;
	delete[] CTs;
	delete[] numRows;
	delete[] mPreviousRowToCluster; mPreviousRowToCluster = NULL;
	delete db;
}

uint32 OntwarKClusterer::ProvideDataGenInfo(CodeTable ***CTs, uint32 **numTransactions) {
	char temp[100];

	// Determine the compression tag of the original compression run
	string compressTag = mCtDir;
	if(compressTag.find_last_of("\\/") == compressTag.length()-1)			// remove trailing slash or backslash
		compressTag = compressTag.substr(0, compressTag.length()-1);
	compressTag = compressTag.substr(compressTag.find_last_of("\\/")+1);	// remove path before tag

	// Find and read the appropiate database
	mDbName = compressTag.substr(0, compressTag.find_first_of("-"));
	Database *db = Database::RetrieveDatabase(mDbName);

	// Find k
	string ontwarTag = mConfig->Read<string>("ontwardir");
	string ontwarPath = mCtDir + ontwarTag + "/";
	Bass::SetWorkingDir(ontwarPath);
	size_t pos = ontwarTag.find_first_of("K");
	if(pos == string::npos)
		throw string("OntwarKClusterer::ProvideDataGenInfo() -- Cannot find k in ontwarDir.");
	pos++;
	mK = atoi(ontwarTag.c_str() + pos);

	// Read CTs defining the clusters
	*CTs = new CodeTable *[mK];
	string filename;
	for(uint32 i=0; i<mK; i++) {
		sprintf_s(temp, 100, "%d", i+1);
		filename = ontwarPath + "cluster" + temp + ".ct";
		(*CTs)[i] = CodeTable::LoadCodeTable(filename, db);
	}

	// Let's init some variables
	uint32 **clusters = new uint32 *[mK];
	for(uint32 i=0; i<mK; i++) {
		clusters[i] = new uint32[db->GetNumRows()];
		(*CTs)[i]->AddOneToEachUsageCount();
	}
	uint32 *numRows = new uint32[mK];
	ItemSet **dbRows = db->GetRows();

	// Find clustering
	ReassignRows(db, *CTs, clusters, numRows);

	// Determine cluster sizes (transactions! not rows)
	(*numTransactions) = new uint32[mK];
	for(uint32 k=0; k<mK; k++) {
		(*numTransactions)[k] = 0;
		for(uint32 i=0; i<numRows[k]; i++)
			(*numTransactions)[k] += dbRows[clusters[k][i]]->GetUsageCount();
	}

	// Cleanup
	for(uint32 i=0; i<mK; i++)
		delete clusters[i];
	delete[] clusters;
	delete[] numRows;
	delete db;

	return mK;
}

void OntwarKClusterer::SplitDB(const string &ontwarDir) {
	Logger::SetLogToDisk(false);

	LOG_MSG(string("Splitting up DB with OntwarK experiment found in directory:"));
	LOG_MSG("\t" + ontwarDir);
	char temp[100];

	// Determine the compression tag of the original compression run
	string compressTag = mCtDir;
	if(compressTag.find_last_of("\\/") == compressTag.length()-1)			// remove trailing slash or backslash
		compressTag = compressTag.substr(0, compressTag.length()-1);
	compressTag = compressTag.substr(compressTag.find_last_of("\\/")+1);	// remove path before tag

	// Find and read the appropiate database
	mDbName = compressTag.substr(0, compressTag.find_first_of("-"));
	Database *db = Database::RetrieveDatabase(mDbName);

	// Find k
	string ontwarTag = ontwarDir;
	if(ontwarTag.find_last_of("\\/") == ontwarTag.length()-1)			// remove trailing slash or backslash
		ontwarTag = ontwarTag.substr(0, ontwarTag.length()-1);
	ontwarTag = ontwarTag.substr(ontwarTag.find_last_of("\\/")+1);	// remove path before tag
	size_t pos = ontwarTag.find_first_of("K");
	if(pos == string::npos)
		throw string("OntwarKClusterer::SplitDB() -- Cannot find k in ontwarDir.");
	pos++;
	mK = atoi(ontwarTag.c_str() + pos);

	// Read CTs defining the clusters
	CodeTable **CTs = new CodeTable *[mK];
	string filename;
	for(uint32 i=0; i<mK; i++) {
		sprintf_s(temp, 100, "%d", i+1);
		filename = ontwarDir + "cluster" + temp + ".ct";
		CTs[i] = CodeTable::LoadCodeTable(filename, db);
	}

	// Let's init some variables
	uint32 **clusters = new uint32 *[mK];
	for(uint32 i=0; i<mK; i++) {
		clusters[i] = new uint32[db->GetNumRows()];
		CTs[i]->AddOneToEachUsageCount();
	}
	uint32 *numRows = new uint32[mK];
	ItemSet **dbRows = db->GetRows();
	mPreviousRowToCluster = new uint32[db->GetNumRows()];
	mPreviousRowToCluster[0] = mK; // make sure the clustering is marked 'changed' in the first run.

	// Find clustering
	ReassignRows(db, CTs, clusters, numRows);

	double dbSize = 0.0, ctSize = 0.0, totalSize = 0.0;
	string summaryFile = Bass::GetWorkingDir() + "krimp.txt";
	FILE *file;
	if(fopen_s(&file, summaryFile.c_str(), "w+") != 0)
		throw string("Cannot write Krimp summary.");

	// Set compression settings
	string patType, settings;
	IscOrderType order;
	ItemSetCollection::ParseTag(compressTag, mDbName, patType, settings, order);
	mConfig->Set("dbname",mDbName);
	mConfig->Set("algo", string("coverpartial"));
	mConfig->Set("isctype",patType);
	mConfig->Set("iscminsup",settings);
	mConfig->Set("iscorder",ItemSetCollection::IscOrderTypeToString(order));

	size_t pos2;
	pos = compressTag.find('-');					// 1st
	pos = compressTag.find_first_of('-',pos+1);		// 2nd
	pos = compressTag.find_first_of('-',pos+1);		// 3rd
	pos2= compressTag.find_first_of('-',pos+1);		// 4th
	string pruneStrategy = compressTag.substr(pos+1, pos2-pos-1);
	mConfig->Set("prunestrategy", pruneStrategy);

	// Store each cluster as DB and compress it
	for(uint32 k=0; k<mK; k++) {
		ItemSet **iss = new ItemSet *[numRows[k]];
		for(uint32 j=0; j<numRows[k]; j++)
			iss[j] = dbRows[clusters[k][j]]->Clone();
		Database *clusterDb = new Database(iss, numRows[k], db->HasBinSizes());
		clusterDb->SetAlphabet(new alphabet(*db->GetAlphabet()));
		clusterDb->CountAlphabet();
		clusterDb->ComputeStdLengths();
		sprintf_s(temp, 100, "%d", k+1);
		filename = string("cluster") + temp + ".db";
		Database::WriteDatabase(clusterDb, filename, ontwarDir);

		CodeTable *ct = FicMain::CreateCodeTable(clusterDb, mConfig, true);
		filename = ontwarDir + "krimp" + temp + ".ct";
		ct->WriteToDisk(filename);

		CoverStats stats = ct->GetCurStats();
		dbSize += stats.encDbSize;
		ctSize += stats.encCTSize;
		totalSize += stats.encSize;

		// Output summary
		{
			string resultLine;
			sprintf_s(temp,100,"cluster%d\t%.02f\t%.02f\t%.02f\n",k+1,stats.encDbSize,stats.encCTSize,stats.encSize); resultLine += temp;
			fputs(resultLine.c_str(), file);
		}

		delete ct;
		delete clusterDb;
	}

	// Output summary
	{
		string resultLine;
		sprintf_s(temp,100,"%.02f\t%.02f\t%.02f\n",dbSize,ctSize,totalSize); resultLine += temp;
		fputs(resultLine.c_str(), file);
	}
	fclose(file);

	// Cleanup
	for(uint32 i=0; i<mK; i++) {
		delete CTs[i];
		delete clusters[i];
	}
	delete[] CTs;
	delete[] clusters;
	delete[] numRows;
	delete[] mPreviousRowToCluster;
	mPreviousRowToCluster = NULL;
	delete db;
}

void OntwarKClusterer::Classify(const string &ontwarDir) {
	Logger::SetLogToDisk(false);

	LOG_MSG(string("Classifying DB with OntwarK experiment found in directory:"));
	LOG_MSG("\t" + ontwarDir);
	char temp[100];

	// Determine the compression tag of the original compression run
	string compressTag = mCtDir;
	if(compressTag.find_last_of("\\/") == compressTag.length()-1)			// remove trailing slash or backslash
		compressTag = compressTag.substr(0, compressTag.length()-1);
	compressTag = compressTag.substr(compressTag.find_last_of("\\/")+1);	// remove path before tag

	// Find and read the appropiate database
	mDbName = compressTag.substr(0, compressTag.find_first_of("-"));
	Database *db = Database::RetrieveDatabase(mDbName);

	// Find k
	string ontwarTag = ontwarDir;
	if(ontwarTag.find_last_of("\\/") == ontwarTag.length()-1)			// remove trailing slash or backslash
		ontwarTag = ontwarTag.substr(0, ontwarTag.length()-1);
	ontwarTag = ontwarTag.substr(ontwarTag.find_last_of("\\/")+1);	// remove path before tag
	size_t pos = ontwarTag.find_first_of("K");
	if(pos == string::npos)
		throw string("OntwarKClusterer::SplitDB() -- Cannot find k in ontwarDir.");
	pos++;
	mK = atoi(ontwarTag.c_str() + pos);

	// Read CTs defining the clusters
	CodeTable **CTs = new CodeTable *[mK];
	string filename;
	for(uint32 i=0; i<mK; i++) {
		sprintf_s(temp, 100, "%d", i+1);
		filename = ontwarDir + "cluster" + temp + ".ct";
		CTs[i] = CodeTable::LoadCodeTable(filename, db);
	}

	// Let's init some variables
	uint32 **clusters = new uint32 *[mK];
	for(uint32 i=0; i<mK; i++) {
		clusters[i] = new uint32[db->GetNumRows()];
		CTs[i]->AddOneToEachUsageCount();
	}
	uint32 *numRows = new uint32[mK];
	mPreviousRowToCluster = new uint32[db->GetNumRows()];
	mPreviousRowToCluster[0] = mK; // make sure the clustering is marked 'changed' in the first run.

	// Find clustering
	ReassignRows(db, CTs, clusters, numRows);

	string summaryFile = Bass::GetWorkingDir() + "classify.txt";
	FILE *file;
	if(fopen_s(&file, summaryFile.c_str(), "w+") != 0)
		throw string("Cannot write Krimp summary.");

	// Do classification
	// Compute 'confusion matrix' if we have some class info
	float accuracy = 0;
	ClassedDatabase *cdb = (ClassedDatabase*)db;
	uint16 *classDef = cdb->GetClassDefinition();
	uint32 numClasses = cdb->GetNumClasses();
	uint32 *classDist = new uint32[numClasses];
	ItemSet **rows = cdb->GetRows();

	// Classes
	string s = "\t\t";
	for(uint32 j=0; j<numClasses; j++) {
		sprintf(temp, "%d\t\t", classDef[j]);
		s += temp;
	} s += "\n";
	fputs(s.c_str(), file);

	// Cluster k
	uint32 largestVal;
	for(uint32 k=0; k<mK; k++) {
		for(uint32 j=0; j<numClasses; j++)
			classDist[j] = 0;
		for(uint32 i=0; i<numRows[k]; i++) {
			for(uint32 j=0; j<numClasses; j++)
				if(classDef[j] == cdb->GetClassLabel(clusters[k][i])) {
					classDist[j] += rows[clusters[k][i]]->GetUsageCount();
					break;
				}
		}
		sprintf(temp, "c%d\t\t", k+1);
		s = temp;
		largestVal = 0;
		for(uint32 j=0; j<numClasses; j++) {
			sprintf(temp, "%d\t\t", classDist[j]);
			s += temp;
			if(classDist[j] > largestVal)
				largestVal = classDist[j];
		}
		accuracy += largestVal;
		s += "\n";
		fputs(s.c_str(), file);
	}
	accuracy /= db->GetNumTransactions();
	sprintf(temp, "Accuracy = %.03f\n", accuracy);
	fputs(temp, file);
	delete[] classDist;

	fclose(file);

	// Cleanup
	for(uint32 i=0; i<mK; i++) {
		delete CTs[i];
		delete clusters[i];
	}
	delete[] CTs;
	delete[] clusters;
	delete[] numRows;
	delete[] mPreviousRowToCluster;
	mPreviousRowToCluster = NULL;
	delete db;
}

void OntwarKClusterer::Dissimilarity(const string &ontwarDir) {
	Logger::SetLogToDisk(false);

	LOG_MSG(string("Computing dissimilarities with OntwarK experiment found in directory:"));
	LOG_MSG("\t" + ontwarDir);
	char temp[100];

	// Determine the compression tag of the original compression run
	string compressTag = mCtDir;
	if(compressTag.find_last_of("\\/") == compressTag.length()-1)			// remove trailing slash or backslash
		compressTag = compressTag.substr(0, compressTag.length()-1);
	compressTag = compressTag.substr(compressTag.find_last_of("\\/")+1);	// remove path before tag

	// Find and read the appropiate database
	mDbName = compressTag.substr(0, compressTag.find_first_of("-"));
	Database *db = Database::RetrieveDatabase(mDbName);

	// Find k
	string ontwarTag = ontwarDir;
	if(ontwarTag.find_last_of("\\/") == ontwarTag.length()-1)			// remove trailing slash or backslash
		ontwarTag = ontwarTag.substr(0, ontwarTag.length()-1);
	ontwarTag = ontwarTag.substr(ontwarTag.find_last_of("\\/")+1);	// remove path before tag
	size_t pos = ontwarTag.find_first_of("K");
	if(pos == string::npos)
		throw string("OntwarKClusterer::SplitDB() -- Cannot find k in ontwarDir.");
	pos++;
	mK = atoi(ontwarTag.c_str() + pos);

	// Read CTs defining the clusters
	CodeTable **CTs = new CodeTable *[mK];
	string filename;
	for(uint32 i=0; i<mK; i++) {
		sprintf_s(temp, 100, "%d", i+1);
		filename = ontwarDir + "cluster" + temp + ".ct";
		CTs[i] = CodeTable::LoadCodeTable(filename, db);
	}

	// Let's init some variables
	uint32 **clusters = new uint32 *[mK];
	for(uint32 i=0; i<mK; i++) {
		clusters[i] = new uint32[db->GetNumRows()];
		CTs[i]->AddOneToEachUsageCount();
	}
	uint32 *numRows = new uint32[mK];
	ItemSet **dbRows = db->GetRows();
	mPreviousRowToCluster = new uint32[db->GetNumRows()];
	mPreviousRowToCluster[0] = mK; // make sure the clustering is marked 'changed' in the first run.

	// Find clustering
	ReassignRows(db, CTs, clusters, numRows);

	double dbSize = 0.0, ctSize = 0.0, totalSize = 0.0;
	string summaryFile = Bass::GetWorkingDir() + "dissimilarity.txt";
	FILE *file;
	if(fopen_s(&file, summaryFile.c_str(), "w+") != 0)
		throw string("Cannot write DS file.");

	// Determine databases
	Database **dbs = new Database *[mK];
	for(uint32 k=0; k<mK; k++) {
		ItemSet **iss = new ItemSet *[numRows[k]];
		for(uint32 j=0; j<numRows[k]; j++)
			iss[j] = dbRows[clusters[k][j]]->Clone();
		dbs[k] = new Database(iss, numRows[k], db->HasBinSizes());
		dbs[k]->SetAlphabet(new alphabet(*db->GetAlphabet()));
		dbs[k]->CountAlphabet();
		dbs[k]->ComputeStdLengths();
	}

	// Compute distances
	double avg = 0.0, ds = 0, minDS = DOUBLE_MAX_VALUE, maxDS = 0.0;
	uint32 nr = 0;
	for(uint32 i=0; i<mK-1; i++) {
		for(uint32 j=i+1; j<mK; j++) {
			ds = DissimilarityTH::BerekenAfstandTussen(dbs[i], dbs[j], CTs[i], CTs[j]);
			avg += ds;
			if(ds < minDS) minDS = ds;
			if(ds > maxDS) maxDS = ds;
			nr++;
			sprintf_s(temp, 100, "%.02f\t", ds);
			fputs(temp, file);
		}
		fputs("\n", file);
	}
	avg /= nr;
	sprintf_s(temp, 100, "%.02f\t%.02f\t%.02f\n", minDS, avg, maxDS);
	fputs(temp, file);

	// Cleanup
	fclose(file);
	for(uint32 i=0; i<mK; i++) {
		delete dbs[i];
		delete CTs[i];
		delete clusters[i];
	}
	delete[] dbs;
	delete[] CTs;
	delete[] clusters;
	delete[] numRows;
	delete[] mPreviousRowToCluster;
	mPreviousRowToCluster = NULL;
	delete db;
}

void OntwarKClusterer::ComponentsAnalyser(const string &ontwarDir) {
	Logger::SetLogToDisk(false);

	LOG_MSG(string("Components analysis"));
	char temp[100];
	string filename;

	// Determine the compression tag of the original compression run
	string compressTag = mCtDir;
	if(compressTag.find_last_of("\\/") == compressTag.length()-1)			// remove trailing slash or backslash
		compressTag = compressTag.substr(0, compressTag.length()-1);
	compressTag = compressTag.substr(compressTag.find_last_of("\\/")+1);	// remove path before tag

	// Find and read the appropiate database
	mDbName = compressTag.substr(0, compressTag.find_first_of("-"));
	Database *db = Database::RetrieveDatabase(mDbName);

	// Find and read native code table
	CodeTable *CTnative = FicMain::LoadLastCodeTable(mConfig, db, mCtDir);
	
	// Find k
	string ontwarTag = ontwarDir;
	if(ontwarTag.find_last_of("\\/") == ontwarTag.length()-1)			// remove trailing slash or backslash
		ontwarTag = ontwarTag.substr(0, ontwarTag.length()-1);
	ontwarTag = ontwarTag.substr(ontwarTag.find_last_of("\\/")+1);	// remove path before tag
	size_t pos = ontwarTag.find_first_of("K");
	if(pos == string::npos)
		throw string("OntwarKClusterer::ComponentAnalyser() -- Cannot find k in ontwarDir.");
	pos++;
	mK = atoi(ontwarTag.c_str() + pos);

	// Read CTs defining the clusters
	CodeTable **CTs = new CodeTable *[mK];
	for(uint32 i=0; i<mK; i++) {
		sprintf_s(temp, 100, "%d", i+1);
		filename = ontwarDir + "cluster" + temp + ".ct";
		CTs[i] = CodeTable::LoadCodeTable(filename, db);
	}

	// Let's init some variables
	uint32 **clusters = new uint32 *[mK];
	for(uint32 i=0; i<mK; i++) {
		clusters[i] = new uint32[db->GetNumRows()];
		CTs[i]->AddOneToEachUsageCount();
	}
	uint32 *numRows = new uint32[mK];
	ItemSet **dbRows = db->GetRows();
	mPreviousRowToCluster = new uint32[db->GetNumRows()];
	mPreviousRowToCluster[0] = mK; // make sure the clustering is marked 'changed' in the first run.

	// Find clustering
	ReassignRows(db, CTs, clusters, numRows);

	string summaryFile = Bass::GetWorkingDir() + "analysis.txt";
	FILE *file;
	if(fopen_s(&file, summaryFile.c_str(), "w+") != 0)
		throw string("Cannot write summary file.");

	fputs(compressTag.c_str(), file); fputs("\n", file);
	fputs(ontwarTag.c_str(), file); fputs("\n", file);

	// Perform analysis
	islist *nativeList = CTnative->GetItemSetList();
	islist **components = new islist *[mK];
	islist::iterator *compIters = new islist::iterator[mK];
	uint64 nativeCntSum = CTnative->GetCurStats().usgCountSum;
	uint64 *cntSums = new uint64[mK];
	for(uint32 i=0; i<mK; i++) {
		components[i] = CTs[i]->GetItemSetList();
		compIters[i] = components[i]->begin();
		cntSums[i] = CTs[i]->GetCurStats().usgCountSum;
	}

	fputs("#items\tNative\t", file);
	for(uint32 i=0; i<mK; i++) {
		sprintf_s(temp, 100, "CT%d\t", i+1);
		fputs(temp, file);
	}
	fputs("\n", file);

	for(islist::iterator nativeIter=nativeList->begin(); nativeIter!=nativeList->end();	nativeIter++) {
		sprintf_s(temp, 100, "%d\t%5.02f\t", (*nativeIter)->GetLength(), CalcCodeLength((*nativeIter)->GetUsageCount(), nativeCntSum));
		fputs(temp, file);
		for(uint32 k=0; k<mK; k++) {
			if((compIters[k] != components[k]->end()) && (*nativeIter)->Equals(*compIters[k])) {
				sprintf_s(temp, 100, "%7.02f\t", CalcCodeLength((*compIters[k])->GetUsageCount(), cntSums[k]));
				compIters[k]++;
			} else
				sprintf_s(temp, 100, "      -\t");
			fputs(temp, file);
		}
		fputs("\n", file);
	}

	for(uint32 i=0; i<mK; i++) {
		delete components[i];
	}
	delete[] compIters;
	delete[] components;
	delete[] cntSums;
	delete nativeList;

	// Cleanup
	fclose(file);
	for(uint32 i=0; i<mK; i++) {
		delete CTs[i];
		delete clusters[i];
	}
	delete[] CTs;
	delete[] clusters;
	delete[] numRows;
	delete[] mPreviousRowToCluster;
	mPreviousRowToCluster = NULL;
	delete CTnative;
	delete db;
}

RemovalInfoK OntwarKClusterer::FindMinSizeRemoval(Database *db, CodeTable **CTs, uint32 **clusters) {
	RemovalInfoK removal;
	removal.result = DOUBLE_MAX_VALUE;
	removal.ctElem = NULL;
	removal.ctIdx = 0;
	double size = 0.0;
	uint32 *numRows = new uint32[mK];
	//char temp[100];

	for(uint32 k=0; k<mK; k++) {
		//sprintf(temp, "!! CT%d", k+1);
		//LOG_MSG(string(temp));

		islist *ctList = CTs[k]->GetItemSetList();
		for(islist::iterator is=ctList->begin(); is!=ctList->end(); is++) {
			CTs[k]->Del(*is, false, false);

			size = 0.0;

			if(ReassignRows(db, CTs, clusters, numRows)) {		// Row assignment changed
				for(uint32 i=0; i<mK; i++) {
					CTs[i]->BackupStats();
					CTs[i]->CoverMaskedDB(CTs[i]->GetCurStats(), clusters[i], numRows[i], true);
					size += CTs[i]->GetCurSize();
					CTs[i]->RollbackCounts();
					CTs[i]->RollbackStats();
				}

			} else {											// Row assignment unchanged
				// Only CTs[k] changed
				if((*is)->GetUsageCount() == 1) {		// Cover unchanged, only countsum
					CTs[k]->BackupStats();
					CTs[k]->CalcTotalSize(CTs[k]->GetCurStats());
					for(uint32 i=0; i<mK; i++)
						size += CTs[i]->GetCurSize();
					CTs[k]->RollbackStats();

				} else {							// Cover als changes
					CTs[k]->BackupStats();
					CTs[k]->CoverMaskedDB(CTs[k]->GetCurStats(), clusters[k], numRows[k], true);
					for(uint32 i=0; i<mK; i++)
						size += CTs[i]->GetCurSize();
					CTs[k]->RollbackCounts();
					CTs[k]->RollbackStats();

				}
			}

			CTs[k]->RollbackLastDel();

			if(size < removal.result) {
				//LOG_MSG(string("\tBEST"));
				removal.result = size;
				removal.ctElem = *is;
				removal.ctIdx = k;
			}
		}
		delete ctList;
	}
	delete[] numRows;

	return removal;
}
void OntwarKClusterer::ApplyRemoval(RemovalInfoK &removal, Database *db, CodeTable **CTs, uint32 **clusters) {
	// First remove picked CT element
	char temp[100];
	sprintf(temp, " from CT%d to obtain result = %.03f.", removal.ctIdx+1, removal.result);
	LOG_MSG(string("Removing ") + removal.ctElem->ToString() + temp);
	uint32 cnt = removal.ctElem->GetUsageCount();
	CTs[removal.ctIdx]->Del(removal.ctElem, true, false);

	// Find correct clustering and then adjust counts and sizes
	uint32 *numRows = new uint32[mK];
	if(ReassignRowsAndStore(db, CTs, clusters, numRows)) {	// Row assignment changed
		for(uint32 i=0; i<mK; i++)
			CTs[i]->CoverMaskedDB(CTs[i]->GetCurStats(), clusters[i], numRows[i], true);

	} else {												// Row assignment unchanged
		// Only CTs[k] changed
		if(cnt == 1) {						// Cover unchanged, only countsum
			CTs[removal.ctIdx]->CalcTotalSize(CTs[removal.ctIdx]->GetCurStats());

		} else {							// Cover als changes
			CTs[removal.ctIdx]->CoverMaskedDB(CTs[removal.ctIdx]->GetCurStats(), clusters[removal.ctIdx], numRows[removal.ctIdx], true);

		}
	}

	// Some output
	string s;
	for(uint32 i=0; i<mK; i++) {
		sprintf(temp, "|c%d| = %d", i+1, numRows[i]);
		s += temp;
		if(i<mK-1)
			s+= ", ";
	}
	LOG_MSG(s);

	delete[] numRows;
}
bool OntwarKClusterer::ReassignRows(Database *db, CodeTable **CTs, uint32 **clusters, uint32 *numRows) {
	uint32 dbRows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	for(uint32 k=0; k<mK; k++)
		numRows[k] = 0;
	bool changed = false;

	// For each row, test in which cluster it fits best and assign it accordingly.
	double best, size;
	uint32 bestK;
	for(uint32 i=0; i<dbRows; i++) {
		best = DOUBLE_MAX_VALUE;
		for(uint32 k=0; k<mK; k++) {
			size = CTs[k]->CalcTransactionCodeLength(rows[i], db->GetStdLengths());
			if(size < best) {
				best = size;
				bestK = k;
			}
		}
		if(mPreviousRowToCluster == NULL || mPreviousRowToCluster[i] != bestK)
			changed = true;
		clusters[bestK][numRows[bestK]++] = i;
	}
	return changed;
}
bool OntwarKClusterer::ReassignRowsAndStore(Database *db, CodeTable **CTs, uint32 **clusters, uint32 *numRows) {
	uint32 dbRows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	for(uint32 k=0; k<mK; k++)
		numRows[k] = 0;
	bool changed = false;

	// For each row, test in which cluster it fits best and assign it accordingly.
	double best, size;
	uint32 bestK;
	for(uint32 i=0; i<dbRows; i++) {
		best = DOUBLE_MAX_VALUE;
		for(uint32 k=0; k<mK; k++) {
			size = CTs[k]->CalcTransactionCodeLength(rows[i], db->GetStdLengths());
			if(size < best) {
				best = size;
				bestK = k;
			}
		}
		if(mPreviousRowToCluster[i] != bestK)
			changed = true;
		mPreviousRowToCluster[i] = bestK;
		clusters[bestK][numRows[bestK]++] = i;
	}
	return changed;
}

#endif // BLOCK_CLUSTER
