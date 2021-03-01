//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#ifndef __CODETABLE_H
#define __CODETABLE_H

/*
	:: CoverPartial ::
		Optimized cover strategy: only cover the rows in which the candidate occurs.

		:: CoverPartial								(CCCPCodeTable)
			The meanest, smartest, partialisticest cover strategy, uses cached masks,
			and cache-conscious data structures.s
			Can operate on both transactional and binned databases (switches automatically)
			Parallel over candidates, post-prune and resume compatible

		:: CoverPartial								(CCPCodeTable)
			The previously meanest, smartest, partialisticest cover strategy, uses cached masks
			Can operate on both transactional and binned databases (switches automatically)
			Post-prune and resume compatible

		:: CoverPartial Basic						(CPCodeTable)
			The main, smart, partial cover strategy. 
			Can operate on both transactional and binned databases (switches automatically)
			Post-prune and resume compatible

		:: CoverPartial Orderly						(CPOCodeTable)
			For testing code table orders
			Fit for binned databases
			Post-prune and resume compatible

		:: CoverPartial Reorder Orderly				(CPROCodeTable)
			For testing code table orders, reordering the codetable per row to maximize cover
			Fit for binned databases
			Post-prune and resume compatible

		:: CoverPartial	Alternative					(CPACodeTable)
			Playground

		-- obsolete --

		:: CoverPartial Orderly Negative Alphabet	(CPONACodeTable)
			For testing code table orders with overlap and negative alphabet
			Post-prune and resume compatible

	:: CoverFull ::
		Naive cover strategy: try to cover each db row using each element c from CodeTable iteratively

		:: CoverFull					(CFCodeTable)
			The main, brute-force-like, full cover strategy
			Fit for binned databases
			Fully Prune and Resume Compatible


		:: CoverFull Alternative		(CFACodeTable)
			Playground for editing the CF algorithm

			-- obsolete --
		:: CoverFull Orderly			(CFOCodeTable)
			Doesnt seem to work well, error on CF-alphabet counts
			For testing code table orders
			Fully Prune and Resume Compatible

			
		:: CoverFull Minimal			(CFMCodeTable)
			Cover each row from DB up till the new code table element. 
			IF it has actually been used, also cover using the rest of the code table.
			Fully Prune and Resume Compatible

*/

class Database;
class ItemSet;
class ItemSetCollection;

#include <Bass.h>
#include <itemstructs/ItemSet.h>

enum CTInitType {
	InitCTEmpty=0,
	InitCTAlpha
};

enum CTOrder {
	CTRandomOrder,
	CTStackOrder,
	CTStackInverseOrder,
	CTLengthOrder,
	CTLengthInverseOrder,
	CTAreaOrder,
	CTAreaInverseOrder
};

enum ChangeType {
	ChangeNone,
	ChangeAdd,
	ChangeDel
};

struct CoverStats {
	uint64		usgCountSum;
	double		encDbSize;
	double		encCTSize;
	double		encSize;
	uint32		numSetsUsed;
	uint32		alphItemsUsed;
	uint64		numNests;			// only in CPA
	uint64		numCandidates;		// only up2date -after- a fresh Algo::DoeJeDing 
};

struct DelInfo {
	ItemSet *before;
	ItemSet *is;
};

typedef std::list<DelInfo> dellist;

class CodeTable {
public:
	CodeTable();
	CodeTable(const CodeTable &ct);
	virtual ~CodeTable();
	virtual CodeTable* Clone() = 0;

	virtual void		UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength=0, uint32 toMinSup = 0);
	// SetDatabase() should only be used when alphabet is exactly the same! (and even then: things like ABUsedLen may cause problems)
	virtual void		SetDatabase(Database *db);
	Database*			GetDatabase() { return mDB; }

	virtual string		GetShortName()=0;
	//virtual string		GetConfigName()=0;

	virtual bool		NeedsDBOccurrences() { return false; }
	virtual bool		NeedsCachedValues() { return false; }

	virtual void		Add(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE)=0;
	virtual void		Del(ItemSet *is, bool zap, bool keepList)=0;

	virtual void		AddAndCommit(ItemSet *is, uint64 candidateId = UINT64_MAX_VALUE) { THROW_NOT_IMPLEMENTED(); }
	virtual void		DelAndCommit(ItemSet *is, bool zap) { THROW_NOT_IMPLEMENTED(); }
	virtual void		UndoPrune(ItemSet *is) { THROW_NOT_IMPLEMENTED(); }

	virtual void		CommitAdd(bool updateLog=true) =0;
	virtual void		CommitLastDel(bool zap, bool updateLog=true) =0;
	virtual void		CommitAllDels(bool updateLog=true) =0;
	virtual void		RollbackAdd() =0;
	virtual void		RollbackLastDel() =0;
	virtual void		RollbackAllDels() =0;
	virtual void		RollbackCounts() =0;
	void				BackupStats();
	void				RollbackStats();

	// Provides a list of all ItemSet objects (of length >1) in the same order as this CodeTable
	virtual islist*		GetItemSetList() =0;
	// Provides an array of all ItemSet objects (of length >1) in the same order as this CodeTable
	//		(note: delete the array yourself, but not the itemsets in it!)
	virtual ItemSet**	GetItemSetArray();
	// Provides a list of Item Sets with the singletons in the code table (note: you'll have to delete these
	//		item sets yourself, contrary to the sets in the lists returned by the other methods!)
	virtual islist*		GetSingletonList() { THROW_NOT_IMPLEMENTED(); }
	virtual ItemSet**	GetSingletonArray();

	// Provides the full item set list of code table elements ordered (probably) on current use count
	virtual islist*		GetPrePruneList() =0;
	// Provides an item set list of code table elements to be considered for post pruning
	virtual islist*		GetPostPruneList() =0;
	// Provides the full item set list of code table elements with usage count of zero (0)
	virtual islist*		GetSanitizePruneList() =0;

	// During post pruning counts can change once more, here we find such elements
	virtual void		UpdatePostPruneList(islist *pruneList, const islist::iterator &after) =0;
	// During post pruning, the pruneList should be re-sorted
	virtual void		SortPostPruneList(islist *pruneList, const islist::iterator &after);

	/*	Covers the database, updating the coding elements' counts
		Exact cover strategy specified in derivates. 
		Method called iteratively for each candidate by Algo.		*/
	virtual void		CoverDB(CoverStats &stats) =0;
	/* Only cover the rows indicated in `dbMask` */
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows) =0;
	virtual void		CoverMaskedDB(CoverStats &stats, uint32 *rowList, uint32 numRows, bool addOneToCnt) {THROW_NOT_IMPLEMENTED();}

	// Gives code tables the opportunity to do something at the end of a Krimp run.
	virtual void		EndOfKrimp() {}

	/* Computes encoded DB size, CT size and total size based on current counts and countsum. */
	virtual void		CalcTotalSize(CoverStats &stats) { THROW_NOT_IMPLEMENTED(); }

	CoverStats&			GetPrevStats()			{ return mPrevStats; }
	CoverStats&			GetCurStats()			{ return mCurStats; }
	CoverStats&			GetPrunedStats()		{ return mPrunedStats; }

	double				GetPrevSize()			{ return mPrevStats.encSize; }
	double				GetCurSize()			{ return mCurStats.encSize; }
	double				GetPrunedSize()			{ return mPrunedStats.encSize; }

	uint32				GetCurNumSets()			{ return mCurNumSets; }

	virtual void		WriteToDisk(const string &filename);
	// Also check 'static CodeTable* LoadCodeTable()' below.
	virtual void		ReadFromDisk(const string &filename, const bool needFreqs);

	virtual void		AddOneToEachUsageCount() { THROW_NOT_IMPLEMENTED(); }
	virtual void		SetLaplace(uint32 laplace) { mLaplace = laplace; } // all this needs to be prettified when there is time
	virtual void		NormalizeUsageCounts(uint32 total) {}
	virtual void		AddSTSpecialCode() { THROW_DROP_SHIZZLE(); }

	/*	Calculates the encoded length of the specified `db` (assumes equal alphabet) */
	virtual double		CalcEncodedDbSize(Database *db) { THROW_NOT_IMPLEMENTED(); }
	/*	Calculates the encoded length of the specified `transaction` */
	virtual double		CalcTransactionCodeLength(ItemSet *transaction) =0;
	virtual double		CalcTransactionCodeLength(ItemSet *row, double *stLengths) { THROW_NOT_IMPLEMENTED(); }
	virtual double		CalcCodeTableSize() { THROW_NOT_IMPLEMENTED(); }

	virtual Database*	EncodeDB(const bool useSingletons, string outFile)  { THROW_NOT_IMPLEMENTED(); }
	virtual uint32		EncodeRow(ItemSet *row, double &length, uint16 *temp) { THROW_NOT_IMPLEMENTED(); }

	// Writing files
	virtual void		WriteStatsFile(Database *db, ItemSetCollection *isc, const string &outFile);
	virtual void		WriteComparisonFile(CodeTable *compareTo, const string &name1, const string &name2, const string &outFile);
	virtual void		WriteCodeFile(Database *encodedDb, const string &outFile) { THROW_NOT_IMPLEMENTED(); }
	virtual void		WriteCodeLengthsToFile(const string &outFile) { THROW_NOT_IMPLEMENTED(); }
	virtual void		WriteCountsToFile(const string &outFile) { THROW_NOT_IMPLEMENTED(); }
	virtual void		WriteCover(CoverStats &stats, const string &outFile) { THROW_NOT_IMPLEMENTED(); }

	// Write state-change to log
	virtual void		SetCTLogFile(FILE* ctLogFile);
	virtual void		UpdateCTLog();

	// Static
	static CodeTable*	Create(const string &name, ItemSetType dataType);
	// tijdelijk, zou eigenlijk gewoon Create moeten gebruiken, maar nu geen zin om alle algos om te bouwen...
	static CodeTable*	CreateCTForClassification(const string &name, ItemSetType dataType);

	static CodeTable*	LoadCodeTable(const string &filename, Database *db, string type = "coverfull");

	virtual void		SetAlphabetCount(uint32 item, uint32 count) =0;
	virtual uint32		GetAlphabetCount(uint32 item) =0;

	virtual uint32		GetUsageCount(ItemSet* is) { return is->GetUsageCount(); }

protected:
	Database*			mDB;
	double				*mStdLengths;
	uint32				mLaplace;
	bool				mSTSpecialCode;

	ItemSetType			mCoverSetType;

	CoverStats			mPrevStats;
	CoverStats			mCurStats;
	CoverStats			mPrunedStats;

	uint32				mCurNumSets;

	FILE*				mCTLogFile;		// borrowed from Algo
};

#endif // __CODETABLE_H
