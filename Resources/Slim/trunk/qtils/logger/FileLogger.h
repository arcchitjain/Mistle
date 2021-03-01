#ifndef _FILELOGGER_H
#define _FILELOGGER_H

#include "Logger.h"

class QTILS_API FileLogger : public Logger
{
public:
	FileLogger(const string &dir, const string &filename, uint32 maxLevel);
	virtual ~FileLogger();

	// - Sets the logging dir (iff file not open yet)
	virtual void	SetDirectory(const string &dir);

	// - Opens the logfile, if mFileOpen == false
	virtual void	OpenFile();

	// - Log given message
	virtual void	LogMessage(const string &msg);
	// - Log given message only if curLevel <= maxLevel
	virtual void	LogMessage(const string &msg, uint32 curLevel);

protected:
	string		mDir;
	string		mFilename;
	bool		mFileOpen;
	FILE		*mFile;
};

#endif // _FILELOGGER_H
