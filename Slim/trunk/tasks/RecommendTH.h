#ifndef __RECOMMENDATIONTASKHANDLER_H
#define __RECOMMENDATIONTASKHANDLER_H

class ClassedDatabase;
//class CoverSet;
class ItemSet;

#include "TaskHandler.h"

enum RecommendableDatabaseDamageTechnique {
	RDBDtShotgun = 0,
	RDBDtSnipe,
	RDBDtGuts,
	RDBDtDepShotgun,
	RDBDtDepSnipe,
	RDBDtNone,
};
enum RecommendableDmgWhen {
	RDmgLast = 0,
	RDmgFirst,
};
enum RecommendationPossibility {
	RsOneItem = 0,
	RsCTElemOverlap,
	RsCTSetOverlap,
	RsCTSet,
	RsCTElem,
};
enum RecommendationTechnique {
	RtShortestEncodedSize = 0,		// i + argmin_{r\inR}(CT(i+r))
	RtWeightedEncodedSize,			// encSz gewogen rnd keuze uit {R+0}
	RtRandomChoice,					// ongewogen rnd keuze uit {R+0}
	RtMostFrequentItem,				// kiest botweg abElem met hoogste count niet in r
};

class RecommendTH : public TaskHandler {
public:
	RecommendTH(Config *conf);
	virtual ~RecommendTH();

	virtual void		HandleTask();

protected:
	void				AndersNogIets();
	void				OokDatNog(Database *rdb, CodeTable *ct, RecommendationTechnique rTechnique, RecommendationPossibility rPossibility, bool rAllowEmpty, uint16 *rNumVals, uint16** rVals, ItemSet **rSets, double *origEncSzs, double *rcmEncSzs);


	ItemSet**			DeterminePossibleRecommendations(ItemSet *row, islist *itemSets, ItemSet** abSets, uint32 abSize, uint32 &numPosRecommendations, RecommendationPossibility rStylee);
	double				DetermineAugmentedEncodedSize(ItemSet *row, ItemSet *augment, CodeTable *ct);
	double*				DetermineAugmentedEncodedSizes(ItemSet *row, ItemSet **augments, uint32 numAugments, CodeTable *ct, uint32 &minEncSzIdx);
	void				DetermineRecommendedItems(ItemSet *row, ItemSet *augment, uint16 *rVals, uint16 &numRItems);

	double				CalculateRecommendationAccuracy(Database *cdb, uint16 **estVals, uint16 **misVals, uint16 *numMisVals);

	Database*			DamageDatabase(Database * const db, RecommendableDatabaseDamageTechnique weapon, uint16 **misVals, uint16 *numMisVals);
	Database*			DamageDBNone(Database * const db, uint16 **dmgVals, uint16 *dmgNumVals);
	Database*			DamageDBSnipe(Database * const db, uint16 **dmgVals, uint16 *dmgNumVals);

	// auxiliary functions for reading config file
	RecommendationTechnique					StringToRt(string rt);
	RecommendationPossibility				StringToRp(string rp);
	RecommendableDatabaseDamageTechnique	StringToRDBDt(string dbdt);

	ItemSet **mABSets;
	uint32	mABSize;

//	FILE*				mEstOutFile;
};

#endif // __RECOMMENDATIONTASKHANDLER_H
