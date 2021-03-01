#if defined (_WINDOWS)

#include "Thread.h"

ThreadException::ThreadException(DWORD lastError) {
	HLOCAL msgBuffer;
	::FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPSTR)&msgBuffer, 0,	NULL);
	mMessage = (LPSTR)msgBuffer;
	::LocalFree(msgBuffer);
}

Thread::Thread(IRunnable *ptr) {
	mRunnable = ptr;
	mStarted = false;
	mThreadHandle = 0;
}

Thread::~Thread() {
	if(mThreadHandle != 0)
		::CloseHandle(mThreadHandle);
}

void Thread::start(IRunnable *ptr) {
	if(mStarted)
		throw ThreadException("Thread already started.");

	if(!mStarted && mThreadHandle != 0)
		::CloseHandle(mThreadHandle);

	if(ptr != 0)
		mRunnable = ptr;

	if(mRunnable == 0)
		throw ThreadException("An object implementing the IRunnable interface required.");

	DWORD threadID=0;
	mThreadHandle = ::CreateThread(0, 0, ThreadProc, this, 0, &threadID);
	if(mThreadHandle == 0)
		throw ThreadException(::GetLastError());

	::Sleep(0);
}

void Thread::stop() {
	checkAlive();
	mRunnable->stop();
}

void Thread::suspend() {
	checkAlive();
	checkThreadHandle();
	if(::SuspendThread(mThreadHandle) == -1)
		throw ThreadException(::GetLastError());
}

void Thread::resume() {
	checkAlive();
	checkThreadHandle();
	if(::ResumeThread(mThreadHandle) == -1)
		throw ThreadException(::GetLastError());
}

void Thread::join(unsigned long timeOut) {
	checkThreadHandle();
	if(isAlive()) {
		DWORD waitResult = ::WaitForSingleObject(mThreadHandle, timeOut);
		if(waitResult == WAIT_FAILED)
			throw ThreadException(::GetLastError());
	}
}
#endif // defined (_WINDOWS)
