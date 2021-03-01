#ifdef BLOCK_TAGGROUP
#include "../global.h"

#include <time.h>

// qtils
#include <Config.h>
#include <ArrayUtils.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>
#include <RandomUtils.h>
#include <logger/Log.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <itemstructs/CoverSet.h>

// croupier
#include <Croupier.h>

#include "../blocks/krimp/codetable/CodeTable.h"
#include "../FicConf.h"
#include "../FicMain.h"

#include "TagGroupTH.h"

TagGroupTH::TagGroupTH(Config *conf) : TaskHandler(conf){

}
TagGroupTH::~TagGroupTH() {
	// not my Config*
}

void TagGroupTH::HandleTask() {
	string command = mConfig->Read<string>("command");

	if(command.compare("findtaggroups") == 0)				FindTagGroups();
	else if(command.compare("emulateflickr") == 0)			EmulateFlickrClusters();
	else if(command.compare("gimmethegain") == 0)			ComputeGainForGivenFullCTandGroupDB();

	else	throw string("TagGroupTH :: Unable to handle task ") + command;
}

string TagGroupTH::BuildWorkingDir() {
	return mConfig->Read<string>("command") + "/";
}
void TagGroupTH::ComputeGainForGivenFullCTandGroupDB() {
	Database *fullDB = Database::RetrieveDatabase(mConfig->Read<string>("db"));
	CodeTable *fullCT = CodeTable::LoadCodeTable(Bass::GetDataDir() + mConfig->Read<string>("db") + ".ct", fullDB);
	Database *groupDB = Database::ReadDatabase(mConfig->Read<string>("group"), Bass::GetDataDir());
	CodeTable *groupCT = FicMain::ProvideCodeTable(groupDB, mConfig, true);

	double gain = fullCT->CalcEncodedDbSize(groupDB) - groupCT->GetCurStats().encDbSize;

	string fullpathoutput = Bass::GetExecDir() + "gain.txt";
	FILE* fp = fopen(fullpathoutput.c_str(),"w");
	fprintf(fp, "%f\n", gain);
	fclose(fp);
	delete fullCT;
	delete groupCT;
	delete fullDB;
	delete groupDB;
}

/////////////////////////////////// FindTagGroups ////////////////////////////////////////////////

void TagGroupTH::FindTagGroups() {
	char temp[500]; string s;
	string ctDir = FicMain::FindCodeTableFullDir(mConfig->Read<string>("ctdir"));
	string tgTag = "";

	{
		tgTag = "taggroups-" + TimeUtils::GetTimeStampString();
		mTGPath = ctDir + tgTag + "/";
		if(!FileUtils::DirectoryExists(mTGPath))
			FileUtils::CreateDir(mTGPath);

		Bass::SetWorkingDir(mTGPath);

		string confFile = mTGPath + mConfig->Read<string>("command") + ".conf";
		mConfig->WriteFile(confFile);
		sprintf_s(temp, 500, "** Experiment :: FindTagGroups\n * Tag:\t\t\t%s\n * Path:\t%s\n", tgTag.c_str(), mTGPath.c_str()); s = temp; LOG_MSG(s);
	}

	// Determine the compression tag of the original compression run
	string iscName, pruneStrategy, timestamp;
	FicMain::ParseCompressTag(ctDir, iscName, pruneStrategy, timestamp);
	mConfig->Set("iscname", iscName);
	mConfig->Set("prunestrategy", pruneStrategy);
	string patternType, dbName;
	{
		string settings;
		IscOrderType orderType;
		ItemSetCollection::ParseTag(iscName, dbName, patternType, settings, orderType);
	}
	if(patternType.compare("all") == 0)	// never use `all' for groups, takes too long --> switch to closed instead
		patternType = "closed";
	
	// Find and read the appropriate database
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	mFullDB = Database::RetrieveDatabase(dbName, dataType);
	mDB = new Database(mFullDB);

	// Find and read the last codetable in the specified ctDir (influenced by 'minsups' config directive!)
	mFullCT = FicMain::LoadLastCodeTable(mConfig, mFullDB, ctDir);
	mCT = FicMain::LoadLastCodeTable(mConfig, mDB, ctDir);
	mFullCT->CalcTotalSize(mFullCT->GetCurStats());
	mCT->CalcTotalSize(mCT->GetCurStats());

	// Settings
	float groupMinSup = mConfig->Read<float>("groupminsup");

	bool testAllCandidates = mConfig->Read<bool>("testallcandidates", true);
	uint32 minNumElems = mConfig->Read<uint32>("minnumelems", 2);
	bool addCTsize = mConfig->Read<bool>("addctsize", false);
	uint32 minRestDbSize = mConfig->Read<uint32>("minrestdbsize", 100);
	uint32 maxNumGroups = mConfig->Read<uint32>("maxnumgroups", 100);
	mExclusiveGrowing = mConfig->Read<bool>("exclusivegrowing", false); // 05.07.2011 new feature: when growing candidate groups, allow each CTelem in at most one candidate group!

	if(maxNumGroups == 0) maxNumGroups = UINT32_MAX_VALUE;

	LOG_MSG(string("\n -- Settings -- "));
	sprintf_s(temp, 500, " Group minsup = %.02f", groupMinSup); s = temp; LOG_MSG(s);
	sprintf_s(temp, 500, " Min # CT elems = %d", minNumElems); s = temp; LOG_MSG(s);
	LOG_MSG(string(""));
	sprintf_s(temp, 500, " Try all candidates = %s", testAllCandidates?"yes":"no"); s = temp; LOG_MSG(s);
	sprintf_s(temp, 500, " Add CT size = %s", addCTsize?"yes":"no"); s = temp; LOG_MSG(s);
	sprintf_s(temp, 500, " Max # groups = %d", maxNumGroups); s = temp; LOG_MSG(s);
	sprintf_s(temp, 500, " Min # transactions restDB = %d", minRestDbSize); s = temp; LOG_MSG(s);

	LOG_MSG(string(" -- "));

	sprintf_s(temp, 500, "\nBase compressibility = %f\n", mCT->GetCurSize()/mDB->GetStdSize()); s = temp; LOG_MSG(s);

	// Init member vars
	mGroupDB = NULL; mGroupCT = NULL;

	mGroupNumElems = 0, mGroupNumRows = 0, mGroupNumTrans = 0, mSkipNumElems = 0;
	mGroupElems = new uint32[0];
	mGroupRows = new uint32[0];

	mSkipElems = NULL;
	if(testAllCandidates)
		mSkipElems = new uint32[0];

	mGroupElemsUnion = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
	mGroupValues = new uint16[0];
	mGroupValCounts = new uint32[0];
	mGroupNumValues = 0;

	uint32 numGroupsFound = 0;

	mGroupConfig = new Config(mConfig);
	sprintf_s(temp, 500, "%s_grp-%s-%.5fd", dbName.c_str(), patternType.c_str(), groupMinSup);
	mGroupConfig->Set("iscname", temp);
	s = string("Itemset collection used for groups: ") + temp; LOG_MSG(s);
	LOG_MSG(Bass::GetExecName());
	if(addCTsize)
		LOG_MSG("!! Taking CT size into account for gain! (Not the default.)");
	else
		LOG_MSG("Using normal compression gain criterion.");

	// Open file
	s = mTGPath + tgTag + ".csv";
	fopen_s(&mFpCsv, s.c_str(), "w");
	fprintf(mFpCsv, "group;#CTelems;#rows;#trans;relGroupSize;compr;restCompr;comprGain;dbSize;ctSize\n");

	s = mTGPath + "groups.txt";
	fopen_s(&mFpGroups, s.c_str(), "w");
	string fimiFilename = mTGPath + "itemsets.fi";
	fopen_s(&mFpItemSets, fimiFilename.c_str(), "w");
	fprintf_s(mFpItemSets, "ficfi-1.0\n%d\n", mFullDB->HasBinSizes()?1:0);

	// While the rest of our database is large enough to find another group
	while(mDB->GetNumTransactions() > minRestDbSize && numGroupsFound < maxNumGroups) {
		mNumElems = mCT->GetCurNumSets();
		if(mNumElems < minNumElems) {
			LOG_MSG(string("\n ** < [minNumElems] CT elements left, no group possible, breaking. **\n"));
			if(mCover != NULL) {
				for(uint16 i=0; i<mNumElems; i++) {
					delete[] mCover[i];
					delete[] mOverlaps[i];
				}
			}
			delete[] mCover;		mCover = NULL;
			delete[] mCoverCounts;	mCoverCounts = NULL;
			delete[] mRowCounts;	mRowCounts = NULL;
			delete[] mNumOverlaps;	mNumOverlaps = NULL;
			delete[] mOverlaps;		mOverlaps = NULL;
			delete[] mCtElems;		mCtElems = NULL;
			delete[] mElemInCandGroup; mElemInCandGroup = NULL;
			break;
		}
		if(mNumElems > UINT16_MAX_VALUE)
			throw string("FindTagGroups -- |CT| > UINT16_MAX_VALUE, currently not allowed.");

		// Get CT elements
		mCtElems = mCT->GetItemSetArray();

		// Find overlaps
		mNumOverlaps = new uint16[mNumElems];
		mOverlaps = new uint16 *[mNumElems];
		uint32 totalOverlaps = 0;
		ItemSet *is;
		for(uint16 i=0; i<mNumElems; i++) {
			mNumOverlaps[i] = 0;
			mOverlaps[i] = new uint16[mNumElems];
			for(uint16 j=0; j<mNumElems; j++)
				mOverlaps[i][j] = 0;
		}
		for(uint16 i=0; i<mNumElems; i++) {
			is = mCtElems[i];
			for(uint16 j=i+1; j<mNumElems; j++) {
				if(is->Intersects(mCtElems[j])) {
					mOverlaps[i][mNumOverlaps[i]++] = j;
					mOverlaps[j][mNumOverlaps[j]++] = i;
					++totalOverlaps;
				}
			}
		}
		for(uint16 i=0; i<mNumElems; i++) { // Replace temp arrays with arrays of correct length (conserve some memory)
			uint32 num = mNumOverlaps[i];
			uint16 *tmp = new uint16[num];
			memcpy_s(tmp, num*sizeof(uint16), mOverlaps[i], num*sizeof(uint16));
			delete[] mOverlaps[i];
			mOverlaps[i] = tmp;
		}
		sprintf_s(temp, 500, "\n ------------ Attempting to find group %d ------------\n", numGroupsFound+1); s = temp; LOG_MSG(s);
		sprintf_s(temp, 500, " # %d CT elements\n # %d overlaps found (avg of %d/element)\n", mNumElems, totalOverlaps, 2*totalOverlaps/mNumElems); s = temp; LOG_MSG(s);

		// Compute & memorise cover
		mCover = new uint32 *[mNumElems];
		mCoverCounts = new uint32[mNumElems];
		mRowCounts = new uint32[mDB->GetNumRows()];
		for(uint32 i=0; i<mNumElems; i++) {
			mCoverCounts[i] = 0;
			mCover[i] = new uint32[mCtElems[i]->GetSupport()]; // upper bound for usage of CT element
			// actual row count is in mCoverCounts[] after cover computation
		}
		{
			CoverSet *coverSet = CoverSet::Create(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
			for(uint32 i=0; i<mDB->GetNumRows(); i++) {
				coverSet->InitSet(mDB->GetRow(i));
				mRowCounts[i] = mDB->GetRow(i)->GetUsageCount();
				for(uint32 j=0; j<mNumElems; j++)
					if(coverSet->Cover(mCtElems[j]))
						mCover[j][mCoverCounts[j]++] = i;
			}
			for(uint32 j=0; j<mNumElems; j++)
				if(mCoverCounts[j] > mCtElems[j]->GetSupport()) {
					sprintf_s(temp, 500, "actual usage %u > %u support (%s)", mCoverCounts[j], mCtElems[j]->GetSupport(), mCtElems[j]->ToString(true, true).c_str());
					s = temp;
					throw s;
				}
				delete coverSet;
		}
		LOG_MSG("Computed cover.");

		// Prepare for actual search
		double bestGain = 0.0;
		uint32 *bestGroupElems = new uint32[0]; // best group so far..
		uint32 bestGroupNumElems = 0;
		mElemInCandGroup = new bool[mNumElems];
		for(uint32 i=0; i<mNumElems; i++)
			mElemInCandGroup[i] = false;

		// Foreach CT element
		for(uint16 i=0; i<mNumElems; i++) {
			if(mExclusiveGrowing && mElemInCandGroup[i])
				continue;

			mGain = 0.0;
			mGroupDB = GetDeltaDB(i);						// Init a new group with a single CT element
			AddToGroup(i);
			mGroupCT = FicMain::CreateCodeTable(mGroupDB, mGroupConfig, true);
			//s = " ! " + mCtElems[i]->ToString(); LOG_MSG(s);

			float scoreTerm;
			uint32 best;
			while((best = FindBestCtElem(scoreTerm)) < mNumElems && mGroupNumRows < mDB->GetNumRows()) {	// Continue hillclimb while a BestCtElem can be found 
																												// and the group is not the entire database
				Database *deltaDB = GetDeltaDB(best);
				Database *newGroupDB = Database::Merge(mGroupDB, deltaDB);		// newGroup --> current candidate group
				CodeTable *newGroupCT = FicMain::CreateCodeTable(newGroupDB, mGroupConfig, true);

				if(mGroupCT->GetCurStats().encDbSize == 0) {
					LOG_WARNING(string("Countsum Laplace applied. (dbSize was 0)"));
					mGroupCT->GetCurStats().usgCountSum++;
					mGroupCT->CalcTotalSize(mGroupCT->GetCurStats());
				}

				double prevSize = mGroupCT->GetCurStats().encDbSize + mCT->CalcEncodedDbSize(deltaDB);

				if(newGroupCT->GetCurStats().encDbSize < prevSize) {					// Accept candidate group if new combined size is better than previous
					delete mGroupDB;
					delete mGroupCT;
					delete deltaDB;
					mGroupDB = newGroupDB;
					mGroupCT = newGroupCT;
					AddToGroup(best);
					//s = " + " + mCtElems[best]->ToString(); LOG_MSG(s);

				} else {															// Reject candidate group otherwise
					//s = " - " + mCtElems[best]->ToString(); LOG_MSG(s);
					delete newGroupCT;
					delete newGroupDB;
					delete deltaDB;
					if(testAllCandidates)
						RejectCtElem(best); // skip this one and proceed with the next
					else
						break;
				}
			}

			// Largest gain in compressed size of groupDB
			if(addCTsize)				// Non-default criterion
				mGain = mCT->GetCurStats().encCTSize + mCT->CalcEncodedDbSize(mGroupDB) - mGroupCT->GetCurSize();
			else						// Default 'compression gain'
				mGain = mCT->CalcEncodedDbSize(mGroupDB) - mGroupCT->GetCurStats().encDbSize;
			//sprintf_s(temp, 500, "gain = %f", mGain); LOG_MSG(string(temp));

			// Check whether group is best so far
			if(mGroupNumElems >= minNumElems && mGain > bestGain) {
				//sprintf_s(temp, 500, "accept! %f > %f, ctSize = %f, dbSize = %d", mGain, bestGain, mGroupCT->GetCurStats().ctSize, mGroupDB->GetNumRows()); LOG_MSG(string(temp));
				bestGain = mGain;
				delete[] bestGroupElems;
				bestGroupNumElems = mGroupNumElems;
				bestGroupElems = new uint32[bestGroupNumElems];
				memcpy(bestGroupElems, mGroupElems, mGroupNumElems * sizeof(uint32));
			}

			EmptyGroup();
			delete mGroupDB;
			delete mGroupCT;
		}

		if(bestGroupNumElems > 0) {											// Yes, we found a new group! So present it.
			delete mCtElems[bestGroupElems[0]]->Clone(); // seems useless, but delete db also cleans static mess..

			// Output group to screen & fimi candidate file
			uint32 support, numOccs;
			ItemSet *is;
			s = "Found group:"; LOG_MSG(s);
			for(uint32 i=0; i<bestGroupNumElems; i++) {
				AddToGroup(bestGroupElems[i]);
				is = mCtElems[bestGroupElems[i]];
				//s = " + " + is->ToString(); LOG_MSG(s);
				support = mFullDB->GetSupport(is, numOccs);
				s = is->ToString(false);
				if(mFullDB->HasBinSizes())
					fprintf_s(mFpItemSets, "%d: %s (%d,%d)\n", is->GetLength(), s.c_str(), support, numOccs);
				else
					fprintf_s(mFpItemSets, "%d: %s (%d)\n", is->GetLength(), s.c_str(), support);
			}

			// Get rest db and compute compressibility
			Database *restDB = GetRestDB();
			//restDB->Write("rest", mTGPath); // comment for normal operation
			CodeTable *restCT = FicMain::CreateCodeTable(restDB, mConfig, true);
			double restCompressibility = 1.0 - (restCT->GetCurSize() / restDB->GetStdSize());

			SaveGroup(numGroupsFound+1, restCompressibility, bestGain);

			sprintf_s(temp, 500, "\n * %d CT elems, %d/%d rows/transactions (%d/%d left in db)", mGroupNumElems, mGroupNumRows, mGroupNumTrans, mDB->GetNumRows() - mGroupNumRows, mDB->GetNumTransactions() - mGroupNumTrans); s = temp; LOG_MSG(s);
			sprintf_s(temp, 500, " * gain = %.02f, compr = %.02f\n", bestGain, mCompressibility); s = temp; LOG_MSG(s);

			delete mDB;
			mDB = restDB;
			delete mCT;
			mCT = restCT;
			++numGroupsFound;
		} else {
			LOG_MSG(string("No group found, stop here."));
		}

		delete[] bestGroupElems; bestGroupElems = NULL;

		// Cleanup
		for(uint16 i=0; i<mNumElems; i++) {
			delete[] mCover[i];
			delete[] mOverlaps[i];
		}
		delete[] mCover;		mCover = NULL;
		delete[] mCoverCounts;	mCoverCounts = NULL;
		delete[] mRowCounts;	mRowCounts = NULL;
		delete[] mNumOverlaps;	mNumOverlaps = NULL;
		delete[] mOverlaps;		mOverlaps = NULL;
		delete[] mCtElems;		mCtElems = NULL;

		if(bestGroupNumElems == 0) // stop when nothing was found
			break;

		EmptyGroup();
	}
	
	// Save rest DB and its CT
	mDB->Write("rest", mTGPath);
	sprintf_s(temp, 500, "%s/rest.ct", mTGPath.c_str());
	mCT->WriteToDisk(string(temp));

	// Close files
	fclose(mFpCsv); 		mFpCsv = NULL;
	fclose(mFpItemSets);	mFpItemSets = NULL;

	// Intermediate cleanup
	delete[] mGroupValues;  mGroupValues = NULL;
	delete[] mGroupValCounts; mGroupValCounts = NULL;
	delete[] mGroupElems;	mGroupElems = NULL;
	delete[] mGroupRows;	mGroupRows = NULL;
	if(mSkipElems != NULL) { delete[] mSkipElems;	mSkipElems = NULL; }
	delete mGroupElemsUnion;
	delete mCT;				mCT = NULL;
	delete mDB;				mDB = NULL;

	// Test picked CT elements as Krimp candidates on full database
	/*string ficIscFilename = "itemsets.isc";
	ItemSetCollection::ConvertIscFile(mTGPath, "itemsets.fi", mTGPath, ficIscFilename, mFullDB, FicIscFileType, LengthDescIscOrder);
	ItemSetCollection *isc = ItemSetCollection::OpenItemSetCollection(ficIscFilename, mTGPath, mFullDB);
	CodeTable *ct = FicMain::CreateCodeTable(mFullDB, mConfig, false, isc);
	sprintf_s(temp, 500, "\nCT;numSets;countSum;dbSize;ctSize;totalSize"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "ST;0;%" I64d ";%f;%f;%f", mFullDB->GetNumItems(), mFullDB->GetStdDbSize(), mFullDB->GetStdCtSize(), mFullDB->GetStdSize()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "cluster;%d;%" I64d ";%f;%f;%f;%.01f%%", ct->GetCurNumSets(), ct->GetCurStats().usgCountSum, ct->GetCurStats().encDbSize, ct->GetCurStats().encCTSize, ct->GetCurStats().encSize, 100*ct->GetCurStats().encSize/mFullDB->GetStdSize()); s = temp; LOG_MSG(s);
	sprintf_s(temp, 500, "original;%d;%" I64d ";%f;%f;%f;%.01f%%", mFullCT->GetCurNumSets(), mFullCT->GetCurStats().usgCountSum, mFullCT->GetCurStats().encDbSize, mFullCT->GetCurStats().encCTSize, mFullCT->GetCurStats().encSize, 100*mFullCT->GetCurStats().encSize/mFullDB->GetStdSize()); s = temp; LOG_MSG(s);
	LOG_MSG(string(""));
	delete ct;
	delete isc;*/

	// Close group files
	fclose(mFpGroups);	mFpGroups = NULL;

	// Final cleanup
	delete mGroupConfig;	mGroupConfig = NULL;
	delete mFullDB;
	delete mFullCT;
}


///////////////////////// FindTagGroups helper functions ////////////////////////////////


void TagGroupTH::SaveGroup(uint32 groupNum, double restCompressibility, double compressionGain) {
	char temp[500]; string s;

	// Compute support for each item in group (union of all its elements)
	uint32 numVals = mGroupElemsUnion->GetLength();
	uint16 *vals16 = mGroupElemsUnion->GetValues();
	uint32 *vals = new uint32[numVals];
	uint32 *freqs = new uint32[numVals];
	for(uint32 i=0; i<numVals; i++) {
		vals[i] = vals16[i];
		freqs[i] = 0;
		for(uint32 j=0; j<mGroupNumElems; j++)
			if(mCtElems[mGroupElems[j]]->IsItemInSet(vals16[i]))
				freqs[i] += mCtElems[mGroupElems[j]]->GetUsageCount();
	}
	delete[] vals16; vals16 = NULL;

	// Sort group items on supports
	ArrayUtils::MSortDesc(freqs, numVals, vals);

	// Output to log and group.txt
	sprintf_s(temp, 500, " ! "); s = temp;
	fprintf_s(mFpGroups, "%d. [%d] ", groupNum, mGroupNumTrans);
	for(uint32 i=0; i<numVals; i++) {
		sprintf_s(temp, 500, "%d ", vals[i]); s += temp;
		fprintf_s(mFpGroups, "%d (%d)", vals[i], freqs[i]);
		if(i+1 < numVals)
			fprintf_s(mFpGroups, " - ");
	}
	LOG_MSG(s);
	fprintf_s(mFpGroups, "\n");

	// Cleanup values and supports
	delete[] vals;
	delete[] freqs;

	// Construct group database
	ItemSet **iss = new ItemSet *[mGroupNumRows];
	for(uint32 i=0; i<mGroupNumRows; i++)
		iss[i] = mDB->GetRow(mGroupRows[i])->Clone();
	Database *groupDb = new Database(iss, mGroupNumRows, mDB->HasBinSizes(), mGroupNumTrans);
	groupDb->UseAlphabetFrom(mDB);
	groupDb->ComputeEssentials(); // only to compute maxSetLength

	// Write group database to file
	sprintf_s(temp, 500, "group%d", groupNum);
	groupDb->Write(temp, mTGPath);

	// Create codetable for group
	CodeTable *ct = FicMain::CreateCodeTable(groupDb, mGroupConfig, true);
	if(ct->GetCurStats().encDbSize == 0.0) {
		LOG_WARNING("ComputeGroupStats -- countSum Laplace (dbSize == 0)");
		ct->GetCurStats().usgCountSum++;
		ct->CalcTotalSize(ct->GetCurStats());
	}

	// Write CT to file
	sprintf_s(temp, 500, "%s/group%d.ct", mTGPath.c_str(), groupNum);
	ct->WriteToDisk(string(temp));

	// Compute group compressibility
	mCompressibility = 1.0 - (ct->GetCurSize() / groupDb->GetStdSize());

	// Save stats to CSV
	fprintf(mFpCsv, "%d;%d;%d;%d;%.1f;%f;%f;%f;%f;%f\n", groupNum, mGroupNumElems, mGroupNumRows, mGroupNumTrans, 100.0f*((float)mGroupNumTrans/mDB->GetNumTransactions()), mCompressibility, restCompressibility, compressionGain, ct->GetCurStats().encDbSize, ct->GetCurStats().encCTSize);

	// Cleanup
	delete ct;
	delete groupDb;
}

uint32 TagGroupTH::FindBestCtElem(float &scoreTerm) {		// Find best candidate CT elem to be added to the group
	uint32 numCandidates;
	uint32 *candidates = GetCandidates(numCandidates);
	if(numCandidates == 0) {
		delete[] candidates;
		return mNumElems;
	}

	uint32 bestIdx = 0;
	uint32 bestCnt = 0;
	scoreTerm = 0.0f;
	float score;
	ItemSet *cand;
	for(uint32 i=0; i<numCandidates; i++) {
		cand = mCtElems[candidates[i]];
		score = 0.0f;
		ItemSet *is = mGroupElemsUnion->Intersection(cand);
		uint16 *values = is->GetValues();
		for(uint32 k=0; k<is->GetLength(); k++)
			for(uint32 j=0; j<mGroupNumValues; j++)
				if(values[k] == mGroupValues[j]) {
					score += mGroupValCounts[j];
					break;
				}
		delete[] values;
		if(score > scoreTerm) {
			bestIdx = candidates[i];
			bestCnt = cand->GetUsageCount();
			scoreTerm = score;
		} else if(score == scoreTerm && cand->GetUsageCount() > bestCnt) {
			bestIdx = candidates[i];
			bestCnt = cand->GetUsageCount();
		}
		delete is;
	}

	delete[] candidates;
	return bestIdx;
}
uint32* TagGroupTH::GetCandidates(uint32 &numCandidates) {
	numCandidates = 0;
	for(uint32 i=0; i<mGroupNumElems; i++)
		numCandidates += mNumOverlaps[mGroupElems[i]];

	uint32 *cands = new uint32[numCandidates];
	uint32 idx = 0, num;
	for(uint32 i=0; i<mGroupNumElems; i++) { // take union of all overlap arrays
		num = mNumOverlaps[mGroupElems[i]];
		for(uint32 j=0; j<num; j++)
			cands[idx++] = mOverlaps[mGroupElems[i]][j];
	}

	if(numCandidates == 0)
		return cands;

	ArrayUtils::MSortAsc(cands, numCandidates); // sort ascending

	idx = 1;
	for(uint32 i=1; i<numCandidates; i++) { // remove doubles
		if(cands[i-1] != cands[i])
			cands[idx++] = cands[i];
	}
	numCandidates = idx;

	if(mExclusiveGrowing) {
		idx = 0;
		for(uint32 i=0; i<numCandidates; i++) { // remove ctElems that are already part of a candidate group
			if(!mElemInCandGroup[cands[i]])
				cands[idx++] = cands[i];
		}
		numCandidates = idx;
	}

	idx = 0;
	bool add;
	for(uint32 i=0; i<numCandidates; i++) { // remove elements already in the group
		add = true;
		for(uint32 j=0; j<mGroupNumElems; j++) {
			if(mGroupElems[j] == cands[i]) {
				add = false;
				break;
			}
		}
		if(add)
			cands[idx++] = cands[i];
	}
	numCandidates = idx;

	if(mSkipElems != NULL) {
		idx = 0;
		for(uint32 i=0; i<numCandidates; i++) { // remove skipped elements
			add = true;
			for(uint32 j=0; j<mSkipNumElems; j++) {
				if(mSkipElems[j] == cands[i]) {
					add = false;
					break;
				}
			}
			if(add)
				cands[idx++] = cands[i];
		}
		numCandidates = idx;
	}

	return cands;
}

void TagGroupTH::RejectCtElem(uint32 elemIdx) {						// Add current candidate CTelem to skip list
	uint32 *newElems = new uint32[mSkipNumElems+1];
	for(uint32 i=0; i<mSkipNumElems; i++)
		newElems[i] = mSkipElems[i];
	delete[] mSkipElems;
	mSkipElems = newElems;
	mSkipElems[mSkipNumElems++] = elemIdx;
}

Database *TagGroupTH::GetRestDB() {									// Obtain remainder database
	ItemSet **iss = new ItemSet *[mDB->GetNumRows() - mGroupNumRows];
	uint32 skipIdx = 0, numRows = 0, j;
	for(j=0; skipIdx < mGroupNumRows && j<mDB->GetNumRows(); j++) {
		if(j == mGroupRows[skipIdx]) {	// take transactions in db \ group
			++skipIdx;
			continue;
		}
		iss[numRows++] = mDB->GetRow(j)->Clone();
	}
	for(; j<mDB->GetNumRows(); j++)
		iss[numRows++] = mDB->GetRow(j)->Clone();
	if(numRows != mDB->GetNumRows() - mGroupNumRows)
		throw(string("Something's going wrong here."));
	Database *restDB = new Database(iss, numRows, mDB->HasBinSizes(), mDB->GetNumTransactions() - mGroupNumTrans);
	restDB->UseAlphabetFrom(mDB);
	restDB->ComputeEssentials();
	return restDB;
}
Database *TagGroupTH::GetDeltaDB(uint32 elemIdx) {				// Obtain delta database (newGroupDB - groupDB)
	ItemSet **iss = new ItemSet *[mCoverCounts[elemIdx]];
	uint32 groupIdx = 0, deltaIdx = 0, i = 0;
	uint32 transCount = 0;
	for(; i<mCoverCounts[elemIdx] && groupIdx < mGroupNumRows; i++) {
		while(groupIdx < mGroupNumRows && mGroupRows[groupIdx] < mCover[elemIdx][i])
			++groupIdx;
		if(groupIdx == mGroupNumRows)
			break;
		if(mGroupRows[groupIdx] == mCover[elemIdx][i])
			++groupIdx; // already in the group, skip
		else {
			iss[deltaIdx] = mDB->GetRow(mCover[elemIdx][i])->Clone();
			transCount += iss[deltaIdx++]->GetUsageCount();
		}
	}
	for(; i<mCoverCounts[elemIdx]; i++) {
		iss[deltaIdx] = mDB->GetRow(mCover[elemIdx][i])->Clone();
		transCount += iss[deltaIdx++]->GetUsageCount();
	}

	Database *deltaDb = new Database(iss, deltaIdx, mDB->HasBinSizes(), transCount);
	deltaDb->UseAlphabetFrom(mDB);
	deltaDb->ComputeEssentials(); // only to compute maxSetLength
	return deltaDb;
}

void TagGroupTH::AddToGroup(const uint32 ctElemIdx) {
	// Keep track of elems that are part of a candidate group
	mElemInCandGroup[ctElemIdx] = true;

	// Update CT element array
	uint32 *newElems = new uint32[mGroupNumElems+1];
	for(uint32 i=0; i<mGroupNumElems; i++)
		newElems[i] = mGroupElems[i];
	delete[] mGroupElems;
	mGroupElems = newElems;
	mGroupElems[mGroupNumElems++] = ctElemIdx;

	// Update row array
	uint32 *newRows = new uint32[mGroupNumRows + mCoverCounts[ctElemIdx]];
	uint32 groupIdx = 0, addIdx = 0, i = 0;
	for(; groupIdx<mGroupNumRows && addIdx<mCoverCounts[ctElemIdx]; i++) {
		if(mGroupRows[groupIdx] < mCover[ctElemIdx][addIdx]) {			// already in group, not in addition
			newRows[i] = mGroupRows[groupIdx++];
		} else if(mGroupRows[groupIdx] > mCover[ctElemIdx][addIdx]) {	// in addition, not in group
			newRows[i] = mCover[ctElemIdx][addIdx];
			mGroupNumTrans += mRowCounts[mCover[ctElemIdx][addIdx++]];
		} else {														// both in addition and in group
			newRows[i] = mGroupRows[groupIdx++];
			addIdx++; // #trans doesn't change
		}
	}
	while(groupIdx < mGroupNumRows)
		newRows[i++] = mGroupRows[groupIdx++];
	while(addIdx < mCoverCounts[ctElemIdx]) {
		newRows[i++] = mCover[ctElemIdx][addIdx];
		mGroupNumTrans += mRowCounts[mCover[ctElemIdx][addIdx++]];
	}
	delete[] mGroupRows;
	mGroupRows = newRows;
	mGroupNumRows = i;

	// Update union
	mGroupElemsUnion->Unite(mCtElems[ctElemIdx]);
	uint16 *values = mGroupElemsUnion->GetValues();
	uint32 numValues = mGroupElemsUnion->GetLength();
	uint32 *counts = new uint32[numValues];
	uint32 cnt = mCtElems[ctElemIdx]->GetUsageCount();
	for(uint32 i=0; i<numValues; i++)
		if(mCtElems[ctElemIdx]->IsItemInSet(values[i]))
			counts[i] = cnt;
		else
			counts[i] = 0;
	for(uint32 i=0; i<mGroupNumValues; i++)
		for(uint32 j=0; j<numValues; j++)
			if(values[j] == mGroupValues[i]) {
				counts[j] += mGroupValCounts[i];
				break;
			}
	mGroupNumValues = numValues;
	delete[] mGroupValues;		mGroupValues = values;
	delete[] mGroupValCounts;	mGroupValCounts = counts;
}

void TagGroupTH::EmptyGroup() {
	delete[] mGroupElems;
	delete[] mGroupRows;
	delete[] mGroupValues;
	delete[] mGroupValCounts;
	delete mGroupElemsUnion;
	mGroupNumElems = 0, mGroupNumRows = 0, mGroupNumTrans = 0;
	mGroupElems = new uint32[0];
	mGroupRows = new uint32[0];
	mGroupValues = new uint16[0];
	mGroupValCounts = new uint32[0];
	mGroupNumValues = 0;
	mGroupElemsUnion = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
	if(mSkipElems != NULL) {
		delete[] mSkipElems;
		mSkipElems = new uint32[0];
		mSkipNumElems = 0;
	}
}

void TagGroupTH::EmulateFlickrClusters() {
	string dbName = mConfig->Read<string>("dbname");
	string clusterSrc = mConfig->Read<string>("clustersrc");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	float groupMinSup = mConfig->Read<float>("groupminsup");
	float restMinSup = mConfig->Read<float>("restminsup");

	Database *db = Database::RetrieveDatabase(dbName + "6", dataType);
	string clusterFilename = dbName + "_clusters.dat";
	ItemSetCollection *isc = ItemSetCollection::OpenItemSetCollection(clusterFilename, clusterSrc, db, true, FimiIscFileType);

	uint32 numClusters = (uint32)isc->GetNumItemSets();
	ItemSet **clusterDefs = isc->GetLoadedItemSets();
	uint32 numRows = db->GetNumRows();
	ItemSet **rows = db->GetRows();
	ItemSet *is;
	
	uint32 *clusterSize = new uint32[numClusters];
	uint32 *assignments = new uint32[numRows];
	
	for(uint32 i=0; i<numClusters; i++)
		clusterSize[i] = 0;
	for(uint32 i=0; i<numRows; i++)
		assignments[i] = 0;

	uint32 best;
	for(uint32 i=0; i<numRows; i++) {
		best = 0;
		for(uint32 j=0; j<numClusters; j++) {
			is = rows[i]->Intersection(clusterDefs[j]);
			if(is->GetLength() > best) {
				if(assignments[i] != 0)
					--clusterSize[assignments[i]-1];
				assignments[i] = j+1;
				++clusterSize[j];
				best = is->GetLength();
			}/* else if(assignments[i] > 0 && is->GetLength() == best) {
				--clusterSize[assignments[i]-1];
				assignments[i] = 0;
			}*/
			delete is;
		}
	}

	char temp[500];
	sprintf_s(temp, 500, "%s_grp-closed-%.05fd", dbName.c_str(), groupMinSup);
	mConfig->Set("iscname", string(temp));
	sprintf_s(temp, 500, "%s_analysis.csv", dbName.c_str());
	string s = Bass::GetWorkingDir() + temp;
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	if(fp == NULL)
		throw string("Bummer. Could not open CSV for writing.");
	fprintf_s(fp, "%s\n", dbName.c_str());
	fprintf_s(fp, "db;numRows;compressibility;comprSize;stdSize\n");
	uint32 avgNumRows = 0;
	double avgCompressibility = 0.0f, compressibility;

	for(uint32 i=0; i<numClusters; i++) {
		ItemSet **iss = new ItemSet *[clusterSize[i]];
		uint32 idx = 0;
		for(uint32 j=0; j<numRows; j++)
			if(assignments[j] == i+1)
				iss[idx++] = rows[j]->Clone();
		if(idx != clusterSize[i])
			throw string("Flickr emulation cluster size wrrrronnnggg.");

		Database *cdb = new Database(iss, clusterSize[i], db->HasBinSizes());
		cdb->UseAlphabetFrom(db);	cdb->ComputeEssentials();
		sprintf_s(temp, 100, "_c%02d", i+1);
		Database::WriteDatabase(cdb, dbName + temp, Bass::GetWorkingDir());
		CodeTable *ct = FicMain::CreateCodeTable(cdb, mConfig, true);
		compressibility = ct->GetCurSize() / cdb->GetStdSize();
		fprintf_s(fp, "c%d;%d;%f;%f;%f\n", i+1, clusterSize[i], compressibility, ct->GetCurSize(), cdb->GetStdSize());
		avgNumRows += clusterSize[i];
		avgCompressibility += compressibility;
		delete ct;
		delete cdb;
	}
	avgNumRows /= numClusters;
	avgCompressibility /= numClusters;
	fprintf_s(fp, "cAvg;%d;%f\n\n", avgNumRows, avgCompressibility);

	sprintf_s(temp, 500, "%s_rest-closed-%.05fd", dbName.c_str(), restMinSup);
	mConfig->Set("iscname", string(temp));

	ItemSet **iss = new ItemSet *[numRows]; // upper bound, lui he...
	uint32 idx = 0;
	for(uint32 j=0; j<numRows; j++)
		if(assignments[j] == 0)
			iss[idx++] = rows[j]->Clone();
	Database *rdb = new Database(iss, idx, db->HasBinSizes());
	rdb->UseAlphabetFrom(db); 	rdb->ComputeEssentials();
	Database::WriteDatabase(rdb, dbName + "_rest", Bass::GetWorkingDir());
	CodeTable *ct = FicMain::CreateCodeTable(rdb, mConfig, true);
	compressibility = ct->GetCurSize() / rdb->GetStdSize();
	fprintf_s(fp, "rest;%d;%f;%f;%f\n", idx, compressibility, ct->GetCurSize(), rdb->GetStdSize());
	delete ct;
	delete rdb;

	fclose(fp);

	delete[] clusterSize;
	delete[] assignments;
	delete isc;
	delete db;
}

/*
Tried the merging phase at the end of FindTagGroups, to see if merging the found groups could be beneficial for compression. It wasn't.

// Merging phase
double oldSize;
Database **dbs = new Database *[numGroupsFound];
CodeTable **cts = new CodeTable *[numGroupsFound];
for(uint32 i=0; i<numGroupsFound; i++) {
sprintf_s(temp, 500, "group%d", i+1); s = temp;
dbs[i] = Database::ReadDatabase(temp, mSgPath, mFullDB->GetDataType());
s = mSgPath + s + ".ct";
cts[i] = CodeTable::LoadCodeTable(s, dbs[i]);
}
uint32 numGroupsWas = numGroupsFound;
double bestGain;
uint32 bestIdx1, bestIdx2;
Database *bestDb = NULL;
CodeTable *bestCt = NULL;
do {
bestGain = 0.0;
for(uint32 i=0; i<numGroupsFound-1; i++) {
for(uint32 j=i+1; j<numGroupsFound; j++) {
oldSize = cts[i]->GetCurStats().dbSize + cts[j]->GetCurStats().dbSize;
Database *newDb = Database::Merge(dbs[i], dbs[j]);
CodeTable *newCt = FicMain::CreateCodeTable(newDb, mGroupConfig);
if(oldSize - newCt->GetCurStats().dbSize > bestGain) {	// Best so far
bestGain = oldSize - newCt->GetCurStats().dbSize;
bestIdx1 = i;
bestIdx2 = j;
delete bestDb; bestDb = newDb;
delete bestCt; bestCt = newCt;
} else {
delete newDb;
delete newCt;
}
}
}
if(bestGain > 0.0) {
delete dbs[bestIdx1]; delete dbs[bestIdx2];
delete cts[bestIdx1]; delete cts[bestIdx2];
dbs[bestIdx1] = bestDb; bestDb = NULL;
cts[bestIdx1] = bestCt; bestCt = NULL;
delete ItemSet::CreateEmpty(mFullDB->GetDataType(), mFullDB->GetAlphabetSize()); // re-create static mess
sprintf_s(temp, 500, "Merging '%s' with '%s'.", groupDescs[bestIdx1]->ToString(false).c_str(), groupDescs[bestIdx2]->ToString(false).c_str()); s = temp; LOG_MSG(s);
groupDescs[bestIdx1]->Unite(groupDescs[bestIdx2]);
delete groupDescs[bestIdx2];
for(uint32 k=bestIdx2; k<numGroupsFound-1; k++) {
dbs[k] = dbs[k+1];
cts[k] = cts[k+1];
groupDescs[k] = groupDescs[k+1];
}
--numGroupsFound;
}
} while(bestGain > 0.0);
if(numGroupsFound < numGroupsWas) {
LOG_MSG("\n ------------ After merge phase ------------\n");
fprintf_s(mFpGroups, "\nAfter merge phase:\n");
sprintf_s(temp, 500, "# groups: %d (%d gone)\n\ngroup;numRows;gain;gain%%;compr", numGroupsFound, numGroupsWas-numGroupsFound); s = temp; LOG_MSG(s);
double gain, compr, origSize;
delete ItemSet::CreateEmpty(mFullDB->GetDataType(), mFullDB->GetAlphabetSize()); // re-create static mess
for(uint32 i=0; i<numGroupsFound; i++) {
sprintf_s(temp, 500, "merged%d", i+1);
dbs[i]->Write(temp, mSgPath);
sprintf_s(temp, 500, "%s/merged%d.ct", mSgPath.c_str(), i+1);
cts[i]->WriteToDisk(string(temp));

origSize = mFullCT->CalcEncodedDbSize(dbs[i]);
gain = origSize - cts[i]->GetCurStats().dbSize;
compr = 100 * cts[i]->GetCurSize() / dbs[i]->GetStdSize();
sprintf_s(temp, 500, "%d;%d;%.0f;%.1f;%.1f", i+1, dbs[i]->GetNumRows(), gain, 100*gain/origSize, compr); s = temp; LOG_MSG(s);
}
for(uint32 i=0; i<numGroupsFound; i++) {
uint32 numVals = groupDescs[i]->GetLength();
uint16 *vals16 = groupDescs[i]->GetValues();
uint32 *vals = new uint32[numVals];
uint32 *cnts = new uint32[numVals];
for(uint32 k=0; k<numVals; k++) {
vals[k] = vals16[k];
cnts[k] = 0;
for(uint32 j=0; j<dbs[i]->GetNumRows(); j++)
if(dbs[i]->GetRow(j)->IsItemInSet(vals16[k]))
cnts[k] += dbs[i]->GetRow(j)->GetCount();
}
delete[] vals16;
ArrayUtils::MSortUintArrayDesc(cnts, numVals, vals);
s = "\n";
fprintf_s(mFpGroups, "+ ");
for(uint32 k=0; k<numVals; k++) {
fprintf_s(mFpGroups, "%d ", vals[k]);
sprintf_s(temp, 500, "%d ", vals[k]);
s += temp;
}
fprintf_s(mFpGroups, "\n");
LOG_MSG(s); s = "";
for(uint32 k=0; k<numVals; k++) {
sprintf_s(temp, 500, "%.1f ", 100.0f*cnts[k]/dbs[i]->GetNumTransactions());
s += temp;
}
LOG_MSG(s);
delete[] vals; delete[] cnts;
}
} else {
LOG_MSG(string("No merging done."));
}

// Merge cleanup
for(uint32 i=0; i<numGroupsFound; i++) {
delete dbs[i];
delete cts[i];
}
delete[] dbs;
delete[] cts;
*/

#endif // BLOCK_TAGGROUP