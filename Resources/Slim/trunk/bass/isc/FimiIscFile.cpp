#include "../Bass.h"
#include "../itemstructs/ItemSet.h"
#include "../db/Database.h"
#include "ItemSetCollection.h"

#include "FimiIscFile.h"

FimiIscFile::FimiIscFile() : IscFile() {
	mBuffer = new char[ISCFILE_BUFFER_LENGTH];

	mSetArray = NULL;
	mHasSepNumRows = false;
	mNumItemSets = 0;
	mMaxSetLength = UINT32_MAX_VALUE;
	mMinSup = UINT32_MAX_VALUE;
	mFileFormat = FimiIscFileVersion1_1;
	mPatternType = "huh";
	mDatabaseName = "unknown";
}

FimiIscFile::~FimiIscFile() {
	// mFile closed & mBuffer deleted via IscFile::~IscFile()
	delete[] mSetArray;
}

ItemSetCollection* FimiIscFile::Open(const string &filename) {
	if(mFile != NULL)
		THROW("FimiIscFile already opened a file...");
	OpenFile(filename);

	// - Read header (if any, haal iig basale info op)
	ReadFileHeader();		// shared code met Read(..), dus passing via member variables

	ItemSetCollection *isc = new ItemSetCollection(mDatabase, mPreferredDataType);

	if(mMaxSetLength != UINT32_MAX_VALUE) {
		mSetArray = new uint16[mMaxSetLength];	// FimiIscFile heeft geen maxSetLength info, dus default naar |ab|
	} else
		mSetArray = new uint16[(mDatabase != NULL ? mDatabase->GetAlphabetSize() : UINT16_MAX_VALUE)];	// FimiIscFile heeft geen maxSetLength info, dus default naar |ab|

	SetIscProperties(isc);	// shared code met Read(..), dus passing via member variables

	return isc;
}
// Sets isc properties: IscFile, numItemSets, minSup, maxSetLength, dataType
void FimiIscFile::SetIscProperties(ItemSetCollection *isc) {
	isc->SetIscFile(this);
	isc->SetNumItemSets(mNumItemSets);
	if(mMinSup == UINT32_MAX_VALUE || mMaxSetLength == UINT32_MAX_VALUE)
		ScanForIscProperties();
	isc->SetMinSupport(mMinSup);
	isc->SetMaxSetLength(mMaxSetLength);
	isc->SetHasSepNumRows(mHasSepNumRows);
	isc->SetDataType(mPreferredDataType);
	isc->SetPatternType(mPatternType);
	isc->SetDatabaseName(mDatabaseName);
}
void FimiIscFile::ScanForIscProperties() {
	long pos = ftell(mFile);
	mMaxSetLength = 0;
	mMinSup = UINT32_MAX_VALUE;
	ItemSet *is;
	for(uint32 i=0; i<mNumItemSets; i++) {
		is = ReadNextItemSet();
		if(is->GetLength() > mMaxSetLength)
			mMaxSetLength = is->GetLength();
		if(is->GetSupport() < mMinSup)
			mMinSup = is->GetSupport();
		delete is;
	}
	fseek(mFile, pos, SEEK_SET);
}
ItemSetCollection* FimiIscFile::Read(const string &filename) {
	if(mFile != NULL)
		THROW("File already open.");
	OpenFile(filename);

	// - Read header
	ReadFileHeader();	// shared code met Open(..), dus passing via member variables
	size_t maxVal = (size_t) 0xFFFFFFFFFFFFFFFF;
	if(mNumItemSets > maxVal)
		THROW("IscFile contains more ItemSets than can be loaded on this machine");

	ItemSetCollection *isc = new ItemSetCollection(mDatabase, mPreferredDataType);
	ItemSet **collection = new ItemSet*[(size_t)mNumItemSets];
	if(mFileFormat == FimiIscFileVersion1_0 || mFileFormat == FimiIscFileNoHeader) {
		mSetArray = new uint16[(mDatabase != NULL ? mDatabase->GetAlphabetSize() : UINT16_MAX_VALUE)];	// FimiIscFile heeft geen maxSetLength info, dus default naar |ab|
		uint32 maxLen = 0;
		uint32 minSup = UINT32_MAX_VALUE;
		for(uint32 i=0; i<mNumItemSets; i++) {
			collection[i] = ReadNextItemSet();
			if(collection[i]->GetLength() > maxLen)
				maxLen = collection[i]->GetLength();
			if(collection[i]->GetSupport() < minSup)
				minSup = collection[i]->GetSupport();
		}
		mMaxSetLength = maxLen;
		mMinSup = minSup;
	} else {
		mSetArray = new uint16[mMaxSetLength];	// FimiIscFile heeft geen maxSetLength info, dus default naar |ab|
		for(uint32 i=0; i<mNumItemSets; i++)
			collection[i] = ReadNextItemSet();
	}

	SetIscProperties(isc);	// shared code met Open(..), passing via member variables
	isc->SetLoadedItemSets(collection, mNumItemSets);

	return isc;
}

void FimiIscFile::ReadFileHeader() {
	if(mFile == NULL)
		THROW("File not opened.");
	if(fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile) == NULL)
		THROW("Could not read fimi itemset collection file.");

	if(strcmp(mBuffer, "ficfi-1.1\n") == 0) {
		// Newest format, with numSets, with hasSepNumRows, with Set Lengths, support -and- numRows (so binned db's supported)
		mFileFormat = FimiIscFileVersion1_1;

		char dbFilename[100];
		fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile);
#if defined (_WINDOWS)
		sscanf_s(mBuffer, "dbFilename: %s\n", dbFilename, 100);
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
		sscanf(mBuffer, "dbFilename: %100s\n", dbFilename);
#endif
		mDatabaseName = dbFilename;
		mDatabaseName = mDatabaseName.substr(0,mDatabaseName.find_last_of('.'));
		size_t pos;
		if((pos = mDatabaseName.find_last_of("\\/")) != string::npos)
			mDatabaseName = mDatabaseName.substr(pos+1);

		char patType[10];
		fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile);
#if defined (_WINDOWS)
		sscanf_s(mBuffer, "patternType: %s\n", patType, 10);
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
		sscanf(mBuffer, "patternType: %10s\n", patType);
#endif
		mPatternType = patType;

		// Determine NumSets, doubles as built-in `isFinished` check, 
		fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile);
		string numSetsLineStart(mBuffer, 10);
		if(numSetsLineStart.compare("unfinished") == 0)
			THROW("Cannot read unfinished Fimi ItemSetCollection File");
		sscanf_s(mBuffer, "numFound: %" I64d "", &mNumItemSets);

		// Determine maxSetLength
		fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile);
		sscanf_s(mBuffer, "maxSetLength: %d", &mMaxSetLength);

		// Determine minSup
		fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile);
		sscanf_s(mBuffer, "minSupport: %d", &mMinSup);

		// Determine hasSepNumRows
		fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile);
		uint32 hasSepNumRows;
		sscanf_s(mBuffer, "hasSepNumRows: %d", &hasSepNumRows);
		mHasSepNumRows = (hasSepNumRows == 1);
	} else {
		if(strcmp(mBuffer, "ficfi-1.0\n") == 0) {
			// 1.0 format, with set lengths, support -and- numRows (so binned db's supported)
			mFileFormat = FimiIscFileVersion1_0;
			fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile);
			mHasSepNumRows = (atoi(mBuffer) == 1);
		} else {
			// Old format: no header, no set lengths, no numRows. Support present, though.
			mFileFormat = FimiIscFileNoHeader;
			mHasSepNumRows = false;
			rewind(mFile); // there is no header!
		}
		// Walk through file
		mNumItemSets = 0;
		mMinSup = UINT32_MAX_VALUE;
		mMaxSetLength = UINT32_MAX_VALUE;

		long startPos = ftell(mFile);
		while(fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile) != NULL && strlen(mBuffer)>1 ) {
			if( *mBuffer != '(' && *mBuffer != ' ')
				++mNumItemSets;
		}
		fseek(mFile, startPos, SEEK_SET);
	}
	if(!mHasSepNumRows && mDatabase != NULL && mDatabase->HasBinSizes()) {
		THROW("IscFile does not have binsizes, but the database does. That\'s odd.");
	}

	uint32 pos = (uint32) mFilename.find_last_of("\\/");
	ECHO(2, printf(" * File:\t\t%s\n", pos==string::npos ? mFilename.substr(mFilename.length()-55,mFilename.length()-1).c_str() : mFilename.substr(pos+1,mFilename.length()-1).c_str()));
	ECHO(2, printf(" * Type:\t\tFIMI\n * Has # rows:\t\t%s\n * # ItemSets:\t\t%" I64d "\n", mHasSepNumRows?"yes":"no", mNumItemSets));
}

/* We gaan er vanuit dat deze methode maximaal zo vaak wordt aangeroepen 
	als dat er niet-lege itemsets in de file staan. Zodra er een wel-lege itemset 
	gevonden wordt, skippen we naar de volgende regel.
 - Throws error at EOF
*/
ItemSet* FimiIscFile::ReadNextItemSet() {
	if(fgets(mBuffer, ISCFILE_BUFFER_LENGTH, mFile) == NULL)
		THROW("Error reading from file");

	size_t curLineLen = strlen(mBuffer);
	if(curLineLen <= 1)
		THROW("Empty line in file, that's rather odd, dude.");
	char *bp = mBuffer;

	if(*bp == '(' || *bp == ' ')
		return ReadNextItemSet();		// dit was een lege itemset

	uint32 curNumItems = 0, freq = 0, numRows = 0;
	float stdLen = 0.0f;

	if(mFileFormat != FimiIscFileNoHeader) {
		curNumItems = atoi(bp);
		while(*bp != ':')
			++bp;
		++bp; // skip semicolon
		++bp; // skip space

	} else { // old format
		for(uint32 col=0; col<curLineLen; col++) {
			if(*bp == '(')
				break;
			else if(*bp == ' ')
				curNumItems++;
			++bp;
		}
		bp = mBuffer;
	}

	if(curNumItems > 0) {
		for(uint32 i=0; i<curNumItems; i++) {
			mSetArray[i] = atoi(bp);
			stdLen += (float) mStdLengths[mSetArray[i]];
			while(*bp != ' ')
				++bp;
			++bp;
		}

		while(*bp != '(')
			++bp;
		++bp;
		freq = atoi(bp);

		if(mHasSepNumRows) {
			while(*bp != ',')
				++bp;
			++bp;
			numRows = atoi(bp);
		} else
			numRows = freq;
	}
	return ItemSet::Create(mPreferredDataType, mSetArray, curNumItems, (uint32) mDatabase->GetAlphabetSize(), 0, freq, numRows, stdLen, NULL);
}

bool FimiIscFile::WriteLoadedItemSetCollection(ItemSetCollection * const isc, const string &filename) {
	THROW("You can't write FIMI ItemSetCollections.");
}
void FimiIscFile::WriteLoadedItemSets(ItemSetCollection * const isc) {
	THROW("You can't write FIMI ItemSetCollections.");
}
void FimiIscFile::WriteHeader(const ItemSetCollection *isc, const uint64 numSets) {
	THROW("You can't write FIMI ItemSetCollections.");
}
void FimiIscFile::WriteItemSet(ItemSet *is) {
	THROW("You can't write FIMI ItemSetCollections.");
}
