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
#ifndef __THREAD_H__
#define __THREAD_H__

/////////////////////////////////////////////////////////////////////////////////
#if defined (_WINDOWS)
#include <Windows.h>
#include <string>

/*
	Here is an example of how to use this thread class

	// define a threadable object
	class ThreadableObject : IRunnable {
	public:
		ThreadableObject() {
			_continue = true;	
		}
		virtual uint32 run() {
			while(_continue) {
				// run this thread procedure
			}
			return 0;
		}
		virtual void stop() {
			_continue = false;
		}
	protected:
		bool _continue;		
	};

	// example of usage

	ThreadableObject *obj=0;
	Thread *thread=0;
	try {
		// create the threadable object first
		obj = new ThreadableObject();

		// create and start the thread the thread
		thread = new Thread(obj);
		thread->start();

		// see if the thread exits in 10 seconds
		thread->join(10000);

		if(thread->isAlive()) {
		    // stop it and join until the thread exits
			thread->stop();
			threas->join();
		}
	} 
	catch (ThreadException &e)
	{
		printf(e.Message.c_str());	
	}

	delete obj;
	delete thread;


*/
#include "QtilsApi.h"

struct QTILS_API IRunnable {
	virtual uint32 run() = 0;
	virtual void stop() = 0;
};

class QTILS_API ThreadException {
public:
	ThreadException(DWORD lastError);
	ThreadException(const std::string &msg) { mMessage = msg; }
	std::string mMessage;
};

class QTILS_API Thread {
public:
    Thread(IRunnable *ptr=NULL);
	~Thread();

	void start(IRunnable *ptr=NULL);
	void stop();
    void suspend();
    void resume();
	void join(unsigned long timeOut=INFINITE);

    bool isAlive() { return mStarted; }

protected:
	bool mStarted;
	HANDLE mThreadHandle;
	IRunnable *mRunnable;

	uint32 run() {
		mStarted = true;
        uint32 threadExitCode = mRunnable->run();
		mStarted = false;
		return threadExitCode;
	}
	void checkThreadHandle() {
		if(mThreadHandle == 0)
			throw ThreadException("Thread not yet created, call the start() method.");
	}
	void checkAlive() {
		if(!isAlive())
			throw ThreadException("No Thread alive.");
	}
	static unsigned long __stdcall ThreadProc(void* ptr) {
		return ((Thread *)ptr)->run();
	}
};

#endif // defined (_WINDOWS)
///////////////////////////////////////////////////////////////////////////////
#endif // __THREAD_H__
