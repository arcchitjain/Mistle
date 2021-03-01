#ifndef __CLUSTERER_H
#define __CLUSTERER_H

class Algo;
class Config;
class ItemSetCollection;
class Database;
class CodeTable;

class Clusterer {
public:
	Clusterer(Config *config);
	virtual ~Clusterer();

	virtual void Cluster() =0;

	// Return value is the number of CodeTables
	virtual uint32 ProvideDataGenInfo(CodeTable ***codeTables, uint32 **numRows) =0;

	static Clusterer* CreateClusterer(Config *config);

protected:
	Config			*mConfig;
	string			mBaseDir;
	string			mDbName;

	FILE*			mLogFile;
	FILE*			mSplitFile;
};

#endif // __CLUSTERER_H
