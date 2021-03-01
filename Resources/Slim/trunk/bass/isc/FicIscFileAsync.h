#ifndef __FICISCFILEASYNC_H
#define __FICISCFILEASYNC_H

#include "FicIscFile.h"

#define BUF_SIZE 4096


class BASS_API FicIscFileAsync : public FicIscFile {
public:
	FicIscFileAsync();
	virtual ~FicIscFileAsync();

protected:
	virtual void	OpenFile(const string &filename, const FileOpenType openType = FileReadable);

	virtual size_t	ReadLine(char* buffer, int size);

	char			GetChar();

private:
#if defined (_WINDOWS)
	HANDLE mHFile;
	OVERLAPPED mOverlapped;
	HANDLE mHEvent;
#endif
	char mFileBuf[2][BUF_SIZE];
	int mCurrentBuf;
	int mBytesRead;
	char* mPointer;
	int mCount;
	bool isOverlapped;
	bool eof;

};

#endif // __FICISCFILEASYNC_H
