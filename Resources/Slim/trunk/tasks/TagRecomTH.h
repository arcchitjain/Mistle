#ifndef __TAGRECOMTASKHANDLER_H
#define __TAGRECOMTASKHANDLER_H

enum TagRecomCombineMode {
	TagRecomMin,
	TagRecomMax,
	TagRecomAvg,
	TagRecomSum
};

#include "TaskHandler.h"

class TagRecomTH : public TaskHandler {
public:
	TagRecomTH (Config *conf);
	virtual ~TagRecomTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	void	TagRecommendation();
	void	FastRecommendation();
	void	CondProbRecommendation();
	void	CondProbRecommendation2();

	void	CondProbRecommendation3();
	void	SetOracleSupport(ItemSet *is, Database *db);
	void	SetCTEstimatedSupport(ItemSet *is, ItemSet **ctElems, const uint32 numElems);
	void	CondProbRecommendation4();

	void	TestSupportEstimation();

	void	SlowRecommendation();
	void	LATRE();
	void	BorkursBaseline();
	void	ConvertToLatre();
	void	ConvertGroupsToLatre();

	// Helpers
	void	EstimateSupport(ItemSet *is, ItemSet **mixedElems, const uint32 numMixed);
	void	GenerateCandidates(islist *cands, islist *elems, islist::iterator startFrom, ItemSet *uncovered, ItemSet *currentSet, const bool allowOverlap);
	void	ConvertTrainDbToLatre(string dbName, string path);

	// -- Member variables --

};
#endif // __TAGRECOMTASKHANDLER_H
