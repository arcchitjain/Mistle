#ifndef __LESETSTASKHANDLER_H
#define __LESETSTASKHANDLER_H

#include "TaskHandler.h"

class LESetsTH : public TaskHandler {
public:
	LESetsTH (Config *conf);
	virtual ~LESetsTH ();

	virtual void HandleTask();

protected:
	void	Less();
	void	MassLess();

	void	ConvertLESC();

	void	MassAddLESCHeader();
	void	MassConvertLESC();
};

#endif // __LESETSTASKHANDLER_H
