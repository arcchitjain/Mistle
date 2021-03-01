#ifndef __FIMIISCFILE_H
#define __FIMIISCFILE_H

enum FimiIscFileFormat {
	FimiIscFileNoHeader,
	FimiIscFileVersion1_0,
	FimiIscFileVersion1_1,
};

#include "IscFile.h"

class BASS_API FimiIscFile : public IscFile {
public:
	FimiIscFile();
	//IscFimiFile(const string &filename, Database *db, ItemSetType ist = NoneItemSetType);
	virtual ~FimiIscFile();

	// Opens the file, reads the header, and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Open(const string &filename);

	// Opens the file, reads all data and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Read(const string &filename);

	// Writes the provided ItemSetCollection to file
	virtual bool WriteLoadedItemSetCollection(ItemSetCollection * const isc, const string &fullpath);
	// Writes the loaded ItemSets in the provided ItemSetCollection to the currently opened file
	virtual void WriteLoadedItemSets(ItemSetCollection * const isc);

	virtual ItemSet*	ReadNextItemSet();

	virtual IscFileType	GetType() { return FimiIscFileType; }

protected:
	virtual void		ReadFileHeader();
	void				SetIscProperties(ItemSetCollection *isc);
	// Sets isc properties: IscFile, numItemSets, minSup, maxSetLength, dataType
	void				ScanForIscProperties();

	virtual void		WriteHeader(const ItemSetCollection *isc, const uint64 numSets);

	virtual void		WriteItemSet(ItemSet *is);

	uint16				*mSetArray;		// tmp array of size db->GetMaxSetLength()
	bool				mHasSepNumRows;	// read ItemSet has separate supports for db rows and transactions
	uint32				mMinSup;
	uint32				mMaxSetLength;
	uint64				mNumItemSets;
	string				mPatternType;
	string				mDatabaseName;
	FimiIscFileFormat	mFileFormat;

};
#endif // __FIMIISCFILE_H
