#ifndef __LECODETABLE_H
#define __LECODETABLE_H

class Database;
class ItemSet;
class LowEntropySet;
class LowEntropySetCollection;
#include <itemstructs/ItemSet.h>
#include "LowEntropySet.h"
#include "LEAlgo.h"

enum LECTOrder {
	LECTRandomOrder,
	LECTStackOrder,
	LECTStackInverseOrder,
	LECTLengthOrder,
	LECTLengthInverseOrder,
	LECTAreaOrder,
	LECTAreaInverseOrder
};

enum LEChangeType {
	LEChangeNone,
	LEChangeAdd,
	LEChangeDel
};

struct LECoverStats {
	uint64		setCountSum;
	double		size;
	double		bmSize;
	double		dbSize;
	double		ct1Size;
	double		ct2Size;
	uint32		numSetsUsed;
	uint32		alphItemsUsed;
	double		likelihood;
};

struct LEDelInfo {
	LowEntropySet *before;
	LowEntropySet *les;
};

typedef std::list<LEDelInfo> ledellist;

class LECodeTable {
public:
	LECodeTable();
	virtual ~LECodeTable();

	virtual void		UseThisStuff(Database *db, LEBitmapStrategy bs, uint32 maxSetLen);
	// SetDatabase() should only be used when alphabet is exactly the same! (and even then: things like ABUsedLen may cause problems)
	virtual void		SetDatabase(Database *db);

	virtual string		GetShortName()=0;
	//virtual string		GetConfigName()=0;

	virtual void		Add(LowEntropySet *les, uint64 candidateId = UINT64_MAX_VALUE)=0;
	virtual void		Del(LowEntropySet *les, bool zap)=0;

	virtual void		BackupStats();
	virtual void		RollbackStats();

	virtual void		CommitAdd(bool updateLog=true) =0;
	virtual void		CommitLastDel(bool zap, bool updateLog=true) =0;
	virtual void		RollbackAdd() =0;
	virtual void		RollbackLastDel() =0;
	virtual void		RollbackCounts()=0;

	/*	Covers the database, updating the coding elements' counts
		Exact cover strategy specified in derivates. 
		Method called iteratively for each candidate by Algo.		*/
	virtual void		CoverDB(LECoverStats &stats) =0;

	/* Computes encoded DB size, CT size and total size based on current counts and countsum. */
	virtual double		CalcTotalSize(LECoverStats &stats) { throw string("CodeTable::CalcTotalSize not implemented!"); }

	LECoverStats&		GetPrevStats()			{ return mPrevStats; }
	LECoverStats&		GetCurStats()			{ return mCurStats; }
	LECoverStats&		GetPrunedStats()		{ return mPrunedStats; }

	double				GetPrevSize()			{ return mPrevStats.size; }
	double				GetCurSize()			{ return mCurStats.size; }
	double				GetPrunedSize()			{ return mPrunedStats.size; }

	// Provides an item set list of code table elements to be considered for post pruning
	virtual leslist*		GetPostPruneList() =0;
	// During post pruning counts can change once more, here we find such elements
	virtual void		UpdatePostPruneList(leslist *pruneList, const leslist::iterator &after) =0;

	uint32				GetCurNumSets()			{ return mCurNumSets; }

	virtual void		WriteToDisk(const string &filename);
	virtual void		WriteCoverToDisk(const string &filename)=0;

	virtual void		AddOneToEachCount() { throw string("CodeTable::AddOneToEachCount() not implemented!"); }

	/*	Calculates the encoded length of the specified `db` (assumes equal alphabet) */
	//virtual double		CalcEncodedDbSize(Database *db) { throw string("CodeTable::CalcEncodedDbSize not implemented!"); }
	/*	Calculates the encoded length of the specified `transaction` */
	//virtual double		CalcTransactionCodeLength(ItemSet *transaction) =0;
	//virtual double		CalcCodeTableSize() { throw string("CodeTable::CalcCodeTableSize not implemented!"); }

	// Static
	static LECodeTable*	Create(const string &name);

	virtual void		SetAlphabetCount(uint32 item, uint32 count) =0;
	virtual uint32		GetAlphabetCount(uint32 item) =0;

protected:

	Database*			mDB;
	uint32				mNumDBRows;

	ItemSetType			mCoverSetType;
	LEBitmapStrategy	mBitmapStrategy;

	LECoverStats		mPrevStats;
	LECoverStats		mCurStats;
	LECoverStats		mPrunedStats;

	uint32				mCurNumSets;
	uint32				mPrevNumSets;

	uint32				mMaxSetLen; 
};

#endif // __LECODETABLE_H
