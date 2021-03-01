#ifndef __OVERLAPPINGTILINGMINER_H
#define __OVERLAPPINGTILINGMINER_H

class CoverSet;
class CodeTable;

#include "TilingMiner.h"

class OverlappingTilingMiner : public TilingMiner {
public:
	OverlappingTilingMiner(Config *conf);
	virtual ~OverlappingTilingMiner();

	virtual void		MineTiling();

	virtual string	GenerateOutputFileName(const Database *db);

protected:
	void	MineLargestUncoveredTilesDFSv1(uint32 &minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens, uint32 prefixArea);
	void	__inline PrunePosItems(uint32 minArea, uint16* items, uint32& numDepColumns, uint32** depColumns, uint32* depColLens, uint32 prefixArea, uint32 prefixLen, uint32 *newPrefixRowLens);

	void	MineLargestUncoveredTilesDFSv2(uint32 &minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens, uint32 *prefixUncRowLens, uint32 prefixArea);
	void	__inline PrunePosItemsV2(uint32 minArea, uint16* items, uint32& numDepColumns, uint32** depColumns, uint32* depColLens, uint32 prefixArea, uint32 prefixLen, uint32 *newPrefixRowLens, uint32 *prefixUncRowLens);

	void	MineLargestUncoveredTilesDFSv3(uint32 &minArea, uint16* items, uint32 numDepColumns, uint32** depColumns, uint32 *depColLens, uint16* prefix, uint32 prefixLen, uint32* prefixRowIdxs, uint32 prefixNumRows, uint32 *prefixRowLens, uint32 *prefixUncRowLens, uint32 prefixArea);

	uint32	UpdateUncArea(uint16* tile, uint32 tileWidth, uint32* tidlist, uint32 numTids, uint32 *prevTidList, uint32 prevNumTids, uint32 prevArea);

	virtual string	GenerateTileLogFilename(const Database *db);
	virtual string	GenerateTidsLogFilename(const Database *db);

	virtual void LogTids();

	// stuffs
	CoverSet **mCDB;

	// parameters
	bool	mUpdateUncoveredAreaAbsolute;

	// MinLargestTiles stuff
	uint32	**mItemTidList;
	uint32	*mItemTidListLengths;

	// Unc Num Tids per Item
	uint32	*mUncItemNumTids;
	// Unc Row Length Arras
	uint32	*mUncRowLengths;
	// Unc Row Length Histogram
	uint32	*mRowUncLenPerLenHG;

	// v3
	uint32 **mAuxPrefixRowLens;
	uint32 **mAuxPrefixUncRowLens;
	uint32 **mAuxDepRowLengths;
	uint32 **mAuxDepUncRowLengths;
};

#endif // __OVERLAPPINGTILINGMINER_H
