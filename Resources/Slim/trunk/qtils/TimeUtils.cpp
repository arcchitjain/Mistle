
#include "qlobal.h"

#include <time.h>

#include "TimeUtils.h"

Timer::Timer() {
	mStartTime = time(NULL);
}
Timer::~Timer() {

}
time_t Timer::GetElapsedTime() {
	return time(NULL) - mStartTime;
}

string TimeUtils::GetTimeStampString(time_t t) {
	if(t == 0) time(&t);
	char ts[15];
	struct tm timeInfo;
	localtime_s(&timeInfo, &t);
	sprintf_s(ts, 15, "%04d%02d%02d%02d%02d%02d", 1900+timeInfo.tm_year, timeInfo.tm_mon+1, timeInfo.tm_mday, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
	return string(ts);
}
string TimeUtils::GetDateTimeString(time_t t) {
	if(t == 0) time(&t);
	struct tm timeInfo;
	localtime_s(&timeInfo, &t);
	char ts[20];
	sprintf_s(ts, 20, "%02d/%02d/%04d %02d:%02d:%02d", timeInfo.tm_mday, timeInfo.tm_mon+1, 1900+timeInfo.tm_year, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
	return string(ts);
}
