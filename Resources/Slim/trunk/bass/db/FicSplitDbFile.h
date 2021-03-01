#ifdef BROKEN

#ifndef __FICSPLITDBFILE_H
#define __FICSPLITDBFILE_H

// >= max line length
#define		FICSPLITDBFILE_BUFFER_LENGTH		200000

#include "DbFile.h"

class BASS_API FicSplitDbFile : public DbFile {
public:
	FicSplitDbFile();
	virtual ~FicSplitDbFile();

	virtual Database *Read(string filename);
	virtual bool Write(Database *db, string filename);

	virtual void FindDuplicateRows() {};

	virtual DbFileType GetType() { return FicSplitDbFileType; }

protected:
	uint16 CountCols(char *buffer);

};

#endif // __FICSPLITDBFILE_H

#endif // BROKEN
