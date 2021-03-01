#ifndef __TILINGMINER_H
#define __TILINGMINER_H

#include <Croupier.h>

class Database;
class Config;
class ItemSet;

enum IscOrderType;

enum TileSelectionStylee {
	TILE_SELECTION_FIRST = 1,
	TILE_SELECTION_THINNEST,
	TILE_SELECTION_DENSEST,
	TILE_SELECTION_FATTEST,
};

typedef map<uint16,uint16> ItemMap;typedef pair<uint16,uint16> ItemPair;

namespace tlngmnr {
	class Output;
	class MemoryOut;
	class FSOut;
}

class TilingMiner : public Croupier {
public:
	TilingMiner(Config *conf);
	virtual ~TilingMiner();
	string			GetPatternTypeStr() { return "tiling"; }

	virtual void	SetLogTiles(bool b) { mLogTiles = b; }
	virtual void	SetLogTids(bool b) {mLogTids = b; }

	// --- Stats
	virtual uint32	GetNumTilesMined()	{ return mStatNumTilesMined; }
	virtual uint32	GetMaxSetLength()	{ return mStatMaxSetLength; }
	virtual uint32	GetMinSup()			{ return mStatMinSup; }
	virtual uint32	GetMinArea()		{ return mStatMinArea; }

	// --- Mine to Disk
	virtual bool	MinePatternsToFile(const string &dbFilename, const string &outputFile, Database *db = NULL);
	virtual bool	MinePatternsToFile(Database *db, const string &outputFile);
	virtual string	GenerateOutputFileName(const Database *db);
	
	// --- Mine to Memory
	virtual void	InitOnline(Database *db);
	virtual void	SetIscProperties(ItemSetCollection *isc);

	// Returns an ItemSetCollection (may be entirely loaded or opened from disk)
	virtual ItemSetCollection*	MinePatternsToMemory(Database *db = NULL);

	virtual void	MinerIsErVolVanCBFunc(ItemSet **iss, uint32);
	virtual void	MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets);

protected:
	virtual void	MineTiling();
	virtual void	OutputTile(uint16* items, uint32 numItems, uint32 supp, uint32 area);

	virtual void	OutputBufferedTiles(uint32 chosenIdx = UINT32_MAX_VALUE);
	virtual void	BufferLargestTiles(uint16* items, uint32 numItems, uint32 support, uint32 area);

	void			MineLargestTilesDFSv1(uint32 &minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens);

	virtual string	GenerateTileLogFilename(const Database *db);
	virtual string	GenerateTidsLogFilename(const Database *db);

	virtual uint32		SelectTile();

	TileSelectionStylee StringToTileSelectionStylee(string stylee);
	void				SwapTileBufferEntries(uint32 a, uint32 b);

	virtual void	ResetStats();

	virtual void	InitTilesLogFile();
	virtual void	InitTidsLogFile();

	virtual void	InitAuxiliaries();
	virtual void	InitData();
	virtual void	CleanUpAuxiliaries();
	virtual void	CleanUpData();

	Config	*mConfig;

	// parameters
	uint32	mArgMinTileLength;
	uint32	mMaxNumTiles;
	bool	mColumnPruningEnabled;
	bool	mRowPruningEnabled;
	uint32	mMinerVersion;
	bool	mLogTiles;
	bool	mLogTids;

	// mining stats
	uint32	mStatMaxSetLength;
	uint32	mStatMinArea;
	uint32	mStatMinSup;
	uint64	mStatCoveredArea;
	uint32	mStatNumTilesMined;
	uint32	mStatPruneCount;
	uint32	mStatBranchCount;

	// member variables
	string	mDBName;
	ItemSet **mRows;
	uint32	mNumRows;
	bool	mHasBinSizes;
	FILE*	mLogFileTiles;
	FILE*	mLogFileTids;

	uint32	mABSize;
	uint16	*mAlphabet;

	// current Items
	uint16	*mItems;
	uint32	mNumItems;
	uint32	**mItemTids;
	uint32	*mItemNumTids;
	ItemMap	*mItemTranslationMap;

	// mTileBuffer
	uint16	**mTileBuffer;
	uint32	*mTileBufferLen;
	uint32	*mTileBufferSup;
	uint32	mTileBufferMax;
	uint32	mTileBufferNum;
	uint32	mTileBufferArea;
	uint32	mChosenTileIdx;

	// row-length auxiliary histograms
	uint32	*mRowLenPerLenHG;

	// mTile mask
	uint32	*mRowLengths;
	uint32	*mFullTidList;
	ItemSet *mTile;
	ItemSet	*mTiledItems;

	// -- Offline Mining
	tlngmnr::Output *mOutput;

	// --- Online Mining
	tlngmnr::MemoryOut*	mMemOut;
	bool				mMayHaveMore;
	IscOrderType		mIscOrder;

	// mTileBuffer
	TileSelectionStylee mTileSelectionStylee;
	uint16 *mAuxArrA;
	uint16 *mAuxArrB;
};
#endif // __TILINGMINER_H
