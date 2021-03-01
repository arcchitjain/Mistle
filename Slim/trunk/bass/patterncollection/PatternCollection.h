#ifdef __PCDEV
#ifndef __PATTERNSETCOLLECTION_H
#define __PATTERNSETCOLLECTION_H

class Database;
class Pattern;
class PatternCollectionFile;

//typedef bool (*PatternCompFunc<type T>)(T* a,T* b);

class PatternCollection {
public:
	PatternCollection(Database *db = NULL);
	PatternCollection(PatternCollection *isc);
	virtual ~PatternCollection();

	virtual PatternCollection*	Clone();

	void				LoadPatterns(uint64 numPatterns);

	string				GetTag() const;

	// wat wilde ik hier mee?
	//static void			ParseTag(const string &tag, string &dbName, string &patternType, string &settings, string &patternOrder);

	/* Get */
	virtual string		GetDatabaseName() const	{ return mDatabaseName; }
	virtual void		SetDatabaseName(const string &dbName) { mDatabaseName = dbName; }

	virtual string		GetPatternTypeStr() const { return mPatternType; }
	virtual void		SetPatternTypeStr(const string &pType) { mPatternType = pType; }

	virtual string		GetPatternFilterStr() const { return mPatternFilter; }
	virtual void		SetPatternFilterStr(const string &pFilter) { mPatternFilter = pFilter; }

	virtual string		GetPatternOrder() const { return mPatternOrder; }
	virtual void		SetPatternOrder(const string orderStr) { mPatternOrder = orderStr; }


	// Number of Patterns in the collection (ie. the file)
	uint64				GetNumPatterns() const		{ return mNumPatterns; }
	void				SetNumPatterns(uint64 numPatterns) { mNumPatterns = numPatterns; }

	// Number of Patterns currently loaded
	uint32				GetNumLoadedPatterns() const	{ return mNumLoadedPatterns; }

	void				SetLoadedPatterns(Pattern **collection, uint64 colSize);
	Pattern*			GetLoadedPattern(uint32 i)	{ return mCollection[i]; }
	Pattern**			GetLoadedPatterns()			{ return mCollection; }

	// ~1 Only used during Create::...
//	PatColFile*			GetPatColFile()			{ return mPatColFile; }
//	void				SetPatColFile(PatColFile *patColFile) { mPatColFile = patColFile; }

	//////////////////////////////////////////////////////////////////////////
	// PatternCollection Open/Read/Write/Retrieve/Store
	//////////////////////////////////////////////////////////////////////////
	static PatternCollection* OpenPatternCollection(const string &pcName, const string &pcPath, Database * const db, const bool loadAll = false);
	static void			WritePatternCollection(PatternCollection* const pc, const string &pcName, const string &path = "");

	//static PatternCollection* RetrievePatternCollection(const string &pcTag, Database * const db, const bool loadAll = false);
	//static bool			CanRetrievePatternCollection(const string &pcName);
	//static void			StorePatternCollection(PatternCollection* const pc, const string &iscName = "");

	static void			ConvertPatternCollectionFile(const string &inName, const string &outName, Database * const db, string orderStr);

	static void			SaveChunk(PatternCollection* const pc, const string &outFullpath, const uint32 numChunk);
	static void			MergeChunks(const string &outFullpath, const uint32 numChunks, Database * const db);

	//////////////////////////////////////////////////////////////////////////
	// PatternCollection Order 
	//////////////////////////////////////////////////////////////////////////
	// Sorts the Loaded Patterns in the specified `order`
	virtual void		SortLoaded(string orderStr);
	// Randomizes the order of the Loaded Patterns
	//void				RandomizeOrder();


protected:
	void				ClearLoadedPatterns(const bool zapSets);

	string				mDatabaseName;		// the name of the database the pc was built on. Used for building tag.

	string				mPatternType;		// is, le, etc
	string				mPatternOrder;		// d,u,a etc
	string				mPatternFilter;		// all/cls/etc

	uint64				mNumPatterns;

	uint32				mNumLoadedPatterns;
	uint32				mCurLoadedPattern;

	Pattern				**mCollection;

	// may be NULL
	PatternCollectionFile *mPatternCollectionFile;
};

#endif // __PATTERNSETCOLLECTION_H
#endif // __PCDEV