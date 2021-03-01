#ifndef __CLASSIFYTASKHANDLER_H
#define __CLASSIFYTASKHANDLER_H

#include "TaskHandler.h"

class ClassifyTH : public TaskHandler {
public:
	ClassifyTH (Config *conf);
	virtual ~ClassifyTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	void ClassifyCompress();	// phase 1
	void ClassifyAnalyse();		// phase 2

	void ComputeCodeLengths();
	void ComputeCoverTree();

	string mTimestamp;
	string mTag;
};

#endif // __CLASSIFYTASKHANDLER_H
