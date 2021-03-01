#ifdef BLOCK_TILING
#include "../../global.h"

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
#include <itemstructs/CoverSet.h>

#include "OverlappingTilingMiner.h"

OverlappingTilingMiner::OverlappingTilingMiner(Config *conf) : TilingMiner(conf) {
	// parameters
	mUpdateUncoveredAreaAbsolute = mConfig->Read<bool>("calcUncAbs", true);

	// init
	mCDB = NULL;
	mUncItemNumTids = NULL;
	mUncRowLengths = NULL;
}

OverlappingTilingMiner::~OverlappingTilingMiner() {
}

string OverlappingTilingMiner::GenerateOutputFileName(const Database *db) {
	return "tiling-ov-" + db->GetName() + "-" + TimeUtils::GetTimeStampString() + ".txt";
}
string OverlappingTilingMiner::GenerateTileLogFilename(const Database *db) {
	return "tiling-ov-" + db->GetName() + "-" + TimeUtils::GetTimeStampString() + ".tiles.log";
}
string OverlappingTilingMiner::GenerateTidsLogFilename(const Database *db) {
	return "tiling-ov-" + db->GetName() + "-" + TimeUtils::GetTimeStampString() + ".tids.log";
}

void OverlappingTilingMiner::MineTiling() {
	// --- stats
	ResetStats();

	// Initialize Aux-stuff
	InitAuxiliaries();

	// --- output files
	InitTilesLogFile();
	InitTidsLogFile();

	// Init Items and Data
	InitData();
	mCDB = new CoverSet*[mNumRows];
	for(uint32 r=0; r<mNumRows; r++)
		mCDB[r] = CoverSet::Create(mDatabase, mRows[r]);

	// init mTile
	mTile = ItemSet::CreateEmpty(mDatabase->GetDataType(), mNumItems,0);
	mTiledItems = ItemSet::CreateEmpty(mDatabase->GetDataType(), mNumItems,0);

	// mUnc stuff
	mUncItemNumTids = new uint32[mNumItems];
	mUncRowLengths = new uint32[mNumRows];
	mRowUncLenPerLenHG = new uint32[mNumItems+1];
	memcpy(mUncItemNumTids, mItemNumTids, sizeof(uint32)*mNumItems);
	memcpy(mUncRowLengths, mRowLengths, sizeof(uint32)*mNumRows);

	uint32 minArea = max((uint32)mDatabase->GetMaxSetLength(), mArgMinTileLength > 1 ? 0 : mUncItemNumTids[0]);

	uint16* currentTile = new uint16[mNumItems];
	uint32 *currentTidList = mFullTidList;
	if(mUpdateUncoveredAreaAbsolute == false) {
		currentTidList = new uint32[mNumRows];
		memcpy(currentTidList, mFullTidList, sizeof(uint32)*mNumRows);
	}

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

	if(mMinerVersion == 3 || mMinerVersion == 2) {
		uint32 auxArrLen = mDatabase->GetMaxSetLength()+1;
		mAuxPrefixUncRowLens = new uint32*[auxArrLen];
		mAuxPrefixRowLens = new uint32*[auxArrLen];

		for(uint32 i=0; i<auxArrLen; i++) {
			mAuxPrefixRowLens[i] = new uint32[mNumRows];
			mAuxPrefixUncRowLens[i] = new uint32[mNumRows];
		}
	}
	if(mMinerVersion == 3) {
		uint32 auxArrLen = mDatabase->GetMaxSetLength()+1;
		mAuxDepUncRowLengths = new uint32*[auxArrLen];
		mAuxDepRowLengths = new uint32*[auxArrLen];

		for(uint32 i=0; i<auxArrLen; i++) {
			mAuxDepUncRowLengths[i] = new uint32[mNumRows];
			mAuxDepRowLengths[i] = new uint32[mNumRows];
		}
	}

	if(mMinerVersion == 1 || mMinerVersion == 2 || mMinerVersion == 3) {

		uint32 minArea = 0;
		// Mine First Tile
		if(mArgMinTileLength > 1) {
			uint32 minArea = (uint32)mDatabase->GetMaxSetLength();
		} else if(mArgMinTileLength == 1) {
			minArea = mArgMinTileLength;
		} else {
			minArea = max((uint32)mDatabase->GetMaxSetLength(), mUncItemNumTids[0]);
		} 
		MineLargestTilesDFSv1(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths);
		numOnesCovered += mTileBufferArea;
		printf("R%d:\t%dbC\t%drP\t%dcO\t%dnT\n", ++curNumTiles, mStatBranchCount, numPrunedRows, numOnesCovered, mTileBufferNum);
		if(mLogTiles == true) fprintf(mLogFileTiles, "R%d:\t%dbC\t%drP\t%dcO\t%dnT\n", curNumTiles, mStatBranchCount, numPrunedRows, numOnesCovered, mTileBufferNum);
		mChosenTileIdx = SelectTile();
		OutputBufferedTiles(mChosenTileIdx);
		if(mLogTids == true) LogTids();

		// Mine the next Largest Tile, etc, etc, ad naeseum
		while(mTileBufferArea > 0 && curNumTiles < mMaxNumTiles) {
			for(uint32 i=0; i<mTileBufferLen[mChosenTileIdx]; i++) {
				uint16 tileItem = mTileBuffer[mChosenTileIdx][i];

				mTile->AddItemToSet(tileItem);
				mTiledItems->AddItemToSet(tileItem);
				mTile->Sort(); // looks nice, and is necessary for (faster) covering
			}

			// ... mTileBufferTidList etc
			bool colsEmpty = false;
			for(uint32 r=0; r<mNumRows; r++) {
				if(mCDB[r]->CanCoverWithOverlap(mTileBufferLen[mChosenTileIdx],mTile)) {
					// Check for each abItem of uncCount > 0 is, else zap
					for(uint32 i=0; i<mTileBufferLen[mChosenTileIdx]; i++) {
						if(mCDB[r]->IsItemUncovered(mTileBuffer[mChosenTileIdx][i])) {
							uint16 tileItem = mTileBuffer[mChosenTileIdx][i];
							uint16 tileTrans = mItemTranslationMap->find(tileItem)->second;
							mUncItemNumTids[tileTrans]--;
							mUncRowLengths[r]--;
							if(mUncItemNumTids[tileTrans] == 0)
								colsEmpty = true;
						}
					}
					mStatCoveredArea += mCDB[r]->CoverWithOverlapNoSubsetCheck(mTileBufferLen[mChosenTileIdx],mTile);

					if(mCDB[r]->GetNumUncovered() == 0) {
						numPrunedRows++;
						// Remove this row from mColumns, update mColumnLengths, update rowLengths, currentTidList
					}
				}
			}

			// Prune columns that are fully covered
			if(mColumnPruningEnabled == true && colsEmpty == true) {
				// .. eigenlijk alleen over mTileBuffer lopen, en terugmappen naar posItems
				for(uint32 i=0; i<mNumItems; i++) {
					if(mUncItemNumTids[i]==0) {
						printf("Item %d wordt gezapt\n", mItems[i]);
						if(mLogTiles == true) fprintf(mLogFileTiles, "Item %d wordt gezapt\n", mItems[i]);
						// zap dit item
						for(uint32 t=0; t<mItemNumTids[i]; t++)
							mRowLengths[mItemTids[i][t]]--;
						mItemNumTids[i] = mItemNumTids[mNumItems-1];
						mItemNumTids[mNumItems-1] = 0;

						mUncItemNumTids[i] = mUncItemNumTids[mNumItems-1];
						mUncItemNumTids[mNumItems-1] = 0;

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
				mTileBufferNum--;	// zoveel zitter nog in, minimaal 1

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
							if(mUncItemNumTids[i] > minArea) {
								minArea = mUncItemNumTids[i];
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
					if(mUpdateUncoveredAreaAbsolute == false)
						memcpy(currentTidList,mFullTidList,sizeof(uint32)*mNumRows);
					if(mMinerVersion == 1)
						MineLargestUncoveredTilesDFSv1(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths, 0);
					else if(mMinerVersion == 2)
						MineLargestUncoveredTilesDFSv2(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths, mUncRowLengths, 0);
					else if(mMinerVersion == 3)
						MineLargestUncoveredTilesDFSv3(minArea, posItems, numPosItems, posItemTids, posItemNumTids, currentTile, 0, currentTidList, mNumRows, mRowLengths, mUncRowLengths, 0);
				}
			}

			// output TileBuffer
			if(mTileBufferArea > 0 && mTileBufferNum > 0) {
				numOnesCovered += mTileBufferArea;
				mChosenTileIdx = SelectTile();
				printf("R%d:\t%dbC\t%drP\t%dcO\t%dnT\n", ++curNumTiles, mStatBranchCount, numPrunedRows, numOnesCovered, mTileBufferNum);
				if(mLogTiles == true) fprintf(mLogFileTiles, "R%d:\t%dbC\t%drP\t%dcO\t%dnT\n", curNumTiles, mStatBranchCount, numPrunedRows, numOnesCovered, mTileBufferNum);
				OutputBufferedTiles(mChosenTileIdx);
				if(mLogTids == true) LogTids();
			}
		} 
		delete[] prunedRows;
	} 
	if(mUpdateUncoveredAreaAbsolute != false) {
		currentTidList = NULL;
	}

	if(mMinerVersion == 3 || mMinerVersion == 2) {
		uint32 auxArrLen = mDatabase->GetMaxSetLength()+1;
		for(uint32 i=0; i<auxArrLen; i++) {
			delete[] mAuxPrefixRowLens[i];
			delete[] mAuxPrefixUncRowLens[i];
		}
		delete[] mAuxPrefixUncRowLens;
		delete[] mAuxPrefixRowLens;
	}
	if(mMinerVersion == 3) {
		uint32 auxArrLen = mDatabase->GetMaxSetLength()+1;
		for(uint32 i=0; i<auxArrLen; i++) {
			delete[] mAuxDepUncRowLengths[i];
			delete[] mAuxDepRowLengths[i];
		}
		delete[] mAuxDepUncRowLengths;
		delete[] mAuxDepRowLengths;
	}

	printf("covered area: %" I64d "\n", mStatCoveredArea);

	if(mLogTiles == true) {
		fclose(mLogFileTiles);
		mLogFileTiles = NULL;
	}
	if(mLogTids == true) {
		fclose(mLogFileTids);
		mLogFileTids = NULL;
	}

	// Clean up aux-stuff
	CleanUpAuxiliaries();

	CleanUpData();

	delete[] currentTile;
	delete[] currentTidList;

	for(uint32 i=0; i<mDatabase->GetAlphabetSize(); i++)
		delete[] origPosItemTids[i];
	delete[] origPosItemTids;
	delete[] posItemNumTids;
	delete[] posItemTids;
	delete[] posItems;

	// --- kill tiles
	delete mTile;
	delete mTiledItems;

	// --- uncovered item counts
	delete[] mUncItemNumTids;
	delete[] mUncRowLengths;

	// --- coversets
	for(uint32 i=0; i<mNumRows; i++)
		delete mCDB[i];
	delete[] mCDB;

	delete[] mAlphabet;

	// --- kill row length histograms
	delete[] mRowLenPerLenHG;
	delete[] mRowUncLenPerLenHG;

}

void OverlappingTilingMiner::LogTids() {
	mTile->CleanTheSlate();
	for(uint32 i=0; i<mTileBufferLen[mChosenTileIdx]; i++) {
		uint16 tileItem = mTileBuffer[mChosenTileIdx][i];

		mTile->AddItemToSet(tileItem);
		mTile->Sort(); // looks nice, and is necessary for (faster) covering
	}
	for(uint32 r=0; r<mNumRows; r++) {
		if(mCDB[r]->CanCoverWithOverlap(mTileBufferLen[mChosenTileIdx],mTile)) {
			fprintf(mLogFileTids, "%d ", r);
		}
	}
	fprintf(mLogFileTids, "\n");
	mTile->CleanTheSlate();
}




//////////////////////////////////////////////////////////////////////////
/// Prune Pos Items
//////////////////////////////////////////////////////////////////////////
void OverlappingTilingMiner::PrunePosItems(uint32 minArea, uint16* items, uint32& numDepColumns, uint32** depColumns, uint32* depColLens, uint32 prefixArea, uint32 prefixLen, uint32 *newPrefixRowLens) {
	bool stuffPruned = true;
	while(stuffPruned == true) {
		stuffPruned = false;
		for(uint32 c=0; c<numDepColumns; c++) {
			// Build Length-Histogram
			memset(mRowLenPerLenHG, 0, (numDepColumns + 1) * sizeof(uint32));
			uint32 minLen = mNumItems-1;
			uint32 maxLen = 0;
			for(uint32 i=0; i<depColLens[c]; i++) {
				uint32 len = newPrefixRowLens[depColumns[c][i]];
				if(len < minLen) minLen = len;
				if(len > maxLen) maxLen = len;
				mRowLenPerLenHG[len]++;
			}
			uint32 cUB = 0;
			uint32 rowSum = 0;
			for(uint32 l=minLen; l<=maxLen; l++) {
				rowSum += mRowLenPerLenHG[l];
				if((min(prefixArea, prefixLen*rowSum)+(l*rowSum)) > cUB)
					cUB = min(prefixArea, prefixLen*rowSum)+(l*rowSum);
			}
			if(cUB < minArea) {
				stuffPruned = true;
				// + Pas `prefixRowLens` aan
				for(uint32 i=0; i<depColLens[c]; i++)
					newPrefixRowLens[depColumns[c][i]]--;
				// + zap uit items (nu niet meer geordend)	<- !!! keep sorted, dan kunnen we twee vliegen in een klap slaan
				items[c] = items[numDepColumns-1];
				// + zap uit depColLens
				depColLens[c] = depColLens[numDepColumns-1];
				depColLens[numDepColumns-1] = 0;
				// + zap uit depColumns
				uint32 *tmp = depColumns[c];
				depColumns[c] = depColumns[numDepColumns-1];
				depColumns[numDepColumns-1] = tmp;

				numDepColumns--;
				// + Zorg dat restart van loop goed gaat
				c--;
			}
		}
	}
}
void OverlappingTilingMiner::PrunePosItemsV2(uint32 minArea, uint16* items, uint32& numDepColumns, uint32** depColumns, uint32* depColLens, uint32 prefixArea, uint32 prefixLen, uint32 *newPrefixRowLens, uint32 *newPrefixUncRowLens) {
	bool stuffPruned = true;
	while(stuffPruned == true) {
		stuffPruned = false;
		for(uint32 c=0; c<numDepColumns; c++) {
			// Build Length-Histogram
			memset(mRowLenPerLenHG, 0, (numDepColumns+1) * sizeof(uint32));
			memset(mRowUncLenPerLenHG, 0, (numDepColumns+1) * sizeof(uint32));
			uint32 minLen = mNumItems-1;
			uint32 maxLen = 0;
			for(uint32 i=0; i<depColLens[c]; i++) {
				uint32 len = newPrefixRowLens[depColumns[c][i]];
				uint32 unc = newPrefixUncRowLens[depColumns[c][i]];
				if(len < minLen) minLen = len;
				if(len > maxLen) maxLen = len;
				mRowLenPerLenHG[len]++;
				mRowUncLenPerLenHG[len] += unc;
			}
			uint32 cUB = 0;
			uint32 uncSum = 0, rowSum = 0;
			for(uint32 l=minLen; l<=maxLen; l++) {
				rowSum += mRowLenPerLenHG[l];
				uncSum += mRowUncLenPerLenHG[l];
				//				if((min(prefixArea, prefixLen*rowSum)+(l*rowSum)) > cUB)
				//					cUB = min(prefixArea, prefixLen*rowSum)+(l*rowSum);
				if(min(prefixArea, prefixLen*rowSum) + uncSum > cUB)	// min(prefixArea, prefixLen*nr
					cUB = min(prefixArea, prefixLen*rowSum) + uncSum;
				//if(prefixArea + uncSum > cUB)	// min(prefixArea, prefixLen*nr
				//	cUB = prefixArea + uncSum;
			}
			if(cUB < minArea) {
				stuffPruned = true;
				// + Pas `prefixRowLens` aan
				for(uint32 i=0; i<depColLens[c]; i++) {
					newPrefixRowLens[depColumns[c][i]]--;
					if(mCDB[depColumns[c][i]]->IsItemUncovered(items[c])) {
						newPrefixUncRowLens[depColumns[c][i]]--;
					}
				}
				// + zap uit items (nu niet meer geordend)	<- !!! keep sorted, dan kunnen we twee vliegen in een klap slaan
				items[c] = items[numDepColumns-1];
				// + zap uit depColLens
				depColLens[c] = depColLens[numDepColumns-1];
				depColLens[numDepColumns-1] = 0;
				// + zap uit depColumns
				uint32 *tmp = depColumns[c];
				depColumns[c] = depColumns[numDepColumns-1];
				depColumns[numDepColumns-1] = tmp;

				numDepColumns--;
				// + Zorg dat restart van loop goed gaat
				c--;
			} 
		}
	}
}


uint32 OverlappingTilingMiner::UpdateUncArea(uint16* tile, uint32 tileWidth, uint32* tidlist, uint32 numTids, uint32 *prevTidList, uint32 prevNumTids, uint32 prevArea) {
	uint32 area;
	if(mUpdateUncoveredAreaAbsolute == true) {
		// AbsAreaCalc (make sure that itemset = prev+cur)
		area = 0;
		for(uint32 t=0; t<numTids; t++)
			area += mCDB[tidlist[t]]->NumCoverWithOverlap(tileWidth, mTile);
	} else {
		// RelAreaCalc (make sure that itemset = prev)
		area = prevArea;
		uint16 lastItem = tile[tileWidth-1];
		// tidList \subset prevTidList
		// dus gewoon over prevTidList en dan aanpassen maar!
		if(tileWidth == 1) {
			for(uint32 t=0; t<numTids; t++) {
				if(mCDB[tidlist[t]]->IsItemUncovered(lastItem))
					area++;
			}
		} else {
			uint32 prevWidth = tileWidth-1;
			uint32 p=0, t=0;
			for(; p<prevNumTids && t<numTids; p++) {
				if(tidlist[t] == prevTidList[p]) { 
					if(mCDB[tidlist[t++]]->IsItemUncovered(lastItem))
						area++;
				} else
					area -= mCDB[prevTidList[p]]->NumCoverWithOverlap(prevWidth, mTile);
			}
			for(;p<prevNumTids; p++) {
				area -= mCDB[prevTidList[p]]->NumCoverWithOverlap(prevWidth, mTile);
			}
		}
	}
	return area;
}

//////////////////////////////////////////////////////////////////////////
/// Mine Uncovered Tiles DFS
//////////////////////////////////////////////////////////////////////////
void OverlappingTilingMiner::MineLargestUncoveredTilesDFSv1(uint32 &minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens, uint32 prefixArea) {
	if(numDepColumns == 0)
		return;

	uint32 UB = minArea;

	// prune
	uint32 *newPrefixRowLens = new uint32[mNumRows];
	memcpy(newPrefixRowLens, prefixRowLens, sizeof(uint32) * mNumRows);
	PrunePosItems(minArea, items, numDepColumns, depColumns, depColLens, prefixArea, prefixLen, newPrefixRowLens);

	// for each item i, see what happens when we extend `prefix` with i
	for(uint32 c=0; c<numDepColumns; c++) {

		uint32 cndAreaUB = (prefixLen + 1) * depColLens[c];
		uint16 addedItem = items[c];
		prefix[prefixLen] = items[c];
		if(mUpdateUncoveredAreaAbsolute == true)
			mTile->AddItemToSet(addedItem);
		uint32 cndAreaUnc = UpdateUncArea(prefix, prefixLen+1, depColumns[c], depColLens[c], prefixRowIdxs, prefixNumRows, prefixArea);
		if(mUpdateUncoveredAreaAbsolute == false)
			mTile->AddItemToSet(addedItem);

		// mItemset fixen
		uint32 cndArea = cndAreaUnc;

		if(cndArea >= minArea && prefixLen+1 >= mArgMinTileLength) {
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
				uint32 myMinArea = minArea;
				MineLargestUncoveredTilesDFSv1(minArea, newItems, newNumDepColumns, newDepColumns, newDepColLens, prefix, prefixLen+1, depColumns[c], depColLens[c], rowLengths, cndArea);
				/*if(myMinArea < minArea && items+c+2<numDepColumns) {
				PrunePosItems(minArea, items+c+2, numDepColumns-c-2, depColumns+c+2, depColLens+c+2, prefixArea, prefixLen, rowLenHG, newPrefixRowLens);
				}*/
			}

			delete[] newItems;
			delete[] rowLengths;
			for(uint32 d=0; d<newNumDepColumns; d++)
				delete[] newDepColumns[d];
			delete[] newDepColumns;
			delete[] newDepColLens;
		}
		mTile->RemoveItemFromSet(addedItem);
	}

	delete[] newPrefixRowLens;
}
void OverlappingTilingMiner::MineLargestUncoveredTilesDFSv2(uint32 &minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens, uint32 *prefixUncRowLens, uint32 prefixArea) {
	if(numDepColumns == 0)
		return;

	uint32 UB = minArea;

	// prune
	memcpy(mAuxPrefixRowLens[prefixLen], prefixRowLens, sizeof(uint32) * mNumRows);
	memcpy(mAuxPrefixUncRowLens[prefixLen], prefixUncRowLens, sizeof(uint32) * mNumRows);
	PrunePosItemsV2(minArea, items, numDepColumns, depColumns, depColLens, prefixArea, prefixLen, mAuxPrefixRowLens[prefixLen], mAuxPrefixUncRowLens[prefixLen]);

	// for each item i, see what happens when we extend `prefix` with i
	for(uint32 c=0; c<numDepColumns; c++) {

		uint32 cndAreaUB = (prefixLen + 1) * depColLens[c];
		uint16 addedItem = items[c];
		prefix[prefixLen] = items[c];
		if(mUpdateUncoveredAreaAbsolute == true)
			mTile->AddItemToSet(addedItem);
		uint32 cndAreaUnc = UpdateUncArea(prefix, prefixLen+1, depColumns[c], depColLens[c], prefixRowIdxs, prefixNumRows, prefixArea);
		if(mUpdateUncoveredAreaAbsolute == false)
			mTile->AddItemToSet(addedItem);

		// mItemset fixen
		uint32 cndArea = cndAreaUnc;

		if(cndArea >= minArea && prefixLen+1 >= mArgMinTileLength) {
			BufferLargestTiles(prefix, prefixLen + 1, depColLens[c], cndArea);
			if(cndArea > minArea)
				minArea = cndArea;	// .. prune-kans?
		} 

		uint32 UB = minArea;
		// misschien nog extendable

		// pre-UB calc, assumes full-subsetting
		// ub1 = voor alle lengtes l, bereken max
		uint32 newNumDepColumns = numDepColumns-c-1;
		if(UB >= minArea && newNumDepColumns > 0) {
			// calc D^{prefix + c}

			uint32 *depRowLengths = new uint32[mNumRows];
			memset(depRowLengths, 0, mNumRows * sizeof(uint32));
			uint32 *depUncRowLengths = new uint32[mNumRows];
			memset(depUncRowLengths, 0, mNumRows * sizeof(uint32));
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
						depRowLengths[depColumns[c][curA]]++;
						if(mCDB[depColumns[c][curA]]->IsItemUncovered(items[d]))
							depUncRowLengths[depColumns[c][curA]]++;
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
				uint32 myMinArea = minArea;
				MineLargestUncoveredTilesDFSv2(minArea, newItems, newNumDepColumns, newDepColumns, newDepColLens, prefix, prefixLen+1, depColumns[c], depColLens[c], depRowLengths, depUncRowLengths, cndArea);
				if(myMinArea < minArea && c+1<numDepColumns) {
					uint32 tmpNumDepColumns = numDepColumns-c-1;
					PrunePosItemsV2(minArea, items+c+1, tmpNumDepColumns, depColumns+c+1, depColLens+c+1, prefixArea, prefixLen, mAuxPrefixRowLens[prefixLen], mAuxPrefixUncRowLens[prefixLen]);
					numDepColumns = c+1+tmpNumDepColumns;
				}
			}

			delete[] newItems;
			delete[] depRowLengths;
			delete[] depUncRowLengths;
			for(uint32 d=0; d<newNumDepColumns; d++)
				delete[] newDepColumns[d];
			delete[] newDepColumns;
			delete[] newDepColLens;
		}
		mTile->RemoveItemFromSet(addedItem);
	}
}
void OverlappingTilingMiner::MineLargestUncoveredTilesDFSv3(uint32 &minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens, uint32 *prefixUncRowLens, uint32 prefixArea) {
	if(numDepColumns == 0)
		return;

	uint32 UB = minArea;

	// Prunayeah
	memcpy(mAuxPrefixRowLens[prefixLen], prefixRowLens, sizeof(uint32) * mNumRows);
	memcpy(mAuxPrefixUncRowLens[prefixLen], prefixUncRowLens, sizeof(uint32) * mNumRows);
	PrunePosItemsV2(minArea, items, numDepColumns, depColumns, depColLens, prefixArea, prefixLen, mAuxPrefixRowLens[prefixLen], mAuxPrefixUncRowLens[prefixLen]);

	// for each item i, see what happens when we extend `prefix` with i
	for(uint32 c=0; c<numDepColumns; c++) {

		uint32 cndAreaUB = (prefixLen + 1) * depColLens[c];
		uint16 addedItem = items[c];
		prefix[prefixLen] = items[c];
		if(mUpdateUncoveredAreaAbsolute == true)
			mTile->AddItemToSet(addedItem);
		uint32 cndAreaUnc = UpdateUncArea(prefix, prefixLen+1, depColumns[c], depColLens[c], prefixRowIdxs, prefixNumRows, prefixArea);
		if(mUpdateUncoveredAreaAbsolute == false)
			mTile->AddItemToSet(addedItem);

		// mItemset fixen
		uint32 cndArea = cndAreaUnc;

		if(cndArea >= minArea && prefixLen+1 >= mArgMinTileLength) {
			BufferLargestTiles(prefix, prefixLen + 1, depColLens[c], cndArea);
			if(cndArea > minArea)
				minArea = cndArea;	// .. prune-kans?
		} 

		uint32 UB = minArea;
		// misschien nog extendable

		// pre-UB calc, assumes full-subsetting
		// ub1 = voor alle lengtes l, bereken max
		uint32 newNumDepColumns = numDepColumns-c-1;
		if(UB >= minArea && newNumDepColumns > 0) {
			// calc D^{prefix + c}
			memset(mAuxDepRowLengths[prefixLen], 0, mNumRows * sizeof(uint32));
			memset(mAuxDepUncRowLengths[prefixLen], 0, mNumRows * sizeof(uint32));
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
						mAuxDepRowLengths[prefixLen][depColumns[c][curA]]++;
						if(mCDB[depColumns[c][curA]]->IsItemUncovered(items[d]))
							mAuxDepUncRowLengths[prefixLen][depColumns[c][curA]]++;
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
				uint32 myMinArea = minArea;
				MineLargestUncoveredTilesDFSv3(minArea, newItems, newNumDepColumns, newDepColumns, newDepColLens, prefix, prefixLen+1, depColumns[c], depColLens[c], mAuxDepRowLengths[prefixLen], mAuxDepUncRowLengths[prefixLen], cndArea);
				if(myMinArea < minArea && c+1<numDepColumns) {
					uint32 tmpNumDepColumns = numDepColumns-c-1;
					PrunePosItemsV2(minArea, items+c+1, tmpNumDepColumns, depColumns+c+1, depColLens+c+1, prefixArea, prefixLen, mAuxPrefixRowLens[prefixLen], mAuxPrefixUncRowLens[prefixLen]);
					numDepColumns = c+1+tmpNumDepColumns;
				}
			}

			delete[] newItems;
			//delete[] depRowLengths;
			//delete[] depUncRowLengths;
			for(uint32 d=0; d<newNumDepColumns; d++)
				delete[] newDepColumns[d];
			delete[] newDepColumns;
			delete[] newDepColLens;
		}
		mTile->RemoveItemFromSet(addedItem);
	}
}
#endif // BLOCK_TILING
