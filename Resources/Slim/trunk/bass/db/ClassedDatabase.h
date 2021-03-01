#ifndef __CLASSEDDATABASE_H
#define __CLASSEDDATABASE_H

#include "Database.h"

class BASS_API ClassedDatabase : public Database {
public:
	ClassedDatabase();
	ClassedDatabase(ItemSet **itemsets, uint32 numRows, bool binSizes, uint32 numTransactions = 0, uint64 numItems = 0);
	ClassedDatabase(Database *db, bool extractClassLabels = true);
	ClassedDatabase(ClassedDatabase *database);
	~ClassedDatabase();

	Database**			SplitOnClassLabels();
	Database**			SplitOnClassLabelsComplement();
	virtual Database**	SplitOnItemSet(ItemSet *is);

	virtual void		SwapRows(uint32 row1, uint32 row2);

	void				SetClassLabel(uint32 row, uint16 cl) { mClassLabels[row] = cl; }
	void				SetClassLabels(uint16 *classLabels);
	void				ExtractClassLabels();

	uint16*				GetClassLabels()			{ return mClassLabels; }
	uint16				GetClassLabel(uint32 row)	{ return mClassLabels[row]; }
	uint32*				GetClassSupports()			{ return mClassSupports; }

	virtual DatabaseType GetType() { return DatabaseClassed; }

protected:
	uint16*				mClassLabels;
	uint32*				mClassSupports;

};

#endif // __CLASSEDDATABASE_H
