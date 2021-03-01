#ifndef __TILETASKHANDLER_H
#define __TILETASKHANDLER_H

#include "TaskHandler.h"

class TileTH : public TaskHandler {
public:
	TileTH(Config *conf);
	virtual ~TileTH();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:
	void	MineTiles();
	void	MineTiling();
};
#endif // __TILETASKHANDLER_H
