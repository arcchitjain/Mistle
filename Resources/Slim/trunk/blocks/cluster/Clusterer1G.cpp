#ifdef BLOCK_CLUSTER

#if defined (_WINDOWS)
	#include <direct.h>
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <sys/stat.h>
#endif

#include "../../global.h"

// -- qtils
#include <Config.h>
#include <FileUtils.h>

// -- bass
#include <db/ClassedDatabase.h>
#include <db/DbFile.h>
#include <isc/ItemSetCollection.h>
#include <itemstructs/ItemSet.h>

#include "../../FicConf.h"
#include "../../FicMain.h"
#include "../../algo/Algo.h"

#include "Clusterer1G.h"

/* TODO
 - THINK: a way to gradually split from large to small, instead of repeatedly slicing off small parts of the database
 */

Clusterer1G::Clusterer1G(Config *config) : Clusterer(config) {

	// Read DB
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::ReadDatabase(dbName);
	Database::WriteDatabase(db, dbName, Bass::GetWorkingDir());
	delete db;

	mMaxLevel = mConfig->Read<uint32>("maxlevel");
	mSplitOnSingletons = mConfig->Read<uint32>("splitonsingletons",0) == 1;
	mLaplaceCorrect = mConfig->Read<uint32>("laplace",0) == 1;
	mUseDistance = mConfig->Read<uint32>("usedistance",1) == 1;

	throw string("Clusterer won't work right now, as it used to use FISGenerator which is now unfortunately diseased.");
}
Clusterer1G::~Clusterer1G() {
	//delete mFISGenerator;
}

void Clusterer1G::Cluster() {
	mNumCompresses = 0;
	mNumRowsCompressed = 0;
	mNumCandidatesDone = 0;

	uint32 ctSup = 0;
	mBaseDir = Bass::GetWorkingDir();

	// Start Log
	string logFilename = mBaseDir + "cluster.log";
	mLogFile = fopen(logFilename.c_str(), "w");
	fprintf(mLogFile, "*** Initiating Clustering\nBasedir: %s\nDatabase: %s\n", mBaseDir.c_str(), mDbName.c_str());
	fprintf(mLogFile, "Max level: %d\nSplit on singletons: %s\nLaplace: %s\nUse distance: %s\n\n", mMaxLevel, mSplitOnSingletons?"yes":"no", mLaplaceCorrect?"yes":"no", mUseDistance?"yes":"no");
	logFilename = mBaseDir + "split.log";
	mSplitFile = fopen(logFilename.c_str(), "w");

	{
		// Read DB
		Database *db = LoadDatabase(mBaseDir);

		// Generate FIS
		string iscFullPath = "";//mFISGenerator->Generate(mBaseDir, mDbName, db, ctSup);

		// Read ItemSetCollection
		ItemSetCollection *isc = ItemSetCollection::OpenItemSetCollection(iscFullPath, "", db);

		// Perform compression
		fprintf(mLogFile, " ** Compressing base db\n");
		Algo *algo = NULL;
		try {
			algo = Compress(mBaseDir, db, isc);
		} catch(char *s) {
			printf("*** Fatal error during compression: %s\n", s);
			fprintf(mLogFile, "*** Fatal error during compression: %s\n", s);
			delete isc;
			delete db;
			remove( iscFullPath.c_str() ); 	// Delete Frequent Itemsets
			fclose(mLogFile);
			mLogFile = NULL;
			return;
		}
		delete isc;
		delete db;
		delete algo;
		remove( iscFullPath.c_str() ); 	// Delete Frequent Itemsets
	}

	// Start recursive clustering
	string subDir = "";
	RecurseCluster(subDir, ctSup, 1);

	// Cleanup
	fprintf(mLogFile, "*** Finished Clustering\n\n");
	fprintf(mLogFile, " # compressions: %d\n", mNumCompresses);
	fprintf(mLogFile, " # rows/compression: %d\n", mNumRowsCompressed/mNumCompresses);
	fprintf(mLogFile, " # candidates tested: %" I64d "\n", mNumCandidatesDone);
	fclose(mLogFile);
	mLogFile = NULL;
	fclose(mSplitFile);
	mSplitFile = NULL;
}

void Clusterer1G::RecurseCluster(string subDir, uint32 ctSup, uint32 level) {
	fprintf(mLogFile, " ** Entering: '%s'\n", subDir.c_str());

	uint32 maxMinSupNeg = 0, maxMinSupPos = 0;
	double maxDistance = 0.0, maxSizeNN = 0.0, maxSizePP = 0.0, maxSizeNP = 0.0, maxSizePN = 0.0, maxSizePos = 0.0, maxSizeNeg = 0.0, parentSize = 0.0;
	double minSize = 0.0;
	uint32 maxId = 0;
	ItemSet *maxSet = NULL;

	{
		uint32 id = 0;
		string dir = mBaseDir + subDir;
		Database *db = LoadDatabase(dir);
		CodeTable *ct = LoadCodeTable(dir, db, ctSup);
		ct->CalcTotalSize(ct->GetCurStats());
        parentSize = ct->GetCurSize();

		if(db->GetType() == DatabaseClassed) {
			fprintf(mSplitFile, "[%d,%d,%.0f;", db->GetNumRows(), ct->GetCurNumSets(), parentSize);
			ClassedDatabase *cdb = (ClassedDatabase*)db;
			uint32 *freqs = cdb->GetClassSupports();
			for(uint32 i=0; i<cdb->GetNumClasses(); i++)
				fprintf(mSplitFile, " %d", freqs[i]);
			fprintf(mSplitFile, "]\n");
		} else
			fprintf(mSplitFile, "[%d,%d,%.0f]\n", db->GetNumRows(), ct->GetCurNumSets(), parentSize);

		if(mLaplaceCorrect) { // Laplace correct codetable (and sizes).
			ct->AddOneToEachUsageCount();
			ct->CalcTotalSize(ct->GetCurStats());
			parentSize = ct->GetCurSize();
		}
		minSize = parentSize;

		if(db->GetNumRows() > 1 && (ct->GetCurNumSets() > 0 || mSplitOnSingletons)) {
			// CodeTable sets
			double dist, size;
			Database *dbNeg, *dbPos;
			string dirNeg, dirPos;
			uint32 minSupNeg, minSupPos;
			Algo *algoNeg, *algoPos;
			double sizePos, sizeNeg;
			char buffer[40];

			islist *ctList = ct->GetItemSetList();
			islist *alphList = NULL;
			if(mSplitOnSingletons) {
				alphList = ct->GetSingletonList();
				islist::iterator ci, cend=alphList->end();
				for(ci=alphList->begin(); ci != cend; ++ci)
					ctList->push_back(*ci);
			}
			
			// Test split on each CT set
			for(islist::iterator ctiter = ctList->begin(); ctiter != ctList->end(); ++ctiter) {
				{
					uint32 freq = ((ItemSet*)(*ctiter))->GetSupport();
					if(freq == 0) {
						fprintf(mLogFile, "  * Skipping %s, occurs in not even a single row!\n", (ItemSet*)(*ctiter)->ToString().c_str());
						continue;
//					} else if(freq < 10 || freq > db->GetNumRows() - 10) {
//						fprintf(mLogFile, "  * Skipping %s, occurs in less than 10 rows (or more than |db|-10).\n", (ItemSet*)(*ctiter)->ToString().c_str());
//						continue;
					} else if(freq == db->GetNumRows()) {
						fprintf(mLogFile, "  * Skipping %s, occurs in every row!\n", (ItemSet*)(*ctiter)->ToString().c_str());
						continue;
					}
				}
				fprintf(mLogFile, "  * Testing split on: %s\n", (ItemSet*)(*ctiter)->ToString().c_str());

#if _MSC_VER >= 1400
				_itoa_s(id, buffer, 40, 10);
#elif _MSC_VER >= 1310
				itoa(id, buffer, 10);
#endif

				// Split database
				Database **dbs = db->SplitOnItemSet((ItemSet*)(*ctiter));
				dbNeg = dbs[0]; dbPos = dbs[1];

				// Write databases
				dirNeg = dir + buffer + "-/";
#if defined (_WINDOWS)
				_mkdir(dirNeg.c_str());
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
         mkdir(dirNeg.c_str(), 0777); 
#endif
				Database::WriteDatabase(dbNeg, dirNeg);
				dirPos = dir + buffer + "+/";
#if defined (_WINDOWS)
				_mkdir(dirPos.c_str());
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
         mkdir(dirPos.c_str(), 0777); 
#endif
				Database::WriteDatabase(dbPos, dirPos);

				{
					// Compress -
					fprintf(mLogFile, "   - Compressing set%d-\n", id);
					string iscFullPath = ""; minSupNeg = 1;//mFISGenerator->Generate(dirNeg, mDbName, dbNeg, minSupNeg);
					ItemSetCollection *isc = ItemSetCollection::OpenItemSetCollection(iscFullPath, "", dbNeg);
					try {
						algoNeg = Compress(dirNeg, dbNeg, isc);
					} catch(char *s) {
						printf("*** Fatal error during compression: %s\n", s); return;
					}
					delete isc;
					remove( iscFullPath.c_str() ); 	// Delete Frequent Itemsets
				}
				{
					// Compress +
					fprintf(mLogFile, "   - Compressing set%d+\n", id);
					string iscFullPath = ""; minSupPos = 1; //mFISGenerator->Generate(dirPos, mDbName, dbPos, minSupPos);
					ItemSetCollection *isc = ItemSetCollection::OpenItemSetCollection(iscFullPath, "", dbPos);
					try {
						algoPos = Compress(dirPos, dbPos, isc);
					} catch(char *s) {
						printf("*** Fatal error during compression: %s\n", s); return;
					}
					delete isc;
					remove( iscFullPath.c_str() ); 	// Delete Frequent Itemsets
				}

				if(mLaplaceCorrect) { // Laplace correction
					CodeTable *ct = algoNeg->GetCodeTable();
					ct->AddOneToEachUsageCount();
					ct->CalcTotalSize(ct->GetCurStats());
					ct = algoPos->GetCodeTable();
					ct->AddOneToEachUsageCount();
					ct->CalcTotalSize(ct->GetCurStats());
				}

				// check whether L(a,b) > L(a) + L(b)
				if(parentSize > algoNeg->GetCodeTable()->GetCurSize() + algoPos->GetCodeTable()->GetCurSize()) {
					fprintf(mLogFile, "   - Candidate!\n");

					CodeTable *ctNeg = algoNeg->GetCodeTable();
					sizeNeg = ctNeg->GetCurSize();

					CodeTable *ctPos = algoPos->GetCodeTable();
					sizePos = ctPos->GetCurSize();

					if(mUseDistance) { // -- Use distance
						if(!mLaplaceCorrect)
							ctNeg->AddOneToEachUsageCount();
						if(!mLaplaceCorrect)
							ctPos->AddOneToEachUsageCount();

						// Compute distance
						double sizeNN = ctNeg->CalcEncodedDbSize(dbNeg);
						double sizePP = ctPos->CalcEncodedDbSize(dbPos);
						double sizePN = ctPos->CalcEncodedDbSize(dbNeg);
						double sizeNP = ctNeg->CalcEncodedDbSize(dbPos);

						dist = max((sizeNP - sizePP) / sizePP, (sizePN - sizeNN) / sizeNN);
						fprintf(mLogFile, "   - Distance: %f.2\n", dist);

						if(dist > maxDistance) {
							fprintf(mLogFile, "   - New maximum distance!\n");
							maxId = id;
							maxMinSupPos = minSupPos;
							maxMinSupNeg = minSupNeg;
							maxSet = ((ItemSet*)(*ctiter));
							maxSizeNeg = sizeNeg;
							maxSizePos = sizePos;

							maxDistance = dist;
							maxSizeNN = sizeNN;
							maxSizePP = sizePP;
							maxSizeNP = sizeNP;
							maxSizePN = sizePN;
						}
					} else { // -- Minimize size only
						size = algoNeg->GetCodeTable()->GetCurSize() + algoPos->GetCodeTable()->GetCurSize();

						if(size < minSize) {
							fprintf(mLogFile, "   - New minimum size: %f.2\n", size);
							maxId = id;
							maxMinSupPos = minSupPos;
							maxMinSupNeg = minSupNeg;
							maxSet = ((ItemSet*)(*ctiter));
							maxSizeNeg = sizeNeg;
							maxSizePos = sizePos;

							minSize = size;
						}
					}
				}

				delete algoNeg; algoNeg = NULL;
				delete algoPos; algoPos = NULL;
				delete dbs[0];
				delete dbs[1];
				delete[] dbs;
				id++;
			}
			if(maxSet != NULL)
				maxSet = maxSet->Clone();

			delete ctList;
			if(mSplitOnSingletons) {
				for(islist::iterator iter = alphList->begin(); iter != alphList->end(); ++iter)
					delete (ItemSet*)(*iter);
				delete alphList;
			}
		}

		// Cleanup
		delete ct;
		delete db;
	}

	// Recurse
	if((mUseDistance && maxDistance > 0.0f) || (!mUseDistance && minSize < parentSize)) {
		for(uint32 i=1; i<level; i++)
			fprintf(mSplitFile, "\t");
		if(mUseDistance)
			fprintf(mSplitFile, "%d: %s [%.02f; nn%.02f,pn%.0f,pp%.0f,np%.0f; %.0f>%.0f]\n", maxId, maxSet->ToString().c_str(), maxDistance, maxSizeNN, maxSizePN, maxSizePP, maxSizeNP, parentSize, maxSizeNeg+maxSizePos);
		else
			fprintf(mSplitFile, "%d: %s [%.0f>%.0f]\n", maxId, maxSet->ToString().c_str(), parentSize, minSize);

		bool recurse = true;
		if(mMaxLevel != 0 && level == mMaxLevel) {
			recurse = false;
			fprintf(mLogFile, " ** Split on set%d (%s), but further recursion not allowed (level = %d).\n", maxId, maxSet->ToString().c_str(), level);
		} else
			fprintf(mLogFile, " ** Splitting on set%d (%s) and recursing.\n", maxId, maxSet->ToString().c_str());

		char buffer[10];
#if _MSC_VER >= 1400
		_itoa_s(maxId, buffer, 10, 10);
#elif _MSC_VER >= 1310
		itoa(maxId, buffer, 10);
#endif

		// Negative DB
		for(uint32 i=1; i<level; i++)
			fprintf(mSplitFile, "\t");
		fprintf(mSplitFile, "- ");

		string newDir = subDir + buffer + "-/";
		if(recurse)
			RecurseCluster(newDir, maxMinSupNeg, level+1);
		else {
			Database *db = LoadDatabase(mBaseDir + newDir);
			CodeTable *ct = LoadCodeTable(mBaseDir + newDir, db, maxMinSupNeg);
			ct->CalcTotalSize(ct->GetCurStats());
			if(db->GetType() == DatabaseClassed) {
				fprintf(mSplitFile, "[%d,%d,%.0f;", db->GetNumRows(), ct->GetCurNumSets(), ct->GetCurSize());
				ClassedDatabase *cdb = (ClassedDatabase*)db;
				uint32 *freqs = cdb->GetClassSupports();
				for(uint32 i=0; i<cdb->GetNumClasses(); i++)
					fprintf(mSplitFile, " %d", freqs[i]);
				fprintf(mSplitFile, "] max depth!\n");
			} else
				fprintf(mSplitFile, "[%d,%d,%.0f] max depth!\n", db->GetNumRows(), ct->GetCurNumSets(), ct->GetCurSize());
			delete db;
			delete ct;
		}

		// Positive DB
		for(uint32 i=1; i<level; i++)
			fprintf(mSplitFile, "\t");
		fprintf(mSplitFile, "+ ");

		newDir = subDir + buffer + "+/";
		if(recurse)
			RecurseCluster(newDir, maxMinSupPos, level+1);
		else {
			Database *db = LoadDatabase(mBaseDir + newDir);
			CodeTable *ct = LoadCodeTable(mBaseDir + newDir, db, maxMinSupPos);
			ct->CalcTotalSize(ct->GetCurStats());
			if(db->GetType() == DatabaseClassed) {
				fprintf(mSplitFile, "[%d,%d,%.0f;", db->GetNumRows(), ct->GetCurNumSets(), ct->GetCurSize());
				ClassedDatabase *cdb = (ClassedDatabase*)db;
				uint32 *freqs = cdb->GetClassSupports();
				for(uint32 i=0; i<cdb->GetNumClasses(); i++)
					fprintf(mSplitFile, " %d", freqs[i]);
				fprintf(mSplitFile, "] max depth!\n");
			} else
				fprintf(mSplitFile, "[%d,%d,%.0f] max depth!\n", db->GetNumRows(), ct->GetCurNumSets(), ct->GetCurSize());
			delete db;
			delete ct;
		}

	} else {
		fprintf(mLogFile, " ** No split!\n");
	}

	delete maxSet;
	fprintf(mLogFile, " ** Leaving: '%s'\n", subDir.c_str());
}

CodeTable *Clusterer1G::LoadCodeTable(const string &dir, Database *db, uint32 ctSup) {
	char sCtSup[40];
#if _MSC_VER >= 1400
	_itoa_s(ctSup, sCtSup, 40, 10);
#elif _MSC_VER >= 1310
	itoa(ctSup, sCtSup, 10);
#endif
	string ctFilename = dir + "/ct-" + mDbName + "-" + sCtSup + ".ct";
	if(!FileUtils::FileExists(ctFilename))
		return NULL;
	CodeTable *ct = CodeTable::Create("coverfull", db->GetDataType());
	ct->UseThisStuff(db, db->GetDataType(), InitCTEmpty, 0, 1);
	ct->ReadFromDisk(ctFilename, true);
	return ct;
}

Database *Clusterer1G::LoadDatabase(const string &dir) {
	DbFile *dbFile = DbFile::Create(DbFileTypeFic);
	dbFile->SetPreferredDataType(ItemSet::StringToType(mConfig->Read<string>("dbindatatype")));

	string fullname = dir + mDbName + ".db";
	Database *db = NULL;
	try {
		db = dbFile->Read(fullname);
	} catch(char *) {
	} catch(string) {
	}
	delete dbFile;
	if(!db)
		throw string("Could not read input database: ") + fullname;
	return db;
}
void Clusterer1G::WriteDatabase(const string &dir, Database *db) {
	DbFileType outDbType = mConfig->KeyExists("dboutencoding") ? DbFile::StringToType(mConfig->Read<string>("dboutencoding")) : DbFileTypeFic;
	if(outDbType == DbFileTypeNone)
		throw string("Unknown database encoding ('dbOutEncoding').");
	DbFile *dbFile = DbFile::Create(outDbType);
	string outDbFullPath = dir + mDbName + "." + (mConfig->KeyExists("dboutext") ? mConfig->Read<string>("dboutext") : "db");
	if(!dbFile->Write(db, outDbFullPath)) {
		delete dbFile;
		throw string("Could not write database!\n");
	}
	delete dbFile;
}
Algo *Clusterer1G::Compress(string &outDir, Database *db, ItemSetCollection *isc) {
	mNumCompresses++;
	mNumRowsCompressed += db->GetNumRows();
	mNumCandidatesDone += isc->GetNumItemSets();

	// Build Algo
	Algo *algo = Algo::CreateAlgo(mConfig->Read<string>("algo"), db->GetDataType());
	if(algo == NULL)
		throw string("Unknown algorithm.");
	ECHO(1, printf("** Compression :: \n"));
	ECHO(3, printf(" * Initiating...\n"));
	ECHO(1, printf(" * Algorithm:\t\t%s\n", mConfig->Read<string>("algo").c_str()));

	algo->SetOutputDir(outDir);
	algo->SetTag(mDbName);

	// Further configure Algo
	CTInitType	ctInit    = mConfig->Read<uint32>("ctinit") == 0 ? InitCTEmpty : InitCTAlpha;
	{
		PruneStrategy	pruneStrategy = (PruneStrategy)(mConfig->Read<uint32>("prunestrategy"));
		uint32		reportSup = mConfig->Read<uint32>("reportsup", 0);
		uint32		reportCnd = mConfig->Read<uint32>("reportcnd", 0);
		bool		reportAcc = mConfig->Read<bool>("reportacc", false);
		ReportType	reportSupWhat = Algo::StringToReportType(mConfig->Read<string>("reportsupwhat", "all"));
		ReportType	reportCndWhat = Algo::StringToReportType(mConfig->Read<string>("reportcndwhat", "all"));
		ReportType	reportAccWhat = Algo::StringToReportType(mConfig->Read<string>("reportaccwhat", "all"));

		algo->UseThisStuff(db, isc, db->GetDataType(), ctInit, true);
		algo->SetPruneStrategy(pruneStrategy);
		algo->SetReporting(reportSup, reportCnd, reportAcc);
		algo->SetReportStyle(reportSupWhat,reportCndWhat,reportAccWhat);


		// To screen
		ECHO(1, printf(" * Pruning:\t\t"));
		switch(pruneStrategy) {
		case NoPruneStrategy:
			ECHO(1, printf("none\n"));
			break;
		case PreAcceptPruneStrategy:
			ECHO(1, printf("pre-decide\n"));
			break;
		case PostAcceptPruneStrategy:
			ECHO(1, printf("post-decide\n"));
			break;
		case PostEstimationPruning:
			ECHO(1, printf("post-estimation-decide\n"));
			break;
		default:
			ECHO(1, printf("?? %d ??\n", pruneStrategy));
			break;
		}
	}

	// Write Run-Time Info file
	char rti[100];
	sprintf(rti, "__%s %s %s %s", FicMain::ficVersion.c_str(), algo->GetShortName().c_str(), mConfig->Read<string>("dbindatatype").c_str(), algo->GetShortPruneStrategyName().c_str());
	string rtiFile = algo->GetOutDir() + "/" + rti;
	FILE *rtifp = fopen(rtiFile.c_str(), "w");
	fclose(rtifp); /* check success ? */

	// Put algo to work
	CodeTable* ct = algo->DoeJeDing(0, 0);
	throw string("ct en db moeten nu wel apart gedelete worden...");
	ECHO(3, printf("** Finished compression.\n"));

	return algo;
}

#endif // BLOCK_CLUSTER
