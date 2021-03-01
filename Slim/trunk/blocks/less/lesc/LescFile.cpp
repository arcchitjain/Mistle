#ifdef BLOCK_LESS

#include "../../global.h"

#include <db/Database.h>
#include <itemstructs/ItemSet.h>

#include <sys/stat.h>
#include <FileUtils.h>

#include "LowEntropySet.h"
#include "LowEntropySetInstantiation.h"
#include "LowEntropySetCollection.h"

#include "LescFile.h"

LescFile::LescFile() {
	mFile = NULL;
	mFilename = "";
	mBuffer = new char[LESCFILE_BUFFER_LENGTH];

	mPreferredItemSetDataType = BM128ItemSetType;

	mDatabase = NULL;
	//mStdLengths = NULL;

	mSetArray = NULL;
	mOnArray = NULL;
	mInstCounts = NULL;
}
LescFile::~LescFile() {
	Close();
	delete[] mBuffer;
}

LowEntropySetCollection* LescFile::Open(const string &filename, Database * const db) {
	if(mFile != NULL)
		throw string("FicIscFile already opened a file...");
	OpenFile(filename);

	mDatabase = db;
	//mStdLengths = db->GetStdLengths();

	// - Read header
	ReadFileHeader();	// shared code met Read(..), dus passing via member variables

	mSetArray = new uint16[mMaxSetLength];
	mOnArray = new uint16[mMaxSetLength];
	mInstCounts = new uint32[(1 << mMaxSetLength)];

	LowEntropySetCollection *lesc = new LowEntropySetCollection(db, mPreferredItemSetDataType);
	lesc->SetLescFile(this);
	lesc->SetNumLowEntropySets(mNumPatterns);
	lesc->SetHasSepNumRows(mHasSepNumRows);
	lesc->SetMaxSetLength(mMaxSetLength);
	lesc->SetMaxEntropy(mMaxEntropy);
	lesc->SetLescOrder(mOrder);
	lesc->SetPatternType(mPatternType);
	lesc->SetDatabaseName(mDatabaseName);

	return lesc;
}
LowEntropySetCollection* LescFile::Read(const string &filename, Database * const db) {
	if(mFile != NULL)
		throw string("FicIscFile::Read -- FicIscFile already opened a file...");
	OpenFile(filename, LescFileReadable);

	mDatabase = db;
	//mStdLengths = db->GetStdLengths();

	// - Read header
	ReadFileHeader();	// shared code met Read(..), dus passing via member variables
	mSetArray = new uint16[mMaxSetLength * 2];
	mOnArray = new uint16[mMaxSetLength];
	mInstCounts = new uint32[(1 << mMaxSetLength)];

	LowEntropySet **collection = new LowEntropySet*[(size_t)mNumPatterns];
	for(uint32 i=0; i<mNumPatterns; i++) {
		collection[i] = ReadNextPattern();
	}

	LowEntropySetCollection *lesc = new LowEntropySetCollection(db, mPreferredItemSetDataType);
	lesc->SetLescFile(this);
	lesc->SetNumLowEntropySets(mNumPatterns);
	lesc->SetHasSepNumRows(mHasSepNumRows);
	lesc->SetLoadedPatterns(collection, mNumPatterns);
	lesc->SetMaxSetLength(mMaxSetLength);
	lesc->SetMaxEntropy(mMaxEntropy);
	lesc->SetLescOrder(mOrder);
	lesc->SetPatternType(mPatternType);
	lesc->SetDatabaseName(mDatabaseName);

	return lesc;
}

void LescFile::ReadFileHeader() {
	if(mFile == NULL)
		throw string("LescFile::ReadFileHeader -- File not opened.");
	if(fgets(mBuffer, LESCFILE_BUFFER_LENGTH, mFile) == NULL)
		throw string("LescFile::ReadFileHeader - Could not read LE-set collection file.");

	uint32 versionMajor, versionMinor;
	if(sscanf_s(mBuffer, "ficlesc-%d.%d\n", &versionMajor, &versionMinor) < 2)
		throw string("LescFile::ReadFileHeader - Can only read ficlesc files.");
	if(versionMajor != 1 || versionMinor != 0)
		throw string("LescFile::ReadFileHeader - Can only read ficlesc version 1.0 files.");

	//mFileFormat = FicLescFileVersion1_0;
	fgets(mBuffer, LESCFILE_BUFFER_LENGTH, mFile);
	uint32 sepNumRows = 0;
	char orderStr[10];
	char patTypeStr[10];
	char dbNameStr[100];

	//ficlesc-1.0
	//mi: numSets=67767 maxEnt=3.33 maxLen=5 sepRows=1 order=a patType=lesc dbName=mammals
	if(sscanf_s(mBuffer, "mi: numSets=%" I64d " maxEnt=%f maxLen=%d sepRows=%d lescOrder=%s patType=%s dbName=%s\r", &mNumPatterns, &mMaxEntropy, &mMaxSetLength, &sepNumRows, orderStr, 10, patTypeStr, 10, dbNameStr, 100) < 3)
		throw string("LescFile::ReadFileHeader - Version 1.0 incorrect header.");
	mHasSepNumRows = sepNumRows == 1;
	mOrder = LowEntropySetCollection::StringToLescOrderType(orderStr);
	mPatternType = patTypeStr;
	mDatabaseName = dbNameStr;

	if(mDatabase != NULL && mDatabase->HasBinSizes() && !mHasSepNumRows) {
		throw string("LescFile::ReadFileHeader - Reading a non-binned PSC for a binned-Database, that's odd.");
	}

	//uint32 pos = (uint32) mFilename.find_last_of("\\/");
	//ECHO(2, printf(" * File:\t\t%s\n", pos==string::npos ? mFilename.substr(mFilename.length()-55,mFilename.length()-1).c_str() : mFilename.substr(pos+1,mFilename.length()-1).c_str()));
	//ECHO(2, printf(" * Type:\t\tFIS\n * # ItemSets:\t\t%d\n * Min support:\t\t%d\n * Max set length:\t%d\n", mNumItemSets, mMinSup, mMaxSetLength));
}
LowEntropySet* LescFile::ReadNextPattern() {
	if(fgets(mBuffer, LESCFILE_BUFFER_LENGTH, mFile) == NULL)
		throw string("LescFile::ReadNextLowEntropySet -- Error reading from file");
	size_t curLineLen = strlen(mBuffer);
	if(curLineLen <= 1)
		throw string("LescFile::ReadNextLowEntropySet -- Empty line in file, that's rather odd, dude.");

	char *bp = mBuffer, *bpN;
	//uint32 freq = 0, numRows = 0;
	float entropy = 0.0;

	// 1 0.7532 0.2162 0.7838 ; 472 1711 ; 35
	// -> length
	uint32 numAttributes = strtoul(bp,&bpN,10);
	bp = bpN + 1;

	uint32 abetSize = 0; // mDatabase->GetAlphabetSize();
	uint32 numRows = mDatabase->GetNumRows();

	uint32 numInsts = 1 << numAttributes;
	LowEntropySet *leset = new LowEntropySet(entropy, 1);
	LowEntropySetInstantiation **insts = new LowEntropySetInstantiation*[numInsts];

	if(numAttributes > 0) {
		// -> entropy
		entropy = (float) strtod(bp,&bpN);
		bp = bpN + 1;

		// -> skip other floats
		bp = bp + 7 + 7 + 2;

		// -> inst counts
		for(uint32 i=0; i<numInsts; i++) {
			mInstCounts[i] = strtoul(bp,&bpN,10);
			bp = bpN + 1;
		}
		bp = bp + 2;

		// read attributes
		for(uint32 i=0; i<numAttributes; i++) {
			mSetArray[i] = (uint16) strtoul(bp,&bpN,10);
			//stdLen += (float) mStdLengths[mSetArray[i]];
			bp = bpN+1;
		}

		// !i! bitmasks 1x maken

		for(uint32 i=0; i<numInsts; i++) {
			uint32 numOn = 0;
			uint32 curOn = i;
			uint32 shift = 0;

			while(curOn > 0) {	// er staan nog bits aan
				if((curOn & 1) == 1)
					mOnArray[numOn++] = mSetArray[shift];
				curOn = curOn >> 1;
				shift++;
			}
			insts[i] = new LowEntropySetInstantiation(leset, ItemSet::Create(mPreferredItemSetDataType, mOnArray, numOn, abetSize, mInstCounts[i], mInstCounts[i], mInstCounts[i]), (double) mInstCounts[i] / numRows, i);
		}
	}
	ItemSet *attrDef = ItemSet::Create(mPreferredItemSetDataType, mSetArray, numAttributes, abetSize, 1, numRows, numRows);

	leset->SetAttributeDefinition(attrDef);
	leset->SetInstantiations(insts);
	leset->SetEntropy(entropy);

	return leset;
}



void LescFile::OpenFile(const string &filename, const LescFileOpenType openType /* = LescFileReadable */) {
	if(mFile != NULL)
		throw string("LescFile::OpenFile -- Filehandle already open");

	if(fopen_s(&mFile, filename.c_str(), openType == LescFileReadable ? "r" : "w"))
		throw string("LescFile::OpenFile -- Unable to open file");

	mFilename = filename;
}
void LescFile::Close() {
	CloseFile();
	mDatabase = NULL;

	delete[] mSetArray;
	mSetArray = NULL;

	delete[] mOnArray;
	mOnArray = NULL;

	delete[] mInstCounts;
	mInstCounts = NULL;

	mDatabaseName = "";
	mPatternType = "";
	mNumPatterns = 0;
	mMaxSetLength = 0;
	mMaxEntropy = 0;
	//mOrder = 0;
	mTag = "";
	mHasSepNumRows = false;
}

void LescFile::CloseFile() {
	if(mFile != NULL)
		fclose(mFile);
	mFile = NULL;
	mFilename = "";
}

int64 LescFile::GetFileSize() { 
	if(mFile == NULL || ferror(mFile))
		throw string("Unable to retrieve file size - file error or not opened.");
#if defined (_WINDOWS)
	struct _stati64 file;
	_fstati64(mFile->_file, &file);
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	struct stat file;
	fstat(mFile->_fileno, &file);
#endif
	return file.st_size;
}

void LescFile::Delete() {
	if(mFile != NULL)
		fclose(mFile);
	mFile = NULL;
	remove( mFilename.c_str() );
	mFilename = "";
}

bool LescFile::WriteLoadedLowEntropySetCollection(LowEntropySetCollection * const lesc, const string &fullpath) {
	if(mFile != NULL)
		throw string("LescFile::WriteLoadedLowEntropySetCollection -- LescFile already opened a file...");
	OpenFile(fullpath, LescFileWritable);

	uint32 numLESets = lesc->GetNumLoadedLowEntropySets();
	//if(!lesc->HasMinSupport()) lesc->DetermineMinSupport();
	//if(!lesc->HasMaxSetLength())	lesc->DetermineMaxSetLength();
	WriteHeader(lesc, numLESets);

	// - Write LowEntropySets
	LowEntropySet **collection = lesc->GetLoadedLowEntropySets();
	if(mSetArray != NULL)
		delete[] mSetArray;
	mSetArray = new uint16[lesc->GetMaxSetLength()];

	for(uint32 i=0; i<numLESets; i++)
		WriteLowEntropySet(collection[i]);

	return true;	
}

// Side effect: sets mHasSepNumRows, such that WriteItemSet knows what to write
void LescFile::WriteHeader(const LowEntropySetCollection *lesc, const uint64 numSets) {
	if(mFile == NULL)
		throw string("LescFile::WriteHeader -- No file handle open");
	if(ferror(mFile))
		throw string("LescFile::WriteHeader -- Error writing LescFile header");
	/*if(!lesc->HasMinSupport()) 
		throw string("LescFile::WriteHeader -- no minimum support available");*/
	if(!lesc->HasMaxSetLength())	
		throw string("LescFile::WriteHeader -- no maximum set length available");

	double entropy = lesc->GetMaxEntropy();
	uint64 numLESets = numSets;
	//uint32 minSup = isc->GetMinSupport();
	uint32 maxSetLength = lesc->GetMaxSetLength();

	bool hasSepNumRows = lesc->HasSepNumRows();
	mHasSepNumRows = hasSepNumRows;

	string orderStr = LowEntropySetCollection::LescOrderTypeToString(lesc->GetLescOrder());
	string pTypeStr = "lesc";//lesc->GetDataType();
	string dbNameStr = lesc->GetDatabaseName();

	if(mSetArray != NULL)
		delete[] mSetArray;
	mSetArray = new uint16[maxSetLength];	// je zult zo wel willen schrijven, en ik weet hoeveel max.

	// - Write header
	//ficlesc-1.0
	//mi: numSets=67767 maxEnt=3.33 maxLen=5 sepRows=1 lescOrder=lAeA patType=lesc dbName=mammals
	//if(sscanf_s(mBuffer, "mi: numSets=%" I64d " maxEnt=%lf maxLen=%d sepRows=%d lescOrder=%s patType=%s dbName=%s\n", &mNumPatterns, &mMaxEntropy, &mMaxSetLength, &sepNumRows, orderStr, 10, patTypeStr, 10, dbNameStr, 100) < 3)
	fprintf(mFile, "ficlesc-1.0\n");
	fprintf(mFile, "mi: numSets=%" I64d " maxEnt=%lf maxLen=%d sepRows=%d lescOrder=%s patType=%s dbName=%s\n", numLESets, entropy, maxSetLength, hasSepNumRows?1:0, orderStr.c_str(), pTypeStr.c_str(), dbNameStr.c_str());
}

void LescFile::WriteLoadedLowEntropySets(LowEntropySetCollection * const lesc) {
	if(mFile == NULL)
		throw string("LescFile::WriteLoadedLowEntropySets -- No file handle open");
	if(ferror(mFile))
		throw string("LescFile::WriteLoadedLowEntropySets -- Error writing LESets");

	// - Write LE sets
	uint32 numLESets = lesc->GetNumLoadedLowEntropySets();
	LowEntropySet **collection = lesc->GetLoadedLowEntropySets();
	for(uint32 i=0; i<numLESets; i++)
		WriteLowEntropySet(collection[i]);
}

void LescFile::WriteLowEntropySet(LowEntropySet * les) {
	if(mFile == NULL)
		throw string("LescFile::WriteLowEntropySet -- No file handle open");
	if(ferror(mFile))
		throw string("LescFile::WriteLowEntropySet -- Error writing ItemSet");

	uint32 len = les->GetLength();
	les->GetAttributeDefinition()->GetValuesIn(mSetArray);

	uint32 numInsts = les->GetNumInstantiations();
	LowEntropySetInstantiation **insts = les->GetInstantiations();

	char buffer[LESCFILE_BUFFER_LENGTH];
	char *bp = buffer;

	// length
	_itoa_s(len, buffer, LESCFILE_BUFFER_LENGTH, 10);
	bp += strlen(buffer);
	*bp = ' ';
	bp++;

	// entropy
	/**bp = '0'; bp++;
	*bp = '.'; bp++;
	*bp = '0'; bp++;
	*bp = '0'; bp++;
	*bp = ' '; bp++;*/

	char floatbuffer[100];

	int decimal;
	int sign;
	double entropy = les->GetEntropy();
	if(entropy == 0.000)
		printf("h\n");
	_fcvt_s(floatbuffer, 100, entropy, 5, &decimal, &sign);
	if(entropy < 1) {
		*bp = '0';
		bp += 1;
	} else {
		memcpy(bp,floatbuffer,sizeof(char) * decimal);
		bp += decimal;
	}
	*bp = '.';
	bp += 1;
	uint32 restsize = 4;
	for(; decimal<0; decimal++) {
		*bp = '0';
		bp += 1;
		restsize--;
	}
	memcpy(bp,floatbuffer + decimal, sizeof(char) * restsize);
	bp += restsize;
	*bp = ' ';
	bp += 1;

	// all-0 freq
	*bp = '0'; bp++;
	*bp = '.'; bp++;
	*bp = '0'; bp++;
	*bp = '0'; bp++;
	*bp = '0'; bp++;
	*bp = '0'; bp++;
	*bp = ' '; bp++;

	// all-1 freq
	*bp = '0'; bp++;
	*bp = '.'; bp++;
	*bp = '0'; bp++;
	*bp = '0'; bp++;
	*bp = '0'; bp++;
	*bp = '0'; bp++;
	*bp = ' '; bp++;

	*bp = ';';
	bp += 1;

	// inst counts
	for(uint32 i=0; i<numInsts; i++) {
		*bp = ' ';
		bp++;
		_itoa_s(insts[i]->GetItemSet()->GetSupport(), bp, LESCFILE_BUFFER_LENGTH - (bp - buffer), 10);
		bp += strlen(bp);
	}

	*bp = ' ';
	bp++;
	*bp = ';';
	bp++;

	// set itself
	for(uint32 j=0; j<len; j++) {
		*bp = ' ';
		bp++;
		_itoa_s(mSetArray[j], bp, LESCFILE_BUFFER_LENGTH - (bp - buffer), 10);
		bp += strlen(bp);
	}
	*bp = '\n';
	bp++;

	fwrite(buffer,sizeof(char),bp - buffer,mFile);
}

#endif // BLOCK_LESS
