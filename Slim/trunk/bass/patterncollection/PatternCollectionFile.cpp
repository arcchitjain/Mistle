#ifdef __PCDEV
#include "../blobal.h"

#include "../db/Database.h"
#include "../itemstructs/ItemSet.h"

#include <sys/stat.h>
#include <FileUtils.h>

#include "PatternCollection.h"

#include "PatternCollectionFile.h"

PatternCollectionFile::PatternCollectionFile() {
	mFile = NULL;
	mFileName = "";
	mFileMode = FileReadable;
	mFileVersionMajor = 1;
	mFileVersionMinor = 0;

	mBuffer = new char[PATTERNCOLLECTIONFILE_BUFFER_LENGTH];

}
PatternCollectionFile::~PatternCollectionFile() {
	Close();
	delete[] mBuffer;
}

PatternCollection* PatternCollectionFile::Open(const string &filename) {
	if(mFile != NULL)
		THROW("File handle already open");
	OpenFile(filename);

	mPatternCollection = new PatternCollection();

	// - Read header
	ReadFileHeader();	// shared code met Read(..), dus passing via member variables

	// - Set variables in pc
	mPatternCollection->SetNumPatterns(mNumPatterns);
	mPatternCollection->SetPatternFilterStr(mPatternFilterStr);
	mPatternCollection->SetPatternOrderStr(mPatternOrderStr);

	return mPatternCollection;
}
void PatternCollectionFile::Open(const string &filename, PatternCollection *pc) {
	if(mFile != NULL)
		THROW("File handle already open");
	OpenFile(filename);

	// *pc is an empty pattern collection
	if(pc == NULL)

	mPatternCollection = pc;

	// - Read header
	ReadFileHeader();	// shared code met Read(..), dus passing via member variables

	// - Set variables in pc
	pc->SetNumPatterns(mNumPatterns);
	pc->SetPatternFilterStr(mPatternFilterStr);
	pc->SetPatternOrderStr(mPatternOrderStr);

	return pc;
}


void PatternCollectionFile::Close() {
	CloseFile();

	mDatabaseNameStr = "";
	mPatternTypeStr = "";
	mPatternOrderStr = 0;
	mNumPatterns = 0;
}


PatternCollection* PatternCollectionFile::Read(const string &filename, Database * const db) {
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

void PatternCollectionFile::ReadFileHeader() {
	if(mFile == NULL)
		THROW("File not opened.");
	if(fgets(mBuffer, PATTERNCOLLECTIONFILE_BUFFER_LENGTH, mFile) == NULL)
		THROW("Could not read pattern collection file.");

	uint32 versionMajor, versionMinor;
	if(sscanf_s(mBuffer, "jam-pc-%d.%d\n", &versionMajor, &versionMinor) < 2)
		THROW("Can only read pattern collection files.");
	if(versionMajor != 1 || versionMinor != 0)
		THROW("Can only read jam-pattern collection version 1.0 files.");

	mFileVersionMajor = versionMajor;
	mFileVersionMinor = versionMinor;

	if(mFileVersionMajor == 1 && mFileVersionMinor == 0) {
		fgets(mBuffer, PATTERNCOLLECTIONFILE_BUFFER_LENGTH, mFile);
		char orderStr[100];
		char patTypeStr[100];
		char dbNameStr[100];

		//jam-pc-1.0
		//mi: numPatterns=67767 dbName=<string> patType=<string> order=<string>
#if defined (_MSC_VER) && defined (_WINDOWS)
		if(sscanf_s(mBuffer, "mi: numSets=%I64d dbName=%s patType=%s order=%s\r", &mNumPatterns, dbNameStr, 100, patTypeStr, 100, orderStr, 100) < 3)
#elif defined (__GNUC__) && (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
		if(sscanf(mBuffer, "mi: numSets=%lu dbName=%s patType=%s order=%s\r", &mNumPatterns, dbNameStr, patTypeStr, orderStr) < 3)
#endif
			THROW("Incorrect version 1.0 header.";

		mDatabaseName = dbNameStr;
		mPatternType = patTypeStr;
		mOrder = orderStr;

		ECHO(3, printf(" * File:\t\t%s\n", StringUtils::AbbreviatePath(mFilename, 55).c_str()));
		ECHO(3, printf(" * Type:\t\tFIS\n * # ItemSets:\t\t%d\n * Min support:\t\t%d\n * Max set length:\t%d\n", mNumItemSets, mMinSup, mMaxSetLength));
	}
}
Pattern* PatternCollectionFile::ReadPattern(uint64 id) {
	if(mFile == NULL)
		THROW("No file open");
	if(mFileMode == FileWritable || mFileMode == FileBinaryWritable)
		THROW("In write-mode");

	if(id < mCurPatternId) {
		// roll back
		fseek(mFile, mPatternsStartingPoint, SEEK_SET);
		mCurPatternId = 0;
	} 
	if(id > mCurPatternId) {
		// skip ahead
		while(mCurPatternId != id) {
			Pattern *p = ReadPattern();
			delete p;
		} 

	} /* else ... (mCurPatternId == id) */
	return ReadPattern();
}
/*
Pattern* PatternCollectionFile::ReadPattern() {
	// ...
	// afh van mPatternTypeStr
	// een Pattern...::Read(mFile) ?
	// nee, is lastig. beter gewoon aangepaste pcf's per pattern
}
*/

void PatternCollectionFile::OpenFile(const string &filename, const FileOpenType openType /* = FileReadable */) {
	if(mFile != NULL)
		THROW("Filehandle already open");

	if(fopen_s(&mFile, filename.c_str(), FileUtils::OpenTypeToStr(openType))
		THROW("Unable to open file");

	mFileName = filename;
}

void PatternCollectionFile::CloseFile() {
	if(mFile != NULL)
		fclose(mFile);
	mFile = NULL;
	mFileName = "";

}

__int64 PatternCollectionFile::GetFileSize() { 
	if(mFile == NULL || ferror(mFile))
		throw string("Unable to retrieve file size - file error or not opened.");
	struct _stati64 file;
	_fstati64(mFile->_file, &file);
	return file.st_size;
}

void PatternCollectionFile::Delete() {
	if(mFile != NULL)
		fclose(mFile);
	mFile = NULL;
	remove( mFilename.c_str() );
	mFilename = "";
}

bool PatternCollectionFile::WriteLoadedLowEntropySetCollection(LowEntropySetCollection * const lesc, const string &fullpath) {
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
void PatternCollectionFile::WriteHeader(const LowEntropySetCollection *lesc, const uint64 numSets) {
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
	//if(sscanf_s(mBuffer, "mi: numSets=%I64d maxEnt=%lf maxLen=%d sepRows=%d lescOrder=%s patType=%s dbName=%s\n", &mNumPatterns, &mMaxEntropy, &mMaxSetLength, &sepNumRows, orderStr, 10, patTypeStr, 10, dbNameStr, 100) < 3)
	fprintf(mFile, "ficlesc-1.0\n");
#if defined (_MSC_VER) && defined (_WINDOWS)
	fprintf(mFile, "mi: numSets=%I64d maxEnt=%lf maxLen=%d sepRows=%d lescOrder=%s patType=%s dbName=%s\n", numLESets, entropy, maxSetLength, hasSepNumRows?1:0, orderStr.c_str(), pTypeStr.c_str(), dbNameStr.c_str());
#elif defined (__GNUC__) && (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	fprintf(mFile, "mi: numSets=%I64d maxEnt=%lf maxLen=%d sepRows=%d lescOrder=%s patType=%s dbName=%s\n", numLESets, entropy, maxSetLength, hasSepNumRows?1:0, orderStr.c_str(), pTypeStr.c_str(), dbNameStr.c_str());
#endif
}

void PatternCollectionFile::WriteLoadedLowEntropySets(LowEntropySetCollection * const lesc) {
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

void PatternCollectionFile::WriteLowEntropySet(LowEntropySet * les) {
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
		_itoa_s(insts[i]->GetItemSet()->GetFrequency(), bp, LESCFILE_BUFFER_LENGTH - (bp - buffer), 10);
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

#endif
