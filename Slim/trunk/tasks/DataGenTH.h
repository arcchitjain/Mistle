#ifndef __DGENTASKHANDLER_H
#define __DGENTASKHANDLER_H

#include "TaskHandler.h"

class DataGenTH : public TaskHandler {
public:
	DataGenTH(Config *conf);
	virtual ~DataGenTH();

	virtual void HandleTask();

protected:
	void	CheckGenDbForOrigRows();
	void	CheckGenDbForPrivacy();
	void	GenerateDBs();
	void	GenerateDBsFromFirstCT();
	void	GenerateDBsClassBased();
	void	CompareISCs();
};

#endif // __DGENTASKHANDLER_H
