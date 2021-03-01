#ifndef __TAGGROUPTASKHANDLER_H
#define __TAGGROUPTASKHANDLER_H

#include "TaskHandler.h"

enum TGorder {
	RandomTGorder,
	AscLenTGorder,
	DescLenTGorder,
	CTTGorder,
};

class TagGroupTH : public TaskHandler {
public:
	TagGroupTH (Config *conf);
	virtual ~TagGroupTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	void	FindTagGroups();

	void	EmulateFlickrClusters();
	void	ComputeGainForGivenFullCTandGroupDB(); // and write to gain.txt -- for use within another app

	// --- Helpers for FindTagGroups() --
	
	void	SaveGroup(uint32 groupNum, double restCompressibility, double compressionGain);

	uint32	FindBestCtElem(float &score);
	uint32	*GetCandidates(uint32 &numCandidates);
	void	RejectCtElem(uint32 elemIdx);

	Database *GetRestDB();
	Database *GetDeltaDB(uint32 elemIdx);
	void	AddToGroup(const uint32 ctElemIdx);
	void	EmptyGroup();

	// -- Member variables --

	string mTGPath;

	FILE *mFpCsv;
	FILE *mFpGroups;
	FILE *mFpItemSets;

	Database *mGroupDB;
	CodeTable *mGroupCT;
	Config *mGroupConfig;

	uint32 mGroupNumElems, mGroupNumRows, mGroupNumTrans, mSkipNumElems;
	uint32 *mGroupElems, *mGroupRows, *mSkipElems;

	ItemSet *mGroupElemsUnion;
	bool *mElemInCandGroup;
	bool mExclusiveGrowing;

	uint32 mGroupNumValues;
	uint16 *mGroupValues;
	uint32 *mGroupValCounts;

	uint16 *mNumOverlaps;
	uint16 **mOverlaps;

	uint32 mNumElems;
	ItemSet **mCtElems;

	uint32 **mCover;
	uint32 *mCoverCounts;
	uint32 *mRowCounts;

	double mGain, mCompressibility;

	Database *mDB;
	Database *mFullDB;
	CodeTable *mCT;
	CodeTable *mFullCT;
};
#endif // __TAGGROUPTASKHANDLER_H
