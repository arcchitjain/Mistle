#ifndef _STDOUTLOGGER_H
#define _STDOUTLOGGER_H

#include "Logger.h"

class QTILS_API StdOutLogger : public Logger
{
public:
	StdOutLogger(uint32 maxLevel);
	virtual ~StdOutLogger();

	// - Log given message
	virtual void	LogMessage(const string &msg);
	// - Log given message only if curLevel <= maxLevel
	virtual void	LogMessage(const string &msg, uint32 curLevel);
	
	void			InitProgress(uint32 max, string message = "");
	void			ShowProgress(uint32 current);
	void			EndProgress();

protected:
	bool			mProgress;
	uint32			mProgressMax;
	string			mMessage;
};

#endif // _STDOUTLOGGER_H
