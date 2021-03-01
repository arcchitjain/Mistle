#ifndef __EXCEPTIONALTASKHANDLER_H
#define __EXCEPTIONALTASKHANDLER_H

#include "TaskHandler.h"

class ExceptionalTH : public TaskHandler {
public:
	ExceptionalTH (Config *conf);
	virtual ~ExceptionalTH ();

	virtual void HandleTask();
	virtual string BuildWorkingDir();

protected:


};
#endif // __EXCEPTIONALTASKHANDLER_H
