#ifndef __STREAMTASKHANDLER_H
#define __STREAMTASKHANDLER_H

#include "TaskHandler.h"

class StreamTH : public TaskHandler {
public:
	StreamTH(Config *conf);
	virtual ~StreamTH();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:

	void CheckCTs();
	void FindTheCTs();
	void FindCT();
	void EncodeStream();
	void GenerateStream();

	Database* LoadClassedDbAsStream(const string dbName, const ItemSetType dataType, const uint32 randomizeClasses, uint32 &numClasses, uint32 **classSizes);
};

#endif // __STREAMTASKHANDLER_H
