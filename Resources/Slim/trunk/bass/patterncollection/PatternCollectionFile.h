#ifdef __PCDEV
#ifndef __PATTERNCOLLECTIONFILE_H
#define __PATTERNCOLLECTIONFILE_H

#include <FileUtils.h>

// >= max line length
#define	PATTERNCOLLECTIONFILE_BUFFER_LENGTH		500000

class Database;
class Pattern;
class PatternCollection;

class PatternCollectionFile {
public:
	/* Initializes mBuffer and mDatabase and mSetArray with NULL */
	PatternCollectionFile();

	/* Closes file link and deletes the read buffer */
	virtual ~PatternCollectionFile();

	// Opens the file, reads the header, and preps the returned PatternCollection with this info.
	virtual PatternCollection*	Open(const string &filename);
	virtual void		Close();

	virtual void		Delete();

	// Opens the file, reads header, preps a PatternCollection, and reads as much data as it may.
	// - maxNumReadPatterns defaults to 'as much as fits in memory', if all : pcf->SetFullyLoaded(true)
	virtual PatternCollectionFile*	Read(const string &filename, uint64 maxNumReadPatterns = UINT64_MAX_VALUE);

	// Writes the loaded Patterns in the provided PatternCollection to a new file
	virtual bool		WriteLoadedPatternCollection(PatternCollection * const pc, const string &fullpath);

	// Writes the loaded Patterns in the provided PatternCollection to the currently opened file
	virtual void		WriteLoadedPatterns(PatternCollection * const pc);


protected:
	/* Opens a filehandle, remembers the filename */
	virtual void		OpenFile(const string &filename, const FileOpenType openType = FileReadable);
	/* Universal implementation of PatternCollectionFile::Close() fpcloses the file */
	virtual void		CloseFile();

	virtual void		ReadFileHeader();

	virtual Pattern*	ReadPattern();
	virtual Pattern*	ReadPattern(uint64 id);

	// Writes the file header, using the provided number of patterns (aka you decide loaded or all Patterns)
	virtual void		WriteHeader(const PatternCollection *pc, const uint64 numPatterns);

	virtual void		WritePattern(Pattern *p);

	virtual string		GetFileName() { return mFileName; }
	virtual __int64		GetFileSize();

	FILE				*mFile;
	string				mFileName;
	FileOpenType		mFileMode;
	char				*mBuffer;

	uint64				mCurPatternId;
	uint64				mPatternsStartingPoint;

	// member variables for passing
	uint8				mFileVersionMajor;
	uint8				mFileVersionMinor;
	uint64				mNumPatterns;
	string				mDatabaseNameStr;
	string				mPatternTypeStr;
	string				mPatternFilterStr;
	string				mPatternOrderStr;

	friend class PatternCollection;		// ConvertPC heeft OpenFile, WriteHeader & WritePattern nodig
};
#endif // __PATTERNCOLLECTIONFILE_H
#endif // __PCDEV