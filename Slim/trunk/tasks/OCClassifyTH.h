#ifndef __OCCLASSIFYTASKHANDLER_H
#define __OCCLASSIFYTASKHANDLER_H

#include "TaskHandler.h"

class OCClassifyTH : public TaskHandler {
public:
	OCClassifyTH (Config *conf);
	virtual ~OCClassifyTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	void ClassifyCompress();	// phase 1
	void ClassifyAnalyse();		// phase 2

	string mTimestamp;
	string mTag;
};

#endif // __OCCLASSIFYTASKHANDLER_H
