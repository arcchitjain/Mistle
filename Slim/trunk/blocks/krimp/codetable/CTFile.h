#ifndef __CTFILE_H
#define __CTFILE_H

enum CTFileFormat {
	CTFileNoHeader,
	CTFileVersion1_0
};

// >= max line length
#define		CTFILE_BUFFER_LENGTH		200000


// Currently only used for READING CodeTables. Not used for writing, because of speed.
class CTFile {
public:
	// Creates new CTFile, open it using Open(&filename)
	CTFile(Database *db);
	// Creates new CTFile and tries to open filename
	CTFile(const string &filename, Database *db);

	~CTFile();

	void				Open(const string &filename);
	void				Close();

	uint32				GetNumSets()					{ return mNumSets; }
	uint32				GetAlphLen()					{ return mAlphLen; }
	uint32				GetMaxSetLen()					{ return mMaxSetLen; }

	void				SetNeedSupports(bool f)			{ mNeedSupports = f; }
	bool				GetNeedSupports()				{ return mNeedSupports; }

	void				ReadHeader();
	ItemSet*			ReadNextItemSet();

protected:
	uint32				CountCols(char *buffer);

	Database			*mDB;

	string				mFilename;
	FILE				*mFile;
	char				*mBuffer;

	uint16				*mSet;
	double				*mStdLengths;

	uint32				mNumSets;
	uint32				mAlphLen;
	uint32				mMaxSetLen;

	bool				mNeedSupports;
	CTFileFormat		mFileFormat;
	bool				mHasNumOccs;
};
#endif // __CTFILE_H
