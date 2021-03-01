#include "../Bass.h"
#include "../itemstructs/ItemSet.h"
#include "../db/Database.h"
#include "ItemSetCollection.h"

#include "FicIscFile.h"


FicIscFile::FicIscFile() : IscFile() {
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
	mFileFormat = FicIscFileVersion1_1;
}

FicIscFile::~FicIscFile() {
	// mFile closed & mBuffer deleted via IscFile::~IscFile()
	delete[] mSetArray;
}

ItemSetCollection* FicIscFile::Open(const string &filename) {
	if(mFile != NULL)
		THROW("FicIscFile already opened a file...");
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
ItemSetCollection* FicIscFile::Read(const string &filename) {
	OpenFile(filename, FileReadable);

	// - Read header
	ReadFileHeader();	// shared code met Read(..), dus passing via member variables
	mSetArray = new uint16[mMaxSetLength];

	ItemSet **collection = new ItemSet*[(size_t)mNumItemSets];
	for(uint32 i=0; i<mNumItemSets; i++) {
		collection[i] = ReadNextItemSet();
	}

	ItemSetCollection *isc = new ItemSetCollection(mDatabase, mPreferredDataType);
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
void FicIscFile::ReadFileHeader() {
	ReadLine(mBuffer, ISCFILE_BUFFER_LENGTH);

	uint32 versionMajor, versionMinor;
	if(sscanf_s(mBuffer, "ficfis-%d.%d\n", &versionMajor, &versionMinor) < 2)
		THROW("Can only read ficfis files.");
	if(versionMajor != 1 || (versionMinor != 3 && versionMinor != 2))
		THROW("Can only read ficfis version 1.2 and 1.3 files.");

	if(versionMinor == 2) { // Version 1.2, with new header, set lengths and numRows (numOccs) when necessary
		mFileFormat = FicIscFileVersion1_2;
		ReadLine(mBuffer, ISCFILE_BUFFER_LENGTH);
		uint32 sepNumRows = 0;
		if(sscanf_s(mBuffer, "%d %d %d %d\n", &mNumItemSets, &mMinSup, &sepNumRows, &mMaxSetLength) < 3)
			THROW("Version 1.2 incorrect header.");
		mHasSepNumRows = sepNumRows == 1;
	} else if(versionMinor == 3) {
		mFileFormat = FicIscFileVersion1_3;
		ReadLine(mBuffer, ISCFILE_BUFFER_LENGTH);
		uint32 sepNumRows = 0;
		char iscOrderStr[10];
		char patTypeStr[10];
		char dbNameStr[100];
		if(sscanf_s(mBuffer, "mi: numSets=%" I64d " minSup=%d maxLen=%d sepRows=%d iscOrder=%s patType=%s dbName=%s\n", &mNumItemSets, &mMinSup, &mMaxSetLength, &sepNumRows, iscOrderStr, 10, patTypeStr, 10, dbNameStr, 100) < 3)
			THROW("Version 1.3 incorrect header.");
		mHasSepNumRows = sepNumRows == 1;
		mIscOrder = ItemSetCollection::StringToIscOrderType(iscOrderStr);
		mPatternType = patTypeStr;
		mDatabaseName = dbNameStr;
	}
	if(mDatabase != NULL && mDatabase->HasBinSizes() && !mHasSepNumRows) {
		THROW("Reading a non-binned ISC for a binned-Database, that's odd.");
	}
			

	//uint32 pos = (uint32) mFilename.find_last_of("\\/");
	//ECHO(2, printf(" * File:\t\t%s\n", pos==string::npos ? mFilename.substr(mFilename.length()-55,mFilename.length()-1).c_str() : mFilename.substr(pos+1,mFilename.length()-1).c_str()));
	//ECHO(2, printf(" * Type:\t\tFIS\n * # ItemSets:\t\t%d\n * Min support:\t\t%d\n * Max set length:\t%d\n", mNumItemSets, mMinSup, mMaxSetLength));
}
ItemSet* FicIscFile::ReadNextItemSet() {
	try{
		size_t curLineLen = ReadLine(mBuffer, ISCFILE_BUFFER_LENGTH);
		if(curLineLen <= 1)
			THROW("Empty line in file, that's rather odd, dude.");
	} catch(...) {
		throw;
	}
	char *bp = mBuffer, *bpN;

	if(*bp == '(' || *bp == ' ')
		return ReadNextItemSet();		// dit was een lege itemset

	uint32 curNumItems = 0, freq = 0, numRows = 0;
	float stdLen = 0.0f;

	if(mFileFormat == FicIscFileVersion1_2 || mFileFormat == FicIscFileVersion1_3) { // version 1.2, with set lengths
		curNumItems = strtoul(bp,&bpN,10);
		bp = bpN + 2;
	}

	if(curNumItems > 0) {
		if(mStdLengths != NULL) {
			for(uint32 i=0; i<curNumItems; i++) {
				mSetArray[i] = (uint16) strtoul(bp,&bpN,10);
				stdLen += (float) mStdLengths[mSetArray[i]];
				bp = bpN+1;
			}
		} else {
			for(uint32 i=0; i<curNumItems; i++) {
				mSetArray[i] = (uint16) strtoul(bp,&bpN,10);
				bp = bpN+1;
			}
		}
		++bp;
		freq = strtoul(bp,&bpN,10);

		if(mHasSepNumRows) {
			bp = bpN + 1;
			numRows = strtoul(bp,&bpN,10);
		} else
			numRows = freq;
	}
	return ItemSet::Create(mPreferredDataType, mSetArray, curNumItems, (uint32) mDatabase->GetAlphabetSize(), 0, freq, numRows, stdLen, NULL);
}

bool FicIscFile::WriteLoadedItemSetCollection(ItemSetCollection * const isc, const string &fullpath) {
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

// Side effect: sets mHasSepNumRows, such that WriteItemSet knows what to write
void FicIscFile::WriteHeader(const ItemSetCollection *isc, const uint64 numSets) {
	if(mFile == NULL)
		THROW("No file handle open");
	if(ferror(mFile))
		THROW("Error writing FicIscFile header");
	if(!isc->HasMinSupport()) 
		THROW("No minimum support available");
	if(!isc->HasMaxSetLength())	
		THROW("No maximum set length available");

	uint64 numItemSets = numSets;
	uint32 minSup = isc->GetMinSupport();
	uint32 maxSetLength = isc->GetMaxSetLength();

	bool hasSepNumRows = isc->HasSepNumRows();
	mHasSepNumRows = hasSepNumRows;

	string orderStr = ItemSetCollection::IscOrderTypeToString(isc->GetIscOrder());
	string pTypeStr = isc->GetPatternType();
	string dbNameStr = isc->GetDatabaseName();

	if(mSetArray != NULL)
		delete[] mSetArray;
	mSetArray = new uint16[maxSetLength];	// je zult zo wel willen schrijven, en ik weet hoeveel max.

	// - Write header
	fprintf(mFile, "ficfis-1.3\n");
	fprintf(mFile, "mi: numSets=%" I64d " minSup=%d maxLen=%d sepRows=%d iscOrder=%s patType=%s dbName=%s\n", numItemSets, minSup, maxSetLength, hasSepNumRows?1:0, orderStr.c_str(), pTypeStr.c_str(), dbNameStr.c_str());
}

void FicIscFile::WriteLoadedItemSets(ItemSetCollection * const isc) {
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

void FicIscFile::WriteItemSet(ItemSet * is) {
	if(mFile == NULL)
		THROW("No file handle open");
	if(ferror(mFile))
		THROW("Error writing ItemSets");

	uint32 len = is->GetLength();
	is->GetValuesIn(mSetArray);

	char buffer[ISCFILE_BUFFER_LENGTH];
	char *bp = buffer;

	_itoa_s(len, buffer, ISCFILE_BUFFER_LENGTH, 10);
	bp += strlen(buffer);
	*bp = ':';
	*(bp+1) = ' ';
	bp += 2;

	for(uint32 j=0; j<len; j++) {
		_itoa_s(mSetArray[j], bp, ISCFILE_BUFFER_LENGTH - (bp - buffer), 10);
		bp += strlen(bp);
		*bp = ' ';
		bp++;
	}
	if(mHasSepNumRows) {
		*bp = '(';
		bp++;
		_itoa_s(is->GetSupport(), bp, ISCFILE_BUFFER_LENGTH - (bp - buffer), 10);
		bp += strlen(bp);
		*bp = ',';
		bp++;
		_itoa_s(is->GetNumOccs(), bp, ISCFILE_BUFFER_LENGTH - (bp - buffer), 10);
		bp += strlen(bp);
		*bp = ')';
		*(bp+1) = '\n';
		bp+=2;
	} else {
		*bp = '(';
		bp++;
		_itoa_s(is->GetSupport(), bp, ISCFILE_BUFFER_LENGTH - (bp - buffer), 10);
		bp += strlen(bp);
		*bp = ')';
		*(bp+1) = '\n';
		bp+=2;
	}
	fwrite(buffer,sizeof(char),bp - buffer,mFile);
}

/*uint32 FicIscFile::ScanPastSupport(uint32 support) {
	long curPos = ftell(mFile);
	size_t slen;
	int cursup;
	uint32 skipped = 0;
	int sup = (int)support;
	while(fgets(mBuffer, FICISCFILE_BUFFER_LENGTH, mFile) != NULL) {
		slen = strlen(mBuffer);
		if(slen > 1 && *mBuffer != '(') {
			char *bp = mBuffer + slen - 1;
			while(*bp != '(')
				--bp;
			bp++;
			cursup = atoi(bp);
			if(cursup < sup)
				break;
		}
		skipped++;
		curPos = ftell(mFile);
	}
	fseek(mFile, curPos, SEEK_SET); // check return val
	return skipped;
}*/

/*void FicIscFile::ScanPastCandidate(uint32 candi) {
	// We gaan er vanuit dat we nu bij candi 0 staan
	if(candi > mNumItemSets)
		throw string("Hebberd!");
	for(uint32 i=0; i<=candi; i++) {	// <= want we willen die candidate ook skippen, want candidate count begint bij 0...
		if(fgets(mBuffer, FICISCFILE_BUFFER_LENGTH, mFile) == NULL)
			break;
	}
}*/


/*FicIscFile::FicIscFile(const string &filename, Database *db, ItemSetType ist) : IscFile(db, ist) {
mBuffer = new char[FICISCFILE_BUFFER_LENGTH];
mFileFormat = FicIscFileVersion1_1;
Open(filename);
}*/
