#ifndef __BINARYFICISCFILEASYNC_H
#define __BINARYFICISCFILEASYNC_H

#include "BinaryFicIscFile.h"

#define BUF_SIZE 4096


class BASS_API BinaryFicIscFileAsync : public BinaryFicIscFile {
public:
	BinaryFicIscFileAsync();
	virtual ~BinaryFicIscFileAsync();

protected:
	virtual void	OpenFile(const string &filename, const FileOpenType openType = FileReadable);

	virtual size_t	ReadLine(char* buffer, int size);

	virtual void	ReadBuf(char* buffer, uint32 size);

	char			GetChar();

private:
	void			RefillBuffer();

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

#endif // __BINARYFICISCFILEASYNC_H
