#ifndef __ONTWARKCLUSTERER_H
#define __ONTWARKCLUSTERER_H

#include "Ontwar2Clusterer.h"

struct RemovalInfoK {
	double	result;
	ItemSet	*ctElem;
	uint32	ctIdx;
};

class OntwarKClusterer : public Clusterer {
public:
	OntwarKClusterer(Config *config, const string &ctDir = "");
	virtual ~OntwarKClusterer();

	virtual void Cluster();
	virtual uint32 ProvideDataGenInfo(CodeTable ***codeTables, uint32 **numTransactions);

	void SplitDB(const string &ontwarDir);
	void Classify(const string &ontwarDir);
	void Dissimilarity(const string &ontwarDir);
	void ComponentsAnalyser(const string &ontwarDir);

protected:
	RemovalInfoK FindMinSizeRemoval(Database *db, CodeTable **CTs, uint32 **clusters);
	void		ApplyRemoval(RemovalInfoK &info, Database *db, CodeTable **CTs, uint32 **clusters);
	bool		ReassignRows(Database *db, CodeTable **CTs, uint32 **clusters, uint32 *numRows);
	bool		ReassignRowsAndStore(Database *db, CodeTable **CTs, uint32 **clusters, uint32 *numRows);

	string		mCtDir;
	CTOntwarType mType;
	uint32		mK;
	uint32		*mPreviousRowToCluster;
};

#endif // __ONTWARKCLUSTERER_H
