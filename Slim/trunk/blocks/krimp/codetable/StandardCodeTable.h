#ifndef __STANDARDCODETABLE_H
#define __STANDARDCODETABLE_H

/*				CT(ST) -- The Standard Codetable				*/
// Note: not a Real CodeTable, as this one is based only on singleton frequencies.

class CodeTable;
class Database;
class ItemSet;

class StandardCodeTable {
public:
	// Create ST with 0-length codes
	StandardCodeTable(uint32 numCounts);

	// Induce ST from given counts
	StandardCodeTable(uint32 *counts, uint32 numCounts);

	// Induce ST from frequencies in given Database
	StandardCodeTable(Database *db);

	~StandardCodeTable();

	/* --- Manipulation --- */
	void AddLaplace();
	void RemoveLaplace();

	/* --- Calculation of (standard) lengths --- */
	double GetCodeLength(uint32 singleton);
	double GetCodeLength(ItemSet *is);
	double GetCodeLength(uint16 *set, uint32 numItems);

	//static? double GetStandardSize(Database *db);

	// Get the usual array
	double* GetStdLengths() { return mCodeLengths; }

protected:
	void	UpdateCodeLengths();

	double	*mCodeLengths;
	uint32	*mCounts;
	uint64	mCountSum;
	uint32	mNumCounts;

	uint32 mLaplace;
};

#endif // __STANDARDCODETABLE_H
