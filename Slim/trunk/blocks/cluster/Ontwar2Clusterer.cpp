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

#include "Ontwar2Clusterer.h"

//uint32 gCTcounter;
//char gTemp[100];

Ontwar2Clusterer::Ontwar2Clusterer(Config *config, const string &ctDir) : Clusterer(config) {
	if(ctDir.length() > 0)
		mCtDir = ctDir;
	else
		mCtDir = FicMain::FindCodeTableFullDir(config->Read<string>("ctdir"));

	string typeStr = config->Read<string>("type");
	if(typeStr.compare("minsize") == 0)
		mType = CTOntwarMinSize;
	else if(typeStr.compare("maxminds") == 0)
		mType = CTOntwarMaxMinDS;
	else if(typeStr.compare("maxmaxds") == 0)
		mType = CTOntwarMaxMaxDS;
	else
		throw string("Ontwar2Clusterer -- Unknown ontwar type.");
}
Ontwar2Clusterer::~Ontwar2Clusterer() {
	
}

void Ontwar2Clusterer::Cluster() {
	LOG_MSG("Initiating Ontwar2 experiment with directory:");
	LOG_MSG("\t" + mCtDir);

	// Determine the compression tag of the original compression run
	string compressTag = mCtDir;
	if(compressTag.find_last_of("\\/") == compressTag.length()-1)			// remove trailing slash or backslash
		compressTag = compressTag.substr(0, compressTag.length()-1);
	compressTag = compressTag.substr(compressTag.find_last_of("\\/")+1);	// remove path before tag

	// Find and read the appropiate database
	mDbName = compressTag.substr(0, compressTag.find_first_of("-"));
	Database *db = Database::RetrieveDatabase(mDbName);

	// Find and read the last codetable in the specified ctDir
	// NOTE: We may want to try other code tables than the last (= lowest minsup)
	CodeTable *ct1 = FicMain::LoadLastCodeTable(mConfig, db, mCtDir);
	CodeTable *ct2 = FicMain::LoadLastCodeTable(mConfig, db, mCtDir); // hmm, ct->Clone() doesn't exist (yet)

	// Basic check
	if(ct1->GetCurNumSets() < 2)
		throw string("Ontwar2Clusterer::Cluster() -- Bummer, loaded codetable doesn't have enough elements do to anything useful.");

	// Let's init some variables
	uint32 *cluster1 = new uint32[db->GetNumRows()];
	uint32 *cluster2 = new uint32[db->GetNumRows()];
	double originalSize = ct1->CalcEncodedDbSize(db) + ct1->CalcCodeTableSize();
	ct1->AddOneToEachUsageCount(); ct2->AddOneToEachUsageCount();
	double startSize = ct1->CalcEncodedDbSize(db) + 2 * ct1->CalcCodeTableSize();
	uint32 originalNumSets = ct1->GetCurNumSets();
	RemovalInfo removal;
	double prevResult;

	// DS type loop
	if(mType == CTOntwarMaxMaxDS || mType == CTOntwarMaxMinDS) {
		prevResult = 0.0;
		removal = FindMaxDSRemoval(db, ct1, ct2, cluster1, cluster2);
		while(removal.result > prevResult) {
			ApplyRemoval(removal, db, ct1, ct2, cluster1, cluster2); // removes CT elem and adjusts counts
			ct1->AddOneToEachUsageCount(); ct2->AddOneToEachUsageCount();
			prevResult = removal.result;
			removal = FindMaxDSRemoval(db, ct1, ct2, cluster1, cluster2);
		}
	} else { // min size loop
		prevResult = DOUBLE_MAX_VALUE; //startSize;
		removal = FindMinSizeRemoval(db, ct1, ct2, cluster1, cluster2);
		while(removal.result < prevResult) {
			ApplyRemoval(removal, db, ct1, ct2, cluster1, cluster2); // removes CT elem and adjusts counts
			ct1->AddOneToEachUsageCount(); ct2->AddOneToEachUsageCount();
			prevResult = removal.result;
			removal = FindMinSizeRemoval(db, ct1, ct2, cluster1, cluster2);
		}
	}

	uint32 numRows1, numRows2;
	ReassignRows(db, ct1, cluster1, numRows1, ct2, cluster2, numRows2);
	ct1->CoverMaskedDB(ct1->GetCurStats(), cluster1, numRows1);
	ct2->CoverMaskedDB(ct2->GetCurStats(), cluster2, numRows2);
	double finalSize = ct1->GetCurSize() + ct2->GetCurSize();

	// Output some stuff
	char temp[500];
	sprintf(temp, "Clustered <> original:\t%.0f %s %.0f", finalSize, finalSize<originalSize?"<":">", originalSize);
	LOG_MSG(temp);
	sprintf(temp, "Clustered <> start:\t\t%.0f %s %.0f", finalSize, finalSize<startSize?"<":">", startSize);
	LOG_MSG(temp);
	sprintf(temp, "Original:  %d ctElems, %d rows, %.0f bits", originalNumSets, db->GetNumRows(), originalSize);
	LOG_MSG(temp);
	sprintf(temp, "Cluster 1: %d ctElems, %d rows, %.0f bits", ct1->GetCurNumSets(), numRows1, ct1->GetCurSize());
	LOG_MSG(temp);
	sprintf(temp, "Cluster 2: %d ctElems, %d rows, %.0f bits", ct2->GetCurNumSets(), numRows2, ct2->GetCurSize());
	LOG_MSG(temp);

	// Output dissimilarity between clusters
	if(mType == CTOntwarMaxMinDS || mType == CTOntwarMaxMaxDS) {
		sprintf(temp, "Dissimilarity: %.02f", prevResult);
		LOG_MSG(temp);
	}

	// Write resulting CTs to disk
	string ctFilename = Bass::GetWorkingDir() + "cluster1.ct";
	ct1->WriteToDisk(ctFilename);
	ctFilename = Bass::GetWorkingDir() + "cluster2.ct";
	ct2->WriteToDisk(ctFilename);

	// Compute 'confusion matrix' if we have some class info
	if(db->GetType() == DatabaseClassed) {
		LOG_MSG("Class distribution");

		ClassedDatabase *cdb = (ClassedDatabase*)db;
		uint16 *classDef = cdb->GetClassDefinition();
		uint32 numClasses = cdb->GetNumClasses();
		uint32 *classDist = new uint32[numClasses];

		// Classes
		string s = "\t\t";
		for(uint32 j=0; j<numClasses; j++) {
			sprintf(temp, "%d\t\t", classDef[j]);
			s += temp;
		}
		LOG_MSG(s);

		// Cluster 1
		for(uint32 j=0; j<numClasses; j++)
			classDist[j] = 0;
		for(uint32 i=0; i<numRows1; i++) {
			for(uint32 j=0; j<numClasses; j++)
				if(classDef[j] == cdb->GetClassLabel(cluster1[i])) {
					classDist[j]++;
					break;
				}
		}
		s = "c1\t\t";
		for(uint32 j=0; j<numClasses; j++) {
			sprintf(temp, "%d\t\t", classDist[j]);
			s += temp;
		}
		LOG_MSG(s);

		// Cluster 2
		for(uint32 j=0; j<numClasses; j++)
			classDist[j] = 0;
		for(uint32 i=0; i<numRows2; i++) {
			for(uint32 j=0; j<numClasses; j++)
				if(classDef[j] == cdb->GetClassLabel(cluster2[i])) {
					classDist[j]++;
					break;
				}
		}
		s = "c2\t\t";
		for(uint32 j=0; j<numClasses; j++) {
			sprintf(temp, "%d\t\t", classDist[j]);
			s += temp;
		}
		LOG_MSG(s);

		delete[] classDist;
	}

	// TODO
	// -- Compute prediction accuracy
	// -- WRITE RESULTING CLUSTERED DATABASES TO DISK

	// [Recurse for each cluster]

	// Cleanup
	delete[] cluster1;
	delete[] cluster2;
	delete ct1;
	delete ct2;
	delete db;
}

RemovalInfo Ontwar2Clusterer::FindMinSizeRemoval(Database *db, CodeTable *ct1, CodeTable *ct2, uint32 *c1, uint32 *c2) {
	RemovalInfo removal;
	removal.result = DOUBLE_MAX_VALUE;
	removal.ctElem = NULL;
	removal.fromCT2 = false;
	double size;
	uint32 numRows1, numRows2;
	//char temp[100];

	islist *ctList = ct1->GetItemSetList();
	for(islist::iterator is=ctList->begin(); is!=ctList->end(); is++) {
		/*// S
		sprintf(gTemp, "%d", gCTcounter++); 
		LOG_MSG(string("Writing CTs ") + gTemp);
		sprintf(gTemp, "%d", ct1->GetCurStats().countSum); 
		LOG_MSG(string("CT1 countsum = ") + gTemp);
		ct1->WriteToDisk(Bass::GetWorkingDir() + "ct1_" + gTemp + ".ct");
		ct2->WriteToDisk(Bass::GetWorkingDir() + "ct2_" + gTemp + ".ct");
		// E*/

		ct1->Del(*is, false, false);

		ReassignRows(db, ct1, c1, numRows1, ct2, c2, numRows2);
		if(numRows1 != 0 && numRows2 != 0) { // skip if one two clusters is empty
			//sprintf(temp, "|c1|=%d |c2|=%d", numRows1, numRows2);
			//LOG_MSG(string("\t\tCT1 Considering ") + (*is)->ToString() + " -- " + temp);
			ct1->BackupStats(); ct2->BackupStats();
			ct1->CoverMaskedDB(ct1->GetCurStats(), c1, numRows1, true);
			ct2->CoverMaskedDB(ct2->GetCurStats(), c2, numRows2, true);

			size = ct1->GetCurSize() + ct2->GetCurSize();
			if(size < removal.result) {
				//LOG_MSG(string("\tBEST"));
				removal.result = size;
				removal.ctElem = *is;
			}
			ct1->RollbackCounts();	ct2->RollbackCounts();
			ct1->RollbackStats();	ct2->RollbackStats();
		}
		ct1->RollbackLastDel();
	}
	delete ctList;
	ctList = ct2->GetItemSetList();
	for(islist::iterator is=ctList->begin(); is!=ctList->end(); is++) {
		ct2->Del(*is, false, false);
		ReassignRows(db, ct1, c1, numRows1, ct2, c2, numRows2);
		if(numRows1 != 0 && numRows2 != 0) { // skip if one two clusters is empty
			//sprintf(temp, "|c1|=%d |c2|=%d", numRows1, numRows2);
			//LOG_MSG(string("\t\tCT2 Considering ") + (*is)->ToString() + " -- " + temp);
			ct1->BackupStats(); ct2->BackupStats();
			ct1->CoverMaskedDB(ct1->GetCurStats(), c1, numRows1, true);
			ct2->CoverMaskedDB(ct2->GetCurStats(), c2, numRows2, true);
			size = ct1->GetCurSize() + ct2->GetCurSize();
			if(size < removal.result) {
				//LOG_MSG(string("\tBEST"));
				removal.result = size;
				removal.ctElem = *is;
				removal.fromCT2 = true;
			}
			ct1->RollbackCounts();	ct2->RollbackCounts();
			ct1->RollbackStats();	ct2->RollbackStats();
		}
		ct2->RollbackLastDel();
	}
	delete ctList;

	return removal;
}

RemovalInfo Ontwar2Clusterer::FindMaxDSRemoval(Database *db, CodeTable *ct1, CodeTable *ct2, uint32 *c1, uint32 *c2) {
	RemovalInfo removal;
	removal.result = 0.0;
	removal.ctElem = NULL;
	removal.fromCT2 = false;
	double ds;
	uint32 numRows1, numRows2;

	islist *ctList = ct1->GetItemSetList();
	for(islist::iterator is=ctList->begin(); is!=ctList->end(); is++) {
		ct1->Del(*is, false, false);
		ReassignRows(db, ct1, c1, numRows1, ct2, c2, numRows2);
		if(numRows1 != 0 && numRows2 != 0) { // skip if one two clusters is empty
			ct1->CoverMaskedDB(ct1->GetCurStats(), c1, numRows1); ct1->AddOneToEachUsageCount();
			ct2->CoverMaskedDB(ct2->GetCurStats(), c2, numRows2); ct2->AddOneToEachUsageCount();
			ds = ComputeClusterDissimilarity(db, ct1, ct2, c1, c2);
			if(ds > removal.result) {
				removal.result = ds;
				removal.ctElem = *is;
			}
			ct1->RollbackCounts();
			ct2->RollbackCounts();
		}
		ct1->RollbackLastDel();
	}
	delete ctList;
	ctList = ct2->GetItemSetList();
	for(islist::iterator is=ctList->begin(); is!=ctList->end(); is++) {
		ct2->Del(*is, false, false);
		ReassignRows(db, ct1, c1, numRows1, ct2, c2, numRows2);
		if(numRows1 != 0 && numRows2 != 0) { // skip if one two clusters is empty
			ct1->CoverMaskedDB(ct1->GetCurStats(), c1, numRows1); ct1->AddOneToEachUsageCount();
			ct2->CoverMaskedDB(ct2->GetCurStats(), c2, numRows2); ct2->AddOneToEachUsageCount();
			ds = ComputeClusterDissimilarity(db, ct1, ct2, c1, c2);
			if(ds > removal.result) {
				removal.result = ds;
				removal.ctElem = *is;
				removal.fromCT2 = true;
			}
			ct1->RollbackCounts();
			ct2->RollbackCounts();
		}
		ct2->RollbackLastDel();
	}
	delete ctList;

	return removal;
}

void Ontwar2Clusterer::ApplyRemoval(RemovalInfo &removal, Database *db, CodeTable *ct1, CodeTable *ct2, uint32 *c1, uint32 *c2) {
	// First remove picked CT element
	char temp[50];
	sprintf(temp, "%.03f", removal.result);
	if(removal.fromCT2) {			// remove from CT2
		LOG_MSG(string("Removing ") + removal.ctElem->ToString() + " from CT2 to obtain result = "+temp+".");
		ct2->Del(removal.ctElem, true, false);
	} else {						// remove from CT1
		LOG_MSG(string("Removing ") + removal.ctElem->ToString() + " from CT1 to obtain result = "+temp+".");
		ct1->Del(removal.ctElem, true, false);
	}

	// Find correct clustering
	uint32 numC1, numC2;
	ReassignRows(db, ct1, c1, numC1, ct2, c2, numC2);

	sprintf(temp, "|c1| = %d, |c2| = %d", numC1, numC2);
	LOG_MSG(string(temp));

	// Then adjust counts and sizes
	ct1->CoverMaskedDB(ct1->GetCurStats(), c1, numC1);
	ct2->CoverMaskedDB(ct2->GetCurStats(), c2, numC2);
}
void Ontwar2Clusterer::ReassignRows(Database *db, CodeTable *ct1, uint32 *c1, uint32 &numRows1, CodeTable *ct2, uint32 *c2, uint32 &numRows2) {
	uint32 numRows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	numRows1 = 0;
	numRows2 = 0;

	// For each row, test in which cluster it fits best and assign it accordingly.
	for(uint32 i=0; i<numRows; i++) {
		if(ct1->CalcTransactionCodeLength(rows[i]) <= ct2->CalcTransactionCodeLength(rows[i]))
			c1[numRows1++] = i;
		else
 			c2[numRows2++] = i;
	}
	return;
}
double Ontwar2Clusterer::ComputeClusterDissimilarity(Database *db, CodeTable *ct1, CodeTable *ct2, uint32 *c1, uint32 *c2) {
	uint32 c1idx = 0;
	uint32 numRows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	double sizeC1byCT1 = 0.0, sizeC1byCT2 = 0.0, sizeC2byCT2 = 0.0, sizeC2byCT1 = 0.0;

	// Foreach row in db
	for(uint32 i=0; i<numRows; i++) {
		if(c1[c1idx] == i) {	// Row assigned to cluster 1
			++c1idx;
			sizeC1byCT1 += ct1->CalcTransactionCodeLength(rows[i]) * rows[i]->GetUsageCount();
			sizeC1byCT2 += ct2->CalcTransactionCodeLength(rows[i]) * rows[i]->GetUsageCount();

		} else {				// Row assigned to cluster 2
			sizeC2byCT2 += ct2->CalcTransactionCodeLength(rows[i]) * rows[i]->GetUsageCount();
			sizeC2byCT1 += ct1->CalcTransactionCodeLength(rows[i]) * rows[i]->GetUsageCount();
		}
	}
	if(mType == CTOntwarMaxMaxDS)
		return max((sizeC1byCT2 - sizeC1byCT1) / sizeC1byCT1, (sizeC2byCT1 - sizeC2byCT2) / sizeC2byCT2);
	// else CTOntwarMaxMinDS
	return min((sizeC1byCT2 - sizeC1byCT1) / sizeC1byCT1, (sizeC2byCT1 - sizeC2byCT2) / sizeC2byCT2);
}

/*
OLD CODE (consider elements in CT order, decided on best action based on compressed size (not DS)
// ! Init ! 
// This is only one possible init..
// Remove first element from one of two code tables
ct1->Del(*is1, true, false);
++is1; ++is2; // always increment together
ReassignRows(db, ct1, cluster1, clusterInfo1, ct2, cluster2, clusterInfo2);
prevTotalSize = ct1->CalcCodeTableSize() + clusterInfo1.encodedSize + ct2->CalcCodeTableSize() + clusterInfo2.encodedSize;

// ! Loop !
// For each CT element, decide on the best action
while(is1 != isList1->end()) {
bestAction = 0; // keep previous if we do not find anything better
bestTotalSize = prevTotalSize;

// Compute ct1 \ ctElem
ct1->Del(*is1, false, false);
ReassignRows(db, ct1, cluster1, clusterInfo1, ct2, cluster2, clusterInfo2);
totalSize = ct1->CalcCodeTableSize() + clusterInfo1.encodedSize + ct2->CalcCodeTableSize() + clusterInfo2.encodedSize;
if(totalSize < bestTotalSize) {
bestTotalSize = totalSize;
bestAction = 1; // discard candidate from CT1
}
ct1->RollbackLastDel();

// Compute ct2 \ ctElem
ct2->Del(*is2, false, false);
ReassignRows(db, ct1, cluster1, clusterInfo1, ct2, cluster2, clusterInfo2);
totalSize = ct1->CalcCodeTableSize() + clusterInfo1.encodedSize + ct2->CalcCodeTableSize() + clusterInfo2.encodedSize;
if(totalSize < bestTotalSize) {
bestTotalSize = totalSize;
bestAction = 2; // discard candidate from CT2
}
ct2->RollbackLastDel();

// Execute best action
if(bestAction != 0) { // If == 0, do nothing: keep candidate in both code tables.
if(bestAction == 1) {
// Remove candidate from CT1
ct1->Del(*is1, true, false);

} else { // bestAction == 1
// Remove candidate from CT2
ct2->Del(*is2, true, false);

}
prevTotalSize = bestTotalSize;

// TODO: adjust counts of both CT1 and CT2
}

++is1; ++is2; // always increment together
}

// Loop does not automatically end with best clustering, so find it.
ReassignRows(db, ct1, cluster1, clusterInfo1, ct2, cluster2, clusterInfo2);
*/

#endif // BLOCK_CLUSTER
