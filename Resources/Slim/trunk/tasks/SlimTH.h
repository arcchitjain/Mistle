#ifndef __SLIMTASKHANDLER_H
#define __SLIMTASKHANDLER_H

#include "TaskHandler.h"

class Config;

class SlimTH : public TaskHandler {
public:
	SlimTH (Config *conf);
	virtual ~SlimTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

	void Compress(Config *conf, const string tag = "");
	void DoCompress(Config *conf, Database *db, ItemSetCollection *isc, const string &tag, const string &timetag, const uint32 resumeSup, const uint64 resumeCand);

protected:



};

#endif // __SLIMTASKHANDLER_H
