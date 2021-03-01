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
#include "../Bass.h"
#include "../itemstructs/ItemSet.h"
#include "../db/Database.h"
#include "ItemSetCollection.h"

#include "FicIscFileAsync.h"

#if defined (_WINDOWS)

FicIscFileAsync::FicIscFileAsync() : FicIscFile() {
	mHFile = INVALID_HANDLE_VALUE;
}

FicIscFileAsync::~FicIscFileAsync() {
	CloseHandle(mHEvent);
	CloseHandle(mHFile);
}

void FicIscFileAsync::OpenFile(const string &filename, const FileOpenType openType) {
	if (mHFile != INVALID_HANDLE_VALUE) {
		THROW("Already opened a file...");
	}

	if (openType != FileReadable && openType != FileBinaryReadable) {
		THROW("Writing files asynchronously not supported.");
	}

	printf("Opening %s as async\n", filename.c_str());

	mHFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (mHFile == INVALID_HANDLE_VALUE) {
		THROW("Could not open file");
	}

	mHEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (mHEvent == NULL) {
		THROW("Could not create event");
	}

	memset(&mOverlapped, 0, sizeof(mOverlapped));
	mOverlapped.hEvent = mHEvent;
    
    mCurrentBuf = 1;
    mCount = 0;
    eof = false;
	isOverlapped = false;

	if (!ReadFile(mHFile, mFileBuf[0], BUF_SIZE, (LPDWORD) &mBytesRead, &mOverlapped)) {
		DWORD error = GetLastError();
		if (error == ERROR_HANDLE_EOF) {
			eof = true;
		} else if (error == ERROR_IO_PENDING) {
			isOverlapped = true;
		} else {
			THROW("Error while reading file");
		}
	}
}

size_t FicIscFileAsync::ReadLine(char* buffer, int size) {
    size_t len = 0;
    while (len < (size_t) (size - 1) && (buffer[len++] = GetChar()) != '\n') {
        ;
    }
    buffer[len] = 0;
    return len;
}

char FicIscFileAsync::GetChar() {
	if (mCount == 0) {
		if (eof) {
			THROW("End of file");
		}
		if (isOverlapped && !GetOverlappedResult(mHFile, &mOverlapped, (LPDWORD) &mBytesRead, TRUE)) {
            THROW("Error while reading file");
		}
		mCount = mBytesRead;

		LARGE_INTEGER offset;
        offset.LowPart = mOverlapped.Offset;
        offset.HighPart = mOverlapped.OffsetHigh;
		offset.QuadPart += mBytesRead;
		mOverlapped.Offset = offset.LowPart;
		mOverlapped.OffsetHigh = offset.HighPart;
		if (ReadFile(mHFile, mFileBuf[mCurrentBuf], BUF_SIZE, (LPDWORD) &mBytesRead, &mOverlapped)) {
			isOverlapped = false;
		} else {
			DWORD error = GetLastError();
			if (error == ERROR_HANDLE_EOF) {
				eof = true;
			} else if (error == ERROR_IO_PENDING) {
				isOverlapped = true;
			} else {
				THROW("Error while reading file");
			}
		}

		mCurrentBuf ^= 1;
		mPointer = mFileBuf[mCurrentBuf];
    }
    mCount--;
    return *mPointer++;
}

#endif // defined (_WINDOWS)
