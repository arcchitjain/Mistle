#ifndef __ITEMSETCOLLECTION_H
#define __ITEMSETCOLLECTION_H

#include "../BassApi.h"
#include "../itemstructs/ItemSet.h"
#include "IscFile.h"

// For some reason GCC fails to compile if IscOrderType is not defined in itemstructs/ItemSet.h
enum IscOrderType;

class Database;
class ItemSet;
class IscFile;

enum IscFileType;

typedef bool (*CompFunc)(ItemSet* a,ItemSet *b);

class BASS_API ItemSetCollection {
public:
	ItemSetCollection(Database *db = NULL, ItemSetType ist = NoneItemSetType);
//	ItemSetCollection(Database *db, const string &filename, ItemSetType ist = NoneItemSetType);
	ItemSetCollection(ItemSetCollection *isc);
	virtual ~ItemSetCollection();

	ItemSetCollection*	Clone();

	//////////////////////////////////////////////////////////////////////////
	// Loading-wise, ISC initatede via OpenItemSetCollection
	//////////////////////////////////////////////////////////////////////////
	/* Reads and returns next ItemSet from file */
	ItemSet*			GetNextItemSet();

	/*	. Reads 'numSets' ItemSets from file into mCollection[0->n] 
		. mCollection should be ClearLoadedItemSets(true) before use
		. Does not calculate minsup and all such
	*/
	void				LoadItemSets(uint64 numSets);

	/* Scans to an ItemSet (if any) with a lower support than provided */
	uint32				ScanPastSupport(const uint32 support);
	/* Scans to candidate candi+1 */
	void				ScanPastCandidate(const uint64 candi);

	void				Rewind();

	//////////////////////////////////////////////////////////////////////////
	// ISC Properties
	//////////////////////////////////////////////////////////////////////////
	string				GetTag() const;
	static void			ParseTag(const string &tag, string &dbName, string &patternType, string &settings, IscOrderType &orderType);

	/* Get */
	//Database*			GetDatabase() const		{ return mDatabase; }
	size_t				GetAlphabetSize() const;

	string				GetDatabaseName() const	{ return mDatabaseName; }
	void				SetDatabaseName(const string &dbName) { mDatabaseName = dbName; }

	string				GetPatternType() const { return mPatternType; }
	void				SetPatternType(const string &pType) { mPatternType = pType; }

	uint32				GetMaxSetLength() const	{ return mMaxSetLength; }
	bool				HasMaxSetLength() const	{ return mHasMaxSetLength; }
	// Als maxSetLen UINT32MAX dan wordt mHasMaxSetLength false.
	void				SetMaxSetLength(uint32 maxSetLen);
	void				SetHasMaxSetLength(const bool hasMaxSetLength) { mHasMaxSetLength = hasMaxSetLength; }
	void				DetermineMaxSetLength();

	ItemSetType			GetDataType() const		{ return mDataType; }
	void				SetDataType(const ItemSetType ist) { mDataType = ist; }

	// Minimal support of sets in collection
	bool				HasMinSupport() const	{ return mHasMinSup; }
	uint32				GetMinSupport()	const	{ return mMinSup; }
	void				SetMinSupport(uint32 ms);
	void				DetermineMinSupport();

	// Do the ItemSets in the Collection have separate number of rows/occurrences?
	bool				HasSepNumRows() const	{ return mHasSepNumRows; }
	void				SetHasSepNumRows(bool snr) { mHasSepNumRows = snr; }

	// Number of Item Sets in the collection (ie. the file)
	uint64				GetNumItemSets() const		{ return mNumItemSets; }
	void				SetNumItemSets(uint64 numItemSets) { mNumItemSets = numItemSets; }

	// Number of Item Sets currently loaded
	uint32				GetNumLoadedItemSets() const	{ return mNumLoadedItemSets; }
	void				SetLoadedItemSets(ItemSet **collection, uint64 colSize);
	ItemSet*			GetLoadedItemSet(uint32 i)	{ return mLoadedItemSets[i]; }
	ItemSet**			GetLoadedItemSets()			{ return mLoadedItemSets; }

	// Buffersize currently only used/tested through MergeChunks.
	uint32				GetBufferSize() const { return mLoadedBufferSize; }
	void				SetBufferSize(uint32 bS) { mLoadedBufferSize = bS; }

	// ~1 Only used during Create::...
	IscFile*			GetIscFile()			{ return mIscFile; }
	void				SetIscFile(IscFile *iscFile) { mIscFile = iscFile; }

	//static void		FilterEmerging(float emerging, Database *partialDb, Database *fullDb, const string &fileName);

	//////////////////////////////////////////////////////////////////////////
	// ISC Add/Remove
	//////////////////////////////////////////////////////////////////////////

	// Removes the elements of `arr` from collection
	// - assumes all of collection is in mLoadedItemSets
	// - assumes both this and `arr` have the same IscOrder
	// - deletes elements in mLoadedItemSets, resizes
	// - leaves arr alone
	void				RemoveOrderedSubset(ItemSet** arr, uint32 num);


	//////////////////////////////////////////////////////////////////////////
	// ISC Open/Read/Write/Retrieve/Store
	//////////////////////////////////////////////////////////////////////////
	// OpenItemSetCollection, the real thang.
	static ItemSetCollection* OpenItemSetCollection(const string &iscName, const string &iscPath, Database * const db, const bool loadAll = false, const IscFileType iscType = NoneIscFileType);
	// OpenItemSetCollection for when there is no interaction with the DB, such that the ItemSetType can be chosen arbitrarily
	static ItemSetCollection* OpenItemSetCollection(const string &iscName, const string &iscPath, const ItemSetType isType, const bool loadAll = false, const IscFileType iscType = NoneIscFileType);

	static void			WriteItemSetCollection(ItemSetCollection* const isc, const string &iscName, const string &path = "", const IscFileType iscFileType = NoneIscFileType);

	static ItemSetCollection* RetrieveItemSetCollection(const string &iscTag, Database * const db, const bool loadAll = false, const IscFileType iscFileType = NoneIscFileType);
	static bool			CanRetrieveItemSetCollection(const string &iscName);
	static void			StoreItemSetCollection(ItemSetCollection* const isc, const string &iscName = "", const IscFileType iscFileType = FicIscFileType);

	static void			ConvertIscFile(const string &inPath, const string &inIscFileName, const string &outPath, const string &outIscFileName, Database *db, IscFileType outType, IscOrderType orderType, bool translate = false, bool zapFromFile = false, uint32 minSup = 0, const ItemSetType dataType = NoneItemSetType); 

	static void			SaveChunk(ItemSetCollection* const isc, const string &outFullpath, const uint32 numChunk, IscFileType outType);
	static void			MergeChunks(const string &inPath, const string &outPath, const string &iscFileName, const uint32 numChunks, IscFileType outType, Database * const db, ItemSetType dataType, IscFileType chunkType);

	//////////////////////////////////////////////////////////////////////////
	// ItemSet Collection Order 
	//////////////////////////////////////////////////////////////////////////
	// Sorts the Loaded ItemSets in the specified `order`
	void				SortLoaded(IscOrderType order);

	// Randomizes the order of the Loaded ItemSets
	void				RandomizeOrder();

	void				SetIscOrder(IscOrderType io) { mIscOrder = io; }
	IscOrderType		GetIscOrder() const { return mIscOrder; }

	static IscOrderType	StringToIscOrderType(string order);
	static string		IscOrderTypeToString(IscOrderType type);

	static void			MergeSort(ItemSet **itemSetArray, uint32 numItemSets, CompFunc cf);
	static CompFunc		GetCompareFunction(IscOrderType order);

protected:

	void				ClearLoadedItemSets(const bool zapSets);

	static string		DeterminePathFileExt(string &path, string &filename, string &extension, const IscFileType iscFileType = NoneIscFileType);

	/* --- Manipulate --- */

	/* Normalizes the collection, necessary for conversion */
	//void				Normalize(bool translate, bool forceNormalize=true);

	// Normalization Helper Function - sorts items within sets
	void				SortWithinItemSets();

	/* Normalization Helper Function - translates item names back to original for all Loaded ItemSets */
	void				TranslateBackward(ItemTranslator * const it);
	/* Normalization Helper Function - translates item names back to frequency sorted [0-N] values for all Loaded ItemSets */
	void				TranslateForward(ItemTranslator * const it);

	/* Sorts the item sets collection according to the provided order type */
	void				BubbleSort(CompFunc cf);

	// Statisch
	static void			MSortMergeSort(ItemSet **itemSetArray, uint32 lo, uint32 hi, CompFunc cf, ItemSet ** auxArr);
	static void			MSortMerge(ItemSet **itemSetArray, uint32 lo, uint32 m, uint32 hi, CompFunc cf, ItemSet ** auxArr);
	static void			WriteBufferAdd(ItemSet *is, uint32 &idx, ItemSet** buffer, uint32 len, IscFile* fOut);
	static void			WriteBufferFlush(ItemSet **buffer, uint32 &len, IscFile* fOut);


	// Niet Statisch
	void				MergeSort(CompFunc cf);


	/* Comparators */
	static bool			CompareFreqDescLengthDesc(ItemSet *a, ItemSet *b);
	static bool			CompareFreqDescLengthAsc(ItemSet *a, ItemSet *b);
	static bool			CompareAreaDescLengthDesc(ItemSet *a, ItemSet *b);
	static bool			CompareAreaDescSupportDesc(ItemSet *a, ItemSet *b);
	static bool			CompareAreaDescSquareDesc(ItemSet *a, ItemSet *b);
	static bool			CompareLenAscFreqDesc(ItemSet *a, ItemSet *b);
	static bool			CompareLenDescFreqDesc(ItemSet *a, ItemSet *b);
	static bool			CompareLenAscFreqAsc(ItemSet *a, ItemSet *b);
	static bool			CompareStdSzDescLenDescFreqDesc(ItemSet *a, ItemSet *b);
	static bool			CompareStdSzAscLenDescFreqDesc(ItemSet *a, ItemSet *b);
	static bool			CompareLexAsc(ItemSet *a, ItemSet *b);
	static bool			CompareLexDesc(ItemSet *a, ItemSet *b);
	static bool			CompareIdAscLexAsc(ItemSet *a, ItemSet *b);
	static bool			CompareUsageDescLengthDesc(ItemSet *a, ItemSet *b);

	/* DB Occurances*/
	void				DetermineDBOccurrence(ItemSet *is);


	Database		*mDatabase;			// the database we'll let the isc loose on

	string			mDatabaseName;		// the name of the database the isc was built on. Used for building tag.
	string			mPatternType;		// 'all' or 'cls' or 'huh' etc used for building tag

	uint64			mNumItemSets;

	ItemSet			**mLoadedItemSets;
	uint32			mSizeLoadedItemSets;
	uint32			mNumLoadedItemSets;
	uint32			mCurLoadedItemSet;
	uint32			mLoadedBufferSize;

	bool			mHasMinSup;
	uint32			mMinSup;

	bool			mHasSepNumRows;

	bool			mHasMaxSetLength;
	uint32			mMaxSetLength;

	ItemSetType		mDataType;

	IscFile			*mIscFile;
	IscOrderType	mIscOrder;

	ItemSet			*mWriteBuffer;
	uint32			mWriteBufferLen;
	uint32			mWriteBufferCur;
	IscFile			*mWriteBufferFile;

private:
#ifdef _DEBUG
	void Voorbeeldig();
#endif
};

#endif // __ITEMSETCOLLECTION_H
