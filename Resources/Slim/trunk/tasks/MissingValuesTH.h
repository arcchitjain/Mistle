#ifndef __MISSINGVALUESTASKHANDLER_H
#define __MISSINGVALUESTASKHANDLER_H

class ClassedDatabase;
//class CoverSet;
class ItemSet;

#include "TaskHandler.h"

enum EstimationTechnique {
//	EMVtMostUsedItemSet = 0,
//	EMVtMostUsedItem,
	EMVtMostFrequentItem = 0,
	EMVtShortestEncodedLength,
	EMVtKrimpCompletion,
	EMVtKrimpMinimisation,
	EMVtRandomChoice
};
enum DatabaseDamageTechnique {
	DBDtShotgun = 0,
	DBDtSnipe,
	DBDtGuts,
	DBDtDepShotgun,
	DBDtDepSnipe
};
enum DmgWhen {
	DmgLast = 0,
	DmgFirst,
};

class MissingValuesTH : public TaskHandler {
public:
	MissingValuesTH(Config *conf);
	virtual ~MissingValuesTH();

	virtual void		HandleTask();

protected:
	void				Miss();
//	void				Miss2();
	//void				DamageForNORM();
	//void				AccurFromNORM();

	void				DamageForNir();
	void				AccurFromNir();

	void				DamageForMatlab();
	void				AccurForMatlab();

	Database*			DamageDatabase(Database * const db, DatabaseDamageTechnique weapon, uint16 **misVals, uint16 *numMisVals);

	// n holes per database, pos >1 per transaction, dependent on class label, classlabel not missing
	Database*			DepShotgunDB(Database * const db, uint16 **misVals, uint16 *numMisVals);
	// n holes per database, pos >1 per transaction
	Database*			ShotgunDB(Database * const db, uint16 **misVals, uint16 *numMisVals);
	// n holes per transaction
	Database*			SnipeDB(Database * const db, uint16 **misVals, uint16 *numMisVals);
	// n hole per transaction, dependent on class label, classlabel not missing
	Database*			DepSnipeDB(Database * const db, uint16 **misVals, uint16 *numMisVals);
	// removes values of whole column
	Database*			GutsDB(Database * const db, uint16 **misVals, uint16 *numMisVals);

//	ClassedDatabase*	DamageDatabase2(Database * const db, DatabaseDamageTechnique weapon);
//	ClassedDatabase*	ShotgunDB2(Database * const db);
//	ClassedDatabase*	GutsDB2(Database * const db);

	EstimationTechnique StringToEMVt(string emvt);
	DatabaseDamageTechnique	StringToDBDt(string dbdt);

	double				CalculateEstimationAccuracy(Database *cdb, uint16 **estVals, uint16 **misVals, uint16 *numMisVals);
	uint32*				CalculateEpsDeltaAccuracy(Database *dmgdb, ItemSetCollection *origFIS, uint16 **estVals, uint16 **misVals, uint16 *numMisVals);

	void				ColumnBasedEstimation(Database *cdb, CodeTable *ct, EstimationTechnique method, uint16 **estVals);
	ItemSet**			DetermineMissingColumns(ItemSet *row, ItemSet** colDef, uint32 numCols, uint32 &numMisCols);
	//ItemSet**			DeterminePossibleCodeTableElements(ItemSet *row, ItemSet **missingCols, uint32 numMisCols, islist *ctElems, uint32 &numPosCTElems, uint32 abSize);

	ItemSet*			ChooseMostUsedItemSet(ItemSet **ctElems, uint32 numCTElems);
	uint16				DetermineMostUsedItem(uint16 *colItems, uint32 numColItems, ItemSet **ctElems, uint32 numCTElems);

	uint16**			DeterminePosEstimates(uint32 numMisCols, ItemSet** misCols, uint32 &numPosEstimates);
	void				RecursiveDeterminePosEstimates(uint32 curMisCol, uint32 &curPosEstimateIdx, uint16 **posEstimates, uint16* curChoices, uint32 numMisCols, ItemSet** misCols, uint16 **mcVals);

	FILE*				mEstOutFile;
};

#endif // __MISSINGVALUESTASKHANDLER_H
