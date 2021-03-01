#ifndef __DBONTWARRERAAR_H
#define __DBONTWARRERAAR_H

#include "Clusterer.h"

enum DBOntwarInit {
	DBONTWAR_INIT_RANDOM = 0,
	DBONTWAR_INIT_ENCSZ,
	DBONTWAR_INIT_ENCSZDIFMEAN,
};
enum DBOntwarStylee {
	DBONTWAR_STYLEE_KEEPSWAPPING = 0,
	DBONTWAR_STYLEE_MINIMIZESZ,
};

class DBOntwarreraar : public Clusterer {
public:
	DBOntwarreraar(Config *config);
	virtual ~DBOntwarreraar();

	virtual void Cluster();
	virtual uint32 ProvideDataGenInfo(CodeTable ***codeTables, uint32 **numRows) { return 0; }
	void Dissimilarity(const string &ontwarDir, bool minCTs = false);

protected:
	//Database		*mDB;

	uint64			mNumRowsCompressed;
	uint64			mNumCandidatesDone;
	uint32			mNumCompresses;
};

#endif // __DBONTWARRERAAR_H
