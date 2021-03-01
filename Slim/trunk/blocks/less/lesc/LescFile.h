#ifndef __LESCFILE_H
#define __LESCFILE_H

enum LescFileOpenType {
	LescFileReadable,
	LescFileWritable
};

// >= max line length
#define	LESCFILE_BUFFER_LENGTH		500000

class Database;
class LowEntropySet;
class LowEntropySetCollection;

#if defined (_MSC_VER)
	enum LescOrderType;
#elif defined (__GNUC__)
	#include "LowEntropySetCollection.h"
#endif

class LescFile {
public:
	/* Initializes mBuffer and mDatabase and mSetArray with NULL */
	LescFile();

	/* Closes file link and deletes the read buffer */
	virtual ~LescFile();

	// Opens the file, reads the header, and preps the returned LowEntropySetCollection with this info. Requires `db` for StdSizes.
	virtual LowEntropySetCollection*	Open(const string &filename, Database * const db);
	virtual void	Close();

	// Opens the file, reads all data and preps the returned LowEntropySetCollection with this info. Requires `db` for StdSizes.
	virtual LowEntropySetCollection*	Read(const string &filename, Database * const db);

	virtual LowEntropySet*	ReadNextPattern();

	// Writes the loaded ItemSets in the provided ItemSetCollection to a new file
	virtual bool WriteLoadedLowEntropySetCollection(LowEntropySetCollection * const lesc, const string &fullpath);

	// Writes the loaded ItemSets in the provided ItemSetCollection to the currently opened file
	virtual void WriteLoadedLowEntropySets(LowEntropySetCollection * const lesc);
	/*


	// Iterative reading from IscFile
	//virtual uint32	ScanPastSupport(uint32 support)=0;
	//virtual void		ScanPastCandidate(uint32 candi)=0;

	// Iterative Writing to IscFile


	*/
	void				SetPreferredDataType(ItemSetType ist) { mPreferredItemSetDataType = ist; }
	ItemSetType			GetPreferredDataType() { return mPreferredItemSetDataType; }

	void				Delete();

protected:
	/* Opens a filehandle, remembers the filename */
	virtual void		OpenFile(const string &filename, const LescFileOpenType openType = LescFileReadable);
	/* Universal implementation of LescFile::Close() fpcloses the file */
	virtual void		CloseFile();

	void				ReadFileHeader();

	// Writes the file header, using the provided number of sets (aka you decide loaded or all Item Sets)
	virtual void		WriteHeader(const LowEntropySetCollection *lesc, const uint64 numSets);

	virtual void		WriteLowEntropySet(LowEntropySet *les);

	string				GetFileName() { return mFilename; }
	int64				GetFileSize();

	FILE				*mFile;
	string				mFilename;

	char				*mBuffer;
	uint16				*mSetArray;
	uint16				*mOnArray;
	uint32				*mInstCounts;

	ItemSetType			mPreferredItemSetDataType;

	Database	*mDatabase;
	//double		*mStdLengths;


	// Via ReadFileHeader ingelezen stuff die naar ISC gepass't moet worden:
	string				mDatabaseName;
	string				mPatternType;
	uint64				mNumPatterns;
	uint32				mMaxSetLength;
	float				mMaxEntropy;
	LescOrderType		mOrder;
	string				mTag;
	bool				mHasSepNumRows;	// read ItemSet has separate supports for db rows and transactions


	friend class LowEntropySetCollection;	// ConvertLESC heeft OpenFile, WriteHeader & WriteLESet nodig
};
#endif // __LESCFILE_H
