#include "../global.h"

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <ArrayUtils.h>
#include <StringUtils.h>
#include <RandomUtils.h>
#include <TimeUtils.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <isc/ItemTranslator.h>

// croupier
#include <Croupier.h>
#include <afopt/AfoptCroupier.h>
#include <tiles/TileMiner.h>

// tmp test stuff
#include "../blocks/krimp/codetable/CodeTable.h"
#include "../blocks/krimp/codetable/coverpartial/CPAOCodeTable.h"
#include "../FicMain.h"



#include "TestTH.h"

TestTH::TestTH(Config *conf) : TaskHandler(conf){

}
TestTH::~TestTH() {
	// not my Config*
}

void TestTH::HandleTask() {
	string command = mConfig->Read<string>("command");

			if(command.compare("testshizzle") == 0)			TestShizzle();
	else	if(command.compare("arnok") == 0)				ArnoKPatterns();
	else	if(command.compare("dealer") == 0)				Dealer();
	else	if(command.compare("minefis") == 0)				TestMineFrequentItemSets();
	else	if(command.compare("minemfis") == 0)			TestMineMostFrequentItemsets();
	else	if(command.compare("robust") == 0)				TestCandidateSetRobustness();
	else	if(command.compare("minekrimp") == 0)			TestKrimpOnTheFly();

#ifdef BLOCK_TILING
	else	if(command.compare("tilesmem") == 0)			TestMineTilesToMemory();
#endif

	else	throw string(__FUNCTION__) + ": Unable to handle task `" + command + "`";
}

string TestTH::BuildWorkingDir() {
	//char s[100];
	//_i64toa_s(Logger::GetCommandStartTime(), s, 100, 10);
	return mConfig->Read<string>("command") + "/";
}

void TestTH::TestCandidateSetRobustness() {
	Bass::SetMineToMemory(true);
	string dbName = mConfig->Read<string>("dbname");
	string iscTag = mConfig->Read<string>("iscname");
	string prune = mConfig->Read<string>("pruneStrategy");

	Database *db = Database::RetrieveDatabase(dbName);

	CodeTable *ct = FicMain::ProvideCodeTable(db, mConfig, false);
	double tes = ct->GetCurSize();

	IscFileType storeIscFileType = mConfig->KeyExists("iscStoreType") ? IscFile::ExtToType(mConfig->Read<string>("iscStoreType")) : BinaryFicIscFileType;

	uint32 ctLength = ct->GetCurNumSets();
	islist *ctList = ct->GetItemSetList();
	ItemSet **ctArray = ItemSet::ConvertItemSetListToArray(ctList);

	FILE* fp = NULL;
	// Ignore All of CT
	string test = mConfig->Read<string>("test", "noct");
	if(test.compare("noct")==0) {
		string outFullpath = Bass::GetWorkingDir() + "csr-noct-" + iscTag + "-" + prune + ".txt";
		fp = fopen( outFullpath.c_str(), "w");

		bool beenMined = false;
		ItemSetCollection *isc = FicMain::ProvideItemSetCollection(iscTag, db, beenMined, true, true, storeIscFileType);

		uint64 numSets = isc->GetNumItemSets();
		ItemSet::SortItemSetArray(ctArray, ctLength, isc->GetIscOrder()); 
		isc->RemoveOrderedSubset(ctArray, ctLength);
		uint64 numASets = isc->GetNumItemSets();
		CodeTable *act = FicMain::CreateCodeTable(db, mConfig, false, isc);
		double ates = act->GetCurSize();

		fprintf(fp, "ST\t%.2f\t%d\t0\t0\n", db->GetStdDbSize(), db->GetAlphabetSize());
		CoverStats ctstats = ct->GetCurStats();
		fprintf(fp, "CT\t%.2f\t%d\t%d\t%d\n", tes, ctstats.alphItemsUsed, ct->GetCurNumSets(), ctstats.numSetsUsed);	// expand with #ab,#sets, etc
		CoverStats actstats = act->GetCurStats();
		fprintf(fp, "CT*\t%.2f\t%d\t%d\t%d\n", ates, actstats.alphItemsUsed, act->GetCurNumSets(), actstats.numSetsUsed);
		printf("%.2f and %.2f with a absolute difference of %.2f\n", tes, ates, max(tes,ates)-min(tes,ates));

		delete act;
	} else if(test.compare("noctelem")==0) {
		string outFullpath = Bass::GetWorkingDir() + "csr-noctelem-" + iscTag + "-" + prune + ".txt";
		fp = fopen( outFullpath.c_str(), "w");

		fprintf(fp, "ST\t%.2f\t%d\t0\t0\n", db->GetStdDbSize(), db->GetAlphabetSize());
		CoverStats ctstats = ct->GetCurStats();
		fprintf(fp, "CT\t%.2f\t%d\t%d\t%d\n", tes, ctstats.alphItemsUsed, ct->GetCurNumSets(), ctstats.numSetsUsed);	// expand with #ab,#sets, etc
		// Ignore one of CT
		for(uint32 i=0; i<ctLength; i++) {
			bool beenMined = false;
			ItemSetCollection *isc = FicMain::ProvideItemSetCollection(iscTag, db, beenMined, true, true);

			uint64 numSets = isc->GetNumItemSets();
			isc->RemoveOrderedSubset(ctArray+i, 1);
			uint64 numASets = isc->GetNumItemSets();
			CodeTable *act = FicMain::CreateCodeTable(db, mConfig, false, isc);
			double ates = act->GetCurSize();

			CoverStats actstats = act->GetCurStats();
			fprintf(fp, "CT*%d\t%.2f\t%d\t%d\t%d\n", i, ates, actstats.alphItemsUsed, act->GetCurNumSets(), actstats.numSetsUsed);
			printf("elem %d: %.2f and %.2f with a absolute difference of %.2f\n", i, tes, ates, max(tes,ates)-min(tes,ates));

			delete act;
			delete isc;
		}
	}
	fclose(fp);

	delete[] ctArray;
	delete ctList;
	delete ct;

	delete db;
}

void TestTH::Dealer() {
	//string iscName = "ticTacToe-all-1a";
	//Croupier *croupier = Croupier::Create(iscName);
	// OR

	// Mine patterns
	//string outputFilename = croupier->MinePatternsToFile();
	//printf("Patterns are in %s\n", outputFilename.c_str());
	
	Bass::SetMineToMemory(true);
	string dbName = mConfig->Read<string>("dbname");
	string iscTag = mConfig->Read<string>("iscname");
	Database *db = Database::RetrieveDatabase(dbName);
	
	for(uint32 i=0; i<90; i++) {
		printf("Going for run #%d\n", i+1);
		//Croupier *croupier = Croupier::Create(mConfig);

		//ItemSetCollection *isc = croupier->MinePatternsToMemory(db);
		ItemSetCollection *isc = FicMain::MineItemSetCollection(iscTag, db, false, true);
		printf("Just mined %d patterns for you\n", isc->GetNumItemSets());

		// ORDER DOES MATTER; delete isc BEFORE delete croupier IS JUST FINE, THE OTHER WAY AROUND IS NOT.
		//delete croupier;
		delete isc;
	}
	delete db;
}

void TestTH::TestShizzle() {	
	string name = "coverpartial-orderly-alt-length";

	CPAOCodeTable *t2CT = new CPAOCodeTable();
	CodeTable *tstCT = new CPAOCodeTable(name);
	CPAOCodeTable *cCT = ((CPAOCodeTable*)tstCT);
	CPAOCodeTable *cCT2 = ((CPAOCodeTable*)tstCT);
	uint32 nA = cCT->GetAlphabetSize();
	nA = cCT2->GetAlphabetSize();
	nA = t2CT->GetAlphabetSize();

	delete tstCT;

	/*
	ItemSet *a = ItemSet::CreateEmpty(BM128ItemSetType, 100, 1, 5);
	ItemSet *b = ItemSet::CreateEmpty(BM128ItemSetType, 100, 1, 3);

	if(*a > b)
		printf("a > b");
	else if(*b > a)
		printf("a < b");
	else
		printf("a == b");

	delete a;
	delete b;

	return;

	ItemSet *a = ItemSet::CreateEmpty(BM128ItemSetType, 100, 1, 5);
	ItemSet *b = ItemSet::CreateEmpty(BM128ItemSetType, 100, 1, 3);

	if(*a > b)
		printf("a > b");
	else if(*b > a)
		printf("a < b");
	else
		printf("a == b");

	delete a;
	delete b;

	return;

	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);

	string iscName = mConfig->Read<string>("iscName");
	ItemSetCollection *isc = ItemSetCollection::RetrieveItemSetCollection(iscName, db, false);

	delete isc;
	delete db;
	*/
}

/*
	// global	mCDB
	// local	items, tids, minLen, maxLen, maxSup, minSup
*/

//double Eclat::grow(multiset<Item> *items, unsigned supp, int *itemset, int depth, int *comb, int cl)

void TestTH::SetupEclat() {
	mMinSup = mConfig->Read<uint32>("minsup", 2);
	mMaxSup = mConfig->Read<uint32>("maxsup", UINT32_MAX_VALUE);

	mMinLen = mConfig->Read<uint32>("minlen", 1);
	mMinLen = mMinLen > mABSize ? mABSize : (mMinLen == 0 ? 1 : mMinLen);
	mMaxLen = mConfig->Read<uint32>("maxlen", mDB->GetMaxSetLength());

	uint32 *abNR = mDB->GetAlphabetNumRows();
	uint32 numPosItems = mABSize;
	for(uint32 i=0; i<mABSize; i++) {
		if(abNR[i] < mMinSup) {
			numPosItems = i;
			break;
		}
	}

	mItemNumTids = new uint32[numPosItems];
	memset(mItemNumTids, 0, sizeof(uint32)*numPosItems);

	mItems = new uint16[numPosItems];
	uint32 curItemIdx = 0;
	alphabet *ab = mDB->GetAlphabet();
	alphabet::iterator ait=ab->begin(), aend = ab->end();
	for(; ait != aend; ++ait) {
		if(ait->first < numPosItems)
			mItems[curItemIdx++] = ait->first;
	}
	mItemTids = new uint32*[mABSize];
	for(uint32 i=0; i<numPosItems; i++) {
		mItemTids[i] = new uint32[abNR[i]];
	}

	uint16 *auxArr = new uint16[mABSize];
	for(uint32 r=0; r<mNumRows; r++) {
		uint32 rowLen = mRows[r]->GetLength();
		if(rowLen >= mMinLen) {
			mRows[r]->GetValuesIn(auxArr);
			for(uint32 v=0; v<rowLen; v++) {
				if(auxArr[v] < numPosItems)
					mItemTids[auxArr[v]][mItemNumTids[auxArr[v]]++] = r;
			}
		}
	}
	delete[] auxArr;
	mNumItems = numPosItems;
}
void TestTH::CleanupEclat() {
	for(uint32 i=0; i<mNumItems; i++) {
		delete[] mItemTids[i];
	} delete[] mItemTids;
	delete[] mItemNumTids;
	delete[] mItems;
}
void TestTH::AdjustEclatParameters() {
	// update stats ahv huidge cover/toegevoegde set
}

void TestTH::TestMineFrequentItemSets() {
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("dataType", "bm128"));
	mDB = Database::RetrieveDatabase(dbName, dataType);
	mRows = mDB->GetRows();
	mNumRows = mDB->GetNumRows();
	mABSize = (uint32) mDB->GetAlphabetSize();

	SetupEclat();
	MineFIS();
	CleanupEclat();

	delete mDB;
}
void TestTH::MineFIS() {
	uint16 *itemset = new uint16[mNumItems];
	uint16 *combin0r = new uint16[mNumItems];

	MineFISDFS(mItems, mNumItems, mItemTids, mItemNumTids, mNumRows, itemset, 0, combin0r, 0);

	delete[] itemset;
	delete[] combin0r;
}
void TestTH::MineFISDFS(uint16 *posItems, uint32 numPosItems, uint32** posItemTids, uint32* posItemNumTids, 
						uint32 prefixSup, uint16 *prefix, uint32 prefixLen, uint16 *comb, uint32 combLen) 
{
	const int sw = 2;

	for(uint32 i=0; i<numPosItems; i++) {
		uint32 newCombLen = combLen;

		uint16 *newPosItems = new uint16[numPosItems];
		uint32 newNumPosItems = 0;
		uint32 **newPosItemTids = new uint32*[numPosItems];
		uint32 *newPosItemNumTids = new uint32[numPosItems];
		memset(newPosItemNumTids, 0, sizeof(uint32)*numPosItems);

		prefix[prefixLen] = posItems[i];

		for(uint32 j=i+1; j<numPosItems; j++) {

			// eigenlijk diffsets, maar nu ff tidlists door intersectie te nemen
			uint32 *newTidlist = new uint32[posItemNumTids[j]];
			uint32 newNumTids = 0;

			uint32 curA=0, numA = posItemNumTids[i];
			uint32 curB=0, numB = posItemNumTids[j];

			while(curA < numA && curB < numB) {
				if(posItemTids[i][curA] > posItemTids[j][curB])
					curB++;
				else if(posItemTids[i][curA] < posItemTids[j][curB])
					curA++;
				else {						// gelijk, voeg toe, up a&b
					newTidlist[newNumTids++] = posItemTids[i][curA]; curA++; curB++;
				}
			}	// issa intersecca, so fsck the rest!

			if(newNumTids == posItemNumTids[i]) {
				comb[newCombLen++] = posItems[j];
				delete[] newTidlist;
			} else if(newNumTids > mMinSup) {
				newPosItems[newNumPosItems] = posItems[j];
				newPosItemTids[newNumPosItems] = newTidlist;
				newPosItemNumTids[newNumPosItems] = newNumTids;
				newNumPosItems++;
			} else {
				delete[] newTidlist;
			}
		}
		// Print
		if(prefixLen+1 >= mMinLen && prefixLen+1 <= mMaxLen && posItemNumTids[i] < mMaxSup)
			OutputSet(prefix,prefixLen+1,posItemNumTids[i],comb,newCombLen);

		// Search On
		if(newNumPosItems > 0)
			MineFISDFS(newPosItems, newNumPosItems, newPosItemTids, newPosItemNumTids, posItemNumTids[i], prefix, prefixLen+1,comb, newCombLen);

		// Clean Up
		for(uint32 j=0; j<newNumPosItems; j++) {
			delete[] newPosItemTids[j];
		} delete[] newPosItemTids;
		delete[] newPosItemNumTids;
		delete[] newPosItems;

	}
}
void TestTH::OutputSet(uint16 *is, uint32 isLen, uint32 supp) {
	printf("%d : ", isLen);
	for(uint32 i=0; i<isLen; i++) {
		printf("%d ", is[i]);
	} printf("(%d)\n", supp);

}
void TestTH::OutputSet(uint16 *is, uint32 isLen, uint32 supp, uint16 *comb, uint32 combLen) {
	printf("%d : ", isLen);
	for(uint32 i=0; i<isLen; i++) {
		printf("%d ", is[i]);
	} printf("(%d) [", supp);
	for(uint32 i=0; i<combLen; i++) {
		printf("%d ", comb[i]);
	} printf("]\n");
}
void TestTH::BufferSet(uint16 *is, uint32 isLen, uint32 supp, uint16 *comb, uint32 combLen) {
	if(supp < mISBufferSup)
		return;
	else if(supp > mISBufferSup) {
		for(uint32 i=0; i<mISBufferNum; i++)
			delete[] mISBuffer[i];
		mISBufferNum = 0;
		mISBufferSup = supp;
	}

	if(mISBufferNum >= mISBufferMax) {
		uint32 newISBufferMax = mISBufferMax * 2;
		uint16** newISBuffer = new uint16*[newISBufferMax];
		uint32* newISBufferLen = new uint32[newISBufferMax];
		memcpy(newISBuffer, mISBuffer, sizeof(uint16*)*mISBufferMax);
		memcpy(newISBufferLen, mISBufferLen, sizeof(uint32)*mISBufferMax);
		delete[] mISBuffer;
		delete[] mISBufferLen;
		mISBuffer = newISBuffer;
		mISBufferLen = newISBufferLen;
		mISBufferMax = newISBufferMax;
	}
	mISBuffer[mISBufferNum] = new uint16[isLen];
	memcpy(mISBuffer[mISBufferNum], is, sizeof(uint16)*isLen);
	mISBufferLen[mISBufferNum] = isLen;
	mISBufferNum++;
}
void TestTH::OutputBufferedSets() {
	for(uint32 i=0; i<mISBufferNum; i++) {
		//OutputTile(mLogFile, mTileBuffer[i],mTileBufferLen[i],mTileBufferArea);		
		OutputSet(mISBuffer[i],mISBufferLen[i],mISBufferSup,NULL, 0);
	}
}


void TestTH::TestMineMostFrequentItemsets() {
	// wellicht db lazy tid'en

	// init DB
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("dataType", "bm128"));
	mDB = Database::RetrieveDatabase(dbName, dataType);
	mRows = mDB->GetRows();
	mNumRows = mDB->GetNumRows();
	mABSize = (uint32) mDB->GetAlphabetSize();

	// init ISBuffer
	mISBufferSup = 0;
	mISBufferNum = 0;
	mISBufferMax = 10;
	mISBuffer = new uint16*[mISBufferMax];
	mISBufferLen = new uint32[mISBufferMax];


	SetupEclat();
	MineMFIS();
	CleanupEclat();

	// kill ISBuffer
	for(uint32 i=0; i<mISBufferNum; i++)
		delete[] mISBuffer[i];
	delete[] mISBuffer;
	delete[] mISBufferLen;

	// kill DB
	delete mDB;
}
void TestTH::MineMFIS() {
	uint16 *itemset = new uint16[mNumItems];
	uint16 *combin0r = new uint16[mNumItems];

	MineMFISDFS(mItems, mNumItems, mItemTids, mItemNumTids, mNumRows, itemset, 0, combin0r, 0);

	OutputBufferedSets();

	delete[] itemset;
	delete[] combin0r;
}
void TestTH::MineMFISDFS(uint16 *posItems, uint32 numPosItems, uint32** posItemTids, uint32* posItemNumTids, 
						uint32 prefixSup, uint16 *prefix, uint32 prefixLen, uint16 *comb, uint32 combLen) 
{
	for(uint32 i=0; i<numPosItems; i++) {
		if(posItemNumTids[i] < mMinSup)
			continue;

		uint32 newCombLen = combLen;

		uint16 *newPosItems = new uint16[numPosItems];
		uint32 newNumPosItems = 0;
		uint32 **newPosItemTids = new uint32*[numPosItems];
		uint32 *newPosItemNumTids = new uint32[numPosItems];
		memset(newPosItemNumTids, 0, sizeof(uint32)*numPosItems);

		prefix[prefixLen] = posItems[i];

		for(uint32 j=i+1; j<numPosItems; j++) {

			// eigenlijk diffsets, maar nu ff tidlists door intersectie te nemen
			uint32 *newTidlist = new uint32[posItemNumTids[j]];
			uint32 newNumTids = 0;

			uint32 curA=0, numA = posItemNumTids[i];
			uint32 curB=0, numB = posItemNumTids[j];

			while(curA < numA && curB < numB) {
				if(posItemTids[i][curA] > posItemTids[j][curB])
					curB++;
				else if(posItemTids[i][curA] < posItemTids[j][curB])
					curA++;
				else {						// gelijk, voeg toe, up a&b
					newTidlist[newNumTids++] = posItemTids[i][curA]; curA++; curB++;
				}
			}	// issa intersecca, so fsck the rest!

			if(newNumTids == posItemNumTids[i]) {
				comb[newCombLen++] = posItems[j];
				delete[] newTidlist;
			} else if(newNumTids > mMinSup) {
				newPosItems[newNumPosItems] = posItems[j];
				newPosItemTids[newNumPosItems] = newTidlist;
				newPosItemNumTids[newNumPosItems] = newNumTids;
				newNumPosItems++;
			} else {
				delete[] newTidlist;
			}
		}
		// Print
		if(prefixLen+1 >= mMinLen && prefixLen+1 <= mMaxLen) {
			if(posItemNumTids[i] < mMaxSup) {
				BufferSet(prefix,prefixLen+1,posItemNumTids[i],comb,newCombLen);
				if(posItemNumTids[i] > mMinSup)
					mMinSup = posItemNumTids[i];
			}
		}

		// Search On
		if(newNumPosItems > 0)
			MineMFISDFS(newPosItems, newNumPosItems, newPosItemTids, newPosItemNumTids, posItemNumTids[i], prefix, prefixLen+1,comb, newCombLen);

		// Clean Up
		for(uint32 j=0; j<newNumPosItems; j++) {
			delete[] newPosItemTids[j];
		} delete[] newPosItemTids;
		delete[] newPosItemNumTids;
		delete[] newPosItems;

	}
}

/*void TestTH::KrimpCandidate() {

}*/

/*void TestTH::KrimpCandidates() {

}*/


void TestTH::TestKrimpOnTheFly() {
	// init DB
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("dataType", "bm128"));
	mDB = Database::RetrieveDatabase(dbName, dataType);
	mRows = mDB->GetRows();
	mNumRows = mDB->GetNumRows();
	mABSize = (uint32) mDB->GetAlphabetSize();

	// init CT
	string ctName = mConfig->Read<string>("ctName", "coverfull");
	CodeTable *mCT = CodeTable::Create(ctName, mDB->GetDataType());

	// init Levels
	uint32 mNumLevels = 1;
	uint32 *mLevelMinLen = new uint32[mNumLevels];
	uint32 *mLevelMaxLen = new uint32[mNumLevels];
	uint32 *mLevelU2D = new uint32[mNumLevels];

	uint16 ***mLevelBuffer = new uint16**[mNumLevels];

	uint32 **mLevelBufferLen = new uint32*[mNumLevels];
	uint32 *mLevelBufferMax = new uint32[mNumLevels];
	uint32 *mLevelBufferNum = new uint32[mNumLevels];
	uint32 *mLevelBufferSup = new uint32[mNumLevels];

	for(uint32 i=0; i<mNumLevels; i++) {
		mLevelMinLen[i] = 1;
		mLevelMaxLen[i] = UINT32_MAX_VALUE;
		mLevelU2D[i] = UINT32_MAX_VALUE;	// == not up to date at all

		mLevelBufferNum[i] = 0;
		mLevelBufferSup[i] = 0;
		mLevelBufferMax[i] = 10;
		mLevelBuffer[i] = new uint16*[mLevelBufferMax[i]];
		for(uint32 j=0; j<mLevelBufferMax[i]; j++)
			mLevelBuffer[i][j] = new uint16[mDB->GetMaxSetLength()];

		//mLevelEclat[i] = new Eclat(i);
	}


	bool finished = false;
	bool accepted = false;
	uint32 accCndIdx = UINT32_MAX_VALUE;
	while(!finished) {
		accepted = false;
		accCndIdx = UINT32_MAX_VALUE;

		for(uint32 i=0; i<mNumLevels && !accepted; i++) {	// levels op descending volgorde houden!
			if(mLevelU2D[i] > mMaxSup) {
				// mine l
				// mLevelEclat[i]->GetUpToDate();
				// order buffer
				// ..
			}

			for(uint32 j=0; j<mLevelBufferNum[i]; j++) {
				//accCndIdx = KrimpCandidates(i);
				if(accCndIdx != UINT32_MAX_VALUE) {
					// candidate accepted
					accepted = true;

				}
			}
			if(accepted == true) {
				//
			}

		}
		if(!accepted) {
			// deze minsup hebben we gehad, loweren die handel
		}
	}

	/*
	while niet klaar
		foreach level l, in cand volgorde
			if(mLevelU2D[l] > mMaxSup)
				mine l
				order buffer in cand volgorde
			foreach cand c in buffer
				if accepted
					stel mLevelU2D's op MAX in
					break, break

	*/

	for(uint32 i=0; i<mNumLevels; i++) {
		for(uint32 j=0; j<mLevelBufferMax[i]; j++)
			delete[] mLevelBuffer[i][j];
		delete[] mLevelBuffer[i];
	}

	delete[] mLevelBufferNum;
	delete[] mLevelBufferSup;
	delete[] mLevelBufferMax;

	delete[] mLevelMinLen;
	delete[] mLevelMaxLen;
	delete[] mLevelU2D;

	delete mCT;
	delete mDB;
}

void TestTH::ArnoKPatterns() {

	Bass::SetMineToMemory(true);

	string dbName = mConfig->Read<string>("dbname");
	string dbBaseName = dbName.substr(0,dbName.find_first_of('['));
	string iscTag = mConfig->Read<string>("iscname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	Database *db = Database::RetrieveDatabase(dbName, dataType);
	AfoptCroupier *croup = new AfoptCroupier(iscTag);
	ItemSetCollection *isc = croup->MinePatternsToMemory(db);

	string patternFullPath = Bass::GetWorkingDir() + iscTag + "-patterns.txt";
	string occsFullPath = Bass::GetWorkingDir() + iscTag + "-occurences.txt";
	string matrixFullPath = Bass::GetWorkingDir() + iscTag + "-matrix.txt";

	ItemSet **rows = db->GetRows();
	uint32 numRows = db->GetNumRows();
	uint32 numPats = isc->GetNumLoadedItemSets();

	FILE *pfp = fopen(patternFullPath.c_str(), "w");
	for(uint32 p=0; p<numPats; p++) {
		string isstr = isc->GetLoadedItemSet(p)->ToString(false,true);
		fprintf(pfp,"%s\n",isstr.c_str());
	}
	fclose(pfp);

	FILE *ofp = fopen(occsFullPath.c_str(), "w");
	for(uint32 r=0; r<numRows; r++) {
		bool eerste = true;
		for(uint32 p=0; p<numPats; p++) {
			if(rows[r]->IsSubset(isc->GetLoadedItemSet(p))) {
				if(eerste == true) {
					fprintf(ofp, "%d", p);
				} else {
					fprintf(ofp, "\t%d", p);
				}
				eerste = false;
			}
		}
		fprintf(ofp, "\n");
	}
	fclose(ofp);
	
	FILE *mfp = fopen(matrixFullPath.c_str(), "w");
	for(uint32 r=0; r<numRows; r++) {
		for(uint32 p=0; p<numPats-1; p++)
			fprintf(mfp,"%c\t", rows[r]->IsSubset(isc->GetLoadedItemSet(p)) ? '1' : '0');
		fprintf(mfp,"%c\n", rows[r]->IsSubset(isc->GetLoadedItemSet(numPats-1)) ? '1' : '0');
	}
	fclose(mfp);

	delete isc;
	delete croup;
	delete db;
}

#ifdef BLOCK_TILING
void TestTH::TestMineTilesToMemory() {
	// get dbfilename and outputfilename from config
	string dbName = mConfig->Read<string>("dbname");

	Database *db = Database::RetrieveDatabase(dbName, Uint16ItemSetType);
	TileMiner *miner = new TileMiner(mConfig);
	ItemSetCollection *isc = miner->MinePatternsToMemory(db);
	delete miner;
	delete db;
	delete isc;
}
#endif
