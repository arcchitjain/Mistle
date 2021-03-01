#ifndef __FICDBFILE_H
#define __FICDBFILE_H

/// fic db header format
// 1.5 --> 	fprintf(fp, "mi: nR=%d nT=%d nI=%" I64d " aS=%d sS=%.5lf mL=%d b?=%d c?=%d\n", db->GetNumRows(), db->GetNumTransactions(), ni, a->size(), db->GetStdDbSize(), db->GetMaxSetLength(), hasBinSizes?1:0, isDbClassed?1:0);
// 1.6 --> 	fprintf(fp, "mi: nR=%d nT=%d nI=%" I64d " aS=%d sS=%.5lf mL=%d b?=%d c?=%d tid?=%d\n", db->GetNumRows(), db->GetNumTransactions(), ni, a->size(), db->GetStdDbSize(), db->GetMaxSetLength(), hasBinSizes?1:0, isDbClassed?1:0, hasTransactionIds?1:0);

// >= max line length
#define		FICDBFILE_BUFFER_LENGTH		500000

#include "DbFile.h"

class BASS_API FicDbFile : public DbFile {
public:
	FicDbFile();
	virtual ~FicDbFile();

	virtual Database *Read(string filename);
	virtual bool Write(Database *db, const string &fullpath);

	virtual DbFileType GetType() { return DbFileTypeFic; }

protected:
	uint32 CountCols(char *buffer);

};

#endif // __FICDBFILE_H
