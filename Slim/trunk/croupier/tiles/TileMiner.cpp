#ifdef BLOCK_TILING
#include "../clobal.h"

#include <string>
#include <stack>

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <ArrayUtils.h>
#include <Templates.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <itemstructs/CoverSet.h>
#include <isc/ItemSetCollection.h>

#include "TileOut.h"

#include "TileMiner.h"

Tile::Tile(uint16* items, uint32 numItems, uint32 supp, uint32 area) : mItems(NULL), mNumItems(numItems), mSupp(supp), mArea(area) {
	mItems = new uint16[mNumItems];
	memcpy_s(mItems, mNumItems*sizeof(uint16), items, numItems*sizeof(uint16));
}
Tile::~Tile() {
	if (mItems) delete[] mItems;
}

// Operator for equality comparison for object
bool Tile::operator==(const Tile& t) const {
	// VS2010 only likes safe function calls
	//return (mArea == t.mArea) && ((mSupp*mSupp + mNumItems*mNumItems) == (t.mSupp*t.mSupp + t.mNumItems*t.mNumItems)) && equal<uint16*,uint16*>(mItems,mItems+mNumItems,t.mItems);

	if((mArea == t.mArea) && ((mSupp*mSupp + mNumItems*mNumItems) == (t.mSupp*t.mSupp + t.mNumItems*t.mNumItems)))
		return memcmp(mItems, t.mItems, mNumItems * sizeof(uint16)) == 0;
	return false;
}

bool Tile::operator<(const Tile& t) const{
	if (mArea < t.mArea)
		return true;
	else if ((mArea == t.mArea) && ((mSupp*mSupp + mNumItems*mNumItems) < (t.mSupp*t.mSupp + t.mNumItems*t.mNumItems))) // prefer squared tiles
		return true;
	else if ((mArea == t.mArea) && ((mSupp*mSupp + mNumItems*mNumItems) == (t.mSupp*t.mSupp + t.mNumItems*t.mNumItems)))
		return lexicographical_compare(mItems, mItems + mNumItems, t.mItems, t.mItems + t.mNumItems);
	return false;
}

TileMiner::TileMiner(Config *conf) : Croupier(conf), mTopKTiles(gt<Tile>) {
	mConfig = conf;
	mUseMinerVersion = mConfig->Read<uint32>("version", 1);
	mMinArea = mConfig->Read<uint32>("minarea",2);
	mHasBinSizes = false;
	mTopK = mConfig->Read<uint32>("top-k",0);
	mMinTileLength = mConfig->Read<uint32>("minTileLength", 1);
	mMaxTileLength = mConfig->Read<uint32>("maxTileLength", UINT32_MAX_VALUE);


	mOutput = NULL;
	mIscOrder = NoneIscOrder;
}
TileMiner::TileMiner() : Croupier(), mTopKTiles(gt<Tile>) {
	// Using defaults, adjust using SetVariable(..)
	mUseMinerVersion = 1;
	mMinArea = 2;
	mTopK = 0;
	mMinTileLength = 1;
	mMaxTileLength = UINT32_MAX_VALUE;

	mOutput = NULL;
	mIscOrder = NoneIscOrder;
}

TileMiner::~TileMiner() {
	// clean up output
	delete mOutput;

	// not my Config*

	for(uint32 i=0; i<mABSize; i++)
		delete[] mColumns[i];
	delete[] mColumns;
	delete[] mColumnLengths;
	delete[] mAlphabet;
}

string TileMiner::GenerateOutputFileName(const Database *db) {
	char buf[200];
	_itoa_s(mMinArea, buf, 200, 10);
	return Bass::GetWorkingDir() + "tiles-" + db->GetName() + "-" + buf + ".txt";
}

bool TileMiner::MinePatternsToFile(const string &dbFilename, const string &outputFileName, Database *db) {
	MinePatternsToFile(db, outputFileName);
	return true;
}
bool TileMiner::MinePatternsToFile(Database *db, const string &outputFileName) {
	mDatabase = db;

	if(mDatabase->HasBinSizes())
		THROW("Cant do binsizes yet...");

	// Stel Output correct in
	mOutput = new tlmnr::FSout(outputFileName, this);

	MineTiles();

	// Kill output appropriately
	delete mOutput;
	mOutput = NULL;

	return true;
}

ItemSetCollection* TileMiner::MinePatternsToMemory(Database *db) {
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
		ECHO(2, printf("TileMiner cant handle binsizes proplerly yet..."));

	mDbName = db->GetName();

	// prep da mining shizzle
	InitOnline(db);

	// do da mining dance
	MineTiles();
	// mine has been putted out

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

	// Kill output appropriately
	delete mOutput;
	mOutput = NULL;

	printf("\r * Mining:\t\tFinished mining %" I64d " ItemSets.\t\n", isc->GetNumItemSets());
	return isc;
}

void TileMiner::InitOnline(Database *db) {
	Croupier::InitOnline(db);

	mMemOut = new tlmnr::MemoryOut(db, this);
	mOutput = mMemOut;

	uint32 bufferSize = (uint32)(Bass::GetRemainingMaxMemUse() / (uint64)(2.5 * sizeof(uint32) + ItemSet::MemUsage(db)));
	//printf("buffersize = %d, want\n mem remaining: %I64d\n vage num bytes: %I64d\n per itemset: %d\n", bufferSize, Bass::GetRemainingMaxMemUse(), (uint64)(2.5 * sizeof(uint32) + ItemSet::MemUsage(db)), ItemSet::MemUsage(db));
	if(bufferSize == 0) bufferSize = 1;
	mMemOut->InitBuffer(bufferSize);
	mMayHaveMore = true;
}


void TileMiner::OutputTile(uint16* items, uint32 numItems, uint32 supp, uint32 area) {
	mNumTilesMined++;
	if(numItems > mMaxSetLength)
		mMaxSetLength = numItems;
	if(supp < mMinSup)
		mMinSup = supp;

	mOutput->OutputSet(numItems, items, supp, area, 0);
}

void TileMiner::MineTiles() {
	mNumTilesMined = 0;
	mMaxSetLength = 0;
	mMinSup = UINT32_MAX_VALUE;

	mRows = mDatabase->GetRows();
	mABSize = (uint32) mDatabase->GetAlphabetSize();
	mNumRows = mDatabase->GetNumRows();
	mColumns = new uint32*[mABSize];
	mAlphabet = new uint16[mABSize];
	alphabet *ab = mDatabase->GetAlphabet();

	uint32 i=0;
	alphabet *abIdx = new alphabet();
	for(alphabet::iterator ait = ab->begin(); ait != ab->end(); ++ait) {
		abIdx->insert(alphabetEntry(ait->first, i));
		mAlphabet[i++] = ait->first;
	}

	mColumnLengths = new uint32[mABSize];
	uint32 *abNR = mDatabase->GetAlphabetNumRows();
	for(uint32 i=0; i<mABSize; i++) {
		mColumnLengths[i] = 0;
		mColumns[i] = new uint32[abNR[i]];
	}
	uint16 *tmpArr = new uint16[mDatabase->GetMaxSetLength()];
	uint32 *fullTidList = new uint32[mNumRows];
	uint32* rowLengths = new uint32[mNumRows];

	for(uint32 r=0; r<mNumRows; r++) {
		fullTidList[r] = r;
		uint32 numValues = mRows[r]->GetLength();
		rowLengths[r] = numValues;
		mRows[r]->GetValuesIn(tmpArr);
		for(uint32 v=0; v<numValues; v++) {
			// now does not use item itself, but it's index in the alphabet
			uint32 vIdx = abIdx->find(tmpArr[v])->second;
			mColumns[vIdx][mColumnLengths[vIdx]++] = r;
		}
	}

	uint16* currentTile = new uint16[mABSize];
	uint32* currentTidList = new uint32[mNumRows];
	memcpy(currentTidList,fullTidList,sizeof(uint32)*mNumRows);

	if(mUseMinerVersion == 1) {
		MineTilesDFSv1(mMinArea, mAlphabet, mABSize, mColumns, mColumnLengths, currentTile, 0, currentTidList, mNumRows, rowLengths);
	} else if(mUseMinerVersion == 2) {
		mPruneCount = 0;
		mOrigIdxs = new uint32[mABSize];
		for(uint32 i=0; i<mABSize; i++)	// memcpy from auxArr
			mOrigIdxs[i] = i;
		mAuxIdxs = new uint32[mABSize];
		ArrayUtils::InitMergeSort<uint32,uint32>(mABSize, true);
		MineTilesDFSv2(mMinArea, mAlphabet, mABSize, mColumns, mColumnLengths, currentTile, 0, currentTidList, mNumRows, rowLengths);
		ArrayUtils::CleanUpMergeSort<uint32,uint32>();
		delete[] mAuxIdxs;
		delete[] mOrigIdxs;
		printf("pruned %d search branches\n", mPruneCount);
	}
	if (mTopK) {
		while (!mTopKTiles.empty()) {
			const Tile* t = mTopKTiles.top();
			OutputTile(t->mItems, t->mNumItems, t->mSupp, t->mArea);
			mTopKTiles.pop();
			delete t;
		}

	}
	mOutput->SetFinished();

	delete[] currentTile;
	delete[] currentTidList;

	delete[] fullTidList;
	delete[] tmpArr;
	delete[] rowLengths;

	delete abIdx;
}

void TileMiner::MineTilesDFSv1(uint32 minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens) {
	if(numDepColumns == 0)
		return;

	// prune
	uint32 UB = minArea;
	uint32 updatedMinArea = minArea;
	bool stuffPruned = true;
	uint32 *newPrefixRowLens = new uint32[mNumRows];
	memcpy(newPrefixRowLens, prefixRowLens, sizeof(uint32) * mNumRows);
	uint32 *rowLenHG = new uint32[mABSize+1];
	while(stuffPruned == true) {
		stuffPruned = false;
		for(uint32 c=0; c<numDepColumns; c++) {
			// Build Length-Histogram
			memset(rowLenHG, 0, mABSize * sizeof(uint32));
			uint32 minLen = mABSize-1;
			uint32 maxLen = 0;
			for(uint32 i=0; i<depColLens[c]; i++) {
				uint32 len = newPrefixRowLens[depColumns[c][i]];
				if(len < minLen) minLen = len;
				if(len > maxLen) maxLen = len;
				rowLenHG[len]++;
			}
			uint32 cUB = 0;
			uint32 sum = 0;
			for(uint32 l=minLen; l<=maxLen; l++) {
				sum += rowLenHG[l];
				if(((prefixLen + l) * sum) > cUB)
					cUB = (prefixLen + l) * sum;
			}
			if(cUB < minArea) {
				stuffPruned = true;
				// + Pas `prefixRowLens` aan
				for(uint32 i=0; i<depColLens[c]; i++) {
					newPrefixRowLens[depColumns[c][i]]--;
				}
				// + zap uit items (nu niet meer geordend)
				items[c] = items[numDepColumns-1];
				// + zap uit depColLens
				depColLens[c] = depColLens[numDepColumns-1];
				depColLens[numDepColumns-1] = 0;
				// + zap uit depColumns
				delete[] depColumns[c];
				depColumns[c] = depColumns[numDepColumns-1];
				depColumns[numDepColumns-1] = NULL;

				numDepColumns--;
				// + Zorg dat restart van loop goed gaat
				c--;
			}
		}
	}
	delete[] rowLenHG;

	// for each item i, see what happens when we extend `prefix` with i
	for(uint32 c=0; c<numDepColumns; c++) {
		uint32 UB = minArea;
		uint32 cndArea = (prefixLen + 1) * depColLens[c];
		prefix[prefixLen] = items[c];

		if(cndArea > minArea && prefixLen+1 >= mMinTileLength && prefixLen+1 <= mMaxTileLength) {
			if (mTopK) {
				/*
					In order to find the top-k largest tiles, we adapt the LTM algorithm as follows.
					Initially the minimum area threshold is zero. Then, after the algorithm has
					generated the first k large tiles, it increases the minimum area threshold to the
					size of the smallest of these k tiles. From here on, the minimum area threshold
					can be increased every time a large tile is generated w.r.t. the current threshold.
					All generated tiles that do not have an area larger than the increased minimum
					area threshold can of course be removed.

					See: Tiling Databases by Floris Geerts, Bart Goethals, and Taneli Mielikäinen in Discovery Science; Vol. 3245, pages 278–289, Springer, 2004.
				*/
				mTopKTiles.push(new Tile(prefix, prefixLen + 1, depColLens[c], cndArea));
				stack<Tile*> bottomTiles;
				while (mTopKTiles.size() > mTopK) {
					bottomTiles.push(mTopKTiles.top());
					mTopKTiles.pop();
				}
				while (!bottomTiles.empty()) {
					if (bottomTiles.top()->mArea == mTopKTiles.top()->mArea) {
						mTopKTiles.push(bottomTiles.top());
						bottomTiles.pop();
					} else {
						delete bottomTiles.top();
						bottomTiles.pop();
					}
					updatedMinArea = mTopKTiles.top()->mArea;
				}
			} else {
				// deez is al goed genoeg
				OutputTile(prefix, prefixLen + 1, depColLens[c], cndArea);
			}
		} 

		if(UB >= minArea) {
			// calc D^{prefix + c}
			uint32 *rowLengths = new uint32[mNumRows];
			memset(rowLengths, 0, mNumRows * sizeof(uint32));

			uint32 newNumDepColumns = numDepColumns-c-1;
			uint32 **newDepColumns = new uint32*[newNumDepColumns];
			uint32 *newDepColLens = new uint32[newNumDepColumns];
			memset(newDepColLens, 0, newNumDepColumns * sizeof(uint32));
			uint32 newIdx = 0;
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
				}	// issa intersecca, so fsck the rest!
				newDepColumns[newIdx] = newDepCol;
				newDepColLens[newIdx] = newDepColLen;
				newIdx++;
			}
			// <- tha intersecca issa dun.

			uint16 *newItems = new uint16[newNumDepColumns];
			memcpy(newItems, items+c+1, newNumDepColumns * sizeof(uint16));
			MineTilesDFSv1(updatedMinArea, newItems, newNumDepColumns, newDepColumns, newDepColLens, prefix, prefixLen+1, depColumns[c], depColLens[c], rowLengths);
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

void TileMiner::MineTilesDFSv2(uint32 minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens) {
	if(numDepColumns == 0)
		return;

	uint32 UB = minArea;
	uint32 updatedMinArea = minArea;

	// prune
	bool stuffPruned = true;
	uint32 *newPrefixRowLens = new uint32[mNumRows];
	memcpy(newPrefixRowLens, prefixRowLens, sizeof(uint32) * mNumRows);
	//	uint16 *speelruimte = new uint16[numDepColumns];
	//	memset(speelruimte, 0, numDepColumns * sizeof(uint16));
	uint32 *rowLenHG = new uint32[mABSize+1];
	while(stuffPruned == true) {
		// 'speelruimte' per cUB uitrekenen, dan kunnen we die bij stuffPruned re-run evt skippen
		stuffPruned = false;
		for(uint32 c=0; c<numDepColumns; c++) {
			// if(speelruimte[c] == 0) { ... }

			// Build Length-Histogram
			memset(rowLenHG, 0, mABSize * sizeof(uint32));
			uint32 minLen = mABSize-1;
			uint32 maxLen = 0;
			for(uint32 i=0; i<depColLens[c]; i++) {
				uint32 len = newPrefixRowLens[depColumns[c][i]];
				if(len < minLen) minLen = len;
				if(len > maxLen) maxLen = len;
				rowLenHG[len]++;
			}
			uint32 cUB = 0;
			uint32 sum = 0;
			for(uint32 l=minLen; l<=maxLen; l++) {
				sum += rowLenHG[l];
				if(((prefixLen + l) * sum) > cUB)
					cUB = (prefixLen + l) * sum;
			}
			if(cUB < minArea) {
				stuffPruned = true;
				// + Pas `prefixRowLens` aan
				for(uint32 i=0; i<depColLens[c]; i++) {
					newPrefixRowLens[depColumns[c][i]]--;
				}
				// + zap uit items (nu niet meer geordend)	<- !!! keep sorted, dan kunnen we twee vliegen in een klap slaan
				items[c] = items[numDepColumns-1];
				// + zap uit depColLens
				depColLens[c] = depColLens[numDepColumns-1];
				depColLens[numDepColumns-1] = 0;
				// + zap uit depColumns
				delete[] depColumns[c];
				depColumns[c] = depColumns[numDepColumns-1];
				depColumns[numDepColumns-1] = NULL;

				numDepColumns--;
				// + Zorg dat restart van loop goed gaat
				c--;
			}
		}
	}
	delete[] rowLenHG;

	// for each item i, see what happens when we extend `prefix` with i
	for(uint32 c=0; c<numDepColumns; c++) {
		uint32 UB = minArea;
		uint32 cndArea = (prefixLen + 1) * depColLens[c];
		prefix[prefixLen] = items[c];

		if(cndArea > minArea && prefixLen+1 >= mMinTileLength && prefixLen+1 <= mMaxTileLength) {
			if (mTopK) {
				/*
					In order to find the top-k largest tiles, we adapt the LTM algorithm as follows.
					Initially the minimum area threshold is zero. Then, after the algorithm has
					generated the first k large tiles, it increases the minimum area threshold to the
					size of the smallest of these k tiles. From here on, the minimum area threshold
					can be increased every time a large tile is generated w.r.t. the current threshold.
					All generated tiles that do not have an area larger than the increased minimum
					area threshold can of course be removed.

					See: Tiling Databases by Floris Geerts, Bart Goethals, and Taneli Mielikäinen in Discovery Science; Vol. 3245, pages 278–289, Springer, 2004.
				*/
				mTopKTiles.push(new Tile(prefix, prefixLen + 1, depColLens[c], cndArea));
				stack<Tile*> bottomTiles;
				while (mTopKTiles.size() > mTopK) {
					bottomTiles.push(mTopKTiles.top());
					mTopKTiles.pop();
				}
				while (!bottomTiles.empty()) {
					if (bottomTiles.top()->mArea == mTopKTiles.top()->mArea) {
						mTopKTiles.push(bottomTiles.top());
						bottomTiles.pop();
					} else {
						delete bottomTiles.top();
						bottomTiles.pop();
					}
					updatedMinArea = mTopKTiles.top()->mArea;
				}
			} else {
				// deez is al goed genoeg
				OutputTile(prefix, prefixLen + 1, depColLens[c], cndArea);
			}
		} 

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
				bool reorder = true;
				bool superduper = true;
				// Order tha shizzle, if not ordered
				if(reorder == true) {
					// -> ifso, copy and reorder items as well
					memcpy(mAuxIdxs, mOrigIdxs, sizeof(uint32)*newNumDepColumns);
					ArrayUtils::MSortDesc(newDepColLens, newNumDepColumns, mAuxIdxs);
					uint32 **newestDepColumns = new uint32*[newNumDepColumns];
					uint16 *newestItems = new uint16[newNumDepColumns];
					for(uint32 i=0; i<newNumDepColumns; i++) {
						//printf("%d ", newDepColLens[i]);
						newestItems[i] = newItems[mAuxIdxs[i]];
						newestDepColumns[i] = newDepColumns[mAuxIdxs[i]];
					}
					//printf("\n");
					delete[] newDepColumns;
					delete[] newItems;
					newItems = newestItems;
					newDepColumns = newestDepColumns;
				}

				// Calc UB using ordered depColLens, assumes full dep
				if(superduper == true) {
					// assumes newItems.newDepColumns.newDepColLens is sorted >
					uint32 cndUB = 0;
					for(uint32 u=0; u<newNumDepColumns; u++) {
						if((prefixLen + 2 + u) * newDepColLens[u] > cndUB) {
							cndUB = (prefixLen + 2 + u) * newDepColLens[u];
						} // heeft geen zin om op (prefixLen+1+newNumDepColumns) * newDepColLens[u] te break'en
					}
					if(cndUB >= minArea)
						MineTilesDFSv2(updatedMinArea, newItems, newNumDepColumns, newDepColumns, newDepColLens, prefix, prefixLen+1, depColumns[c], depColLens[c], rowLengths);
					else
						mPruneCount++;
				} else
					MineTilesDFSv2(updatedMinArea, newItems, newNumDepColumns, newDepColumns, newDepColLens, prefix, prefixLen+1, depColumns[c], depColLens[c], rowLengths);
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

void TileMiner::MinerIsErVolVanCBFunc(ItemSet **buf, uint32 numSets) {
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
void TileMiner::MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets) {
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
void TileMiner::SetIscProperties(ItemSetCollection *isc) {
	isc->SetDatabaseName(mDbName);
	isc->SetPatternType(GetPatternTypeStr());
	isc->SetMinSupport(1/*mMinSup*/);
	isc->SetMaxSetLength(mMaxSetLength);
	isc->SetHasSepNumRows(mHasBinSizes);
	uint8 ol = Bass::GetOutputLevel();
	Bass::SetOutputLevel(1);
	isc->SortLoaded(mIscOrder);
	Bass::SetOutputLevel(ol);
}
#endif
