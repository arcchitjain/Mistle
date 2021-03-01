#ifndef __DATABASE_H
#define __DATABASE_H

#include "../Bass.h"
#include "../itemstructs/ItemSet.h"
#include "DbFile.h"

class ItemTranslator;

enum DatabaseType {
	DatabasePlain,
	DatabaseClassed,
};

/*
	Rows are the physical, in memory, ItemSets. These can be grouped, and thus have counts >1. 
	Transactions are the virtual, ungrouped, ItemSets.
*/

class BASS_API Database {
public:
	Database();
	Database(Database *database);
	Database(ItemSet **itemsets, uint32 numRows, bool binSizes, uint32 numTransactions = 0, uint64 numItems = 0);
	virtual ~Database();
	virtual Database*	Clone();

	void				Write(const string &dbName, const string &path = "", const DbFileType dbType = DbFileTypeNone);

	void				CleanUpStaticMess();
	virtual DatabaseType GetType() { return DatabasePlain; }

	ItemSet*			GetRow(uint32 row)			{ return mRows[row]; }
	ItemSet**			GetRows()					{ return mRows; }
	ItemSet**			GetRows() const				{ return mRows; }

	void				SetRows(ItemSet **rows)		{ mRows = rows; }
	bool				IsItemSetInDatabase(ItemSet *is);

	uint32				GetNumRows() const			{ return mNumRows; }
	void				SetNumRows(uint32 nr)		{ mNumRows = nr; }
	uint32				GetNumTransactions() const	{ return mNumTransactions; }
	void				SetNumTransactions(uint32 nt){ mNumTransactions = nt; }

	ItemSetType 		GetDataType() const			{ return mDataType; }
	void				SetDataType(ItemSetType ist){ mDataType = ist; }

	string				GetName() const				{ return mDbName; }
	void				SetName(const string &name) { mDbName = name; }

	bool				HasBinSizes() const		{ return mHasBinSizes; }
	void				SetHasBinSizes(bool hbs)	{ mHasBinSizes = hbs; }

	bool				HasTransactionIds() const	{ return mHasTransactionIds; }
	void				SetHasTransactionIds(bool hti) { mHasTransactionIds = hti; }

	uint16				GetMaxSetLength() const		{ return mMaxSetLength; }
	void				SetMaxSetLength(uint32 msl)	{ mMaxSetLength = msl; }

	uint16*				GetAlphabetArr() const		{ return mAlphabetArr; }
	alphabet*			GetAlphabet() const		{ return mAlphabet; }
	void				SetAlphabet(alphabet *a)	{ mAlphabet = a; }
	void				UseAlphabetFrom(Database *db, bool originalStdLengths = false);
	size_t				GetAlphabetSize() const { return mAlphabet->size(); }
	void				SetAlphabetSize(const uint32 newSize); // watch out with this one!

	// Creates a caller-owned array of caller-owned ItemSets corresponding to the alphabet elements of the database
	ItemSet**			GetAlphabetSets() const;

	uint32*				GetAlphabetNumRows() const	{ return mAlphabetNumRows; }
	void				SetAlphabetNumRows(uint32 *abRo) { mAlphabetNumRows = abRo; }

	uint32**			GetAlphabetDbOccs() const	{ return mAlphabetDbOccs; }

	ItemTranslator*		GetItemTranslator()		{ return mItemTranslator; }
	void				SetItemTranslator(ItemTranslator *it) { mItemTranslator = it; }

	double*				GetStdLengths() const		{ return mStdLengths; }
	double				GetStdLength(uint32 item) const		{ return mStdLengths[item]; }//mg - get standard length of alphabet
	void				SetStdLengths(double *sl)	{ mStdLengths = sl; }

	bool				GetStdLaplace() const		{ return mStdLaplace; } // can only be set via ComputeStdLengths()

	double				GetStdDbSize() const		{ return mStdDbSize; }
	void				SetStdDbSize(double dbs)	{ mStdDbSize = dbs; }
	double				GetStdCtSize() const		{ return mStdCtSize; }
	void				SetStdCtSize(double cts)	{ mStdCtSize = cts; }
	double				GetStdSize() const			{ return mStdDbSize + mStdCtSize; }

	uint64				GetNumItems() const			{ return mNumItems; }
	void				SetNumItems(uint64 ni)		{ mNumItems = ni; }

	//////////////////////////////////////////////////////////////////////////
	// Path/Filename functions
	//////////////////////////////////////////////////////////////////////////
	bool				HasFilename() const			{ return mFilename.compare("") != 0; }
	string				GetFilename() const			{ return mFilename; }
	bool				HasPath() const				{ return mPath.compare("") != 0; }
	string				GetPath() const				{ return mPath; }
	bool				HasFullPath() const			{ return HasPath() && HasFilename(); }
	string				GetFullPath() const			{ return mPath + mFilename; }

	//////////////////////////////////////////////////////////////////////////
	// DB Analysis/Statistical/Ladiblabla Operations
	//////////////////////////////////////////////////////////////////////////

	// calculates support and numOccs of given item set and returns this
	uint32				GetSupport(ItemSet *is, uint32 &numOccs); 
	// calculates support of given item sets and sets this
	void				CalcSupport(ItemSet *is);

	// calculates standard length of given item set and sets this
	void				CalcStdLength(ItemSet *is);

	// Calculates and sets `is->occurences', and calculates the number of occurences (and support) if requested
	void				DetermineOccurrences(ItemSet* is, bool calcNumOcc=true); 
	
	//	Creates extra data structures that enable faster determining of tid-lists
	void				EnableFastDBOccurences();	

	bool				ComputeEssentials();
	void				CountAlphabet(bool countAlphabetNumRows = false);
	bool				ComputeStdLengths(const bool laplace = false);

	//////////////////////////////////////////////////////////////////////////
	// DB Transformation Operations
	//////////////////////////////////////////////////////////////////////////
	void				Transform(string commandlist);
	Database*			Transpose();
	void				Bin();
	void				UnBin();
	void				RandomizeRowOrder(uint32 seed = 0);
	void				MultiplyCounts(uint32 multiplier);
	void				SortWithinRows();
	void				RemoveDuplicateTransactions();
	void				RemoveDoubleItems(); // uint16, only after sorting within rows!
	void				RemoveTransactionsLongerThan(uint32 maxLen);
	void				RemoveFrequentItems(float frequencyThreshold);
	void				RemoveItems(ItemSet *zapItems);
	void				RemoveItems(uint16 *zapItems, uint32 numItems);

	//////////////////////////////////////////////////////////////////////////
	// Split DB operations
	//////////////////////////////////////////////////////////////////////////
	virtual Database**	Split(const uint32 numParts);
	virtual Database**	SplitOnItemSet(ItemSet *is);
	ItemSet***			SplitForCrossValidation(const uint32 numFolds, uint32 *partitionSizes, bool randomizeRowOrder = false);
	Database**			SplitForCrossValidationIntoDBs(const uint32 numFolds, bool randomizeRowOrder = false);
	void				SplitForClassification(const uint32 numTargets, const uint32 *targets, Database **trainingDbs);

	static Database*	Merge(Database *db1, Database *db2);
	static Database*	Merge(Database **dbs, uint32 numDbs);

	Database*			Subset(uint32 fromRow, uint32 toRow);

	//////////////////////////////////////////////////////////////////////////
	// Add/Remove row operations
	//////////////////////////////////////////////////////////////////////////
	void				RemoveFromDatabase(ItemSet *row);

	//////////////////////////////////////////////////////////////////////////
	// Column Definitions
	//////////////////////////////////////////////////////////////////////////
	bool				HasColumnDefinition() const { return mHasColumnDefinition; }
	ItemSet**			GetColumnDefinition() const { return mColumnDefinition; }
	uint32				GetNumColumns() const { return mNumColumns; }
	void				SetColumnDefinition(ItemSet** colDef, uint32 numColumns);

	//////////////////////////////////////////////////////////////////////////
	// Read/Write
	//////////////////////////////////////////////////////////////////////////
	static Database*	ReadDatabase (const string &dbName, const string &path = "", const ItemSetType dataType = NoneItemSetType, const DbFileType dbType = DbFileTypeNone);
	static string		WriteDatabase (Database* const db, const string &dbName, const string &path = "", const DbFileType dbType = DbFileTypeNone);

	static Database*	RetrieveDatabase(const string &dbName, const ItemSetType dataType = NoneItemSetType, const DbFileType dbType = DbFileTypeNone);
	static void			StoreDatabase(Database* const db, const string &dbName);

	// Returns full path (path + filename)
	static string		DeterminePathFileExt(string &path, string &filename, string &extension, const DbFileType dbType = DbFileTypeNone);

	void				WriteStdLengths(const string &filename);
	void				ReadStdLengths(const string &filename);

	//////////////////////////////////////////////////////////////////////////
	// Alphabet Translation
	//////////////////////////////////////////////////////////////////////////
	// Translate from original Fimi alphabet to sorted alphabet Fic stylee
	void				TranslateForward();
	// Translate from Fic alphabet back to original Fimi alphabet
	void				TranslateBackward();


	//////////////////////////////////////////////////////////////////////////
	// Classes
	//////////////////////////////////////////////////////////////////////////
	uint32				GetNumClasses()						{ return mNumClasses; }
	uint16*				GetClassDefinition()				{ return mClassDefinition; }
	void				SetClassDefinition(uint32 numClasses, uint16 *classes);
	bool				HasClassDefinition()				{ return mNumClasses != 0; }

	// ergens nodig.
	virtual void		SwapRows(uint32 row1, uint32 row2);
	// Swaps numSwaps (defaults to mNumItems, if set to 0) ones in the database, keeping row and column counts the same.
	virtual void		SwapRandomise(uint64 numSwaps = 0);

protected:
	//////////////////////////////////////////////////////////////////////////
	// Housekeeping
	//////////////////////////////////////////////////////////////////////////
	void				SetPath(string p) { mPath = p; }
	void				SetFilename(string f) { mFilename = f; }
	bool				DetermineAlphabet();
	void				DetermineMaxSetLength();
	ItemTranslator*		BuildItemTranslator();


	ItemSet		**mRows;
	ItemSetType	mDataType;
	uint32		mNumRows;			// -- Number of (physical) rows in database
	uint32		mNumTransactions;	// -- Number of transactions, ==mNumRows iff !mHasBinSizes, >=mNumRows otherwise.
	uint64		mNumItems;
	uint16		mMaxSetLength;
	bool		mHasBinSizes;
	bool		mHasTransactionIds;
	bool		mFastGenerateDBOccs;

	string		mDbName;			// de naam van de database. later ook nog een content-code.
	string		mFilename;			// filename of the database, without path.
	string		mPath;				// path of database, without filename.

	alphabet	*mAlphabet;
	size_t		mAlphabetSize;
	uint16		*mAlphabetArr;
	uint32		*mAlphabetNumRows;	// lengte mAlphabet, bevat numRows per ab elem
	uint32		**mAlphabetDbOccs;	// tid list per alphabet item, generated at EnableFastDBOccurences()

	ItemTranslator	*mItemTranslator;

	double		*mStdLengths;
	bool		mStdLaplace;		// Laplace applied to mStdLengths or not
	double		mStdDbSize;
	double		mStdCtSize;

	// Column Definition
	bool		mHasColumnDefinition;
	ItemSet		**mColumnDefinition;
	uint32		mNumColumns;

	// Class Labels
	uint32		mNumClasses;
	uint16*		mClassDefinition;

	// FimiDbFile is our homie, as it might need to access SortWithinRows stuff
	friend class FimiDbFile;
	// DbFile is also our homie, as it needs to access SetFilename() and SetPath()
	friend class FicDbFile;
};

#endif // __DATABASE_H
