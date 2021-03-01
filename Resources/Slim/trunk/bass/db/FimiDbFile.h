#ifndef __FIMIDBFILE_H
#define __FIMIDBFILE_H

// accepted formats:
// 1)	item_0 item_1 .. item_n
// 2)	[tid] item_0 item_1 .. item_n
// (tid = transaction id, uint64, does not need to be unique)

// >= max line length
#define		FIMIDBFILE_BUFFER_LENGTH		200000

#include "DbFile.h"

class BASS_API FimiDbFile : public DbFile {
public:
	FimiDbFile();
	virtual ~FimiDbFile();

	virtual Database *Read(string filename);
	virtual bool Write(Database *db, const string &fullpath);
	virtual DbFileType GetType() { return DbFileTypeFimi; }

protected:
	uint16 CountCols(char *buffer);

};

#endif // __FIMIDBFILE_H