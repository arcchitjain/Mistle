#ifndef __TILEMINER_H
#define __TILEMINER_H

#include <queue>

#include "../Croupier.h"

class Database;
class Config;
class ItemSet;

#include "../bass/isc/ItemSetCollection.h" //enum IscOrderType;

namespace tlmnr {
	class Output;
	class MemoryOut;
	class FSOut;
}

#include <utility> // Standard definitions for the relational operators !=, >, <= and >=.

// This namespace declares template functions for four relational operators (!=,>, <=, and >=), deriving their behavior from operator== (for !=) and from opertator< (for >,<=, and >=)
using namespace rel_ops;

struct Tile {
	Tile(uint16* items, uint32 numItems, uint32 supp, uint32 area);
	~Tile();

	  // Operator for equality comparison for object
	  bool operator== (const Tile& t) const;

	  // Operator for less-than inequality comparison for object
	  bool operator< (const Tile& t) const;


	uint16* mItems;
	uint32 mNumItems;
	uint32 mSupp;
	uint32 mArea;
};

class CROUPIER_API TileMiner : public Croupier {
public:
	TileMiner(Config *conf);
	TileMiner();
	virtual ~TileMiner();
	string			GetPatternTypeStr() { return "tiles"; }

	/* --- Get/Set Properties --- */
	void			SetMinerVersion(uint32 minerVersion) { mUseMinerVersion = minerVersion; }

	uint32			GetMinArea() { return mMinArea; }		
	void			SetMinArea(uint32 minArea) { mMinArea = minArea; }

	uint32			GetTopK() { return mTopK; }
	void			SetTopK(uint32 topK) { mTopK = topK; }

	uint32			GetMinTileLength() { return mMinTileLength; }
	void			SetMinTileLength(uint32 minTileLength) { mMinTileLength = minTileLength; }

	uint32			GetMaxTileLength() { return mMaxTileLength; }
	void			SetMaxTileLength(uint32 maxTileLength) { mMaxTileLength = maxTileLength; }

	// --- Mining Statistics
	uint64			GetNumTilesMined()		{ return mNumTilesMined; }
	uint16			GetMaxSetLength()		{ return mMaxSetLength; }
	uint64			GetNumPruned()			{ return mPruneCount; }
	void			ResetMiningStatistics();

	/* --- Offline Dealing --- */
	virtual bool	MinePatternsToFile(const string &dbFilename, const string &outputFile, Database *db = NULL);
	virtual bool	MinePatternsToFile(Database *db, const string &outputFile);
	virtual string	GenerateOutputFileName(const Database *db);

	/* --- Online Dealing (pass patterns in memory) --- */
	virtual bool	CanDealOnline()					{ return true; }
	virtual void	InitOnline(Database *db);
	virtual void	SetIscProperties(ItemSetCollection *isc);

	// Returns an ItemSetCollection (may be entirely loaded or opened from disk)
	virtual ItemSetCollection*	MinePatternsToMemory(Database *db = NULL);

	virtual void	MinerIsErVolVanCBFunc(ItemSet **iss, uint32);
	virtual void	MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets);


protected:
	void			MineTiles();
	void inline		OutputTile(uint16* items, uint32 numItems, uint32 supp, uint32 area);

	void			MineTilesDFSv1(uint32 minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens);
	void			MineTilesDFSv2(uint32 minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens);


	Config			*mConfig;
	uint32			mMinArea;
	uint32			mUseMinerVersion;
	bool			mHasBinSizes;
	uint32			mTopK;
	uint32			mMinTileLength;
	uint32			mMaxTileLength;


	priority_queue<Tile*, vector<Tile*>, bool(*)(const Tile*, const Tile*)> mTopKTiles;

	// --- Mining Stats
	uint64	mNumTilesMined;
	uint32	mMinSup;
	uint16	mMaxSetLength;
	uint32	mPruneCount;

	ItemSet **mRows;
	uint32	mNumRows;
	uint32	mABSize;
	uint16	*mAlphabet;
	uint32	**mColumns;
	uint32	*mColumnLengths;

	uint32	*mOrigIdxs;
	uint32	*mAuxIdxs;
	
	// -- Offline Mining

	tlmnr::Output *mOutput;

	// --- Online Mining
	tlmnr::MemoryOut*	mMemOut;
	bool				mMayHaveMore;
	IscOrderType		mIscOrder;


};
#endif // __TILEMINER_H
