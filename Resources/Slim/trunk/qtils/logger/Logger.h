#ifndef _LOGGER_H
#define _LOGGER_H

#include "../QtilsApi.h"

// ------------ The actual class ------------

class StdOutLogger;
class FileLogger;

class QTILS_API Logger
{
public:
	Logger(uint32 maxLevel);
	virtual ~Logger();

	// - Log given message
	virtual void	LogMessage(const string &msg) =0;
	// - Log given message only if curLevel <= maxLevel
	virtual void	LogMessage(const string &msg, uint32 curLevel) =0;

	// Get & Set
	uint32			GetMaxLevel()					{ return mMaxLevel; }
	void			SetMaxLevel(uint32 ml)			{ mMaxLevel = ml; }
	uint32			IsEnabled()						{ return mEnabled; }
	void			SetEnabled(bool e)				{ mEnabled = e; }

	// -------------------- Static access -----------------

	// - Start/End a new command (log in AppLog)
	static void		CommandInit(const string &taskClass, const string &command);
	static void		CommandEnd();

	// - Regular logging method(s)
	// Log to all, but based on current level
	static void		LogMessageToAll(string message);
	// Force log to specified logs (independent of level)
	static void		LogMessageTo(string message, const uint32 logs);
	// Errors and warnings: Always logged to all
	static void		LogError(string errorMessage, bool warningOnly = false);
	// Debug messages: Only logged when compiled in debug mode (to all, based on current level)
	static void		LogDebug(string debugMessage);

	// - Levels
	static void		EnterLevel()					{ ++sCurrentLevel; }
	static void		LeaveLevel()					{ if(sCurrentLevel>0) --sCurrentLevel; }
	static uint32	GetCurrentLevel()				{ return sCurrentLevel; }

	// - Set/Get
	static void		SetAllEnabled(bool e);
	static void		SetAllMaxLevels(uint32 maxLevels);
	static void		SetAppLogDir(const string &dir) { sAppLogDir = dir; }
	static void		SetCommandLogDir(const string &dir);
	static void		SetLogToDisk(const bool ltd) { sLogToDisk = ltd; }

	static uint64	GetCommandStartTime()			{ return (uint64)sCommandStart; }

	// - Modify current set of loggers
	static void		Init(bool logToDisk, uint32 stdOutMaxLevel = 1, uint32 summaryMaxLevel = 2, uint32 extendedMaxLevel = 100);
	static void		CleanUp();

	static void		AddLogger(Logger *logger);
	static void		RemoveLogger(Logger *logger);
	static uint32	GetNumExtraLoggers() { return sNumExtraLoggers; }

	// - Default loggers available to directly set MaxLevel
	static StdOutLogger		*sStdOut;
	static FileLogger		*sSummary;
	static FileLogger		*sExtended;

protected:
	bool			mEnabled;
	uint32			mMaxLevel;

	// ------- Static ---------
	static void		WriteToAppLog(string line);

	static uint32	sCurrentLevel;
	static bool		sLogToDisk;

	static Logger	**sExtraLoggers;
	static uint32	sNumExtraLoggers;

	static string	sCommandTask;
	static string	sCommand;
	static time_t	sCommandStart;

	static string	sAppLogDir;
	static string	sCmdLogDir;
};

#endif // _LOGGER_H
