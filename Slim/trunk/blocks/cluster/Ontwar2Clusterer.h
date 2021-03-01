#ifndef __ONTWAR2CLUSTERER_H
#define __ONTWAR2CLUSTERER_H

struct RemovalInfo {
	double	result;
	ItemSet	*ctElem;
	bool	fromCT2;
};
enum CTOntwarType {
	CTOntwarMaxMaxDS,
	CTOntwarMaxMinDS,
	CTOntwarMinSize
};

#include "Clusterer.h"

class Ontwar2Clusterer : public Clusterer {
public:
	Ontwar2Clusterer(Config *config, const string &ctDir = "");
	virtual ~Ontwar2Clusterer();

	virtual void Cluster();
	virtual uint32 ProvideDataGenInfo(CodeTable ***codeTables, uint32 **numRows) { return 0; }
	
protected:
	RemovalInfo FindMinSizeRemoval(Database *db, CodeTable *ct1, CodeTable *ct2, uint32 *c1, uint32 *c2);
	RemovalInfo FindMaxDSRemoval(Database *db, CodeTable *ct1, CodeTable *ct2, uint32 *c1, uint32 *c2);
	void		ApplyRemoval(RemovalInfo &info, Database *db, CodeTable *ct1, CodeTable *ct2, uint32 *c1, uint32 *c2);
	void		ReassignRows(Database *db, CodeTable *ct1, uint32 *c1, uint32 &numRows1, CodeTable *ct2, uint32 *c2, uint32 &numRows2);
	double		ComputeClusterDissimilarity(Database *db, CodeTable *ct1, CodeTable *ct2, uint32 *c1, uint32 *c2);

	string		mCtDir;
	CTOntwarType mType;
};

#endif // __ONTWAR2CLUSTERER_H
