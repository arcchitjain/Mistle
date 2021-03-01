#include "../Bass.h"

#include "../itemstructs/ItemSet.h"
#include "../itemstructs/BM128ItemSet.h"
#include "../itemstructs/BAI32ItemSet.h"
#include "../itemstructs/Uint16ItemSet.h"
#include "../db/Database.h"
#include "ItemSetCollection.h"

#include "BinaryFicIscFile.h"


// version 1.0
// BM128: <byte:num><4x dword mask><dword:freq><?dword:rows>
// UINT16: <byte:num>[<word item>]<dword:freq><?dword:rows>

// version 1.1
// header: like v1.0
// BM128: <4x dword mask><dword:freq><?dword:rows>
// UINT16: <byte:num>[<word item>]<dword:freq><?dword:rows>

// version 1.2
// header: like v1.0 + dType=%s
// data: like v 1.1
// - 1.2.1 added BAI32 Support

BinaryFicIscFile::BinaryFicIscFile() : IscFile() {
	mBuffer = new char[ISCFILE_BUFFER_LENGTH];

	mDatabaseName = "unknown";
	mPatternType = "huh";
	mSetArray = NULL;
	mHasSepNumRows = false;
	mNumItemSets = 0;
	mMaxSetLength = 0;
	mMinSup = UINT32_MAX_VALUE;
	mIscOrder = UnknownIscOrder;
	mIscTag = "";

	mFileFormat = BinFicIscFileVersion1_2;
	mFileDataType = BM128ItemSetType;

	mNumBMBytes = 0;
	mNumBMDWords = 0;
}

BinaryFicIscFile::~BinaryFicIscFile() {
	delete[] mSetArray;
	// mFile closed & mBuffer deleted via IscFile::~IscFile()
}

void BinaryFicIscFile::OpenFile(const string &filename, const FileOpenType openType) {
    if(mFile != NULL)
		THROW("Filehandle already open");

	//printf("Opening %s as binary\n", filename.c_str());
	FileOpenType fileOpenType = (openType == FileReadable ? FileBinaryReadable : (openType == FileWritable ? FileBinaryWritable : openType));

	if(fopen_s(&mFile, filename.c_str(), FileUtils::FileOpenTypeToString(fileOpenType).c_str()))
        THROW("Unable to open file");

    mFilename = filename;
}

ItemSetCollection* BinaryFicIscFile::Open(const string &filename) {
	if(mFile != NULL)
		THROW("BinaryFicIscFile already opened a file...");
	OpenFile(filename);

	// - Read header
	ReadFileHeader();	// shared code met Read(..), dus passing via member variables
	mSetArray = new uint16[mMaxSetLength];

	ItemSetCollection *isc = new ItemSetCollection(mDatabase, mPreferredDataType);
	isc->SetIscFile(this);
	isc->SetNumItemSets(mNumItemSets);
	isc->SetHasSepNumRows(mHasSepNumRows);
	isc->SetMaxSetLength(mMaxSetLength);
	isc->SetMinSupport(mMinSup);
	isc->SetPatternType(mPatternType);
	isc->SetIscOrder(mIscOrder);
	isc->SetDatabaseName(mDatabaseName);

	return isc;
}

ItemSetCollection* BinaryFicIscFile::Read(const string &filename) {
	OpenFile(filename, FileReadable);

	// - Read header
	ReadFileHeader();	// shared code met Read(..), dus passing via member variables
	mSetArray = new uint16[mMaxSetLength];

	ItemSet **collection = new ItemSet*[(size_t)mNumItemSets];
	for(uint32 i=0; i<mNumItemSets; i++) {
		collection[i] = ReadNextItemSet();
	}

	ItemSetCollection *isc = new ItemSetCollection(mDatabase, mPreferredDataType);
	isc->SetDataType(mPreferredDataType);
	isc->SetIscFile(this);
	isc->SetNumItemSets(mNumItemSets);
	isc->SetHasSepNumRows(mHasSepNumRows);
	isc->SetLoadedItemSets(collection, mNumItemSets);
	isc->SetMaxSetLength(mMaxSetLength);
	isc->SetMinSupport(mMinSup);
	isc->SetIscOrder(mIscOrder);
	isc->SetPatternType(mPatternType);
	isc->SetDatabaseName(mDatabaseName);

	return isc;
}

void BinaryFicIscFile::ReadFileHeader() {
	ReadLine(mBuffer, ISCFILE_BUFFER_LENGTH);

	uint32 versionMajor, versionMinor;
	if(sscanf_s(mBuffer, "binficfis-%d.%d\n", &versionMajor, &versionMinor) < 2)
		THROW("Can only read binficfis files.");
	if(versionMajor != 1 || !(versionMinor == 1 || versionMinor == 2))
		THROW("Can only read binficfis version 1.1 and 1.2 files.");

	if(versionMinor == 1)
		mFileFormat = BinFicIscFileVersion1_1;
	else
		mFileFormat = BinFicIscFileVersion1_2;

	ReadLine(mBuffer, ISCFILE_BUFFER_LENGTH);
	uint32 sepNumRows = 0;
	char iscOrderStr[10];
	char patTypeStr[10];
	char dbNameStr[100];
	char dataTypeStr[10];
	if(mFileFormat == BinFicIscFileVersion1_1) {
		if(sscanf_s(mBuffer, "mi: numSets=%" I64d " minSup=%d maxLen=%d sepRows=%d iscOrder=%s patType=%s dbName=%s\n", &mNumItemSets, &mMinSup, &mMaxSetLength, &sepNumRows, iscOrderStr, 10, patTypeStr, 10, dbNameStr, 100) < 3)
			THROW("Incorrect header.");
		mFileDataType = mDatabase->GetDataType();
	} else if(mFileFormat == BinFicIscFileVersion1_2) {
		if(sscanf_s(mBuffer, "mi: numSets=%" I64d " minSup=%d maxLen=%d sepRows=%d iscOrder=%s patType=%s dbName=%s dType=%s\n", &mNumItemSets, &mMinSup, &mMaxSetLength, &sepNumRows, iscOrderStr, 10, patTypeStr, 10, dbNameStr, 100, dataTypeStr, 10) < 3)
			THROW("Incorrect header.");
		string dataTypeStrStr = dataTypeStr;
		mFileDataType = ItemSet::StringToType(dataTypeStrStr);
	}

	mHasSepNumRows = sepNumRows == 1;
	mIscOrder = ItemSetCollection::StringToIscOrderType(iscOrderStr);
	mPatternType = patTypeStr;
	mDatabaseName = dbNameStr;
	mNumBMDWords = 4;
	mNumBMBytes = sizeof(uint32) * mNumBMDWords;

	if(mPreferredDataType != mFileDataType)
		THROW("Preferred data type does not equal the data type of the file. Things are bound to go sour, and we don't like sour. Sweeten it!");

	if (mFileDataType == BM128ItemSetType) {
		if(mFileFormat == BinFicIscFileVersion1_0)
			mBinaryLen = sizeof(uint8) + mNumBMBytes + sizeof(uint32) + (mHasSepNumRows ? sizeof(uint32) : 0);
		else 
			mBinaryLen = mNumBMBytes + sizeof(uint32) + (mHasSepNumRows ? sizeof(uint32) : 0);
	} else if (mFileDataType == Uint16ItemSetType) {
		mBinaryLen = sizeof(uint16) + sizeof(uint32) + (mHasSepNumRows ? sizeof(uint32) : 0);
	} else if (mFileDataType == BAI32ItemSetType) {
		uint32 abetlen = (uint32) mDatabase->GetAlphabetSize();
		mNumBMDWords = (abetlen / 32) + (abetlen % 32 > 0 ? 1 : 0);
		mNumBMBytes = mNumBMDWords * sizeof(uint32);
		mBinaryLen = mNumBMBytes + sizeof(uint32) + (mHasSepNumRows ? sizeof(uint32) : 0);
	}

	if(mDatabase != NULL && mDatabase->HasBinSizes() && !mHasSepNumRows) {
		THROW("Reading a non-binned ISC for a binned-Database, that's odd.");
	}
}

void BinaryFicIscFile::ReadBuf(char* buffer, uint32 count) {
	size_t numRead = fread(buffer, 1, count, mFile);
	if(numRead < 0) {
        throw string("BinaryFicIscFile::ReadBuf -- Error reading from file");
	}
}

ItemSet* BinaryFicIscFile::ReadNextItemSet() {
	if (mFileDataType == BM128ItemSetType) {
		return ReadNextBM128ItemSet();
	} else if (mFileDataType == Uint16ItemSetType) {
		return ReadNextUint16ItemSet();
	} else if (mFileDataType == BAI32ItemSetType) {
		return ReadNextBAI32ItemSet();
	}
	return NULL;
}

BM128ItemSet* BinaryFicIscFile::ReadNextBM128ItemSet() {
	uint32 curNumItems = 0, freq = 0, numRows = 0, *mask;

	double stdLen = 0.0;
	uint32 lastItem;
	uint32 idx = 0;
	if(mFileFormat == BinFicIscFileVersion1_1 || mFileFormat == BinFicIscFileVersion1_2) {
		ReadBuf(mBuffer, mBinaryLen);
		mask = (uint32*) (mBuffer);
		freq = *((uint32*) (mBuffer + 16));
		if (mHasSepNumRows) {
			numRows = *((uint32*) (mBuffer + 20));
		} else {
			numRows = freq;
		}
		if(mStdLengths != NULL) {
			for(uint32 i=0; i<4; i++) {
				if (mask[i]) {
					uint32 x = 0x80000000;
					for(uint32 j=0; j<32; j++) {
						if (mask[i] & x) {
							stdLen += mStdLengths[idx];
							curNumItems++;
							lastItem = idx;
						}
						x >>= 1;
						idx++;
					}
				} else {
					idx += 32;
				}
			}
		} else {
			for(uint32 i=0; i<4; i++) {
				if (mask[i]) {
					uint32 x = 0x80000000;
					for(uint32 j=0; j<32; j++) {
						if (mask[i] & x) {
							curNumItems++;
							lastItem = idx;
						}
						x >>= 1;
						idx++;
					}
				} else {
					idx += 32;
				}
			}
		}
	}
	if(mFileDataType == mPreferredDataType) {
		BM128ItemSet* is = new BM128ItemSet(0, freq, numRows, stdLen);
		is->CopyMaskFrom(mask, curNumItems, lastItem);
		return is;
	} else
		THROW("OOOWH, SHIT!");
}

BAI32ItemSet* BinaryFicIscFile::ReadNextBAI32ItemSet() {
	uint32 curNumItems = 0, freq = 0, numRows = 0, *mask;

	double stdLen = 0.0;
	uint32 lastItem;
	uint32 idx = 0;
	
	ReadBuf(mBuffer, mBinaryLen);
	mask = (uint32*) (mBuffer);
	freq = *((uint32*) (mBuffer + 16));
	if (mHasSepNumRows) {
		numRows = *((uint32*) (mBuffer + 20));
	} else {
		numRows = freq;
	}
	if(mStdLengths != NULL) {
		for(uint32 i=0; i<mNumBMDWords; i++) {
			if (mask[i]) {
				uint32 x = 0x00000001;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						stdLen += mStdLengths[idx];
						curNumItems++;
						lastItem = idx;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
	} else {
		for(uint32 i=0; i<mNumBMDWords; i++) {
			if (mask[i]) {
				uint32 x = 0x00000001;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						curNumItems++;
						lastItem = idx;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
	}
	
	if(mFileDataType == mPreferredDataType) {
		BAI32ItemSet* is = new BAI32ItemSet((uint32)mDatabase->GetAlphabetSize(), 0, freq, numRows, stdLen);
		is->CopyMaskFrom(mask, curNumItems, lastItem);
		return is;
	} else
		THROW("OOOWH, SHIT!");

}

Uint16ItemSet* BinaryFicIscFile::ReadNextUint16ItemSet() {
	uint32 curNumItems = 0, freq = 0, numRows = 0;

	ReadBuf(mBuffer + 2, mBinaryLen);
	
	curNumItems = *((uint16*) (mBuffer + 2));
	freq = *((uint32*) (mBuffer + 4));
	if (mHasSepNumRows) {
		numRows = *((uint32*) (mBuffer + 8));
	} else {
		numRows = freq;
	}

	ReadBuf((char*) mSetArray, curNumItems * sizeof(mSetArray[0]));

	double stdLen = 0.0;
	if(mStdLengths != NULL) {
		for (uint32 i = 0; i < curNumItems; i++) {
			stdLen += mStdLengths[mSetArray[i]];
		}
	}

	if(mPreferredDataType == Uint16ItemSetType) {
		return new Uint16ItemSet(mSetArray, curNumItems, 0, freq, numRows, stdLen);
	} /*else if(mPreferredDataType == BM128ItemSetType) {
		return new BM128ItemSet(mSetArray, curNumItems, 0, freq, numRows, stdLen);
	} */else
		THROW("AAAHW, SHIT!");
}

bool BinaryFicIscFile::WriteLoadedItemSetCollection(ItemSetCollection * const isc, const string &fullpath) {
	OpenFile(fullpath, FileWritable);

	uint32 numItemSets = isc->GetNumLoadedItemSets();
	if(!isc->HasMinSupport()) isc->DetermineMinSupport();
	if(!isc->HasMaxSetLength())	isc->DetermineMaxSetLength();
	WriteHeader(isc, numItemSets);

	// - Write ItemSets
	ItemSet **collection = isc->GetLoadedItemSets();
	if(mSetArray != NULL)
		delete[] mSetArray;
	mSetArray = new uint16[isc->GetMaxSetLength()];

	for(uint32 i=0; i<numItemSets; i++)
		WriteItemSet(collection[i]);

	return true;	
}

bool BinaryFicIscFile::WriteItemSetCollection(ItemSetCollection * isc, const string &fullpath) {
	OpenFile(fullpath, FileWritable);

	uint64 numItemSets = isc->GetNumItemSets();
	if(!isc->HasMinSupport()) isc->DetermineMinSupport();
	if(!isc->HasMaxSetLength())	isc->DetermineMaxSetLength();
	WriteHeader(isc, numItemSets);

	if(mSetArray != NULL)
		delete[] mSetArray;
	mSetArray = new uint16[isc->GetMaxSetLength()];

	// - Write ItemSets
	for(uint64 i=0; i<numItemSets; i++) {
		ItemSet *is = isc->GetNextItemSet();
		WriteItemSet(is);
		delete is;
	}

	return true;	
}

// Side effect: sets mHasSepNumRows, such that WriteItemSet knows what to write
void BinaryFicIscFile::WriteHeader(const ItemSetCollection *isc, const uint64 numSets) {
	if(mFile == NULL)
		THROW("No file handle open");
	if(ferror(mFile))
		THROW("Error writing BinaryFicIscFile header");
	if(!isc->HasMinSupport()) 
		THROW("No minimum support available");
	if(!isc->HasMaxSetLength())	
		THROW("No maximum set length available");

	uint64 numItemSets = numSets;
	uint32 minSup = isc->GetMinSupport();
	uint32 maxSetLength = isc->GetMaxSetLength();

	bool hasSepNumRows = isc->HasSepNumRows();
	mHasSepNumRows = hasSepNumRows;
	mPreferredDataType = isc->GetDataType();

	if(mPreferredDataType == BM128ItemSetType) {
		mNumBMDWords = 4;
		mNumBMBytes = sizeof(uint32) * mNumBMDWords;
	} else if(mPreferredDataType == BAI32ItemSetType) {
		uint32 abetlen = (uint32) isc->GetAlphabetSize();
		mNumBMDWords = (abetlen / 32) + (abetlen % 32 > 0 ? 1 : 0);
		mNumBMBytes = mNumBMDWords * sizeof(uint32);
	} 

	string orderStr = ItemSetCollection::IscOrderTypeToString(isc->GetIscOrder());
	string pTypeStr = isc->GetPatternType();
	string dbNameStr = isc->GetDatabaseName();
	string dataTypeStr = ItemSet::TypeToString(isc->GetDataType());

	if(mSetArray != NULL)
		delete[] mSetArray;
	mSetArray = new uint16[maxSetLength];	// je zult zo wel willen schrijven, en ik weet hoeveel max.

	// - Write header
	fprintf(mFile, "binficfis-1.%d\n", mFileFormat);
	// BinFicIscFileVersion1_0 & BinFicIscFileVersion1_1
	if(mFileFormat == BinFicIscFileVersion1_0 || mFileFormat == BinFicIscFileVersion1_1)
		fprintf(mFile, "mi: numSets=%" I64d " minSup=%d maxLen=%d sepRows=%d iscOrder=%s patType=%s dbName=%s\n", numItemSets, minSup, maxSetLength, hasSepNumRows?1:0, orderStr.c_str(), pTypeStr.c_str(), dbNameStr.c_str());
	// BinFicIscFileVersion1_2
	else if(mFileFormat == BinFicIscFileVersion1_2)
		fprintf(mFile, "mi: numSets=%" I64d " minSup=%d maxLen=%d sepRows=%d iscOrder=%s patType=%s dbName=%s dType=%s\n", numItemSets, minSup, maxSetLength, hasSepNumRows?1:0, orderStr.c_str(), pTypeStr.c_str(), dbNameStr.c_str(), dataTypeStr.c_str());
	else
		THROW("Unknown File Version");
}

void BinaryFicIscFile::WriteLoadedItemSets(ItemSetCollection * const isc) {
	if(mFile == NULL)
		THROW("No file handle open");
	if(ferror(mFile))
		THROW("Error writing ItemSets");

	// - Write ItemSets
	uint32 numItemSets = isc->GetNumLoadedItemSets();
	ItemSet **collection = isc->GetLoadedItemSets();
	for(uint32 i=0; i<numItemSets; i++)
		WriteItemSet(collection[i]);
}

void BinaryFicIscFile::WriteItemSet(ItemSet* is) {
	if(mFile == NULL)
		THROW("No file handle open");
	if(ferror(mFile))
		THROW("Error writing ItemSet");

	if (mPreferredDataType == BM128ItemSetType) {
		WriteBM128ItemSet((BM128ItemSet*) is);
	} else if (mPreferredDataType == Uint16ItemSetType) {
		WriteUint16ItemSet((Uint16ItemSet*) is);
	} else if (mPreferredDataType == BAI32ItemSetType) {
		WriteBAI32ItemSet((BAI32ItemSet*) is);
	}
}

void BinaryFicIscFile::WriteBM128ItemSet(BM128ItemSet* is) {
	uint8 curNumItems = is->GetLength();
	uint32* mask = is->GetMask();
	uint32 freq = is->GetSupport();
	uint32 numRows = is->GetNumOccs();

	if(mFileFormat == BinFicIscFileVersion1_0)
		fwrite(&curNumItems, sizeof(curNumItems), 1, mFile);
	fwrite(mask, sizeof(uint32), 4, mFile);

	fwrite(&freq, sizeof(uint32), 1, mFile);
	if (mHasSepNumRows) {
		fwrite(&numRows, sizeof(uint32), 1, mFile);
	}
}

void BinaryFicIscFile::WriteBAI32ItemSet(BAI32ItemSet *is) {
	uint32* mask = is->GetMask();
	uint32 freq = is->GetSupport();
	uint32 numRows = is->GetNumOccs();

	fwrite(mask, sizeof(uint32), mNumBMDWords, mFile);

	fwrite(&freq, sizeof(uint32), 1, mFile);
	if (mHasSepNumRows) {
		fwrite(&numRows, sizeof(uint32), 1, mFile);
	}
}

void BinaryFicIscFile::WriteUint16ItemSet(Uint16ItemSet* is) {
	uint16 curNumSets = is->GetLength();
	uint16* set = is->GetSet();
	uint32 freq = is->GetSupport();
	uint32 numRows = is->GetNumOccs();

	fwrite(&curNumSets, sizeof(uint16), 1, mFile);
	fwrite(&freq, sizeof(uint32), 1, mFile);
	if (mHasSepNumRows) {
		fwrite(&numRows, sizeof(uint32), 1, mFile);
	}
	fwrite(set, sizeof(uint16), curNumSets, mFile);
}
