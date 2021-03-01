#ifndef __DISSIMILARITYTASKHANDLER_H
#define __DISSIMILARITYTASKHANDLER_H

#include "TaskHandler.h"

class DissimilarityTH : public TaskHandler {
public:
	DissimilarityTH (Config *conf);
	virtual ~DissimilarityTH ();

	virtual void HandleTask();

	static double	BerekenAfstandTussen(Database *db1, Database *db2, CodeTable *ct1, CodeTable *ct2);

protected:
	void ClassDistances();
	void InnerDBDistances();
	void InnerDBSampleToFullDBDistances();
	void ComputeDSForGivenDBsAndCTs();
};

#endif // __DISSIMILARITYTASKHANDLER_H
