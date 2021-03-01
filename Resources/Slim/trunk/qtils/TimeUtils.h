#ifndef _TIMEUTILS_H
#define _TIMEUTILS_H

#include "QtilsApi.h"

class QTILS_API Timer
{
public:
	Timer(); // creation auto-starts timer
	~Timer();

	time_t		GetStartTime() { return mStartTime; }

	// returns the elapsed time in seconds
	time_t		GetElapsedTime();

protected:
	time_t		mStartTime;
};

class QTILS_API TimeUtils 
{
public: 
	/* ------------ Some static methods -------------- */
	static string		GetTimeStampString(time_t t = 0);
	static string		GetDateTimeString(time_t t = 0);
};

#endif // _TIMEUTILS_H
