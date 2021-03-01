#ifndef __LOWENTROPYSETCOLLECTION_H
#define __LOWENTROPYSETCOLLECTION_H

enum LescOrderType {
	LescOrderUnknown=0,					// 0	u
	LescOrderLengthAscEntropyAsc,		// 1	lAeA
	LescOrderLengthDescEntropyAsc,		// 2	lDeA
	LescOrderEntropyPerAttributeAsc,	//	eaA
	LescOrderEntropyPerAttributeDesc,	//	eaD
};

class Database;
class LowEntropySet;
class LescFile;

typedef bool (*LESCompFunc)(LowEntropySet* a,LowEntropySet* b);

class LowEntropySetCollection {
public:
	LowEntropySetCollection(Database *db = NULL, ItemSetType type = NoneItemSetType);
	LowEntropySetCollection(LowEntropySetCollection *isc);
	virtual ~LowEntropySetCollection();

	LowEntropySetCollection*	Clone();

	//////////////////////////////////////////////////////////////////////////
	// Loading-wise, ISC initatede via OpenItemSetCollection
	//////////////////////////////////////////////////////////////////////////
	/* Reads and returns next LowEntropySet from file */
	LowEntropySet*			GetNextPattern();

	/*	. Reads 'numSets' LowEntropySet from file into mCollection[0->n] 
		. mCollection should be ClearLoadedLowEntropySets(true) before use
		. Does not calculate minsup and all such
	*/
	void				LoadPatterns(uint64 numPatterns);

	/* Scans to candidate candi+1 */
	void				ScanPastCandidate(const uint64 candi);


	//////////////////////////////////////////////////////////////////////////
	// ISC Properties
	//////////////////////////////////////////////////////////////////////////
	string				GetTag() const;
	static void			ParseTag(const string &tag, string &dbName, string &patternType, string &settings, LescOrderType &orderType);

	/* Get */
	Database*			GetDatabase() const		{ return mDatabase; }

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

	void				SetMaxEntropy(double entropy) { mMaxEntropy = entropy; }
	double				GetMaxEntropy() const { return mMaxEntropy; }

	// Do the ItemSets in the Collection have separate number of rows/occurrences?
	bool				HasSepNumRows() const	{ return mHasSepNumRows; }
	void				SetHasSepNumRows(bool snr) { mHasSepNumRows = snr; }

	// Number of Item Sets in the collection (ie. the file)
	uint64				GetNumLowEntropySets() const		{ return mNumPatterns; }
	void				SetNumLowEntropySets(uint64 numLowEntropySets) { mNumPatterns = numLowEntropySets; }

	// Number of Item Sets currently loaded
	uint32				GetNumLoadedLowEntropySets() const	{ return mNumLoadedPatterns; }
	void				SetLoadedPatterns(LowEntropySet **collection, uint64 colSize);
	LowEntropySet*		GetLoadedLowEntropySet(uint32 i)	{ return mCollection[i]; }
	LowEntropySet**		GetLoadedLowEntropySets()			{ return mCollection; }

	// ~1 Only used during Create::...
	LescFile*			GetLescFile()			{ return mLescFile; }
	void				SetLescFile(LescFile *lescFile) { mLescFile = lescFile; }

	//static void		FilterEmerging(float emerging, Database *partialDb, Database *fullDb, const string &fileName);

	//////////////////////////////////////////////////////////////////////////
	// LESC Open/Read/Write/Retrieve/Store
	//////////////////////////////////////////////////////////////////////////
	static LowEntropySetCollection* OpenLowEntropySetCollection(const string &lescName, const string &lescPath, Database * const db, const bool loadAll = false, const ItemSetType dataType = NoneItemSetType);
	static void			WriteLowEntropySetCollection(LowEntropySetCollection* const lesc, const string &lescName, const string &path = "");

	//static LowEntropySetCollection* RetrieveLowEntropySetCollection(const string &iscTag, Database * const db, const bool loadAll = false, const ItemSetType dataType = NoneItemSetType);
	//static bool			CanRetrieveItemSetCollection(const string &iscName);
	//static void			StoreItemSetCollection(ItemSetCollection* const isc, const string &iscName = "");

	static void			ConvertLescFile(const string &inName, const string &outName, Database * const db, LescOrderType orderType, const ItemSetType dataType = NoneItemSetType);

	static void			SaveChunk(LowEntropySetCollection* const lesc, const string &outFullpath, const uint32 numChunk);
	static void			MergeChunks(const string &outFullpath, const uint32 numChunks, Database * const db, ItemSetType dataType);

	//////////////////////////////////////////////////////////////////////////
	// ItemSet Collection Order 
	//////////////////////////////////////////////////////////////////////////
	// Sorts the Loaded ItemSets in the specified `order`
	void				SortLoaded(LescOrderType order);
	// Randomizes the order of the Loaded ItemSets
	//void				RandomizeOrder();

	void				SetLescOrder(LescOrderType io) { mLescOrder = io; }
	LescOrderType		GetLescOrder() const { return mLescOrder; }

	static LescOrderType	StringToLescOrderType(string order);
	static string		LescOrderTypeToString(LescOrderType type);

protected:
	void				ClearLoadedPatterns(const bool zapSets);
	static string		DeterminePathFileExt(string &path, string &filename, string &extension);

	/* DB Occurances*/
	//void				DetermineDBOccurrence(ItemSet *is);

	/* Sorts the low entropy set collection according to the provided order type */
	void				MergeSort(LESCompFunc cf);
	void				MSortMergeSort(uint32 lo, uint32 hi, LESCompFunc cf, LowEntropySet ** auxArr);
	void				MSortMerge(uint32 lo, uint32 m, uint32 hi, LESCompFunc cf, LowEntropySet ** auxArr);

	/* Comparators */
	static LESCompFunc	GetCompareFunction(LescOrderType order);
	static bool			CompareLengthAscEntropyDesc(LowEntropySet *a, LowEntropySet *b);
	static bool			CompareLengthDescEntropyDesc(LowEntropySet *a, LowEntropySet *b);
	static bool			CompareEntropyPerAttributeAsc(LowEntropySet *a, LowEntropySet *b);
	static bool			CompareEntropyPerAttributeDesc(LowEntropySet *a, LowEntropySet *b);

	Database		*mDatabase;			// the database we'll let the isc loose on

	string			mDatabaseName;		// the name of the database the isc was built on. Used for building tag.
	string			mPatternType;		// 'all' or 'cls' or 'huh' etc used for building tag

	uint64			mNumPatterns;

	LowEntropySet	**mCollection;
	uint32			mNumLoadedPatterns;
	uint32			mCurLoadedPattern;

	double			mMaxEntropy;

	bool			mHasSepNumRows;

	bool			mHasMaxSetLength;
	uint32			mMaxSetLength;

	ItemSetType		mDataType;

	LescFile		*mLescFile;
	LescOrderType	mLescOrder;

	size_t			mAbSize;
};

#endif // __LOWENTROPYSETCOLLECTION_H
