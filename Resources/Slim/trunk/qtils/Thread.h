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
