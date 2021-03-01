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
