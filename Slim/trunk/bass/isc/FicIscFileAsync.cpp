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
