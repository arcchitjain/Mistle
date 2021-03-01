#ifndef __FICISCFILE_H
#define __FICISCFILE_H

enum FicIscFileFormat {
	FicIscFileVersion1_1,
	FicIscFileVersion1_2,
	FicIscFileVersion1_3,
};

enum IscOrderType;

#include "IscFile.h"

class BASS_API FicIscFile : public IscFile {
public:
	// Creates new FicIscFile, open it using Open(&filename)
	FicIscFile();
	// Creates new FicIscFile and tries to open filename
	//FicIscFile(const string &filename, Database *db, ItemSetType ist = NoneItemSetType);
	// Zaps the FicIscFile, cleans up it's mess.
	virtual ~FicIscFile();

	// Opens the file, reads the header, and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Open(const string &filename);

	// Opens the file, reads all data and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Read(const string &filename);

	// Writes the provided ItemSetCollection to file (static-wannabee, vanwege overerving..)
	virtual bool WriteLoadedItemSetCollection(ItemSetCollection * const isc, const string &fullpath);
	// Writes the loaded ItemSets in the provided ItemSetCollection to the currently opened file
	virtual void WriteLoadedItemSets(ItemSetCollection * const isc);

	virtual ItemSet*	ReadNextItemSet();

	virtual IscFileType	GetType() { return FicIscFileType; }

protected:
	virtual void		ReadFileHeader();

	// Side effect: sets mHasSepNumRows, such that WriteItemSet knows what to write
	virtual void		WriteHeader(const ItemSetCollection *isc, const uint64 numSets);

	virtual void		WriteItemSet(ItemSet *is);


	// Via ReadFileHeader ingelezen stuff die naar ISC gepass't moet worden:
	string				mDatabaseName;
	string				mPatternType;
	uint64				mNumItemSets;
	uint32				mMaxSetLength;
	uint32				mMinSup;
	IscOrderType		mIscOrder;
	string				mIscTag;
	bool				mHasSepNumRows;	// read ItemSet has separate supports for db rows and transactions

	// Array of size of largest set in the file
	uint16				*mSetArray;
	FicIscFileFormat	mFileFormat;
};
#endif // __FICISCFILE_H
