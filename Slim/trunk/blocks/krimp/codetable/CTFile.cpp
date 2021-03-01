#include "../global.h"

#include <db/Database.h>
#include <itemstructs/ItemSet.h>

#include "CTFile.h"

CTFile::CTFile(Database *db) {
	mBuffer = new char[CTFILE_BUFFER_LENGTH];
	mDB = db;
	mNeedSupports = false;
	mNumSets = 0;
	mAlphLen = 0;
	mMaxSetLen = 0;
	mSet = NULL;
	mFile = NULL;
	mStdLengths = mDB->GetStdLengths();
}

CTFile::CTFile(const string &filename, Database *db) {
	mBuffer = new char[CTFILE_BUFFER_LENGTH];
	mDB = db;
	mNeedSupports = false;
	mNumSets = 0;
	mAlphLen = 0;
	mMaxSetLen = 0;
	mSet = NULL;
	mFile = NULL;
	mStdLengths = mDB->GetStdLengths();
	Open(filename);
}

CTFile::~CTFile() {
	delete[] mBuffer;
	delete[] mSet;
}

// ReadHeader -> Lees header info, bepaal of juiste type file is, etc

void CTFile::Open(const string &filename) {
	if(mFile != NULL)
		throw string("CTFile already opened a file...");

	mFilename = filename;
	mFile = fopen(filename.c_str(), "r");
	if(mFile == NULL || ferror(mFile))
		throw string("Could not open CodeTable file.");
	ReadHeader();
}
void CTFile::Close() {
	mNumSets = 0;
	mAlphLen = 0;
	fclose(mFile);
	mFile = NULL;
	mFilename = "";
}
void CTFile::ReadHeader() {
	if(mFile != NULL) {
		rewind(mFile);
		if(fgets(mBuffer, CTFILE_BUFFER_LENGTH, mFile) == NULL)
			throw string("CTFile::ReadHeader - Could not read CodeTable file.");

		if(strcmp(mBuffer, "ficct-1.0\n") == 0) {
			// CTFileVersion1_0 -- with set lengths and support.
			mFileFormat = CTFileVersion1_0;
			uint32 temp;

			// Read header
			if(fscanf(mFile, "%d %d %d %d\n", &mNumSets, &mAlphLen, &mMaxSetLen, &temp) < 4)
				throw string("CTFile::ReadHeader - Could not read appropiate ficct-1.0 header.");
			if(mAlphLen != mDB->GetAlphabetSize())
				throw string("CTFile::ReadHeader - Alphabet length CodeTable != alphabet length Database.");
			mHasNumOccs = false;//temp == 1; Don't read numOccs, occurences are not determined anyway
			//if(mDB->HasBinSizes() && !mHasNumOccs)
			//	throw string("CTFile::ReadHeader - Database has bin sizes, but code table does not have numOccs.");

		} else {
			// No header -- old format, without support.
			mFileFormat = CTFileNoHeader;
			rewind(mFile);
			mNumSets = 0;
			mAlphLen = 0;
			mMaxSetLen = 1;
			mHasNumOccs = false;
			uint32 cols;
			while(fgets(mBuffer, CTFILE_BUFFER_LENGTH, mFile) != NULL) {
				cols = CountCols(mBuffer);
				if(cols > 2) // with trailing space, at least 3 spaces in line
					++mNumSets;
				else if(cols == 1) // alphabet written without trailing space, 1 space counted
					++mAlphLen;
				if(cols > mMaxSetLen)
					mMaxSetLen = cols - 1;
			}
			rewind(mFile);
		}

		uint32 pos = (uint32) mFilename.find_last_of("\\/");
		ECHO(2, printf(" * File:\t\t%s\n", pos==string::npos ? mFilename.substr(mFilename.length()-55,mFilename.length()-1).c_str() : mFilename.substr(pos+1,mFilename.length()-1).c_str()));
		ECHO(2, printf(" * Type:\t\tCodeTable\n * # ItemSets:\t\t%d\n * Alphabet size:\t%d\n * Max set length:\t%d\n", mNumSets, mAlphLen, mMaxSetLen));

		mSet = new uint16[mMaxSetLen];	// Zou ook uit file gelezen kunnen/moeten worden?

	} else {	// File open error!
		throw string("CTFile::ReadHeader - File not opened.");
	}
}

ItemSet* CTFile::ReadNextItemSet() {
	if(fgets(mBuffer, CTFILE_BUFFER_LENGTH, mFile) == NULL)
		throw string("CTFile::ReadNextItemSet - dude, where's my itemset?");

	uint32 curLineLen = (uint32) strlen(mBuffer);
	if(curLineLen <= 1)
		throw string("CTFile::ReadNextItemSet - dude, no itemset here!");

	// Count items
	char *bp = mBuffer;
	uint32 curNumItems = 0;
	for(uint32 col=0; col<curLineLen; col++) {
		if(*bp == '(')
			break;
		else if(*bp == ' ')
			curNumItems++;
		bp++;
	}

	// Read items
	bp = mBuffer;
	float stdLen = 0.0f;
	for(uint32 i=0; i<curNumItems; i++) {
		mSet[i] = atoi(bp);
		stdLen += (float)mStdLengths[mSet[i]];
		while(*bp != ' ')
			++bp;
		++bp;
	}

	// Read count
	++bp;
	uint32 count = atoi(bp);

	// Determine support & numOcc
	if(mFileFormat == CTFileNoHeader) {
		ItemSet *is = ItemSet::Create(mDB->GetDataType(), mSet, curNumItems, (uint32) mDB->GetAlphabetSize(), count, 0, 0, stdLen, NULL);

		// Count support and numOccs using database
		mDB->CalcSupport(is);

		return is;

	} else { // CTFileVersion1_0 -- Read from file, always available, numOcc when needed
		while(*bp != ',')
			++bp;
		++bp;
		uint32 freq = atoi(bp);
		/*uint32 numOcc = freq;
		if(mHasNumOccs && curNumItems > 1) { // numOcc also from file, but not for alphabet
			while(*bp != ',')
				++bp;
			++bp;
			numOcc = atoi(bp);
		}*/ // -- Doesn't make sense to store/read numOcc, as occurences is NULL anyway
		return ItemSet::Create(mDB->GetDataType(), mSet, curNumItems, (uint32) mDB->GetAlphabetSize(), count, freq, 0, stdLen, NULL);
	}
}

uint32 CTFile::CountCols(char *buffer) {
	uint32 count = 0, index = 0;
	while(buffer[index] != '\n') {
		if(buffer[index++] == ' ')
			count++;
	}
	return count;
}
