#ifndef __STREAMALGO_H
#define __STREAMALGO_H

class Database;
class ItemSetCollection;
class CodeTable;
class Histogram;

class StreamAlgo {
public:
	StreamAlgo(const uint32 blockSize, const uint32 maxIterations, const bool dbSizeOnly);
	virtual ~StreamAlgo();

	CodeTable*	FindGoodEnoughCT(Config *config, Database *db, const uint32 offset, const float acldThresh);

	Histogram*	BuildBlockSizeHistogram(CodeTable *ct, uint32 blockSize, uint32 histNumBins, uint32 blockHistNumSamples);
	bool		DistributionIsTheSame(Database *db, CodeTable *ct, Histogram *hist, uint32 offset, uint32 blockSize);
	bool		AcceptNewCT(CodeTable *ct, CodeTable *newCT, const float acldThreshold, float &acceptedAcldSum);

	// Apply Krimp to a series of databases of increasing size.
	void	SerialKrimp(Config *config, Database *db, const uint32 offset);

	void	SetStorageDir(const string &s) { mStorageDir = s; FileUtils::CreateDirIfNotExists(s); }

protected:
	// No additional computational and memory requirements, only save numCTs CTs to disk.
	void	SerialKrimpNoStatistics(Config *config, Database *db, const uint32 offset);
	// Apply Krimp to a series of databases, keep them in memory and compute additional numbers on-the-fly.
	void	SerialKrimpAndStatistics(Config *config, Database *db, const uint32 offset);

	uint32	mBlockSize;
	uint32	mMaxIterations;
	bool	mDbSizeOnly;
	string	mStorageDir;
};

#endif // __STREAMALGO_H
