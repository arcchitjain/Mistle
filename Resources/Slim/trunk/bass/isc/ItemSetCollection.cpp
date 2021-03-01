#include <omp.h>

#include "../Bass.h"

#include <RandomUtils.h>
#include <SystemUtils.h>
#include <FileUtils.h>

#include "../db/Database.h"
#include "../itemstructs/ItemSet.h"
#include "IscFile.h"

#include "ItemSetCollection.h"

ItemSetCollection::ItemSetCollection(Database *db /* = NULL */, ItemSetType dataType /* = NoneItemSetType */) {
	mDatabase = db;
	mDatabaseName = "unknown";
	mPatternType = "huh";
	mLoadedItemSets = NULL;

	mNumItemSets = 0;
	mSizeLoadedItemSets = 0;
	mNumLoadedItemSets = 0;
	mCurLoadedItemSet = 0;
	mLoadedBufferSize = 0;

	mHasSepNumRows = false;

	mHasMaxSetLength = false;
	mMaxSetLength = 0;

	mHasMinSup = false;
	mMinSup = 0;

	mDataType = dataType == NoneItemSetType && db != NULL ? db->GetDataType() : dataType;
	mIscOrder = UnknownIscOrder;

	mIscFile = NULL;
}

ItemSetCollection::ItemSetCollection(ItemSetCollection *isc) {
	mDatabase = isc->mDatabase;
	mDatabaseName = isc->mDatabaseName;
	mPatternType = isc->mPatternType;

	mNumItemSets = isc->mNumItemSets;

	mHasMinSup = isc->mHasMinSup;
	mMinSup = isc->mMinSup;

	mHasSepNumRows = isc->mHasSepNumRows;

	mMaxSetLength = isc->mMaxSetLength;
	mHasMaxSetLength = isc->mHasMaxSetLength;

	mLoadedItemSets = NULL;
	mNumLoadedItemSets = isc->mNumLoadedItemSets;
	mCurLoadedItemSet = isc->mCurLoadedItemSet;
	ItemSet **collection = isc->mLoadedItemSets;
	if(collection != NULL) {
		mLoadedItemSets = new ItemSet *[mNumLoadedItemSets];
		for(uint32 i=0; i<mNumLoadedItemSets; i++) 
			mLoadedItemSets[i] = collection[i]->Clone();
	}
	mDataType = isc->GetDataType();
	mIscOrder = isc->mIscOrder;

	mIscFile = NULL;
}
ItemSetCollection::~ItemSetCollection() {
	if(mLoadedItemSets != NULL)
		for(uint32 i=0; i<mNumLoadedItemSets; i++)
			delete mLoadedItemSets[i];
	delete[] mLoadedItemSets;
	delete mIscFile;
}

ItemSetCollection* ItemSetCollection::Clone() {
	return new ItemSetCollection(this);
}

size_t ItemSetCollection::GetAlphabetSize() const { 
	// absize moet uiteindelijk gewoon een membervariabele van isc zijn, en dus ook naar/uit de iscFile geschreven/gelezen worden.
	// (want, isc moet niet meer afhankelijk zijn van db)
	// absize wordt momenteel alleen door BinaryFicIscFile gebruikt
	return mDatabase->GetAlphabetSize(); 
}

//////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////

void ItemSetCollection::SetMaxSetLength(uint32 maxSetLen) {
	mMaxSetLength = maxSetLen;
	//mHasMaxSetLength = (maxSetLen != 0 && maxSetLen != UINT32_MAX_VALUE);
	mHasMaxSetLength = maxSetLen != UINT32_MAX_VALUE;
}
void ItemSetCollection::SetMinSupport(uint32 minSup) {
	mMinSup = minSup;
	mHasMinSup = (minSup != 0 && minSup != UINT32_MAX_VALUE);
} 
string ItemSetCollection::GetTag() const {
	string minSupStr;
	if(mHasMinSup) {
		char minSupChAr[20];
		_itoa_s(mMinSup, minSupChAr, 20, 10);
		minSupStr = minSupChAr;
	} else
		minSupStr = "?";
	string orderTypeStr = ItemSetCollection::IscOrderTypeToString(mIscOrder);
	return mDatabaseName + "-" + mPatternType + "-" + minSupStr + orderTypeStr;
}
void ItemSetCollection::ParseTag(const string &tag, string &dbName, string &patternType, string &settings, IscOrderType &orderType) {
	size_t firstDashPos = tag.find('-');
	size_t secondDashPos = tag.find_first_of('-',firstDashPos+1);
	size_t thirdDashPos = tag.find_first_of('-',secondDashPos+1);
	if(firstDashPos == string::npos || secondDashPos == string::npos)
		THROW("Not an ItemSetCollection tag: " + tag);
	
	// dbname-all-10[d], dbname-all-0.1ad
	dbName = tag.substr(0,firstDashPos);
	patternType = tag.substr(firstDashPos+1,secondDashPos-firstDashPos-1);

	string settingsStr = tag.substr(secondDashPos+1,thirdDashPos-secondDashPos-1);
	if(settingsStr.length() == 0)
		THROW("Not an ItemSetCollection tag: " + tag);

	size_t orderPos = string::npos;
	if(settingsStr[settingsStr.length()-1] >= 'a' && settingsStr[settingsStr.length()-1] <= 'z') {
		if(settingsStr.length() > 1 && settingsStr[settingsStr.length()-2] >= 'a' && settingsStr[settingsStr.length()-2] <= 'z') {
			// er zijn twee order-letters
			orderPos = settingsStr.length()-2;
		} else {
			// er is een order-letter
			orderPos = settingsStr.length()-1;
		}
	}
	string orderStr = orderPos == string::npos ? "u" : settingsStr.substr(orderPos);
	settings = settingsStr.substr(0, orderPos);

	orderType = ItemSetCollection::StringToIscOrderType(orderStr);
}

//////////////////////////////////////////////////////////////////////////
// Determines
//////////////////////////////////////////////////////////////////////////
void ItemSetCollection::DetermineMinSupport() {
	mMinSup = UINT32_MAX_VALUE;
	if(mNumLoadedItemSets == 0) {	// Hier kunnen we niets doen
		mHasMinSup = false;
		return;
	}

	for(uint32 i=0; i<mNumLoadedItemSets; i++) {
		if(mLoadedItemSets[i]->GetSupport() < mMinSup)
			mMinSup = mLoadedItemSets[i]->GetSupport();
	}
	mHasMinSup = true;
}
void ItemSetCollection::DetermineMaxSetLength() {
	mMaxSetLength = 0;
	if(mNumLoadedItemSets == 0) {	// Hier kunnen we niets doen
		mHasMaxSetLength = false;
		return;
	}

	for(uint32 i=0; i<mNumLoadedItemSets; i++) {
		if(mLoadedItemSets[i]->GetLength() > mMaxSetLength)
			mMaxSetLength = mLoadedItemSets[i]->GetLength();
	}
	mHasMaxSetLength = true;
}

//////////////////////////////////////////////////////////////////////////
// Load/Read
//////////////////////////////////////////////////////////////////////////

// Use to read (at most) n itemsets from file, use ItemSetCollect::ClearLoadedItemSets() to clean up these itemsets
void ItemSetCollection::LoadItemSets(uint64 numSets) {
	if(mIscFile == NULL)
		THROW("Cant read ItemSetCollection from nowhere...");
	if(mLoadedItemSets != NULL && mLoadedItemSets[mNumLoadedItemSets-1] != NULL)
		THROW("First clean up the in-memory ItemSets before reading more via LoadItemSets");
	if(numSets >= (uint64) UINT32_MAX_VALUE)
		THROW("Cannot load more than 4 billion sets at a time");

	mNumLoadedItemSets = (uint32) numSets;
	if(mNumLoadedItemSets > mSizeLoadedItemSets || mLoadedItemSets == NULL) {
		delete[] mLoadedItemSets;
		mLoadedItemSets = new ItemSet*[mNumLoadedItemSets];
		mSizeLoadedItemSets = mNumLoadedItemSets;
	}

	for(uint32 i=0; i<mNumLoadedItemSets; i++) {
		try {			
			mLoadedItemSets[i] = mIscFile->ReadNextItemSet();
		} catch(...) 
		{	
			mNumLoadedItemSets = i;
		}
	}
}

void ItemSetCollection::SetLoadedItemSets(ItemSet **collection, uint64 colSize) {
	if(mLoadedItemSets != NULL)
		THROW("Eerst je zooi opruimen, gozert.");
	if(colSize >= (uint64) UINT32_MAX_VALUE)
		THROW("Currently cannot handle more than 4 billion loaded item sets");

	mLoadedItemSets = collection;
	mNumLoadedItemSets = (uint32) colSize;
	mCurLoadedItemSet = 0;
}

void ItemSetCollection::ClearLoadedItemSets(bool zapSets) {
	if(mLoadedItemSets != NULL && zapSets)
		for(uint32 i=0; i<mNumLoadedItemSets; i++)
			delete mLoadedItemSets[i];
	delete[] mLoadedItemSets;
	mLoadedItemSets = NULL;
	mNumLoadedItemSets = 0;
	mSizeLoadedItemSets = 0;
	mCurLoadedItemSet = 0;
}


// Iterator-like behaviour, reads the next itemset from the IscFile. Unbuffered as of now.
ItemSet* ItemSetCollection::GetNextItemSet() {
	ItemSet *is = NULL;
	if(mLoadedBufferSize > 0 && mCurLoadedItemSet >= mNumLoadedItemSets) {
		LoadItemSets(mLoadedBufferSize);
		mCurLoadedItemSet = 0;
	} 
	if(mCurLoadedItemSet < mNumLoadedItemSets) {
		is = mLoadedItemSets[mCurLoadedItemSet];
		mLoadedItemSets[mCurLoadedItemSet] = NULL;
		mCurLoadedItemSet++;
	} else {
		is = mIscFile->ReadNextItemSet();
	}
	return is;
}

//////////////////////////////////////////////////////////////////////////
// ISC Add/Remove elements
//////////////////////////////////////////////////////////////////////////

void ItemSetCollection::RemoveOrderedSubset(ItemSet** arr, uint32 num) {
	uint32 cur = 0;
	uint32 insPos = 0;
	ItemSet **filteredItemSets = new ItemSet*[mNumLoadedItemSets];	// precaution for incorrect value of num, saves having to check all the time
	uint32 i=0; 
	for(i=0; i<mNumLoadedItemSets; i++) {
		if(mLoadedItemSets[i]->Equals(arr[cur])) {
			delete mLoadedItemSets[i];
			cur++;
			if(cur == num)
				break;
		} else {
			filteredItemSets[insPos++] = mLoadedItemSets[i];
		}
	}
	//memcpy_s(filteredItemSets+insPos, mNumLoadedItemSets-insPos, mLoadedItemSets+i+1, mNumLoadedItemSets-i-1);
	//memcpy(filteredItemSets+insPos, mLoadedItemSets+i+1, mNumLoadedItemSets-i-1);
	
	for(i=i+1; i<mNumLoadedItemSets; i++) {
		filteredItemSets[insPos] = mLoadedItemSets[i];
		insPos++;
	}
	delete[] mLoadedItemSets;
	mLoadedItemSets = filteredItemSets;
	mNumLoadedItemSets = mNumLoadedItemSets - cur;
	mNumItemSets = mNumItemSets - cur;
}


uint32 ItemSetCollection::ScanPastSupport(const uint32 support) {
	return 1; // !!! mIscFile->ScanPastSupport(support);
}
void ItemSetCollection::ScanPastCandidate(const uint64 candi) {
	// !!! mIscFile->ScanPastCandidate(candi);
	THROW("Not implemented anymore");
}

void ItemSetCollection::Rewind() {
	ItemSetCollection::ClearLoadedItemSets(false); // TODO: verify whether zapping is necessary.
	mIscFile->RewindFile();
}

/* Manipulate */

/*
// !!!
void ItemSetCollection::Normalize(bool translate, bool forceNormalize) {
	if(mDataType != Uint16ItemSetType)
		throw string("Can only normalize itemset collection when loaded with Uint16 data type.");
	ECHO(2,	printf(" * Normalizing:\t\ttranslating"));
	if((!mNormalized || forceNormalize) && translate) {
		this->TranslateForward();
		ECHO(2,	printf("\b\b\bed, "));
	} else
		ECHO(2,	printf("\b\bon skipped, "));

	ECHO(2,	printf("sorting items"));
	if((!mNormalized || forceNormalize)) {
		this->SortWithinItemSets();
		ECHO(2,	printf("\b\b\b\b\b\b\b\b\bed items \n"));
	} else
		ECHO(2,	printf(" skipped\n"));

	mNormalized = true;
}*/

void ItemSetCollection::SortLoaded(IscOrderType order) {
	string sortDir;
	if(order == LengthAscIscOrder)			sortDir = "support and ascending on length";
	else if(order == LengthDescIscOrder)	sortDir = "support and descending on length";
	else if(order == AreaLengthIscOrder)	sortDir = "area and descending on length";
	else if(order == AreaSupportIscOrder)	sortDir = "area and descending on support";
	else if(order == AreaSquareIscOrder)	sortDir = "area and descending on squareness";
	else if(order == LengthAFreqDIscOrder)	sortDir = "length ascending and support descending";
	else if(order == LengthDFreqDIscOrder)	sortDir = "length and support descending";
	else if(order == LengthAFreqAIscOrder)	sortDir = "length and support ascending";
	else if(order == JTestIscOrder)			sortDir = "vage jilles-sortering";
	else if(order == JTestRevIscOrder)		sortDir = "vage jilles-sortering in reverse";

	if(mDataType == Uint16ItemSetType)
		SortWithinItemSets();

	if(order == RandomIscOrder) {
		ECHO(2,	printf(" * Randomizing order:\tin progress..."));
		ItemSet::SortItemSetArray(mLoadedItemSets, mNumLoadedItemSets, order);
		ECHO(2,	printf("\r * Randomizing order:\tdone           \n"));
	} else if(order != NoneIscOrder && mNumLoadedItemSets > 0) {
		ECHO(2,	printf(" * Sorting sets:\tin progress..."));
		ItemSet::SortItemSetArray(mLoadedItemSets, mNumLoadedItemSets, order);
		ECHO(2,	printf("\r * Sorting sets:\tsorted on %s\n", sortDir.c_str())); // spaces to cover 'in progress'
	}
	mIscOrder = order;
}

void ItemSetCollection::RandomizeOrder() {
	RandomUtils::PermuteArray(mLoadedItemSets, mNumLoadedItemSets);
}

CompFunc ItemSetCollection::GetCompareFunction(IscOrderType order) {
	if(order == LengthAscIscOrder)			return CompareFreqDescLengthAsc;
	else if(order == AreaLengthIscOrder)	return CompareAreaDescLengthDesc;
	else if(order == AreaSupportIscOrder)	return CompareAreaDescSupportDesc;
	else if(order == AreaSquareIscOrder)	return CompareAreaDescSquareDesc;
	else if(order == LengthAFreqDIscOrder)	return CompareLenAscFreqDesc;
	else if(order == LengthDFreqDIscOrder)	return CompareLenDescFreqDesc;
	else if(order == LengthAFreqAIscOrder)	return CompareLenAscFreqAsc;
	else if(order == JTestIscOrder)			return CompareStdSzDescLenDescFreqDesc;
	else if(order == JTestRevIscOrder)		return CompareStdSzAscLenDescFreqDesc;
	else if(order == LexAscIscOrder)		return CompareLexAsc;
	else if(order == LexDescIscOrder)		return CompareLexDesc;
	else if(order == IdAscLexAscIscOrder)	return CompareIdAscLexAsc;
	else if(order == UsageDescIscOrder)		return CompareUsageDescLengthDesc;
	//if(order == LengthDescIscOrder)		
	return CompareFreqDescLengthDesc; // default
}
bool ItemSetCollection::CompareLexAsc(ItemSet *a, ItemSet *b) {
	if(a->CmpLexicographically(b) < 0)				return true;
	else											return false;
}
bool ItemSetCollection::CompareLexDesc(ItemSet *a, ItemSet *b) {
	if(a->CmpLexicographically(b) > 0)							return true;
	else											return false;
}
bool ItemSetCollection::CompareIdAscLexAsc(ItemSet *a, ItemSet *b) {
	if(a->GetID() > b->GetID())						return false;
	else if(a->GetID() < b->GetID())				return true;
	else if(a->CmpLexicographically(b) < 0)			return true;
	else											return false;
}

bool ItemSetCollection::CompareLenAscFreqDesc(ItemSet *a, ItemSet *b) {
	if(a->GetLength() > b->GetLength())				return false;
	else if(a->GetLength() < b->GetLength())		return true;
	else if(a->GetSupport() > b->GetSupport())	return true;	// length equal, prefer higher frequency
	else if(a->GetSupport() < b->GetSupport())	return false;	
	else if(a->CmpLexicographically(b) < 0)			return true;	// frequencies & length equal
	else											return false;
}
bool ItemSetCollection::CompareLenDescFreqDesc(ItemSet *a, ItemSet *b) {
	if(a->GetLength() < b->GetLength())				return false;
	else if(a->GetLength() > b->GetLength())		return true;
	else if(a->GetSupport() > b->GetSupport())	return true;	// length equal, prefer higher frequency
	else if(a->GetSupport() < b->GetSupport())	return false;	
	else if(a->CmpLexicographically(b) < 0)			return true;	// frequencies & length equal
	else											return false;
}
bool ItemSetCollection::CompareLenAscFreqAsc(ItemSet *a, ItemSet *b) {
	if(a->GetLength() > b->GetLength())				return false;
	else if(a->GetLength() < b->GetLength())		return true;
	else if(a->GetSupport() < b->GetSupport())	return true;	// length equal, prefer higher frequency
	else if(a->GetSupport() > b->GetSupport())	return false;	
	else if(a->CmpLexicographically(b) < 0)			return true;	// frequencies & length equal
	else											return false;
}


bool ItemSetCollection::CompareFreqDescLengthAsc(ItemSet *a, ItemSet *b) {
	if(a->GetSupport() > b->GetSupport())		return true;
	else if(a->GetSupport() < b->GetSupport())	return false;
	else if(a->GetLength() < b->GetLength())		return true;	// frequencies equal
	else											return false;
}

bool ItemSetCollection::CompareFreqDescLengthDesc(ItemSet *a, ItemSet *b) {
	if(a->GetSupport() > b->GetSupport())		return true;
	else if(a->GetSupport() < b->GetSupport())	return false;
	else if(a->GetLength() > b->GetLength())		return true;	// frequencies equal
	else if(a->GetLength() < b->GetLength())		return false;
	else if(a->CmpLexicographically(b) < 0)			return true;	// frequencies & length equal
	else											return false;
}

bool ItemSetCollection::CompareAreaDescLengthDesc(ItemSet *a, ItemSet *b) {
	uint32 areaA = a->GetSupport() * a->GetLength();
	uint32 areaB = b->GetSupport() * b->GetLength();
	if(areaA > areaB)							return true;
	else if(areaB > areaA)						return false;
	else if(a->GetLength() > b->GetLength())	return true;	// areas equal
	else										return false;
}
bool ItemSetCollection::CompareAreaDescSupportDesc(ItemSet *a, ItemSet *b) {
	uint32 areaA = a->GetSupport() * a->GetLength();
	uint32 areaB = b->GetSupport() * b->GetLength();
	if(areaA > areaB)								return true;
	else if(areaB > areaA)							return false;
	else if(a->GetSupport() > b->GetSupport())	return true;	// areas equal
	else											return false;
}
bool ItemSetCollection::CompareAreaDescSquareDesc(ItemSet *a, ItemSet *b) {
	uint32 areaA = a->GetSupport() * a->GetLength();
	uint32 areaB = b->GetSupport() * b->GetLength();
	if(areaA > areaB)							return true;
	else if(areaB > areaA)						return false;
	else if(abs((int)a->GetLength()-(int)a->GetSupport()) < abs((int)b->GetLength()-(int)b->GetSupport()))	return true;	// areas equal, squareness
	else										return false;
}
bool ItemSetCollection::CompareStdSzDescLenDescFreqDesc(ItemSet *a, ItemSet *b) {
	if(a->GetStandardLength() < b->GetStandardLength())			return true;
	else if(a->GetStandardLength() > b->GetStandardLength())	return false;
	else if(a->GetLength() > b->GetLength())		return true;	// frequencies equal
	else if(a->GetLength() < b->GetLength())		return false;
	else if(a->GetSupport() > b->GetSupport())	return true;
	else if(a->GetSupport() < b->GetSupport())	return false;
	else if(a->CmpLexicographically(b) < 0)			return true;	// frequencies & length equal
	else											return false;

}
bool ItemSetCollection::CompareStdSzAscLenDescFreqDesc(ItemSet *a, ItemSet *b) {
	if(a->GetStandardLength() > b->GetStandardLength())			return true;
	else if(a->GetStandardLength() < b->GetStandardLength())	return false;
	else if(a->GetLength() > b->GetLength())		return true;	// frequencies equal
	else if(a->GetLength() < b->GetLength())		return false;
	else if(a->GetSupport() > b->GetSupport())	return true;
	else if(a->GetSupport() < b->GetSupport())	return false;
	else if(a->CmpLexicographically(b) < 0)			return true;	// frequencies & length equal
	else											return false;

}
bool ItemSetCollection::CompareUsageDescLengthDesc(ItemSet *a, ItemSet *b) {
	if(a->GetUsageCount() > b->GetUsageCount())		return true;
	else if(a->GetUsageCount() < b->GetUsageCount())return false;
	else if(a->GetLength() > b->GetLength())		return true;	// usages equal
	else if(a->GetLength() < b->GetLength())		return false;
	else if(a->CmpLexicographically(b) < 0)			return true;	// usages & length equal
	else											return false;
}


void ItemSetCollection::TranslateBackward(ItemTranslator * const it) {
	for(uint32 i=0; i<mNumLoadedItemSets; i++)
		mLoadedItemSets[i]->TranslateBackward(it);
}
void ItemSetCollection::TranslateForward(ItemTranslator * const it) {
	for(uint32 i=0; i<mNumLoadedItemSets; i++)
		mLoadedItemSets[i]->TranslateForward(it);
}

void ItemSetCollection::SortWithinItemSets() {
	for(uint32 i=0; i<mNumLoadedItemSets; i++)
		mLoadedItemSets[i]->Sort();
}

void ItemSetCollection::BubbleSort(CompFunc cf) {
	ItemSet *t;
	for(uint32 i=1; i<mNumLoadedItemSets; i++)  {
		for(uint32 j=0; j<mNumLoadedItemSets-i; j++)
			if(cf(mLoadedItemSets[j], mLoadedItemSets[j+1])) {
				t = mLoadedItemSets[j];
				mLoadedItemSets[j] = mLoadedItemSets[j+1];
				mLoadedItemSets[j+1] = t;
			}
	}
}

// Statisch
void ItemSetCollection::MergeSort(ItemSet **itemSetArray, uint32 numItemSets, CompFunc cf) {

	if(numItemSets == 0)
		return;
	ItemSet** mergeSortAuxArr;
	if(Bass::GetNumThreads() != 0) {
		mergeSortAuxArr = new ItemSet*[(numItemSets+1)/2];	// single threaded
		MSortMergeSort(itemSetArray, 0, numItemSets-1, cf, mergeSortAuxArr);
	} else {
		printf("parasort\n");
		mergeSortAuxArr = new ItemSet*[(numItemSets+1)];	// parallel
		
		uint32 numThreads = 1;
		uint32 chunk_size = numItemSets / numThreads;

		/*#pragma omp parallel 
		{
			#pragma omp parallel for schedule(static,chunk_size)
			{
				
			}
		}*/
	}
	delete[] mergeSortAuxArr;
}
void ItemSetCollection::MSortMergeSort(ItemSet **itemSetArray, uint32 lo, uint32 hi, CompFunc cf, ItemSet** mergeSortAuxArr) {
	if(lo<hi) {

		uint32 m = (lo+hi)/2;

		MSortMergeSort(itemSetArray, lo, m, cf, mergeSortAuxArr); // single threaded
		MSortMergeSort(itemSetArray, m+1, hi, cf, mergeSortAuxArr);	// single threaded

		/*
		uint32 auxOffset = m - lo;

		omp_set_nested(0);
		//printf_s("%d\n", omp_get_num_threads( ));
		#pragma omp parallel 
		{
			#pragma omp sections
			{
				#pragma omp section
				{
					MSortMergeSort(itemSetArray, lo, m, cf, mergeSortAuxArr);
					printf("%d", omp_get_num_threads());
				}
				#pragma omp section
				{
					MSortMergeSort(itemSetArray, m+1, hi, cf, mergeSortAuxArr+auxOffset);	// parallel
					printf("%d", omp_get_num_threads());
				}
			}
		}
		*/
		MSortMerge(itemSetArray, lo, m, hi, cf, mergeSortAuxArr);
	}
}
void ItemSetCollection::MSortMerge(ItemSet **itemSetArray, uint32 lo, uint32 m, uint32 hi, CompFunc cf, ItemSet** mergeSortAuxArr) {
	uint32 i, j, k;

	i=0; j=lo;
	// copy first half of array a to auxiliary array b
	while (j<=m)
		mergeSortAuxArr[i++]=itemSetArray[j++];

	i=0; k=lo;
	// copy back next-greatest element at each time
	while (k<j && j<=hi)
		if (cf(mergeSortAuxArr[i],itemSetArray[j]))
			itemSetArray[k++]=mergeSortAuxArr[i++];
		else
			itemSetArray[k++]=itemSetArray[j++];

	// copy back remaining elements of first half (if any)
	while (k<j)
		itemSetArray[k++]=mergeSortAuxArr[i++];
}
// Niet Statisch
void ItemSetCollection::MergeSort(CompFunc cf) {
	if(mNumLoadedItemSets == 0)
		return;
	ItemSet** mergeSortAuxArr = new ItemSet*[(mNumLoadedItemSets+1)/2];
	MSortMergeSort(mLoadedItemSets, 0, mNumLoadedItemSets-1, cf, mergeSortAuxArr);
	delete[] mergeSortAuxArr;
}

/*void ItemSetCollection::FilterEmerging(float emerging, Database *partialDb, Database *fullDb, const string &fileName) {
	ECHO(2, printf("** Filtering Emerging Patterns :: "));
	ItemSetCollection *inIsc = new ItemSetCollection(fullDb);
	uint8 outbak = Bass::GetOutputLevel(); 
	if(Bass::GetOutputLevel() == 2) Bass::SetOutputLevel(1);
	inIsc->OpenFile(fileName);
	IscFile *inFile = inIsc->GetIscFile();

	string tmpName = fileName + ".tmp";
	IscFile *outFile = new IscTxtFile();
	outFile->Open(tmpName, FileWritable);

	ItemSet **iss = fullDb->GetRows();
	float constant = fullDb->GetNumRows() / (float)partialDb->GetNumRows();
	uint32 numRows = fullDb->GetNumRows();
	uint32 length = inFile->GetNumItemSets();
	uint32 fullSup;
	ItemSet *is;
	printf("  0%%");
	uint32 percent = length / 100;
	for(uint32 i=0; i<length; i++) {
		if(i % percent == 0)
			printf("\b\b\b\b%3d%%", i/percent);
		fullSup = 0;
		is = inFile->ReadNextItemSet();
		for(uint32 j=0; j<numRows; j++)
			if(iss[j]->IsSubset(is))
				fullSup++;
		if((constant * is->GetFrequency()) / fullSup > emerging)
			outFile->WriteItemSet(is);
		delete is;
	}
	outFile->Close();
	delete inIsc;
	delete outFile;

	remove(fileName.c_str()); // check return val 
	rename(tmpName.c_str(), fileName.c_str()); // check return val

	Bass::SetOutputLevel(outbak);
	ECHO(2, printf("\b\b\b\bdone\n"));
}
*/

/*
	FicMain::ConvertFis(..) {
		-old-
		ItemSetCollection *isc = OpenISC(db);
		isc->ReadCollection();
		isc->CloseFile();
		isc->WriteFile(outName, outOrder, mConfig->Read<uint32>("isctranslate") == 0 ? false : true);

		-new-
		ItemSetCollection::ConvertIscFile(db, inName, inType, outName, outType, translate, orderType);

			-of-

		ItemSetCollection *isc = OpenISC(db);
		isc->ConvertAndWriteTo(outName, outType, translate, orderType);
		isc->CloseFile();
	}
*/

// laatste argument: ItemSetType preferred
void ItemSetCollection::ConvertIscFile(const string &inPath, const string &inIscFileName, const string &outPath, const string &outIscFileName, Database *db, IscFileType outType, IscOrderType orderType, bool translate /* = false */, bool zapInFile /* = false */, uint32 minSup /* = 0 */, const ItemSetType dataType /* = NoneItemSetType */) {
	// memory calculation & alphabet length
	uint64 memUsePre = SystemUtils::GetProcessMemUsage();
	size_t abLen = db->GetAlphabetSize();
	uint64 maxMemUse = Bass::GetMaxMemUse();

	string inFullPath = inPath + inIscFileName;
	string outFullPath = outPath + outIscFileName;

	// geen interactie met DB, dus hoeft niet gelijk te zijn aan db->GetDataType(), maar zo slim mogelijk gekozen.
	ItemSetType sugDataType = db->GetAlphabetSize() > 128 ? BAI32ItemSetType : BM128ItemSetType;
	ItemSetType useDataType = (dataType != NoneItemSetType) ? ((dataType == BM128ItemSetType && sugDataType != BM128ItemSetType) ? sugDataType : dataType) : sugDataType;

	ItemSetCollection *iscOrig = ItemSetCollection::OpenItemSetCollection(inFullPath,"",db,false);

	uint64 numSets = iscOrig->GetNumItemSets();
	uint32 maxLength = iscOrig->GetMaxSetLength();

	uint64 memUseEst = memUsePre + (numSets + numSets/2) * sizeof(ItemSet*) + (numSets * (uint64) ItemSet::MemUsage(useDataType, (uint32) abLen, maxLength));

	if(memUseEst <= maxMemUse) {
		// niet chunken
		iscOrig->LoadItemSets(numSets);

		if(translate)
			iscOrig->TranslateForward(db->GetItemTranslator());
		if(dataType == Uint16ItemSetType)
			iscOrig->SortWithinItemSets();
		if(iscOrig->GetIscOrder() != orderType)
			iscOrig->SortLoaded(orderType);

		string outFullPathTmp = outFullPath + "-tmp";
		try {
			ItemSetCollection::WriteItemSetCollection(iscOrig, outFullPathTmp, "", outType);
			if(zapInFile) {
				iscOrig->GetIscFile()->CloseFile();
				if(!FileUtils::RemoveFile(inFullPath))
					printf("Error removing Isc conversion input file.");
			}

			if(FileUtils::FileExists(outFullPath))
				FileUtils::RemoveFile(outFullPath);
			FileUtils::FileMove(outFullPathTmp, outFullPath);
		} catch(...) {
			delete iscOrig;
			throw;
		}
	} else {
		// > Chunken maar...

		// - in case diskspace to waste

		// - in case er gesort moet worden
		if(iscOrig->GetIscOrder() != orderType) {
			// moet gesort te worden, ruimte zat bovendien

			uint32 numChunks = (uint32) ceil((double) memUseEst / maxMemUse);
			uint32 maxChunkSize = (uint32) ceil((double) numSets / numChunks);

			IscFileType chunkType = (dataType == BM128ItemSetType) ? BinaryFicIscFileType : outType;
			for(uint32 i=0; i<numChunks; i++) {
				if(i == numChunks-1 && numChunks != 1) {
					uint64 numrestsets = numSets - (uint64) ((uint64) i * (uint64) maxChunkSize);
					iscOrig->LoadItemSets(numrestsets);
				} else
					iscOrig->LoadItemSets(maxChunkSize);
				if(translate)						iscOrig->TranslateForward(db->GetItemTranslator());
				if(dataType == Uint16ItemSetType)	iscOrig->SortWithinItemSets();

				iscOrig->SortLoaded(orderType);

				SaveChunk(iscOrig, outFullPath, i, chunkType);

				iscOrig->ClearLoadedItemSets(true);
			}
			if(zapInFile) {
				iscOrig->GetIscFile()->CloseFile();
				if(!FileUtils::RemoveFile(inFullPath))
					printf("Error removing Isc conversion input file.");
			}

			MergeChunks(outPath, outPath, outIscFileName, numChunks, outType, db, iscOrig->GetDataType(), chunkType);
		} else {
			// hoeft niet gesort te worden

			IscFile *iscOut = IscFile::Create(outType);
			iscOut->OpenFile(outFullPath, FileWritable);
			iscOut->WriteHeader(iscOrig, iscOrig->GetNumItemSets());

			uint32 numChunks = (uint32) ceil((double) memUseEst / maxMemUse);
			uint32 maxChunkSize = (uint32) ceil((double) numSets / numChunks);
			for(uint32 i=0; i<numChunks; i++) {
				if(i == numChunks-1 && numChunks != 1) {
					uint64 numrestsets = numSets - (uint64) ((uint64) i * (uint64) maxChunkSize);
					iscOrig->LoadItemSets(numrestsets);
				} else
					iscOrig->LoadItemSets(maxChunkSize);

				if(translate)						iscOrig->TranslateForward(db->GetItemTranslator());
				if(dataType == Uint16ItemSetType)	iscOrig->SortWithinItemSets();

				iscOut->WriteLoadedItemSets(iscOrig);
				iscOrig->ClearLoadedItemSets(true);
			}
			iscOut->CloseFile();
			delete iscOut;

			if(zapInFile) {
				iscOrig->GetIscFile()->CloseFile();
				if(!FileUtils::RemoveFile(inFullPath))
					printf("Error removing Isc conversion input file.");
			}
		}
	}
	delete iscOrig;
}
void ItemSetCollection::SaveChunk(ItemSetCollection* const isc, const string &outFullpath, const uint32 numChunk, IscFileType outType) {
	if(!isc->HasMaxSetLength())		THROW("Original ItemSetCollection does not have required MaxSetLength.");
	if(!isc->HasMinSupport())		THROW("Original ItemSetCollection does not have required MinSup.");

	size_t tempFileNameMaxLength = outFullpath.length() + 16;
	char* tempFileName = new char[tempFileNameMaxLength];	// '-c' = 2, '.tmp' = 4, 32bit uint max 10 chars, stop char
	sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.isc-tmp", outFullpath.c_str(), numChunk);

	IscFile *iscf = IscFile::Create(outType);
	iscf->WriteLoadedItemSetCollection(isc, tempFileName);
	delete iscf;
	delete[] tempFileName;
}

void ItemSetCollection::MergeChunks(const string &inPath, const string &outPath, const string &iscFileName, const uint32 numChunks, IscFileType outType, Database * const db, ItemSetType dataType, IscFileType chunkType) {
	uint64 memUsePre = SystemUtils::GetProcessMemUsage();
	uint64 maxMemUse = Bass::GetMaxMemUse();

	size_t tempFileNameMaxLength = outPath.length() + iscFileName.length() + 20;
	char* tempFileName = new char[tempFileNameMaxLength];	// '-c' = 2, '.isc-tmp' = 8, 32bit uint max 10 chars, stop char

	string inFullPath = inPath + iscFileName;
	string outFullPath = outPath + iscFileName;

	if(numChunks > 20) {
		uint32 percent = 0;
		printf(" * Merging:\t\t %d chunks into %d (%d%%)\t\t\t\r", numChunks, (numChunks+1)/2, percent);

		// zoveel chunks moeten we eerst maar even verder mergen

		ItemSetCollection *chunkOneIsc, *chunkTwoIsc;

		IscFile *chunkOneFile, *chunkTwoFile;
		uint64 chunkOneLeft, chunkTwoLeft, chunkOneNum, chunkTwoNum;
		uint32 newChunkId = 0;
		uint32 numTwoChunks = numChunks;
		bool oddNumChunks = numTwoChunks % 2 == 1;
		numTwoChunks = oddNumChunks ? numTwoChunks - 1 : numTwoChunks;
		for(uint32 i=0; i<numTwoChunks; i+=2) {
			// open twee chunks
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.isc-tmp", inFullPath.c_str(), i);
			string chunkOneFileName = tempFileName;

			// resume-proof :-)
			if(!FileUtils::FileExists(chunkOneFileName)) {
				newChunkId++;
				continue;
			}
			chunkOneIsc = ItemSetCollection::OpenItemSetCollection(tempFileName, inPath, db, false, chunkType);
			chunkOneFile = chunkOneIsc->GetIscFile();
			chunkOneLeft = chunkOneIsc->GetNumItemSets();
			chunkOneNum = chunkOneIsc->GetNumItemSets();

			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.isc-tmp", inFullPath.c_str(), i+1);
			string chunkTwoFileName = tempFileName;
			chunkTwoIsc = ItemSetCollection::OpenItemSetCollection(tempFileName, inPath, db, false, chunkType);
			chunkTwoFile = chunkTwoIsc->GetIscFile();
			chunkTwoLeft = chunkTwoIsc->GetNumItemSets();
			chunkTwoNum = chunkTwoIsc->GetNumItemSets();

			uint64 memUseOneItemSet = (uint64) ((double)1.5 * sizeof(ItemSet*)) +  (uint64) ItemSet::MemUsage(db);
			uint64 memMaxNumLoaded = (maxMemUse - memUsePre) / memUseOneItemSet;
			uint32 memBufferSize = (uint32) ceil((double) memMaxNumLoaded / 2);

			chunkOneIsc->SetBufferSize(memBufferSize);
			chunkTwoIsc->SetBufferSize(memBufferSize);

			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-d%d.isc-tmp", outFullPath.c_str(), newChunkId);
			string combiChunkTempFileName = tempFileName;

			uint64 numSetsToBeMerged = chunkOneLeft + chunkTwoLeft;


			// ISC Setting Parameters
			IscFile *iscf = IscFile::Create(chunkType);
			iscf->OpenFile(combiChunkTempFileName, FileWritable);
			chunkOneIsc->SetMaxSetLength(max(chunkOneIsc->GetMaxSetLength(),chunkTwoIsc->GetMaxSetLength()));
			iscf->WriteHeader(chunkOneIsc, numSetsToBeMerged);

			CompFunc gt = ItemSetCollection::GetCompareFunction(chunkOneIsc->GetIscOrder());
			ItemSet *chunkOneIS = NULL, *chunkTwoIS = NULL;
			uint64 numSetsMerged, numWritten = 0, numRead = 0;
			for(numSetsMerged=0; numSetsMerged<numSetsToBeMerged; numSetsMerged++) {
				if((uint32)floor(100*(double) numWritten/numSetsToBeMerged) > percent) {
					percent = (uint32)floor(100*(double) numWritten/numSetsToBeMerged);
					printf(" * Merging:\t\t%d chunks into %d (%d%%) \t\t\r", numChunks, (numChunks+1)/2, percent);
				}
				if(chunkOneIS == NULL) {
					if(chunkOneLeft > 0) {
						try {
							//chunkOneIS = chunkOneFile->ReadNextItemSet();
							chunkOneIS = chunkOneIsc->GetNextItemSet();
							numRead++;
						} catch(...) {
							printf(" ! MergeChunks -- Error reading the %dth itemset from chunk %s", chunkOneNum-chunkOneLeft, chunkOneFileName.c_str());
						}
						chunkOneLeft--;
					} else 
						break;
				}
				if(chunkTwoIS == NULL) {
					if(chunkTwoLeft > 0) {
						try {
							chunkTwoIS = chunkTwoIsc->GetNextItemSet();
							numRead++;
						} catch(...) {
							printf(" ! MergeChunks -- Error reading the %dth itemset from chunk %s", chunkTwoNum-chunkTwoLeft, chunkTwoFileName.c_str());
						}
						chunkTwoLeft--;
					} else 
						break;
				}
				if(gt(chunkOneIS,chunkTwoIS)) {
					iscf->WriteItemSet(chunkOneIS);
					numWritten++;
					delete chunkOneIS;
					chunkOneIS = NULL;
				} else {
					iscf->WriteItemSet(chunkTwoIS);
					numWritten++;
					delete chunkTwoIS;
					chunkTwoIS = NULL;
				}
			}
			if(chunkOneLeft > 0 || chunkOneIS != NULL) {
				iscf->WriteItemSet(chunkOneIS);
				numWritten++;
				delete chunkOneIS;
				chunkOneIS = NULL;
				for(uint64 j=0; j<chunkOneLeft; j++) {
					if((uint32)floor(100 * (double) numWritten/numSetsToBeMerged) > percent) {
						percent = (uint32)floor(100 *(double) numWritten/numSetsToBeMerged);
						printf(" * Merging:\t\t %d chunks into %d (%d%%)\t\t\t\r", numChunks, (numChunks+1)/2, percent);
					}
					try {
						chunkOneIS = chunkOneIsc->GetNextItemSet();
						numRead++;
					} catch(...) {
						printf(" ! MergeChunks -- Opvullen, Error reading the %dth itemset from chunk %d", chunkOneNum-chunkOneLeft, i);
					}
					iscf->WriteItemSet(chunkOneIS);
					numWritten++;
					delete chunkOneIS;
					chunkOneIS = NULL;
				}
			} else if(chunkTwoLeft > 0 || chunkTwoIS != NULL) {
				iscf->WriteItemSet(chunkTwoIS);
				numWritten++;
				delete chunkTwoIS;
				chunkTwoIS = NULL;
				for(uint64 j=0; j<chunkTwoLeft; j++) {
					if((uint32)floor(100 *(double) numWritten/numSetsToBeMerged) > percent) {
						percent = (uint32)floor(100 * (double) numWritten/numSetsToBeMerged);
						printf(" * Merging:\t\t%d chunks into %d (%d%%)\t\t\t\r", numChunks, (numChunks+1)/2, percent);
					}
					try {
						chunkTwoIS = chunkTwoIsc->GetNextItemSet();
						numRead++;
					} catch(...) {
						printf(" ! MergeChunks -- Opvullen, Error reading the %dth itemset from chunk %d", chunkTwoNum-chunkTwoLeft, i+1);
					}
					iscf->WriteItemSet(chunkTwoIS);
					numWritten++;
					delete chunkTwoIS;
					chunkTwoIS = NULL;
				}
			}

			delete chunkOneIsc;
			delete chunkTwoIsc;
			delete iscf;

			newChunkId++;

			printf(" * Merging:\t\t%d chunks merged into %d.\t\t\t\r", numChunks, (numChunks+1)/2, percent);
			// zap orig chunks
			FileUtils::RemoveFile(chunkOneFileName);
			FileUtils::RemoveFile(chunkTwoFileName);
		}
		if(oddNumChunks) {
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.isc-tmp", inFullPath.c_str(), numChunks-1);
			string tempFromFileName = tempFileName;
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-d%d.isc-tmp", outFullPath.c_str(), newChunkId);
			string tempToFileName = tempFileName;

			FileUtils::FileMove(tempFromFileName, tempToFileName);
			newChunkId++;
		}

		// rename chunks
		for(uint32 i=0; i<newChunkId; i++) {
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-d%d.isc-tmp", inFullPath.c_str(), i);
			string combiChunkTempFileName = tempFileName;
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.isc-tmp", outFullPath.c_str(), i);
			string combiChunkRealFileName = tempFileName;
			FileUtils::FileMove(combiChunkTempFileName, combiChunkRealFileName);
		}

		// rinse and repeat
		MergeChunks(outPath, outPath, iscFileName, newChunkId, outType, db, dataType, chunkType);

	} else {
		uint32 percent = 0;
		printf(" * Merging:\t\t%d chunks into one FicIscFile (%d%%)\t\t\r", numChunks, percent);

		ItemSetCollection **chunkISCs = new ItemSetCollection*[numChunks];
		IscFile **chunkFiles = new IscFile*[numChunks];
		ItemSet **chunkSets = new ItemSet*[numChunks];
		uint64 *chunkLeft = new uint64[numChunks];

		uint64 totalSets = 0; //iscOrig->GetNumItemSets();
		uint32 maxSetLen = 0;

		uint64 memUseOneItemSet = (uint64) ((double)1.5 * sizeof(ItemSet*)) + (uint64) ItemSet::MemUsage(db);
		uint64 memMaxNumLoaded = (maxMemUse - memUsePre) / memUseOneItemSet;
		uint32 memBufferSize = (uint32) ceil((double) memMaxNumLoaded / (numChunks+1));

		ItemSet **wBuffer = new ItemSet*[memBufferSize];
		uint32 wBufferLen = memBufferSize;
		uint32 wBufferCur = 0;

		for(uint32 i=0; i<numChunks; i++) {
			sprintf_s(tempFileName, tempFileNameMaxLength, "%s-c%d.isc-tmp", iscFileName.c_str(), i);
			chunkISCs[i] = ItemSetCollection::OpenItemSetCollection(tempFileName, inPath, db, false, chunkType);
			chunkISCs[i]->SetBufferSize(memBufferSize);
			chunkFiles[i] = chunkISCs[i]->GetIscFile(); 
			chunkLeft[i] = chunkISCs[i]->GetNumItemSets();
			chunkSets[i] = NULL;
			totalSets += chunkLeft[i];
			if(chunkISCs[i]->GetMaxSetLength() > maxSetLen)
				maxSetLen = chunkISCs[i]->GetMaxSetLength();
		}

		// ISC Setting Parameters
		IscFile *fileOut = IscFile::Create(outType);
		fileOut->OpenFile(outFullPath, FileWritable);
		chunkISCs[0]->SetMaxSetLength(maxSetLen);
		fileOut->WriteHeader(chunkISCs[0], totalSets);

		CompFunc gt = ItemSetCollection::GetCompareFunction(chunkISCs[0]->GetIscOrder());

		uint64 i=0;
		uint32 numNonDepletedChunks = numChunks;
		uint64 numMerged = 0;
		for(numMerged=0; numMerged<totalSets; numMerged++) {
			if((uint32)floor(100 * (double) numMerged/totalSets) > percent) {
				percent = (uint32)floor(100 * (double) numMerged/totalSets);
				printf(" * Merging:\t\t%d chunks into one FicIscFile (%d%%)\t\t\r", numChunks, percent);
			}
			// eerst even boekhouding doen
			for(uint32 j=0; j<numNonDepletedChunks; j++) {
				if(chunkSets[j] == NULL) {
					if(chunkLeft[j] > 0) {
						chunkSets[j] = chunkISCs[j]->GetNextItemSet();
						chunkLeft[j]--;
					} else {
						// onbruikbare chunk, moven die handel
						ItemSetCollection *tempISC = chunkISCs[j];
						IscFile *tempISF = chunkFiles[j];
						for(uint32 k=j+1; k<numNonDepletedChunks; k++) {
							chunkISCs[k-1] = chunkISCs[k];
							chunkFiles[k-1] = chunkFiles[k];
							chunkLeft[k-1] = chunkLeft[k];
							chunkSets[k-1] = chunkSets[k];
						}
						numNonDepletedChunks--;
						chunkISCs[numNonDepletedChunks] = tempISC;
						chunkFiles[numNonDepletedChunks] = tempISF;
						string depletedChunkFilename = chunkFiles[numNonDepletedChunks]->GetFileName();
						chunkFiles[numNonDepletedChunks]->CloseFile();
						if(!FileUtils::RemoveFile(depletedChunkFilename))
							THROW("Unable to clean up temporary chunk file.");
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
			// old, non buffered
			//fileOut->WriteItemSet(chunkSets[maxChunkIdx]);
			//delete chunkSets[maxChunkIdx];
			//chunkSets[maxChunkIdx] = NULL;

			// new, buffered
			//WriteBufferAdd(chunkSets[maxChunkIdx]);
			//chunkSets[maxChunkIdx] = NULL;

			// new, wBuffered
			WriteBufferAdd(chunkSets[maxChunkIdx], wBufferCur, wBuffer, wBufferLen, fileOut);
			chunkSets[maxChunkIdx] = NULL;
		}

		if(numNonDepletedChunks == 1) {
			// die ene chunk vinden, en wegschrijven die handel
			// old, unbuffered
			//fileOut->WriteItemSet(chunkSets[0]);
			//delete chunkSets[0];

			// new, buffered
			WriteBufferAdd(chunkSets[0], wBufferCur, wBuffer, wBufferLen, fileOut);

			while(chunkLeft[0] > 0) {
				numMerged++;
				if((uint32)floor(100 * (double) numMerged/totalSets) > percent) {
					percent = (uint32)floor(100 * (double) numMerged/totalSets);
					printf(" * Merging:\t\t%d chunks into one FicIscFile (%d%%)\t\t\r", numChunks, percent);
				}
				chunkSets[0] = chunkISCs[0]->GetNextItemSet();
				chunkLeft[0]--;
				// old, unbuffered
				//fileOut->WriteItemSet(chunkSets[0]);
				//delete chunkSets[0];

				// new, buffered
				WriteBufferAdd(chunkSets[0], wBufferCur, wBuffer, wBufferLen, fileOut);
			}
		}

		WriteBufferFlush(wBuffer, wBufferCur, fileOut);
		fileOut->CloseFile();
		delete fileOut;

		// Clean up temp files
		for(uint32 i=0; i<numChunks; i++) {
			if(chunkFiles[i]->IsOpen()) {
				string chunkFilenameI = chunkFiles[i]->GetFileName();
				chunkFiles[i]->CloseFile();
				if(!FileUtils::RemoveFile(chunkFilenameI))
					THROW("Unable to clean up temporary chunk file.");
			}
			delete chunkISCs[i];
		}
		printf(" * Merging:\t\tMerged %d chunks into one FicIscFile.\t\t\t", numChunks);

		delete[] chunkISCs;
		delete[] chunkFiles;
		delete[] chunkSets;
		delete[] chunkLeft;
		delete[] wBuffer;
	}

	delete tempFileName;
}

void ItemSetCollection::WriteBufferAdd(ItemSet *is, uint32 &idx, ItemSet** buffer, uint32 len, IscFile* fOut) {
	if(idx+1 >= len)
		WriteBufferFlush(buffer, idx, fOut);
	buffer[idx++] = is;
}
void ItemSetCollection::WriteBufferFlush(ItemSet **buffer, uint32 &len, IscFile* fOut) {
	for(uint32 i=0; i<len; i++) {
		fOut->WriteItemSet(buffer[i]);
		delete buffer[i];
		buffer[i] = NULL;
	}
	len = 0;
}

//////////////////////////////////////////////////////////////////////////
// IscOrderType :: string<->type functions
//////////////////////////////////////////////////////////////////////////
IscOrderType ItemSetCollection::StringToIscOrderType(string order) {
	if(order.compare("d")==0)
		return 	LengthDescIscOrder;
	else if(order.compare("a")==0)
		return 	LengthAscIscOrder;
	else if(order.compare("al")==0)
		return 	AreaLengthIscOrder;
	else if(order.compare("as")==0)
		return 	AreaSupportIscOrder;
	else if(order.compare("aq")==0)
		return 	AreaSquareIscOrder;
	else if(order.compare("z")==0)
		return 	LengthAFreqDIscOrder;
	else if(order.compare("x")==0)
		return 	LengthDFreqDIscOrder;
	else if(order.compare("q")==0)
		return 	LengthAFreqAIscOrder;
	else if(order.compare("r")==0)
		return 	RandomIscOrder;
	else if(order.compare("u")==0)
		return 	UnknownIscOrder;
	else if(order.compare("j")==0)
		return 	JTestIscOrder;
	else if(order.compare("jr")==0)
		return 	JTestRevIscOrder;
	else
		return 	UnknownIscOrder;
	THROW("Unknown IscOrderType");
}

string ItemSetCollection::IscOrderTypeToString(IscOrderType type) {
	switch(type) {
		case LengthAscIscOrder:
			return "a";
		case LengthDescIscOrder:
			return "d";
		case AreaLengthIscOrder:
			return "al";
		case AreaSupportIscOrder:
			return "as";
		case AreaSquareIscOrder:
			return "aq";
		case RandomIscOrder:
			return "r";
		case LengthAFreqDIscOrder:
			return "z";
		case LengthDFreqDIscOrder:
			return "x";
		case JTestIscOrder:
			return "j";
		case JTestRevIscOrder:
			return "jr";
		case UnknownIscOrder:
			return "u";
		default:
			return "u";
	}
}

//////////////////////////////////////////////////////////////////////////
// ItemSetCollection Open/Read//Write	Retrieve/Store/CanRetrieve
//////////////////////////////////////////////////////////////////////////
// OpenItemSetCollection, the real thang.
//	iscName & iscPath separately, Database required
ItemSetCollection* ItemSetCollection::OpenItemSetCollection(const string &iscName, const string &iscPath, Database * const db, const bool loadAll /* = false */, const IscFileType iscType /* = NoneIscFileType */) {
	string inIscPath = iscPath;
	string inIscFileName = iscName;
	string inIscFileExtension = "";

	string inIscFullPath = ItemSetCollection::DeterminePathFileExt(inIscPath, inIscFileName, inIscFileExtension, iscType);

	// - Check whether designated file exists
	if(!FileUtils::FileExists(inIscFullPath))
		THROW("Item Set Collection file does not seem to exist: " + inIscFullPath);

	// - Determine type of file to load
	IscFileType inIscType = iscType == NoneIscFileType ? IscFile::ExtToType(inIscFileExtension) : iscType;
	if(inIscType == NoneIscFileType)
		THROW("Unknown itemset collection encoding.");

	// - Create appropriate DbFile to load that type of database
	IscFile *iscFile = IscFile::Create(inIscType);

	// - Determine internal datatype to read into
	iscFile->SetPreferredDataType(db->GetDataType());

	ItemSetCollection *isc = NULL;
	try {	// Memory-leak-protection-try/catch
		isc = loadAll ? iscFile->Read(inIscFullPath, db) : iscFile->Open(inIscFullPath, db);
	} catch(...) {	
		delete iscFile;
		throw;
	}

	if(!isc)
		THROW("Could not read input item set collection: " + inIscFullPath);
	return isc;
}

// OpenItemSetCollection for when there is no interaction with the DB, such that the ItemSetType can be chosen arbitrarily
//	iscName & iscPath separately, isType required
ItemSetCollection* ItemSetCollection::OpenItemSetCollection(const string &iscName, const string &iscPath, const ItemSetType isType, const bool loadAll /* = false */, const IscFileType iscType /* = NoneIscFileType */) {
	string inIscPath = iscPath;
	string inIscFileName = iscName;
	string inIscFileExtension = "";

	string inIscFullPath = ItemSetCollection::DeterminePathFileExt(inIscPath, inIscFileName, inIscFileExtension, iscType);

	// - Check whether designated file exists
	if(!FileUtils::FileExists(inIscFullPath))
		THROW("Item Set Collection file does not seem to exist: " + inIscFullPath);

	// - Determine type of file to load
	IscFileType inIscType = iscType == NoneIscFileType ? IscFile::ExtToType(inIscFileExtension) : iscType;
	if(inIscType == NoneIscFileType)
		THROW("Unknown itemset collection encoding.");

	// - Create appropriate DbFile to load that type of database
	IscFile *iscFile = IscFile::Create(inIscType);

	// - Determine internal datatype to read into
	ItemSetType inIscItemSetType = isType;
	iscFile->SetPreferredDataType(inIscItemSetType);

	ItemSetCollection *isc = NULL;
	try {	// Memory-leak-protection-try/catch
		isc = loadAll ? iscFile->Read(inIscFullPath) : iscFile->Open(inIscFullPath);
	} catch(...) {	
		delete iscFile;
		throw;
	}

	if(!isc)
		THROW("Could not read input item set collection: " + inIscFullPath);
	return isc;
}


void ItemSetCollection::WriteItemSetCollection(ItemSetCollection* const isc, const string &iscName, const string &path /* =  */, const IscFileType iscType /* = NoneIscFileType */) {
	// - Determine whether `iscName` does not abusively contains a path
	size_t iscNameLastDirSep = iscName.find_last_of("\\/");

	// - Determine `outIscPath`
	string outIscPath = (path.length() == 0 && iscNameLastDirSep != string::npos) ? iscName.substr(0,iscNameLastDirSep+1) : path;

	//  - Check whether is a valid (slash or backslash-ended) path
	if(outIscPath.length() != 0 && outIscPath[outIscPath.length()-1] != '/' && outIscPath[outIscPath.length()-1] != '\\')
		outIscPath += '/';

	// - Determine `outIscFileName` = iscName + . + extension
	string outIscFileName = (iscNameLastDirSep != string::npos) ? iscName.substr(iscNameLastDirSep+1) : iscName;
	string outIscFileExtension = "";

	// - Determine `outIscType` (preliminary defaulting, in case extension is provided via `iscName`)
	IscFileType outIscType = (iscType == NoneIscFileType) ? FicIscFileType : iscType;

	if(IscFile::StringToType(iscName) == NoneIscFileType) {
		//  - No extension provided, so extract from IscType
		outIscFileExtension = IscFile::TypeToExt(outIscType);
		outIscFileName += "." + outIscFileExtension;
	} else {
		//  - Extension provided, extract from name
		outIscFileExtension = iscName.substr(iscName.find_last_of('.')+1);
		//  - Redetermine defaulting `iscType` using extension
		outIscType = (iscType == NoneIscFileType && IscFile::ExtToType(outIscFileExtension) == NoneIscFileType) ? FicIscFileType : (iscType == NoneIscFileType ? IscFile::ExtToType(outIscFileExtension) : iscType);
	}

	// - Determine `outDbFullPath` = outDbPath + outDbFileName 
	string outIscFullPath = outIscPath + outIscFileName;

	// - Create dir if not exist
	if(outIscPath.length() > 0 && !FileUtils::DirectoryExists(outIscPath))
		if(!FileUtils::CreateDir(outIscPath))
			THROW("Error: Trouble creating directory for writing ItemSetCollection: " + outIscPath + "\n");

	// - Throws error in case unknown dbType.
	
	if(isc->GetNumLoadedItemSets() == 0) {
		if(isc->GetIscFile()->GetType() == outIscType) {
			IscFile *iscf = isc->GetIscFile();
			string iscfpath = iscf->GetFileName();
			iscf->CloseFile();
			FileUtils::FileMove(iscfpath, outIscFullPath);
			iscf->OpenFile(outIscFullPath);
			iscf->ReadFileHeader();
		} else {
			// file format conversion required
			IscFile *inIscFile = isc->GetIscFile();
			string inIscFilePath = inIscFile->GetFileName();

			IscFile *outIscFile = IscFile::Create(outIscType);
			outIscFile->SetDatabase(inIscFile->GetDatabase());
			outIscFile->SetPreferredDataType(inIscFile->GetPreferredDataType());
			outIscFile->WriteItemSetCollection(isc, outIscFullPath);
			outIscFile->CloseFile();

			inIscFile->CloseFile();
			FileUtils::RemoveFile(inIscFilePath);
			delete inIscFile;

			outIscFile->OpenFile(outIscFullPath);
			outIscFile->ReadFileHeader();
			isc->SetIscFile(outIscFile);
		}
	} else {
		IscFile *outIscFile = IscFile::Create(outIscType);
		if(!outIscFile->WriteLoadedItemSetCollection(isc, outIscFullPath)) {
			delete outIscFile;
			THROW("Error: Could not write itemset collection.");
		}
		if(isc->GetIscFile() == NULL)
			isc->SetIscFile(outIscFile);
		else
			delete outIscFile;
	}
}

ItemSetCollection* ItemSetCollection::RetrieveItemSetCollection(const string &iscName, Database * const db, bool load /* = false */, const IscFileType iscFileType /* = NoneIscFileType */) {
	if(iscName.find_last_of("\\/") != string::npos)
		THROW("Please provide the name of the itemset collection to retrieve, not a path or a filename");

	// - Determine the itemset collection filename
	string inIscFileName = "";
	string iscFileName = iscName;
	string biscFileName = iscName;
	if(iscFileType == NoneIscFileType && IscFile::StringToType(iscName) == NoneIscFileType) {
		iscFileName = iscName + ".isc";
		biscFileName = iscName + ".bisc";
	} else if(iscFileType == FicIscFileType || IscFile::StringToType(iscName) == FicIscFileType) {
		biscFileName = ""; //iscName.substr(0,-4);	// ".isc"
	} else if(iscFileType == BinaryFicIscFileType || IscFile::StringToType(iscName) == BinaryFicIscFileType) {
		iscFileName = ""; //iscName.substr(0,-5);		// ".bisc"
	}

	// - Determine the path to the requested itemset collection
	string inIscPath = "";
	if(FileUtils::FileExists(Bass::GetExecDir() + iscFileName)) {
		inIscPath = Bass::GetExecDir();
		inIscFileName = iscFileName;
	} else if(FileUtils::FileExists(Bass::GetExecDir() + biscFileName)) {
		inIscPath = Bass::GetExecDir();
		inIscFileName = biscFileName;
	} else if(FileUtils::FileExists(Bass::GetWorkingDir() + iscFileName)) {
		inIscPath = Bass::GetWorkingDir();
		inIscFileName = iscFileName;
	} else if(FileUtils::FileExists(Bass::GetWorkingDir() + biscFileName)) {
		inIscPath = Bass::GetWorkingDir();
		inIscFileName = biscFileName;
	} else if(FileUtils::FileExists(Bass::GetExpDir() + iscFileName)) {
		inIscPath = Bass::GetExpDir();
		inIscFileName = iscFileName;
	} else if(FileUtils::FileExists(Bass::GetExpDir() + biscFileName)) {
		inIscPath = Bass::GetExpDir();
		inIscFileName = biscFileName;
	} else if(FileUtils::FileExists(Bass::GetDataDir() + iscFileName)) {
		inIscPath = Bass::GetDataDir();
		inIscFileName = iscFileName;
	} else if(FileUtils::FileExists(Bass::GetDataDir() + biscFileName)) {
		inIscPath = Bass::GetDataDir();
		inIscFileName = biscFileName;
	} else if(FileUtils::FileExists(Bass::GetIscReposDir() + iscFileName)) {
		inIscPath = Bass::GetIscReposDir();
		inIscFileName = iscFileName;
	} else if(FileUtils::FileExists(Bass::GetIscReposDir() + biscFileName)) {
		inIscPath = Bass::GetIscReposDir();
		inIscFileName = biscFileName;
	} else
		THROW("Unable to retrieve " + iscName + " from repository directories.");

	ItemSetCollection *isc = ItemSetCollection::OpenItemSetCollection(inIscFileName, inIscPath, db, load);
	printf("** ItemSetCollection ::\n");
	string filename = isc->GetIscFile()->GetFileName();
	string path = "";
	FileUtils::SplitPathFromFilename(filename, path);
	printf(" * Retrieved:\t\t%s\n", filename.c_str());
	//printf(" * FileType\t\t:%s\n", IscFile::TypeToExt(isc->GetIscFile()->GetType()));
	printf(" * # Sets:\t\t%" I64d " (%s)\n", isc->GetNumItemSets(), load || isc->GetNumItemSets() == isc->GetNumLoadedItemSets() ? "resident" : "hd");
	printf("\n");
	return isc;
}

bool ItemSetCollection::CanRetrieveItemSetCollection(const string &iscName) {
	// - Determine the itemset collection filename
	string iscFileName = iscName;
	string biscFileName = iscName;
	
	if(IscFile::StringToType(iscName) == NoneIscFileType) {
		iscFileName = iscName + ".isc";
		biscFileName = iscName + ".bisc";
	} else if(IscFile::StringToType(iscName) == FicIscFileType) {
		biscFileName = iscName.substr(0,-4);	// ".isc"
	} else if(IscFile::StringToType(iscName) == BinaryFicIscFileType) {
		iscFileName = iscName.substr(0,-5);		// ".bisc"
	}

	if(FileUtils::FileExists(Bass::GetExecDir() + iscFileName) || FileUtils::FileExists(Bass::GetExecDir() + biscFileName))
		return true;
	else if(FileUtils::FileExists(Bass::GetWorkingDir() + iscFileName) || FileUtils::FileExists(Bass::GetWorkingDir() + biscFileName))
		return true;
	else if(FileUtils::FileExists(Bass::GetExpDir() + iscFileName) || FileUtils::FileExists(Bass::GetExpDir() + biscFileName))
		return true;
	else if(FileUtils::FileExists(Bass::GetDataDir() + iscFileName) || FileUtils::FileExists(Bass::GetDataDir() + biscFileName))
		return true;
	else if(FileUtils::FileExists(Bass::GetIscReposDir() + iscFileName) || FileUtils::FileExists(Bass::GetIscReposDir() + biscFileName))
		return true;
	return false;
}

void ItemSetCollection::StoreItemSetCollection(ItemSetCollection * const isc, const string &iscName /* = "" */, const IscFileType iscFileType /* = FicIscFileType */) {
	string outIscName = iscName.length() == 0 ? isc->GetTag() : iscName;
	if(outIscName.find("\\/.") != string::npos)
		THROW("Please only provide a name for the ItemSetCollection, not a full filename");
	ItemSetCollection::WriteItemSetCollection(isc, outIscName, Bass::GetIscReposDir(), iscFileType);
}

string ItemSetCollection::DeterminePathFileExt(string &path, string &filename, string &extension, const IscFileType iscFileType) {
	// - Split path from filename (if required, do nothing otherwise)
	FileUtils::SplitPathFromFilename(filename, path);

	//  - Check whether extension is provided
	if(IscFile::StringToType(filename) == NoneIscFileType) { // - No extension in filename
		if(extension.length() == 0) { // - No separate extension given
			// - Add right extension, .isc as default, 
			filename += "." + ((iscFileType != NoneIscFileType) ? IscFile::TypeToExt(iscFileType) : "isc");
			extension = (iscFileType != NoneIscFileType) ? IscFile::TypeToExt(iscFileType) : "isc";
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

#ifdef _DEBUG
void ItemSetCollection::Voorbeeldig() {
	string dbName = "iris";
	Database *db = Database::RetrieveDatabase(dbName);
	string iscFileName = "iris-all-1.fi";
	string iscPath = Bass::GetDataDir();

	// Considering the ItemSetCollection Iteratively
	ItemSetCollection *isc = ItemSetCollection::OpenItemSetCollection(iscFileName, iscPath, db);
	for(uint32 i=0; i<isc->GetNumItemSets(); i++) {
		ItemSet *is = isc->GetNextItemSet();	// Returns the next Item Set (throws error if unsuccesful)
		// ...
		delete is;								// You own the Item Set, You clean it up.
	}
	delete isc;

	// Considering the ItemSetCollection fully Loaded
	isc = ItemSetCollection::OpenItemSetCollection(iscFileName, iscPath, db, true);	// `true` indicates to load all of the ISC into memory
	for(uint32 i=0; i<isc->GetNumLoadedItemSets(); i++) {	// Could've used `GetNumItemSets()` as all ItemSets are loaded anyhow. This is cleaner.
		ItemSet *is = isc->GetLoadedItemSet(i);
		// ...
	}
	delete isc;	// isc deletes the loaded Item Sets

	// Considering the ItemSetCollection partially Loaded
	isc = ItemSetCollection::OpenItemSetCollection(iscFileName, iscPath, db);
	isc->LoadItemSets(100);
	for(uint32 i=0; i<isc->GetNumLoadedItemSets(); i++) {	// Do -not- use `GetNumItemSets()` here. Obviously.
		ItemSet *is = isc->GetLoadedItemSet(i);
		// ...
	}

	// Writing
	ItemSetCollection::WriteItemSetCollection(isc, isc->GetTag());
	ItemSetCollection::WriteItemSetCollection(isc, "dezenaam.isc");
	ItemSetCollection::WriteItemSetCollection(isc, Bass::GetDataDir() + "dezenaam");
	ItemSetCollection::WriteItemSetCollection(isc, "dezenaam", Bass::GetDataDir());
	ItemSetCollection::WriteItemSetCollection(isc, "dezenaam", Bass::GetDataDir(), FicIscFileType);

	// Storing into the Repository
	ItemSetCollection::StoreItemSetCollection(isc);	// determines the name from the tag, saves into the IscRepos
	ItemSetCollection::StoreItemSetCollection(isc, "dezenaam");	// saves it under "dezenaam.isc" saves into the IscRepos

	delete isc;	// isc deletes the loaded Item Sets

	delete db;

	// ParseTag
	{
		string iscTag = "iris-all-7d";
		string dbName, patternType, settings;
		IscOrderType orderType;
		ItemSetCollection::ParseTag(iscTag, dbName, patternType, settings, orderType);
		// dbName = iris, patternType = all, minSup = 7, orderType = LengthDescIscOrderType
	}
}
#endif
