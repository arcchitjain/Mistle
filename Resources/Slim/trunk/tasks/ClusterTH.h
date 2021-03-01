#ifndef __CLUSTERTASKHANDLER_H
#define __CLUSTERTASKHANDLER_H

#include "TaskHandler.h"

class ClusterTH : public TaskHandler {
public:
	ClusterTH (Config *conf);
	virtual ~ClusterTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	// *** Identifying components in a regular transaction database *** //
	// - Data Driven
	void	OntwarDB();
	void	OntwarDBDissimilarity();
	
	void	WhichRowsWhatCluster();
	
	// - Model Driven
	void	OntwarCT();
	void	OntwarCTSplitDb();
	void	OntwarCTGenerateDB();
	void	OntwarCTDissimilarity();
	void	OntwarCTClassify();
	void	OntwarCTAnalyse();
};

#endif // __CLUSTERTASKHANDLER_H
