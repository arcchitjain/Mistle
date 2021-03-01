#ifdef BLOCK_TILING
#include "../../global.h"

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <ArrayUtils.h>
#include <TimeUtils.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>

// croupier
#include <Croupier.h>

#include "TilingOut.h"
#include "TilingMiner.h"

TilingMiner::TilingMiner(Config *conf) : Croupier(conf) {
	mConfig = conf;

	// --- parameters
	mMinerVersion = mConfig->Read<uint32>("version", 1);
	mArgMinTileLength = mConfig->Read<uint32>("minTileLength", 1);
	mTileSelectionStylee = StringToTileSelectionStylee(mConfig->Read<string>("selectStylee"));
	mMaxNumTiles = mConfig->Read<uint32>("numtiles", 1);
	mColumnPruningEnabled = mConfig->Read<bool>("pruneCols", true);
	mRowPruningEnabled = mConfig->Read<bool>("pruneRows", true);

	// --- init stats
	mStatMinArea = UINT32_MAX_VALUE;
	mStatMinSup = UINT32_MAX_VALUE;
	mStatMaxSetLength = 0;
	mStatNumTilesMined = 0;
	mStatCoveredArea = 0;

	// --- init member vars
	mOutput = NULL;
	mIscOrder = NoneIscOrder;

	mAuxArrA = NULL;
	mAuxArrB = NULL;

	mItems = NULL;
	mItemTids = NULL;
	mItemNumTids = NULL;
	mItemTranslationMap = NULL;

	mRows = NULL;
	mNumRows = 0;
	mNumItems = 0;
	mHasBinSizes = false;
	mAlphabet = NULL;
	mABSize = 0;

	mFullTidList = NULL;
	mRowLengths = NULL;;

	mTile = NULL;
	mTiledItems = NULL;

	mLogTiles = mConfig->Read<bool>("logtiles", 0);
	mLogFileTiles = NULL;

	mLogTids = mConfig->Read<bool>("logtids", 0);
	mLogFileTids = NULL;

	// init TileBuffer
	mTileBufferArea = 0;
	mTileBufferNum = 0;
	mTileBufferMax = 10;
	mTileBuffer = new uint16*[mTileBufferMax];
	mTileBufferLen = new uint32[mTileBufferMax];
	mTileBufferSup = new uint32[mTileBufferMax];
}

TilingMiner::~TilingMiner() {
	// clean up output
	delete mOutput;

	// not my Config*
	//delete mConfig

	// kill TileBuffer
	for(uint32 i=0; i<mTileBufferNum; i++)
		delete[] mTileBuffer[i];
	delete[] mTileBuffer;
	delete[] mTileBufferLen;
	delete[] mTileBufferSup;
}

string TilingMiner::GenerateOutputFileName(const Database *db) {
	return "tiling-nov-" + db->GetName() + "-" + TimeUtils::GetTimeStampString() + ".txt";
}
string TilingMiner::GenerateTileLogFilename(const Database *db) {
	return "tiling-nov-" + db->GetName() + "-" + TimeUtils::GetTimeStampString() + ".tiles.log";
}
string TilingMiner::GenerateTidsLogFilename(const Database *db) {
	return "tiling-nov-" + db->GetName() + "-" + TimeUtils::GetTimeStampString() + ".tids.log";
}


void TilingMiner::MineTiling() {
	// --- stats
	ResetStats();

	// Initialize Aux-stuff
	InitAuxiliaries();

	// --- output files
	InitTilesLogFile();
	InitTidsLogFile();

	// Init Items and Data
	InitData();

	// init mTile
	mTile = ItemSet::CreateEmpty(mDatabase->GetDataType(), mNumItems,0);
	mTiledItems = ItemSet::CreateEmpty(mDatabase->GetDataType(), mNumItems,0);

#if defined (_MSC_VER)
	uint32 minArea = max(mDatabase->GetMaxSetLength(), mArgMinTileLength > 1 ? 0 : mItemNumTids[0]);
#elif defined(__GNUC__)
	uint32 minArea = max((uint32)mDatabase->GetMaxSetLength(), mArgMinTileLength > 1 ? 0 : mItemNumTids[0]);
#endif

	uint16* currentTile = new uint16[mNumItems];
	uint32 *currentTidList = mFullTidList;
	/*hmm weet ff niet
	if(mUpdateUncoveredAreaAbsolute == false) {
		currentTidList = new uint32[mNumRows];
		memcpy(currentTidList, mFullTidList, sizeof(uint32)*mNumRows);
	}*/

	uint32 numPosItems = mNumItems;
	uint16 *posItems = new uint16[numPosItems];
	memcpy(posItems, mItems, sizeof(uint16)*mNumItems);

	uint32** posItemTids = new uint32*[mNumItems];
	uint32** origPosItemTids = new uint32*[mNumItems];
	uint32* posItemNumTids = new uint32[mNumItems];
	memcpy(posItemNumTids, mItemNumTids, sizeof(uint32)*mNumItems);
	for(uint32 i=0; i<numPosItems; i++) {
		posItemTids[i] = new uint32[mItemNumTids[i]];
		origPosItemTids[i] = posItemTids[i];
		memcpy(posItemTids[i], mItemTids[i], sizeof(uint32)*mItemNumTids[i]);
	}

	uint32 *prunedRows = new uint32[mNumRows];
	uint32 numOnesCovered = 0;
	uint32 numPrunedRows = 0;
	uint32 curNumTiles = 0;

	// Mine First Tile
	if(mArgMinTileLength == 1) {
		minArea = mArgMinTileLength;
	}
	MineLargestTilesDFSv1(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths);
	numOnesCovered += mTileBufferArea;
	printf("R%d:\t%dbC\t%drP\t%dcO\t%dnT\n", ++curNumTiles, mStatBranchCount, numPrunedRows, numOnesCovered, mTileBufferNum);
	if(mLogTiles == true) fprintf(mLogFileTiles, "R%d:\t%dbC\t%drP\t%dcO\t%dnT\n", curNumTiles, mStatBranchCount, numPrunedRows, numOnesCovered, mTileBufferNum);
	mChosenTileIdx = SelectTile();
	OutputBufferedTiles(mChosenTileIdx);

	// Mine the next Largest Tile, etc, etc, ad naeseum
	while(mTileBufferArea > 0 && curNumTiles < mMaxNumTiles) {
		for(uint32 i=0; i<mTileBufferLen[mChosenTileIdx]; i++) {
			uint16 tileItem = mTileBuffer[mChosenTileIdx][i];

			mTile->AddItemToSet(tileItem);
			mTiledItems->AddItemToSet(tileItem);
			mTile->Sort(); // looks nice, and is necessary for (faster) covering
		}

		// Apply Tile
		bool colsEmpty = false;
		// ... prune posItems, numPosItems, posItemTids, posItemNumTids ahv mChosenTileIdx


		// Prune columns that are fully covered
		if(mColumnPruningEnabled == true && colsEmpty == true) {
			// .. eigenlijk alleen over mTileBuffer lopen, en terugmappen naar posItems
			for(uint32 i=0; i<mNumItems; i++) {
				if(mItemNumTids[i]==0) {
					printf("Item %d wordt gezapt\n", mItems[i]);
					if(mLogTiles == true) fprintf(mLogFileTiles, "Item %d wordt gezapt\n", mItems[i]);
					// zap dit item
					for(uint32 t=0; t<mItemNumTids[i]; t++)
						mRowLengths[mItemTids[i][t]]--;
					mItemNumTids[i] = mItemNumTids[mNumItems-1];
					mItemNumTids[mNumItems-1] = 0;

					uint32 *tmpCol = mItemTids[i];
					mItemTids[i] = mItemTids[mNumItems-1];
					mItemTids[mNumItems-1] = tmpCol;

					uint32 *tmpOrg = origPosItemTids[i];
					origPosItemTids[i] = origPosItemTids[mNumItems-1];
					origPosItemTids[mNumItems-1] = tmpOrg;

					uint16 tmp = mItemTranslationMap->find(mItems[i])->second;
					mItemTranslationMap->find(mItems[i])->second = mItemTranslationMap->find(mItems[mNumItems-1])->second;
					mItemTranslationMap->find(mItems[mNumItems-1])->second = tmp;

					uint16 tmpItem = mItems[i];
					mItems[i] = mItems[mNumItems-1];
					mItems[mNumItems-1] = tmpItem;

					mNumItems--;
				}
			}
		}

		// Prune rows that are fully covered
		if(mRowPruningEnabled == true && numPrunedRows > 0) {
			// remove pruneRowTids from currenttidlist òf fulltidlist, ff uitzoeken
			// update mNumRows
			// remove pruneRowTids from mItemTids, update mItemNumTids
			// remove rowLengths en uncRowLengths

		}

		mTile->CleanTheSlate();
		bool doYouDigIt = true;

		// More Stuff in the Buff?
		if(mTileBufferNum > 1) {
			// after accept, also consider adding the other, tiles, not overlapping with newlyChosenTileItems
			SwapTileBufferEntries(mTileBufferNum-1,mChosenTileIdx);	// mTileBuffer[mTileBufferNum-1] = chosenTile
			delete[] mTileBuffer[mTileBufferNum-1];
			mTileBufferNum--;	// zoveel zitten er nog in, minimaal 1

			// for each tile in buffer
			for(uint32 i=0; i<mTileBufferNum; i++) {
				// check if it doesn't clash with the found tiles
				for(uint32 j=0; j<mTileBufferLen[i]; j++) {
					if(mTiledItems->IsItemInSet(mTileBuffer[i][j])) {
						SwapTileBufferEntries(mTileBufferNum-1,i);
						delete[] mTileBuffer[mTileBufferNum-1];
						mTileBufferNum--;
						i--;
						break;
					}	}	}
			if(mTileBufferNum > 0)
				doYouDigIt = false;
		}

		if(doYouDigIt == true) {
			mTileBufferArea = 0;
			mTiledItems->CleanTheSlate();

			if(mNumItems > 0) {
				// now find me new shizzle!
				minArea = 1;
				if(mArgMinTileLength == 1) {
					for(uint32 i=0; i<mNumItems; i++) {
						if(mItemNumTids[i] > minArea) {
							minArea = mItemNumTids[i];
						}	
					}	
				}
				numPosItems = mNumItems;
				memcpy(posItems, mItems, sizeof(uint16)*mNumItems);
				memcpy(posItemNumTids, mItemNumTids, sizeof(uint32)*mNumItems);
				memcpy(posItemTids, origPosItemTids, sizeof(uint32*)*mABSize);
				for(uint32 i=0; i<numPosItems; i++) {
					memcpy(posItemTids[i], mItemTids[i], sizeof(uint32)*mItemNumTids[i]);
				}
				// !?! if(mUpdateUncoveredAreaAbsolute == false)
				memcpy(currentTidList,mFullTidList,sizeof(uint32)*mNumRows);
				/* !R!
				if(mMinerVersion == 1)
					MineLargestUncoveredTilesDFSv1(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths, 0);
				else if(mMinerVersion == 2)
					MineLargestUncoveredTilesDFSv2(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths, mUncRowLengths, 0);
				else if(mMinerVersion == 3)
					MineLargestUncoveredTilesDFSv3(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths, mUncRowLengths, 0);
				*/
				//	MineLargestTilesDFSv1(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths);
			}
		}

		// output TileBuffer
		if(mTileBufferArea > 0 && mTileBufferNum > 0) {
			numOnesCovered += mTileBufferArea;
			mChosenTileIdx = SelectTile();
			printf("R%d:\t%dbC\t%drP\t%dcO\t%dnT\n", ++curNumTiles, mStatBranchCount, numPrunedRows, numOnesCovered, mTileBufferNum);
			if(mLogTiles == true) fprintf(mLogFileTiles, "R%d:\t%dbC\t%drP\t%dcO\t%dnT\n", curNumTiles, mStatBranchCount, numPrunedRows, numOnesCovered, mTileBufferNum);
			OutputBufferedTiles(mChosenTileIdx);
		}


	}

}

//////////////////////////////////////////////////////////////////////////
// Mine to Memory
//////////////////////////////////////////////////////////////////////////
void TilingMiner::InitOnline(Database *db) {
	Croupier::InitOnline(db);

	mMemOut = new tlngmnr::MemoryOut(db, this);
	mOutput = mMemOut;

	uint32 bufferSize = (uint32)(Bass::GetRemainingMaxMemUse() / (uint64)(2.5 * sizeof(uint32) + ItemSet::MemUsage(db)));
	//printf("buffersize = %d, want\n mem remaining: %" I64d "\n vage num bytes: %" I64d "\n per itemset: %d\n", bufferSize, Bass::GetRemainingMaxMemUse(), (uint64)(2.5 * sizeof(uint32) + ItemSet::MemUsage(db)), ItemSet::MemUsage(db));
	if(bufferSize == 0) bufferSize = 1;
	mMemOut->InitBuffer(bufferSize);
	mMayHaveMore = true;
}
ItemSetCollection* TilingMiner::MinePatternsToMemory(Database *db) {
	// prep da db shizzle
	printf(" * Mining:\t\tin progress...\r");
	bool deleteDb = false;
	if(db == NULL) {
		db = Database::RetrieveDatabase(mDbName);
		deleteDb = true;
	}
	mOnlineNumChunks = 0;
	mOnlineNumTotalSets = 0;

	if(db->HasBinSizes())
		ECHO(2, printf("TilingMiner cant handle binsizes properly yet..."));

	// prep da mining shizzle
	InitOnline(db);

	// do da mining dance
	MineTiling();
	// mine has been putted out

	mOutput->SetFinished();

	ItemSetCollection *isc = NULL;
	if(mOnlineNumChunks == 0) {
		// All ItemSets aboard!
		isc = mOnlineIsc;
		mOnlineIsc = NULL;
	} else {
		// All Chunks Verzamelen!
		string iscTempPath = Bass::GetWorkingDir();
		string iscTempFileName = mOnlineIsc->GetTag() + "." + IscFile::TypeToExt(mChunkIscFileType);
		string iscTempFullPath = iscTempPath + iscTempFileName;

		ECHO(2, printf(" * Mining:\t\tStoring chunk #%d\t\t\r", mOnlineNumChunks+1));
		ItemSetCollection::SaveChunk(mOnlineIsc, iscTempFullPath, mOnlineNumChunks, mChunkIscFileType);
		delete mOnlineIsc;
		mOnlineIsc = NULL;
		mOnlineNumChunks++;

		ECHO(2, printf(" * Mining:\t\tFinished mining %d chunks\t\t\r", mOnlineNumChunks));

		isc = new ItemSetCollection(db);
		SetIscProperties(isc);
		isc->SetNumItemSets(mOnlineNumTotalSets);
		ItemSetCollection::MergeChunks(iscTempPath, iscTempPath, iscTempFileName, mOnlineNumChunks, mChunkIscFileType, db, isc->GetDataType(), mChunkIscFileType);
		delete isc;
		isc = ItemSetCollection::OpenItemSetCollection(iscTempFileName, iscTempPath, db);
	}
	printf("\r * Mining:\t\tFinished mining %" I64d " ItemSets.\t\n", isc->GetNumItemSets());
	return isc;
}
void TilingMiner::SetIscProperties(ItemSetCollection *isc) {
	isc->SetDatabaseName(mDbName);
	isc->SetPatternType(GetPatternTypeStr());
	isc->SetMinSupport(mStatMinSup);
	isc->SetMaxSetLength(mStatMaxSetLength);
	isc->SetHasSepNumRows(mHasBinSizes);
	uint8 ol = Bass::GetOutputLevel();
	Bass::SetOutputLevel(1);
	isc->SortLoaded(mIscOrder);
	Bass::SetOutputLevel(ol);
}
void TilingMiner::MinerIsErVolVanCBFunc(ItemSet **buf, uint32 numSets) {
	// Save Chunk
	ItemSet **iss = new ItemSet *[numSets]; // iss may be much smaller than buffer
	memcpy_s(iss, numSets*sizeof(ItemSet*), buf, numSets*sizeof(ItemSet*));

	ItemSetCollection *isc = new ItemSetCollection(mDatabase);
	isc->SetLoadedItemSets(iss, numSets);
	ECHO(2, printf(" * Mining:\t\tSorting chunk #%d             \r", mOnlineNumChunks+1));
	SetIscProperties(isc);

	ECHO(2, printf(" * Mining:\t\tStoring chunk #%d             \r", mOnlineNumChunks+1));
	string iscTempPath = Bass::GetWorkingDir() + isc->GetTag() + "." + IscFile::TypeToExt(mChunkIscFileType);
	ItemSetCollection::SaveChunk(isc, iscTempPath, mOnlineNumChunks, mChunkIscFileType);
	ECHO(2, printf(" * Mining:\t\tStored chunk #%d             \r", mOnlineNumChunks+1));
	delete isc;
	mOnlineNumChunks++;
	mOnlineNumTotalSets += numSets;

	uint32 bufferSize = (uint32)((Bass::GetRemainingMaxMemUse()+numSets*sizeof(uint32)) / (uint64)(2.5 * sizeof(uint32) + ItemSet::MemUsage(mDatabase)));
	if(bufferSize == 0) bufferSize = 1;
	mMemOut->ResizeBuffer(bufferSize);
}
void TilingMiner::MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets) {
	ItemSet **iss = new ItemSet *[numSets]; // iss may be much smaller than buffer
	memcpy_s(iss, numSets*sizeof(ItemSet*), buf, numSets*sizeof(ItemSet*));

	if(mOnlineNumChunks > 0) {
		ECHO(2, printf(" * Mining:\t\tSorting chunk #%d             \r", mOnlineNumChunks+1));
	} else
		ECHO(2, printf(" * Mining:\t\tSorting sets...            \r"));
	ItemSetCollection *isc = new ItemSetCollection(mDatabase);
	isc->SetNumItemSets(numSets);
	isc->SetLoadedItemSets(iss, numSets);
	SetIscProperties(isc);

	mOnlineIsc = isc;
	mOnlineNumTotalSets += numSets;
	if(mOnlineNumChunks > 0) {
		;//ECHO(2, printf(" * Mining:\t\tFinished mining %d chunks.         \r", mOnlineNumChunks+1));
	} else
		ECHO(2, printf(" * Mining:\t\tFinished.                          \r"));
}



//////////////////////////////////////////////////////////////////////////
// Mine to File 
//////////////////////////////////////////////////////////////////////////

bool TilingMiner::MinePatternsToFile(const string &dbFilename, const string &outputFileName, Database *db) {
	MinePatternsToFile(db, outputFileName);
	return true;
}
bool TilingMiner::MinePatternsToFile(Database *db, const string &outputFileName) {
	mDatabase = db;

	if(mDatabase->HasBinSizes())
		THROW("Cant do binsizes yet...");

	// Stel Output correct in
	mOutput = new tlngmnr::FSout(outputFileName, this);

	MineTiling();

	// Kill output appropriately
	delete mOutput;
	mOutput = NULL;

	return true;
}


//////////////////////////////////////////////////////////////////////////
/// Buffer/Output Tiles
//////////////////////////////////////////////////////////////////////////
void TilingMiner::OutputTile(uint16* items, uint32 numItems, uint32 supp, uint32 area) {
	mStatNumTilesMined++;
	if(numItems > mStatMaxSetLength)
		mStatMaxSetLength = numItems;
	if(supp < mStatMinSup)
		mStatMinSup = supp;
	if(area < mStatMinArea)
		mStatMinArea = area;

	mOutput->OutputSet(numItems, items, supp, area, 0);
}
void TilingMiner::BufferLargestTiles(uint16* items, uint32 numItems, uint32 support, uint32 area) {
	if(area < mTileBufferArea)
		return;
	else if(area > mTileBufferArea) {
		for(uint32 i=0; i<mTileBufferNum; i++)
			delete[] mTileBuffer[i];
		mTileBufferNum = 0;
		mTileBufferArea = area;
	}

	if(mTileBufferNum >= mTileBufferMax) {
		uint32 newTileBufferMax = mTileBufferMax * 2;
		uint16** newTileBuffer = new uint16*[newTileBufferMax];
		uint32* newTileBufferLen = new uint32[newTileBufferMax];
		uint32* newTileBufferSup = new uint32[newTileBufferMax];
		memcpy(newTileBuffer, mTileBuffer, sizeof(uint16*)*mTileBufferMax);
		memcpy(newTileBufferLen, mTileBufferLen, sizeof(uint32)*mTileBufferMax);
		memcpy(newTileBufferSup, mTileBufferSup, sizeof(uint32)*mTileBufferMax);
		delete[] mTileBuffer;
		delete[] mTileBufferLen;
		delete[] mTileBufferSup;
		mTileBuffer = newTileBuffer;
		mTileBufferLen = newTileBufferLen;
		mTileBufferSup = newTileBufferSup;
		mTileBufferMax = newTileBufferMax;
	}
	mTileBuffer[mTileBufferNum] = new uint16[numItems];
	memcpy(mTileBuffer[mTileBufferNum], items, sizeof(uint16)*numItems);
	mTileBufferLen[mTileBufferNum] = numItems;
	mTileBufferSup[mTileBufferNum] = support;
	mTileBufferNum++;
}
void TilingMiner::OutputBufferedTiles(uint32 chosenIdx) {
	for(uint32 i=0; i<mTileBufferNum; i++) {
		if(i == chosenIdx) {
			OutputTile(mTileBuffer[i], mTileBufferLen[i], mTileBufferSup[i], mTileBufferArea);
		}

		if(mLogTiles == true) {
			if(i == chosenIdx) {
				fprintf(mLogFileTiles, "-> ");
			}

			fprintf(mLogFileTiles, "! ");
			for(uint32 j=0; j<mTileBufferLen[i]; j++) {
				fprintf(mLogFileTiles, "%d ", mTileBuffer[i][j]);
			}
			fprintf(mLogFileTiles, "(%d,%d)\n", mTileBufferSup[i], mTileBufferArea);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
/// Mine Largest Tiles
//////////////////////////////////////////////////////////////////////////
void TilingMiner::MineLargestTilesDFSv1(uint32 &minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens) {
	if(numDepColumns == 0)
		return;

	uint32 UB = minArea;

	// prune
	bool stuffPruned = true;
	uint32 *newPrefixRowLens = new uint32[mNumRows];
	memcpy(newPrefixRowLens, prefixRowLens, sizeof(uint32) * mNumRows);
	while(stuffPruned == true) {
		stuffPruned = false;
		for(uint32 c=0; c<numDepColumns; c++) {
			// Build Length-Histogram
			memset(mRowLenPerLenHG, 0, mNumItems * sizeof(uint32));
			uint32 minLen = mNumItems-1;
			uint32 maxLen = 0;
			for(uint32 i=0; i<depColLens[c]; i++) {
				uint32 len = newPrefixRowLens[depColumns[c][i]];
				if(len < minLen) minLen = len;
				if(len > maxLen) maxLen = len;
				mRowLenPerLenHG[len]++;
			}
			uint32 cUB = 0;
			uint32 sum = 0;
			for(uint32 l=minLen; l<=maxLen; l++) {
				sum += mRowLenPerLenHG[l];
				if(((prefixLen + l) * sum) > cUB)
					cUB = (prefixLen + l) * sum;
			}
			if(cUB < minArea) {
				stuffPruned = true;
				// + Pas `prefixRowLens` aan
				for(uint32 i=0; i<depColLens[c]; i++)
					newPrefixRowLens[depColumns[c][i]]--;
				// + zap uit items (nu niet meer geordend)	<- !!! keep sorted, dan kunnen we twee vliegen in een klap slaan
				items[c] = items[numDepColumns-1];

				depColLens[c] = depColLens[numDepColumns-1];
				depColLens[numDepColumns-1] = 0;

				uint32 *tmp = depColumns[c];
				depColumns[c] = depColumns[numDepColumns-1];
				depColumns[numDepColumns-1] = tmp;

				numDepColumns--;
				c--;
			}
		}
	}

	// for each item i, see what happens when we extend `prefix` with i
	for(uint32 c=0; c<numDepColumns; c++) {
		uint32 cndArea = (prefixLen + 1) * depColLens[c];
		prefix[prefixLen] = items[c];

		if(cndArea >= minArea && prefixLen+1 >= mArgMinTileLength) {
			// deez is al goed genoeg
			BufferLargestTiles(prefix, prefixLen + 1, depColLens[c], cndArea);
			if(cndArea > minArea)
				minArea = cndArea;
		} 

		uint32 UB = minArea;
		// misschien nog extendable

		// pre-UB calc, assumes full-subsetting
		// ub1 = voor alle lengtes l, bereken max
		uint32 newNumDepColumns = numDepColumns-c-1;
		if(UB >= minArea && newNumDepColumns > 0) {
			// calc D^{prefix + c}

			uint32 *rowLengths = new uint32[mNumRows];
			memset(rowLengths, 0, mNumRows * sizeof(uint32));
			// !!! geen memset, maar memcpy van oude rowLengths, en for each depColumns[c] --?

			uint32 **newDepColumns = new uint32*[newNumDepColumns];
			uint32 *newDepColLens = new uint32[newNumDepColumns];
			uint32 newIdx = 0;
			uint16 *newItems = new uint16[newNumDepColumns];
			for(uint32 d=c+1; d<numDepColumns; d++) {

				uint32 oldDepColLen = depColLens[d];
				uint32 *newDepCol = new uint32[oldDepColLen];
				uint32 newDepColLen = 0;

				uint32 curA=0, numA = depColLens[c];
				uint32 curB=0, numB = depColLens[d];

				while(curA < numA && curB < numB) {
					if(depColumns[c][curA] > depColumns[d][curB])
						curB++;
					else if(depColumns[c][curA] < depColumns[d][curB])
						curA++;
					else {						// gelijk, voeg toe, up a&b
						rowLengths[depColumns[c][curA]]++;
						newDepCol[newDepColLen++] = depColumns[c][curA]; curA++; curB++;
					}
				}

				// issa intersecca, so fsck the rest!
				if(newDepColLen > 0) {
					// !!! hier op de juiste plek zetten, dan niet re-sort, whoo-hoo!
					newDepColumns[newIdx] = newDepCol;
					newDepColLens[newIdx] = newDepColLen;
					newItems[newIdx] = items[d];
					newIdx++;
				} else {
					delete[] newDepCol;
				}
			}
			newNumDepColumns = newIdx;
			// <- tha intersecca issa dun.
			if(newNumDepColumns > 0) {
				mStatBranchCount++;
				//uint32 myMinArea = minArea;
				MineLargestTilesDFSv1(minArea, newItems, newNumDepColumns, newDepColumns, newDepColLens, prefix, prefixLen+1, depColumns[c], depColLens[c], rowLengths);
				//minArea = myMinArea;
			}

			delete[] newItems;
			delete[] rowLengths;
			for(uint32 d=0; d<newNumDepColumns; d++)
				delete[] newDepColumns[d];
			delete[] newDepColumns;
			delete[] newDepColLens;
		}
	}

	delete[] newPrefixRowLens;
}

//////////////////////////////////////////////////////////////////////////
// Tile Selection
//////////////////////////////////////////////////////////////////////////
uint32 TilingMiner::SelectTile() {
	if(mTileSelectionStylee == TILE_SELECTION_THINNEST) {
		uint32 minWidth = UINT32_MAX_VALUE;
		uint32 minIdx = UINT32_MAX_VALUE;
		for(uint32 i=0; i<mTileBufferNum; i++) {
			if(minWidth > mTileBufferLen[i]) {
				minWidth = mTileBufferLen[i];
				minIdx = i;
			} else if(minWidth == mTileBufferLen[i]) {
				// if same width, choose lexicographically smallest (this assumes ordered tilebuffer!)
				memcpy_s(mAuxArrA, sizeof(uint16)*minWidth, mTileBuffer[minIdx],sizeof(uint16)*minWidth);
				memcpy_s(mAuxArrB, sizeof(uint16)*minWidth, mTileBuffer[i],sizeof(uint16)*minWidth);
				ArrayUtils::MSortAsc(mAuxArrA,minWidth);
				ArrayUtils::MSortAsc(mAuxArrB,minWidth);
				for(uint32 j=0; j<minWidth; j++) {
					if(mAuxArrA[j] > mAuxArrB[j]) {
						minIdx = i;
						break;
					}
				}
			}
		}
		return minIdx;

	} else if(mTileSelectionStylee == TILE_SELECTION_FATTEST) {
		uint32 maxWidth = UINT32_MIN_VALUE;
		uint32 minIdx = UINT32_MAX_VALUE;
		for(uint32 i=0; i<mTileBufferNum; i++) {
			if(mTileBufferLen[i] > maxWidth) {
				maxWidth = mTileBufferLen[i];
				minIdx = i;
			} else if(maxWidth == mTileBufferLen[i]) {
				// if same width, choose lexicographically smallest (this assumes ordered tilebuffer!)
				memcpy_s(mAuxArrA, sizeof(uint16)*maxWidth, mTileBuffer[minIdx],sizeof(uint16)*maxWidth);
				memcpy_s(mAuxArrB, sizeof(uint16)*maxWidth, mTileBuffer[i],sizeof(uint16)*maxWidth);
				ArrayUtils::MSortAsc(mAuxArrA,maxWidth);
				ArrayUtils::MSortAsc(mAuxArrB,maxWidth);
				for(uint32 j=0; j<maxWidth; j++) {
					if(mAuxArrB[j] < mAuxArrA[j]) {
						minIdx = i;
						break;
					}
				}
			}
		}
		return minIdx;

	} else if(mTileSelectionStylee == TILE_SELECTION_FIRST) {
		return 0;
	} 
	return 0;
}

void TilingMiner::SwapTileBufferEntries(uint32 a, uint32 b) {
	if(a==b)
		return;
	uint16 *tmpArr = mTileBuffer[a];
	mTileBuffer[a] = mTileBuffer[b];
	mTileBuffer[b] = tmpArr;

	uint32 tmpLen = mTileBufferLen[a];
	mTileBufferLen[a] = mTileBufferLen[b];
	mTileBufferLen[b] = tmpLen;
}
TileSelectionStylee TilingMiner::StringToTileSelectionStylee(string stylee) {
	if(stylee.compare("first") == 0)
		return TILE_SELECTION_FIRST;
	else if(stylee.compare("thin") == 0)
		return TILE_SELECTION_THINNEST;
	else if(stylee.compare("fat") == 0)
		return TILE_SELECTION_FATTEST;
	THROW("Unknown TileSelectionStylee");
}
void TilingMiner::InitTilesLogFile() {
	mLogFileTiles = NULL;
	if(mLogTiles == true) {
		string tileLogFilename = Bass::GetWorkingDir() + GenerateTileLogFilename(mDatabase);
		if(fopen_s(&mLogFileTiles, tileLogFilename.c_str(), "w") != 0)
			THROW("TileLogFile cannot be opened. Wat een bummer, geen Hummer!");
	}
}
void TilingMiner::InitTidsLogFile() {
	mLogFileTids = NULL;
	if(mLogTids == true) {
		string tidsLogFilename = Bass::GetWorkingDir() + GenerateTidsLogFilename(mDatabase);
		if(fopen_s(&mLogFileTids, tidsLogFilename.c_str(), "w") != 0)
			THROW("TidsLogFile cannot be opened. Wat een bummer, geen Hummer!");
	}
}
void TilingMiner::InitAuxiliaries() {
	ArrayUtils::InitMergeSort<uint16,uint16>(mDatabase->GetMaxSetLength(), false);
	//ArrayUtils::CreateStaticMess<uint16>(mDatabase->GetMaxSetLength(), false);
	mAuxArrA = new uint16[mDatabase->GetMaxSetLength()];
	mAuxArrB = new uint16[mDatabase->GetMaxSetLength()];
}
void TilingMiner::CleanUpAuxiliaries() {
	ArrayUtils::CleanUpMergeSort<unint16,uint16>();
	//ArrayUtils::CleanUpStaticMess();
	delete[] mAuxArrA; mAuxArrA = NULL;
	delete[] mAuxArrB; mAuxArrB = NULL;
}
void TilingMiner::InitData() {
	mDBName = mConfig->Read<string>("dbname");
	mRows = mDatabase->GetRows();
	mNumRows = mDatabase->GetNumRows();
	mNumItems = (uint32) mDatabase->GetAlphabetSize();
	mABSize = (uint32) mDatabase->GetAlphabetSize();
	mAlphabet = NULL;

	mDatabase->SortWithinRows();

	mHasBinSizes = mDatabase->HasBinSizes();
	if(mHasBinSizes == true)
		THROW("Can't really handle that, yet");

	// --- init row length histograms
	mRowLenPerLenHG = new uint32[mNumItems+1];


	// init mItems
	mItems = new uint16[mNumItems];
	mItemTranslationMap = new ItemMap();

	uint32 itemIdx=0;
	alphabet *ab = mDatabase->GetAlphabet();
	uint32 *itemsups = new uint32[mNumItems];

	uint32 *idxs = new uint32[mNumItems];
	for(alphabet::iterator ait = ab->begin(); ait != ab->end(); ++ait) {
		mItems[itemIdx] = ait->first;
		itemsups[itemIdx] = ait->second;
		idxs[itemIdx] = itemIdx;
		itemIdx++;
	}
	// sorts items on frequencies (if not done already)...
	ArrayUtils::BSortDesc(itemsups, mNumItems, idxs);
	ArrayUtils::Permute(mItems, idxs, mNumItems);
	itemIdx = 0;
	for(uint32 i=0; i<mNumItems; i++) {
		mItemTranslationMap->insert(ItemPair(mItems[i],i));
	}


	// init mItemTids
	mItemTids = new uint32*[mNumItems];
	mItemNumTids = new uint32[mNumItems];
	uint32 *abNR = mDatabase->GetAlphabetNumRows();
	uint32 maxABNR = 0;
	for(uint32 i=0; i<mNumItems; i++) {
		mItemNumTids[i] = 0;
		uint32 nr = abNR[idxs[i]];
		mItemTids[i] = new uint32[nr];
		if(nr > maxABNR)
			maxABNR = nr;
	}
	uint16 *auxArr = new uint16[mDatabase->GetMaxSetLength()];
	mFullTidList = new uint32[mNumRows];
	mRowLengths = new uint32[mNumRows];
	for(uint32 r=0; r<mNumRows; r++) {
		mFullTidList[r] = r;
		uint32 numValues = mRows[r]->GetLength();
		mRowLengths[r] = numValues;
		mRows[r]->GetValuesIn(auxArr);
		for(uint32 v=0; v<numValues; v++) {
			uint16 itemIdx = auxArr[v];
			ItemMap::iterator fnd = mItemTranslationMap->find(itemIdx);
			itemIdx = fnd->second;
			mItemTids[itemIdx][mItemNumTids[itemIdx]++] = r;
		}
	}
	delete[] auxArr;
	mABSize = mNumItems;
}
void TilingMiner::CleanUpData() {
	// --- tid list per item
	for(uint32 i=0; i<mDatabase->GetAlphabetSize(); i++)
		delete[] mItemTids[i];
	delete[] mItemTids;
	delete[] mItemNumTids;

	// --- items and translation
	delete[] mItems;
	delete mItemTranslationMap;

	// --- full tid list and orig rowlenghts
	delete[] mFullTidList;
	delete[] mRowLengths;
}
void TilingMiner::ResetStats() {
	mStatMinArea = UINT32_MAX_VALUE;
	mStatMinSup = UINT32_MAX_VALUE;
	mStatMaxSetLength = 0;
	mStatNumTilesMined = 0;
	mStatBranchCount = 0;
	mStatPruneCount = 0;
	mStatCoveredArea = 0;
}
#endif // BLOCK_TILING
