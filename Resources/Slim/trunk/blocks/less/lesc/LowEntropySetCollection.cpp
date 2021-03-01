#ifdef BLOCK_LESS

//#include "../Bass.h"
#include "../../global.h"


#include <RandomUtils.h>
#include <SystemUtils.h>
#include <FileUtils.h>

#include <db/Database.h>
#include "LowEntropySet.h"
#include "LescFile.h"

#include "LowEntropySetCollection.h"

LowEntropySetCollection::LowEntropySetCollection(Database *db /* = NULL */, ItemSetType dataType /* = NoneItemSetType */) {
	mDatabase = db;
	mDatabaseName = "unknown";
	mPatternType = "huh";
	mCollection = NULL;

	mNumPatterns = 0;
	mNumLoadedPatterns = 0;
	mCurLoadedPattern = 0;

	mHasSepNumRows = false;

	mHasMaxSetLength = false;
	mMaxSetLength = 0;

	mAbSize = 0;

	mDataType = dataType == NoneItemSetType && db != NULL ? db->GetDataType() : dataType;
	mLescOrder = LescOrderUnknown;

	mLescFile = NULL;
}
LowEntropySetCollection::~LowEntropySetCollection() {
	if(mCollection != NULL)
		for(uint32 i=0; i<mNumLoadedPatterns; i++)
			delete mCollection[i];
	delete[] mCollection;
	delete mLescFile;
}

//////////////////////////////////////////////////////////////////////////
// Load/Read
//////////////////////////////////////////////////////////////////////////

// Use to read (at most) n lesets from file, use LowEntropySetCollection::ClearLoadedLowEntropySets() to clean up these lesets
void LowEntropySetCollection::LoadPatterns(uint64 numPatterns) {
	if(mLescFile == NULL)
		throw string("LowEntropySetCollection::LoadLowEntropySets -- Cant read ItemSetCollection from nowhere...");
	if(mCollection != NULL)
		throw string("LowEntropySetCollection::LoadLowEntropySets -- First clean up the in-memory ItemSets before reading more via LoadItemSets");
	if(numPatterns >= (uint64) UINT32_MAX_VALUE)
		throw string("LowEntropySetCollection::LoadLowEntropySets -- Cannot load more than 4 billion sets at a time");

	mNumLoadedPatterns = (uint32) numPatterns;
	mCollection = new LowEntropySet*[mNumLoadedPatterns];

	for(uint32 i=0; i<mNumLoadedPatterns; i++) {
		try {			mCollection[i] = mLescFile->ReadNextPattern();
		} catch(...) {	mNumLoadedPatterns = i;
		}
	}
}

void LowEntropySetCollection::SetLoadedPatterns(LowEntropySet **collection, uint64 colSize) {
	if(mCollection != NULL)
		throw string("Eerst je zooi opruimen, gozert.");
	if(colSize >= (uint64) UINT32_MAX_VALUE)
		throw string("Currently cannot handle more than 4 billion loaded item sets");

	mCollection = collection;
	mNumLoadedPatterns = (uint32) colSize;
	mCurLoadedPattern = 0;
}

void LowEntropySetCollection::ClearLoadedPatterns(bool zapSets) {
	if(mCollection != NULL && zapSets)
		for(uint32 i=0; i<mNumLoadedPatterns; i++)
			delete mCollection[i];
	delete[] mCollection;
	mCollection = NULL;
	mNumLoadedPatterns= 0;
	mCurLoadedPattern = 0;
}


// Iterator-like behavior, reads the next le-set from the LescFile. Unbuffered as of now.
LowEntropySet* LowEntropySetCollection::GetNextPattern() {
	LowEntropySet *le = NULL;
	if(mCurLoadedPattern < mNumLoadedPatterns) {
		le = mCollection[mCurLoadedPattern];
		mCollection[mCurLoadedPattern] = NULL;
		mCurLoadedPattern++;
	} else {
		le = mLescFile->ReadNextPattern();
	}
	return le;
}

void LowEntropySetCollection::SetMaxSetLength(uint32 maxSetLen) {
	mMaxSetLength = maxSetLen;
	//mHasMaxSetLength = (maxSetLen != 0 && maxSetLen != UINT32_MAX_VALUE);
	mHasMaxSetLength = maxSetLen != UINT32_MAX_VALUE;
}




//////////////////////////////////////////////////////////////////////////
// lescOrderType :: string<->type functions
//////////////////////////////////////////////////////////////////////////
LescOrderType LowEntropySetCollection::StringToLescOrderType(string order) {
	if(order.compare("lAeA")==0)
		return 	LescOrderLengthAscEntropyAsc;
	else if(order.compare("lDeA")==0)
		return 	LescOrderLengthDescEntropyAsc;
	else if(order.compare("eaA")==0)
		return 	LescOrderEntropyPerAttributeAsc;
	else if(order.compare("eaD")==0)
		return 	LescOrderEntropyPerAttributeDesc;
	else if(order.compare("u")==0)
		return 	LescOrderUnknown;
	throw string("Unknown LescOrderType");
}

string LowEntropySetCollection::LescOrderTypeToString(LescOrderType type) {
	switch(type) {
		case LescOrderLengthAscEntropyAsc:
			return "lAeA";
		case LescOrderLengthDescEntropyAsc:
			return "lDeA";
		case LescOrderEntropyPerAttributeAsc:
			return "eaA";
		case LescOrderEntropyPerAttributeDesc:
			return "eaD";
		case LescOrderUnknown:
			return "u";
		default:
			return "u";
	}
}

void LowEntropySetCollection::SortLoaded(LescOrderType order) {

	string sortDir;
	if(order == LescOrderLengthAscEntropyAsc)			sortDir = "length ascending, entropy ascending";
	else if(order == LescOrderLengthDescEntropyAsc)		sortDir = "length descending, entropy ascending";
	else if(order == LescOrderEntropyPerAttributeAsc)		sortDir = "entropy per attribute ascending";
	else if(order == LescOrderEntropyPerAttributeDesc)		sortDir = "entropy per attribute descending";

	/*if(order == RandomlescOrder) {
		ECHO(2,	printf(" * Randomizing order:\tin progress..."));
		this->RandomizeOrder();
		ECHO(2,	printf("\r * Randomizing order:\tdone           \n"));
	} else*/ if((order != LescOrderUnknown) && mNumLoadedPatterns > 0) {
		ECHO(2,	printf("\r * Sorting sets:\tin progress...                         "));
		this->MergeSort(LowEntropySetCollection::GetCompareFunction(order));
		ECHO(2,	printf("\r * Sorting sets:\tsorted on %s", sortDir.c_str())); // spaces to cover 'in progress'
	}
	mLescOrder = order;
}
LESCompFunc LowEntropySetCollection::GetCompareFunction(LescOrderType order) {
	if(order == LescOrderLengthAscEntropyAsc)			return CompareLengthAscEntropyDesc;
	else if(order == LescOrderLengthDescEntropyAsc)		return CompareLengthDescEntropyDesc;
	else if(order == LescOrderEntropyPerAttributeAsc)		return CompareEntropyPerAttributeAsc;
	else if(order == LescOrderEntropyPerAttributeDesc)		return CompareEntropyPerAttributeDesc;
	return CompareLengthDescEntropyDesc; // default
}

bool LowEntropySetCollection::CompareLengthAscEntropyDesc(LowEntropySet *a, LowEntropySet *b) {
	if(a->GetLength() > b->GetLength())				return false;	// prefer short
	else if(a->GetLength() < b->GetLength())		return true;
	else if(a->GetEntropy() < b->GetEntropy())		return true;	// length eq. prefer low
	else if(a->GetEntropy() > b->GetEntropy())		return false;	
	else if(a->Compare(b) < 0)						return true;	// frequencies & length equal
	else											return false;
}
bool LowEntropySetCollection::CompareLengthDescEntropyDesc(LowEntropySet *a, LowEntropySet *b) {
	if(a->GetLength() < b->GetLength())				return false;	// prefer long
	else if(a->GetLength() > b->GetLength())		return true;
	else if(a->GetEntropy() < b->GetEntropy())		return true;	// length eq. prefer low
	else if(a->GetEntropy() > b->GetEntropy())		return false;	
	else if(a->Compare(b) < 0)						return true;	// frequencies & length equal
	else											return false;
}
bool LowEntropySetCollection::CompareEntropyPerAttributeAsc(LowEntropySet *a, LowEntropySet *b) {
	if( a->GetEntropy()/a->GetLength() < 
		b->GetEntropy()/b->GetLength())				return true;	// prefer low entropy per attribute
	else if(a->GetEntropy()/a->GetLength() >
		b->GetEntropy()/b->GetLength())				return false;	
	else if(a->GetLength() < b->GetLength())		return false;	// prefer long
	else if(a->GetLength() > b->GetLength())		return true;
	else if(a->Compare(b) < 0)						return true;	// frequencies & length equal
	else											return false;
}
bool LowEntropySetCollection::CompareEntropyPerAttributeDesc(LowEntropySet *a, LowEntropySet *b) {
	if( a->GetEntropy()/a->GetLength() > 
		b->GetEntropy()/b->GetLength())				return true;	// prefer high entropy per attribute
	else if(a->GetEntropy()/a->GetLength() <
		b->GetEntropy()/b->GetLength())				return false;	
	else if(a->GetLength() < b->GetLength())		return false;	// prefer long
	else if(a->GetLength() > b->GetLength())		return true;
	else if(a->Compare(b) < 0)						return true;	// frequencies & length equal
	else											return false;
}

void LowEntropySetCollection::MergeSort(LESCompFunc cf) {
	if(mNumLoadedPatterns == 0)
		return;
	LowEntropySet** mergeSortAuxArr = new LowEntropySet*[(mNumLoadedPatterns+1)/2];
	MSortMergeSort(0, mNumLoadedPatterns-1, cf, mergeSortAuxArr);
	delete[] mergeSortAuxArr;
}

void LowEntropySetCollection::MSortMergeSort(uint32 lo, uint32 hi, LESCompFunc cf, LowEntropySet** mergeSortAuxArr) {
	if(lo<hi) {
		uint32 m = (lo+hi)/2;
		MSortMergeSort(lo, m, cf, mergeSortAuxArr);
		MSortMergeSort(m+1, hi, cf, mergeSortAuxArr);
		MSortMerge(lo, m, hi, cf, mergeSortAuxArr);
	}
}

void LowEntropySetCollection::MSortMerge(uint32 lo, uint32 m, uint32 hi, LESCompFunc cf, LowEntropySet** mergeSortAuxArr) {
	uint32 i, j, k;

	i=0; j=lo;
	// copy first half of array a to auxiliary array b
	while (j<=m)
		mergeSortAuxArr[i++]=mCollection[j++];

	i=0; k=lo;
	// copy back next-greatest element at each time
	while (k<j && j<=hi)
		if (cf(mergeSortAuxArr[i],mCollection[j]))
			mCollection[k++]=mergeSortAuxArr[i++];
		else
			mCollection[k++]=mCollection[j++];

	// copy back remaining elements of first half (if any)
	while (k<j)
		mCollection[k++]=mergeSortAuxArr[i++];
}

// laatste argument: ItemSetType preferred
void LowEntropySetCollection::ConvertLescFile(const string &inFullpath, const string &outFullpath, Database * const db, LescOrderType orderType, const ItemSetType dataType /* = NoneItemSetType */) {
	// memory calculation & alphabet length
	uint64 memUsePre = SystemUtils::GetProcessMemUsage();
	size_t abLen = db->GetAlphabetSize();
	uint64 maxMemUse = Bass::GetMaxMemUse();

	// geen interactie met DB, dus hoeft niet gelijk te zijn aan db->GetDataType(), maar zo slim mogelijk gekozen.
	ItemSetType useDataType = Uint16ItemSetType; 

	LowEntropySetCollection *lescOrig = LowEntropySetCollection::OpenLowEntropySetCollection(inFullpath,"",db,false,useDataType);

	uint64 numPatterns = lescOrig->GetNumLowEntropySets();
	uint32 maxLength = lescOrig->GetMaxSetLength();

	uint64 memUseEst = memUsePre + (numPatterns + numPatterns/2) * sizeof(ItemSet*) + 
						(numPatterns * (uint64) LowEntropySet::MemUsage(useDataType, (uint32) abLen, maxLength));

	if(memUseEst <= maxMemUse) {
		// niet chunken
		lescOrig->LoadPatterns(numPatterns);

		if(lescOrig->GetLescOrder() != orderType)
			lescOrig->SortLoaded(orderType);

		string outFullpathTmp = outFullpath + "-tmp";
		try {
			LowEntropySetCollection::WriteLowEntropySetCollection(lescOrig, outFullpathTmp, "");

			if(FileUtils::FileExists(outFullpath))
				FileUtils::RemoveFile(outFullpath);
			FileUtils::FileMove(outFullpathTmp, outFullpath);
		} catch(...) {
			delete lescOrig;
			throw;
		}
	} else {
		// > Chunken maar...

		// - in case diskspace to waste

		// - in case er gesort moet worden
		if(lescOrig->GetLescOrder() != orderType) {
			// moet gesort te worden, ruimte zat bovendien

			uint32 numChunks = (uint32) ceil((double) memUseEst / maxMemUse);
			uint32 maxChunkSize = (uint32) ceil((double) numPatterns / numChunks);

			for(uint32 i=0; i<numChunks; i++) {
				if(i == numChunks-1 && numChunks != 1) {
					uint64 numrestsets = numPatterns - (uint64) ((uint64) i * (uint64) maxChunkSize);
					ECHO(2,	printf("\n * Sorting sets:\tloading last %d sets                         ", numrestsets));
					lescOrig->LoadPatterns(numrestsets);
					ECHO(2,	printf("\n * Sorting sets:\tloaded last %d sets                         ", numrestsets));
				} else {
					ECHO(2,	printf("\n * Sorting sets:\tloading %d sets                         ", maxChunkSize));
					lescOrig->LoadPatterns(maxChunkSize);
					ECHO(2,	printf("\n * Sorting sets:\tloaded %d sets                         ", maxChunkSize));
				}

				lescOrig->SortLoaded(orderType);

				ECHO(2,	printf("\r * Sorting sets:\tsaving sorted chunk                         "));
				SaveChunk(lescOrig, outFullpath, i);
				ECHO(2,	printf("\r * Sorting sets:\tdone                         \n"));

				lescOrig->ClearLoadedPatterns(true);
			}
			if(false /*&& zapInFile*/) {	// we willen niet zappen, met lescfiles, nu niet iig
				lescOrig->GetLescFile()->CloseFile();
				if(!FileUtils::RemoveFile(inFullpath))
					printf("Error removing lesc conversion input file.");
			}

			MergeChunks(outFullpath, numChunks, db, lescOrig->GetDataType());
		} 
	}
	delete lescOrig;
}
//////////////////////////////////////////////////////////////////////////
// ItemSetCollection Open/Read//Write	Retrieve/Store/CanRetrieve
//////////////////////////////////////////////////////////////////////////
// name & path apart, db vereist
LowEntropySetCollection* LowEntropySetCollection::OpenLowEntropySetCollection(const string &lescName, const string &lescPath, Database * const db, const bool load /* = false */, const ItemSetType dataType /* = NoneItemSetType */) {
	string inLescPath = lescPath;
	string inLescFileName = lescName;
	string inLescFileExtension = "";

	string inLescFullPath = LowEntropySetCollection::DeterminePathFileExt(inLescPath, inLescFileName, inLescFileExtension);

	// - Check whether designated file exists
	if(!FileUtils::FileExists(inLescFullPath))
		throw string("LE-set Collection file does not seem to exist.");

	// - Create appropriate DbFile to load that type of database
	LescFile *lescFile = new LescFile();

	// - Determine internal datatype to read into
	ItemSetType inLescItemSetType = dataType == NoneItemSetType ? (db != NULL ? db->GetDataType() : BM128ItemSetType) : dataType;
	lescFile->SetPreferredDataType(inLescItemSetType);

	LowEntropySetCollection *lesc = NULL;
	try {	// Memory-leak-protection-try/catch
		lesc = load ? lescFile->Read(inLescFullPath, db) : lescFile->Open(inLescFullPath, db);
	} catch(...) {	
		delete lescFile;
		throw;
	}

	if(!lesc)
		throw string("Could not read input le-set collection: " + inLescFullPath);
	return lesc;
}

void LowEntropySetCollection::WriteLowEntropySetCollection(LowEntropySetCollection* const lesc, const string &lescName, const string &path /* =  */) {
	// - Determine whether `lescName` does not abusively contains a path
	size_t lescNameLastDirSep = lescName.find_last_of("\\/");

	// - Determine `outlescPath`
	string outlescPath = (path.length() == 0 && lescNameLastDirSep != string::npos) ? lescName.substr(0,lescNameLastDirSep+1) : path;

	//  - Check whether is a valid (slash or backslash-ended) path
	if(outlescPath.length() != 0 && outlescPath[outlescPath.length()-1] != '/' && outlescPath[outlescPath.length()-1] != '\\')
		outlescPath += '/';

	// - Determine `outlescFileName` = lescName + . + extension
	string outlescFileName = (lescNameLastDirSep != string::npos) ? lescName.substr(lescNameLastDirSep+1) : lescName;
	string outlescFileExtension = "";

	if(lescName.find('.') == string::npos) {
		//  - No extension provided, so extract from lescType
		outlescFileExtension = "lesc";
		outlescFileName += "." + outlescFileExtension;
	} else {
		//  - Extension provided, extract from name
		outlescFileExtension = lescName.substr(lescName.find_last_of('.')+1);
		//  - Redetermine defaulting `lescType` using extension
	}

	// - Determine `outDbFullPath` = outDbPath + outDbFileName 
	string outlescFullPath = outlescPath + outlescFileName;

	// - Create dir if not exist
	if(outlescPath.length() > 0 && !FileUtils::DirectoryExists(outlescPath))
		if(!FileUtils::CreateDir(outlescPath))
			throw string("Error: Trouble creating directory for writing LowEntropySetCollection: ") + outlescPath + "\n";

	// - Throws error in case unknown dbType.
	LescFile *lescFile = new LescFile();
	if(!lescFile->WriteLoadedLowEntropySetCollection(lesc, outlescFullPath)) {
		delete lescFile;
		throw string("Error: Could not write le-set collection.");
	}
	delete lescFile;
}

string LowEntropySetCollection::DeterminePathFileExt(string &path, string &filename, string &extension) {
	// - Split path from filename (if required, do nothing otherwise)
	FileUtils::SplitPathFromFilename(filename, path);

	//  - Check whether extension is provided
	if((filename.find('.') == string::npos)) { // - No extension in filename
		if(extension.length() == 0) { // - No separate extension given
			// - Add right extension, .fic as default, 
			filename += ".lesc";
			extension = "lesc";
		} else {
			if(extension[0] == '.')
				extension = extension.substr(1);
			filename += "." + extension;
		}
	} else
		extension = filename.substr(filename.find_last_of('.')+1);

	// - Determine `inDbFullPath`
	string fullPath = path + filename;

	return fullPath;
}

void LowEntropySetCollection::SaveChunk(LowEntropySetCollection* const lesc, const string &outFullpath, const uint32 numChunk) {
	size_t tempFileNameMaxLength = outFullpath.length() + 16;
	char* tempFileName = new char[tempFileNameMaxLength];	// '-c' = 2, '.tmp' = 4, 32bit uint max 10 chars, stop char
	sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.tmp", outFullpath.c_str(), numChunk);

	LescFile *lescf = new LescFile();
	lescf->WriteLoadedLowEntropySetCollection(lesc, tempFileName);
	delete lescf;
	delete[] tempFileName;
}

void LowEntropySetCollection::MergeChunks(const string &outFullpath, const uint32 numChunks, Database * const db, ItemSetType dataType) {
	size_t tempFileNameMaxLength = outFullpath.length() + 16;
	char* tempFileName = new char[tempFileNameMaxLength];	// '-c' = 2, '.tmp' = 4, 32bit uint max 10 chars, stop char

	if(numChunks > 20) {
		printf("Merging %d into %d chunks\n", numChunks, (numChunks+1)/2);

		// zoveel chunks moeten we eerst maar even verder mergen

		LowEntropySetCollection *chunkOneLesc, *chunkTwoLesc;

		LescFile *chunkOneFile, *chunkTwoFile;
		uint64 chunkOneLeft, chunkTwoLeft, chunkOneNum, chunkTwoNum;
		uint32 newChunkId = 0;
		uint32 numTwoChunks = numChunks;
		bool oddNumChunks = numTwoChunks % 2 == 1;
		numTwoChunks = oddNumChunks ? numTwoChunks - 1 : numTwoChunks;
		for(uint32 i=0; i<numTwoChunks; i+=2) {
			// open twee chunks
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.tmp", outFullpath.c_str(), i);
			string chunkOneFileName = tempFileName;

			// resume-proof :-)
			if(!FileUtils::FileExists(chunkOneFileName)) {
				newChunkId++;
				continue;
			}
			chunkOneLesc = LowEntropySetCollection::OpenLowEntropySetCollection(tempFileName, "", db, false, dataType);
			chunkOneFile = chunkOneLesc->GetLescFile();
			chunkOneLeft = chunkOneLesc->GetNumLowEntropySets();
			chunkOneNum = chunkOneLesc->GetNumLowEntropySets();

			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.tmp", outFullpath.c_str(), i+1);
			string chunkTwoFileName = tempFileName;
			chunkTwoLesc = LowEntropySetCollection::OpenLowEntropySetCollection(tempFileName, "", db, false, dataType);
			chunkTwoFile = chunkTwoLesc->GetLescFile();
			chunkTwoLeft = chunkTwoLesc->GetNumLowEntropySets();
			chunkTwoNum = chunkTwoLesc->GetNumLowEntropySets();

			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-d%d.tmp", outFullpath.c_str(), newChunkId);
			string combiChunkTempFileName = tempFileName;

			uint64 numSetsToBeMerged = chunkOneLeft + chunkTwoLeft;


			// ISC Setting Parameters
			LescFile *lescf = new LescFile();
			lescf->OpenFile(combiChunkTempFileName, LescFileWritable);
			lescf->WriteHeader(chunkOneLesc, numSetsToBeMerged);

			LESCompFunc gt = LowEntropySetCollection::GetCompareFunction(chunkOneLesc->GetLescOrder());
			LowEntropySet *chunkOneLES = NULL, *chunkTwoLES = NULL;
			uint64 numSetsMerged, numWritten = 0, numRead = 0;
			for(numSetsMerged=0; numSetsMerged<numSetsToBeMerged; numSetsMerged++) {
				if(chunkOneLES == NULL) {
					if(chunkOneLeft > 0) {
						try {
							chunkOneLES = chunkOneFile->ReadNextPattern();
							numRead++;
						} catch(...) {
							printf("MergeChunks -- Error reading the %dth itemset from chunk %s", chunkOneNum-chunkOneLeft, chunkOneFileName.c_str());
						}
						chunkOneLeft--;
					} else 
						break;
				}
				if(chunkTwoLES == NULL) {
					if(chunkTwoLeft > 0) {
						try {
							chunkTwoLES = chunkTwoFile->ReadNextPattern();
							numRead++;
						} catch(...) {
							printf("MergeChunks -- Error reading the %dth itemset from chunk %s", chunkTwoNum-chunkTwoLeft, chunkTwoFileName.c_str());
						}
						chunkTwoLeft--;
					} else 
						break;
				}
				if(gt(chunkOneLES,chunkTwoLES)) {
					lescf->WriteLowEntropySet(chunkOneLES);
					numWritten++;
					delete chunkOneLES;
					chunkOneLES = NULL;
				} else {
					lescf->WriteLowEntropySet(chunkTwoLES);
					numWritten++;
					delete chunkTwoLES;
					chunkTwoLES = NULL;
				}
			}
			if(chunkOneLeft > 0 || chunkOneLES != NULL) {
				lescf->WriteLowEntropySet(chunkOneLES);
				numWritten++;
				delete chunkOneLES;
				chunkOneLES = NULL;
				for(uint64 j=0; j<chunkOneLeft; j++) {
					try {
						chunkOneLES = chunkOneFile->ReadNextPattern();
						numRead++;
					} catch(...) {
						printf("MergeChunks -- Opvullen, Error reading the %dth itemset from chunk %d", chunkOneNum-chunkOneLeft, i);
					}
					lescf->WriteLowEntropySet(chunkOneLES);
					numWritten++;
					delete chunkOneLES;
					chunkOneLES = NULL;
				}
			} else if(chunkTwoLeft > 0 || chunkTwoLES != NULL) {
				lescf->WriteLowEntropySet(chunkTwoLES);
				numWritten++;
				delete chunkTwoLES;
				chunkTwoLES = NULL;
				for(uint64 j=0; j<chunkTwoLeft; j++) {
					try {
						chunkTwoLES = chunkTwoFile->ReadNextPattern();
						numRead++;
					} catch(...) {
						printf("MergeChunks -- Opvullen, Error reading the %dth itemset from chunk %d", chunkTwoNum-chunkTwoLeft, i+1);
					}
					lescf->WriteLowEntropySet(chunkTwoLES);
					numWritten++;
					delete chunkTwoLES;
					chunkTwoLES = NULL;
				}
			}
			if(numWritten != numSetsToBeMerged) {
				uint32 la = 10;
			}

			delete chunkOneLesc;
			delete chunkTwoLesc;
			delete lescf;

			newChunkId++;

			// zap orig chunks
			FileUtils::RemoveFile(chunkOneFileName);
			FileUtils::RemoveFile(chunkTwoFileName);
		}
		if(oddNumChunks) {
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.tmp", outFullpath.c_str(), numChunks-1);
			string tempFromFileName = tempFileName;
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-d%d.tmp", outFullpath.c_str(), newChunkId);
			string tempToFileName = tempFileName;

			FileUtils::FileMove(tempFromFileName, tempToFileName);
			newChunkId++;
		}

		// rename chunks
		for(uint32 i=0; i<newChunkId; i++) {
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-d%d.tmp", outFullpath.c_str(), i);
			string combiChunkTempFileName = tempFileName;
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.tmp", outFullpath.c_str(), i);
			string combiChunkRealFileName = tempFileName;
			FileUtils::FileMove(combiChunkTempFileName, combiChunkRealFileName);
		}

		// rinse and repeat
		MergeChunks(outFullpath, newChunkId, db, dataType);

	} else {
		printf("Merging %d chunks into one FicLescFile\n", numChunks, numChunks/2);

		LowEntropySetCollection **chunkLESCs = new LowEntropySetCollection*[numChunks];
		LescFile **chunkFiles = new LescFile*[numChunks];
		LowEntropySet **chunkSets = new LowEntropySet*[numChunks];
		uint64 *chunkLeft = new uint64[numChunks];

		uint64 totalSets = 0; //iscOrig->GetNumItemSets();

		for(uint32 i=0; i<numChunks; i++) {
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.tmp", outFullpath.c_str(), i);
			chunkLESCs[i] = LowEntropySetCollection::OpenLowEntropySetCollection(tempFileName, "", db, false, dataType);
			chunkFiles[i] = chunkLESCs[i]->GetLescFile(); 
			chunkLeft[i] = chunkLESCs[i]->GetNumLowEntropySets();
			chunkSets[i] = NULL;
			totalSets += chunkLeft[i];
		}

		// LESC Setting Parameters
		LescFile *fileOut = new LescFile();
		fileOut->OpenFile(outFullpath, LescFileWritable);
		fileOut->WriteHeader(chunkLESCs[0], totalSets);

		LESCompFunc gt = LowEntropySetCollection::GetCompareFunction(chunkLESCs[0]->GetLescOrder());

		uint64 i=0;
		uint32 numNonDepletedChunks = numChunks;
		uint64 numMerged = 0;
		for(numMerged=0; numMerged<totalSets; numMerged++) {
			// eerst even boekhouding doen
			for(uint32 j=0; j<numNonDepletedChunks; j++) {
				if(chunkSets[j] == NULL) {
					if(chunkLeft[j] > 0) {
						chunkSets[j] = chunkFiles[j]->ReadNextPattern();
						chunkLeft[j]--;
					} else {
						// onbruikbare chunk, moven die handel
						LowEntropySetCollection *tempLESC = chunkLESCs[j];
						LescFile *tempLESF = chunkFiles[j];
						for(uint32 k=j+1; k<numNonDepletedChunks; k++) {
							chunkLESCs[k-1] = chunkLESCs[k];
							chunkFiles[k-1] = chunkFiles[k];
							chunkLeft[k-1] = chunkLeft[k];
							chunkSets[k-1] = chunkSets[k];
						}
						numNonDepletedChunks--;
						chunkLESCs[numNonDepletedChunks] = tempLESC;
						chunkFiles[numNonDepletedChunks] = tempLESF;
						chunkSets[numNonDepletedChunks] = NULL;
						chunkLeft[numNonDepletedChunks] = 0;

						if(numNonDepletedChunks <= 1)
							break;
						j--; // smerige fix
					}
				}
			}
			if(numNonDepletedChunks <= 1)
				break;

			// hoogste vinden

			uint32 maxChunkIdx = 0;
			for(uint32 j=1; j<numNonDepletedChunks; j++) {
				if(gt(chunkSets[j],chunkSets[maxChunkIdx]))
					maxChunkIdx = j;
			}
			fileOut->WriteLowEntropySet(chunkSets[maxChunkIdx]);
			delete chunkSets[maxChunkIdx];
			chunkSets[maxChunkIdx] = NULL;
		}

		if(numNonDepletedChunks == 1) {
			// die ene chunk vinden, en wegschrijven die handel
			fileOut->WriteLowEntropySet(chunkSets[0]);
			delete chunkSets[0];
			while(chunkLeft[0] > 0) {
				chunkSets[0] = chunkFiles[0]->ReadNextPattern();
				chunkLeft[0]--;
				fileOut->WriteLowEntropySet(chunkSets[0]);
				delete chunkSets[0];
			}
		}

		fileOut->CloseFile();
		delete fileOut;

		// Clean up temp files
		for(uint32 i=0; i<numChunks; i++) {
			string chunkFilenameI = chunkFiles[i]->GetFileName();
			chunkFiles[i]->CloseFile();
			if(!FileUtils::RemoveFile(chunkFilenameI))
				throw string("Unable to clean up temporary chunk file.");
			delete chunkLESCs[i];
		}

		delete[] chunkLESCs;
		delete[] chunkFiles;
		delete[] chunkSets;
		delete[] chunkLeft;
	}

	delete tempFileName;
}

#endif // BLOCK_LESS
