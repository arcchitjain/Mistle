#ifndef __CLUSTERER1G_H
#define __CLUSTERER1G_H

#include "Clusterer.h"

class Clusterer1G : public Clusterer {
public:
	Clusterer1G(Config *config);
	~Clusterer1G();

	virtual void Cluster();
	virtual uint32 ProvideDataGenInfo(CodeTable ***codeTables, uint32 **numRows) { return 0; }

	// Determine max recurse level (0 for no max)
	void SetMaxLevel(uint32 ml) { mMaxLevel = ml; }
	uint32 GetMaxLevel() { return mMaxLevel; }

protected:
	void RecurseCluster(string subDir, uint32 ctSup, uint32 level);

	CodeTable* LoadCodeTable(const string &dir, Database *db, uint32 ctSup);
	Database* LoadDatabase(const string &dir);
	void WriteDatabase(const string &dir, Database *db);

	Algo *Compress(string &outDir, Database *db, ItemSetCollection *isc);

	uint32			mMaxLevel;
	bool			mSplitOnSingletons;
	bool			mLaplaceCorrect;
	bool			mUseDistance;

	uint64			mNumRowsCompressed;
	uint64			mNumCandidatesDone;
	uint32			mNumCompresses;
};

#endif // __CLUSTERER_H
