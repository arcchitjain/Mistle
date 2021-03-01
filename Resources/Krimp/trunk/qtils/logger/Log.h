//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#ifndef _LOG_H
#define _LOG_H

#include "../QtilsApi.h"

#include "Logger.h"
#include "StdOutLogger.h"
#include "FileLogger.h"

/* ------------------- HowTo log using this Logger -----------------

// -- In each .cpp file in which you want to use the Logger
#include <logger/Log.h>

// -- In some main() method
Logger::CommandInit(task, command); // logs to applog

Logger::SetAppLogDir(Bass::GetExecDir());
Logger::SetExpLogDir(Bass::GetWorkingDir());
Logger::Init();

[.. Do everything that belongs to (task,command) ..]

Logger::CommandEnd(); // roundup applog
Logger::CleanUp();

// -- Available logs

// Commands, timings and warnings/errors are logged to _fic.log in AppLog

// Default loggers
StdOut		LOGS_STDOUT		Out to screen, as you would expect..
Summary		LOGS_SUMMARY	File _summary.log in ExpLogDir.
Extended	LOGS_EXTENDED	File _extended.log in ExpLogDir.

// Extras
You may add any additional logs (Logger::AddLogger(Logger *l)), specified altogether by LOGS_EXTRAS.

// -- Logging messages

// The basics
LOG_MSG("Log a message to all Loggers"); // log only if current level <= max level for a specific Logger
LOG_MSG_TO("Log a message to specified Logger", LOGS_STDOUT);
LOG_MSG_TO("Log a message to specified Loggers", LOGS_STDOUT | LOGS_SUMMARY);

// Using (verbosity) levels
// A global 'current level' is maintained and can be increased (ENTER_LEVEL) or decreased (LEAVE_LEVEL).
// Initial level: 0. Default max levels: StdOut = 1, Summary = 2, Extended = 100
ENTER_LEVEL();		// level 0 --> 1
LOG_MSG("This message ends up in StdOut,Summary,Extended.");
ENTER_LEVEL();		// level 1 --> 2
LOG_MSG("This message is only visible in Summary and Extended.");
LOG_MSG_TO("Force this message to StdOut (current level doesn't matter with this macro)", LOGS_STDOUT);
LEAVE_LEVEL();		// level 2 --> 1
LOG_MSG("And again, this message ends up in StdOut,Summary,Extended.");

// -- Debug

// Same as LOG_MSG, but only logged if compiled in debug mode.
LOG_DEBUG("Log a message to all Loggers, using current level.");

// -- Errors and warnings

// Always logged to all logs (stdout, summary, extended, applog, any extra)
LOG_WARNING("MyClass::MyMethod() -- This ain't very nice.");
LOG_ERROR("MyClass::MyMethod() -- Something is going completely wrong here.");

// -- Progress bar (displayed on StdOut only)

uint32 max = 1000;
INIT_PROGRESS(max, "Doing something!"); // message displayed on right-side of progress bar
for(uint32 i=1; i<=max; i++) {
	PROGRESS(i); // update progress bar
	if(i==max/2)
		LOG_MSG("Doing something else!");	// new message displayed on right-side of progress bar when updated,
											// but also posted to other loggers (Summary, Extended, ...)
}
END_PROGRESS();

// -- Enable/Disable all logging

ENABLE_LOGS();
DISABLE_LOGS();

----------------------------------------------------------------------- */


// --------------- Some definitions ----------

#define LOGS_ALL							LOGS_STDOUT | LOGS_SUMMARY | LOGS_EXTENDED | LOGS_EXTRA
#define LOGS_STDOUT							1
#define LOGS_SUMMARY						2
#define LOGS_EXTENDED						4
#define LOGS_EXTRA							8

#define ENABLE_LOGS()						Logger::SetAllEnabled(true)
#define DISABLE_LOGS()						Logger::SetAllEnabled(false)
#define ENTER_LEVEL()						Logger::EnterLevel()
#define LEAVE_LEVEL()						Logger::LeaveLevel()

#define LOG_MSG(message)					Logger::LogMessageToAll(message)
#define LOG_MSG_TO(message, logs)			Logger::LogMessageTo(message, logs)
#define LOG_WARNING(warning_msg)			Logger::LogError(warning_msg, true)
#define LOG_ERROR(error_msg)				Logger::LogError(error_msg)
#ifdef _DEBUG
#define LOG_DEBUG(debug_msg)				Logger::LogDebug(debug_msg)
#else
#define LOG_DEBUG(debug_msg)
#endif // _DEBUG

#define INIT_PROGRESS(max,msg)				Logger::StdOut->InitProgress(max, msg)
#define PROGRESS(cur)						Logger::StdOut->ShowProgress(cur)
#define END_PROGRESS()						Logger::StdOut->EndProgress()

#endif // _LOG_H
