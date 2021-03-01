#include "../global.h"

// qtils
#include <Config.h>
#include <RandomUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <db/ClassedDatabase.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>

#include <isc/ItemSetCollection.h>

// here
#include "../blocks/krimp/codetable/CodeTable.h"
#include "../FicMain.h"

#include "RecommendTH.h"

RecommendTH::RecommendTH(Config *conf) : TaskHandler(conf){
}
RecommendTH::~RecommendTH() {
	// not my Config*
}

void RecommendTH::HandleTask() {
	string command = mConfig->Read<string>("command");

	if(command.compare("recommend") == 0)		AndersNogIets();
	else	throw string("MissTH :: Unable to handle task `" + command + "`");
}

void RecommendTH::AndersNogIets() {
#if defined (_MSC_VER)
	Bass::SetWorkingDir(Bass::GetExpDir() + "recom/");
#elif defined (__GNUC__)
	string wd(Bass::GetExpDir() + "recom/");
	Bass::SetWorkingDir(wd);
#endif

	// Control Chaos, or not
	uint32 seed = mConfig->Read<uint32>("chaos",0);
	if(seed > 0)	RandomUtils::Init(seed);
	else			RandomUtils::Init();

	// Determine number of Cross-Validation Folds
	uint32 numFolds = mConfig->Read<uint32>("numfolds", 1);

	// 1. Men lade een database
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);
	ItemSet **rows = db->GetRows();
	uint32 numRows = db->GetNumRows();
	mABSize = (uint32) db->GetAlphabetSize();
	mABSets = db->GetAlphabetSets();

	// 1a. Create Recommendation Data Structures
	uint16 **rcmVals = new uint16*[numRows];
	uint16 **misVals = new uint16*[numRows];
	uint16 *rcmNumVals = new uint16[numRows];
	uint16 *misNumVals = new uint16[numRows];
	ItemSet **rcmSets = new ItemSet*[numRows];
	for(uint32 i=0; i<numRows; i++) {
		rcmVals[i] = new uint16[db->GetMaxSetLength()];	// no code table element will be larger
		misVals[i] = new uint16[db->GetMaxSetLength()];	// no code table element will be larger
		rcmNumVals[i] = 0;
		misNumVals[i] = 0;
		rcmSets[i] = NULL;
	}
	double *orgEncSzs = new double[numRows];
	double *rcmEncSzs = new double[numRows];

	string weaponStr = mConfig->Read<string>("weapon", "");
	RecommendableDatabaseDamageTechnique rWeapon = StringToRDBDt(weaponStr);

	string rTechniqueStr = mConfig->Read<string>("stylee", "minEncSz");
	RecommendationTechnique rTechnique = StringToRt(rTechniqueStr);

	string rPossibilityStr = mConfig->Read<string>("consider", "oneitem");
	RecommendationPossibility rPossibility = StringToRp(rPossibilityStr);

	bool rAllowEmpty = mConfig->Read<bool>("allowempty", true);
	bool rDamagedTraining = mConfig->Read<bool>("dmgFirst", true);
	bool rNeedsCodeTable = true;

	uint32 numReps = 1;
	for(uint32 rep=0; rep<numReps; rep++) {

		if(numFolds == 1) {
			// Test == Training

			Database *rdb = NULL;
			CodeTable *ct = NULL;
			if(rDamagedTraining == true) {
				// 2a. Do Da Damage Dance
				rdb = DamageDatabase(db, rWeapon, misVals, misNumVals);
				// 2b. Men comprimere zulks database
				if(rNeedsCodeTable == true)
					ct = FicMain::CreateCodeTable(rdb, mConfig, true);
			} else {
				// 2a. men comprimere zulks database
				ct = FicMain::ProvideCodeTable(db, mConfig, true);
				// 2b. Damage to the Database!
				if(rNeedsCodeTable == true)
					rdb = DamageDatabase(db, rWeapon, misVals, misNumVals);
			}

			// Appliceer de correctieve
			if(mConfig->Read<uint32>("lpcOffset",0) == 1)
				ct->AddOneToEachUsageCount();

			OokDatNog(rdb, ct, rTechnique, rPossibility, rAllowEmpty, rcmNumVals, rcmVals, rcmSets, orgEncSzs, rcmEncSzs);

			uint32 rcmNumCorrect = 0;
			uint32 rcmNumRows = 0;
			uint32 totNumMiss = 0;
			for(uint32 i=0; i<numRows; i++) {
				totNumMiss += misNumVals[i];
				if(rcmNumVals[i] > 0) {
					printf("%d : ", i);
					printf("%s in %.2f", rdb->GetRow(i)->ToString(false,false).c_str(), orgEncSzs[i]);
					printf(" now with %s in %.2f", rcmSets[i]->ToString(false,false).c_str(), rcmEncSzs[i]);
					printf("\n");
					for(uint32 m=0; m<misNumVals[i]; m++) {
						if(rcmSets[i]->IsItemInSet(misVals[i][m])) {
							rcmNumCorrect++;
						}
					}
					rcmNumRows++;
				} else {
					printf("%d : ", i);
					printf("%s in %.2f", rdb->GetRow(i)->ToString(false,false).c_str(), orgEncSzs[i]);
					printf(" nothing to recommend\n");
				}
			}
			delete ct;
			delete rdb;
			printf("rcmNumCorrect=%d rcmNumRows=%d totNumMis=%d\n", rcmNumCorrect, rcmNumRows, totNumMiss);

		} else {
			// n-Fold Cross Validations

			uint32 rcmNumCorrect = 0;
			uint32 rcmNumRows = 0;
			uint32 totNumMiss = 0;
			db->RandomizeRowOrder(seed);
			Database **foldsDBs = db->SplitForCrossValidationIntoDBs(numFolds);
			Database **inGroupParts = new Database*[numFolds-1];
			for(uint32 f=0; f<numFolds; f++) {
				// Group the ingroups together into one database
				uint32 curIn = 0;
				for(uint32 g=0; g<numFolds; g++)
					if(g != f)
						inGroupParts[curIn++] = foldsDBs[g];
				Database *inGroupDB = Database::Merge(inGroupParts, numFolds-1);

				// Damage to the Database!
				memset(misNumVals, 0, foldsDBs[f]->GetNumRows()*sizeof(uint16));
				Database *rdb = DamageDatabase(foldsDBs[f], rWeapon, misVals, misNumVals);
				uint32 rdbNumRows = rdb->GetNumRows();
				// Ritsel de code table
				CodeTable *ct = FicMain::CreateCodeTable(inGroupDB, mConfig, false);

				// Appliceer de correctieve
				if(mConfig->Read<uint32>("lpcOffset",0) == 1)
					ct->AddOneToEachUsageCount();

				OokDatNog(rdb, ct, rTechnique, rPossibility, rAllowEmpty, rcmNumVals, rcmVals, rcmSets, orgEncSzs, rcmEncSzs);

				for(uint32 i=0; i<rdbNumRows; i++) {
					totNumMiss += misNumVals[i];
					if(rcmNumVals[i] > 0) {
						printf("%d : ", i);
						printf("%s in %.2f", rdb->GetRow(i)->ToString().c_str(), orgEncSzs[i]);
						printf(" now with %s in %.2f", rcmSets[i]->ToString().c_str(), rcmEncSzs[i]);
						printf("\n");
						for(uint32 m=0; m<misNumVals[i]; m++) {
							if(rcmSets[i]->IsItemInSet(misVals[i][m])) {
								rcmNumCorrect++;
							}
						}
						rcmNumRows++;
					} else {
						printf("%d : ", i);
						printf("%s in %.2f", rdb->GetRow(i)->ToString().c_str(), orgEncSzs[i]);
						printf(" nothing to recommend\n");
					}
				}
				delete ct;
				delete rdb;
				delete inGroupDB;
			}
			printf("rcmNumCorrect=%d rcmNumRows=%d totNumMis=%d\n", rcmNumCorrect, rcmNumRows, totNumMiss);
			delete[] inGroupParts;
			for(uint32 f=0; f<numFolds; f++)
				delete foldsDBs[f];
			delete[] foldsDBs;
		}
	}

	delete[] rcmSets;
	for(uint32 i=0; i<numRows; i++) {
		delete[] rcmVals[i];
		delete[] misVals[i];
	} 
	delete[] rcmVals;
	delete[] rcmNumVals;
	delete[] misVals;
	delete[] misNumVals;
	delete[] orgEncSzs;
	delete[] rcmEncSzs;

	for(uint32 i=0; i<mABSize; i++)
		delete mABSets[i];
	delete[] mABSets;

	delete db;
}

void RecommendTH::OokDatNog(Database *rdb, CodeTable *ct, RecommendationTechnique rTechnique, RecommendationPossibility rPossibility, bool rAllowEmpty, uint16 *rNumVals, uint16** rVals, ItemSet **rSets, double *orgEncSzs, double *rcmEncSzs) {
	ItemSet **rows = rdb->GetRows();
	uint32 numRows = rdb->GetNumRows();

	islist *ctElems = ct->GetItemSetList();

	for(uint32 i=0; i<numRows; i++) {
		// per transactie in de testdatabase
		ItemSet *row = rows[i];
		//  bepale men de mogelijke code table elementen
		uint32 numPosRecommendations = 0;
		orgEncSzs[i] = ct->CalcTransactionCodeLength(row);

		ItemSet ** posRecommendations = DeterminePossibleRecommendations(rows[i], ctElems, mABSets, mABSize, numPosRecommendations, rPossibility);
		if(numPosRecommendations > 0) {
			//  per mogelijk element additioneel aan den transactie bepale men de gecodeerde grootte 
			//  kieze men den minst erge der kwalen

			if(rTechnique == RtRandomChoice || rTechnique == RtMostFrequentItem)  {	// Random, vereist geen EncSzs
				uint32 chosenIdx = numPosRecommendations;
				if(rTechnique == RtRandomChoice)
					chosenIdx = RandomUtils::UniformUint32(numPosRecommendations+(rAllowEmpty?1:0));
				else {
					uint32 maxFreq = 0;
					for(uint32 j=0; j<numPosRecommendations; j++)
						if(posRecommendations[j]->GetUsageCount() > maxFreq) {
							maxFreq = posRecommendations[j]->GetUsageCount();
							chosenIdx = j;
						}
				}
				if(chosenIdx != numPosRecommendations) {	// numPosRecommendations == IkKiesNiks
					rcmEncSzs[i] = DetermineAugmentedEncodedSize(rows[i], posRecommendations[chosenIdx], ct);
					DetermineRecommendedItems(rows[i], posRecommendations[chosenIdx], rVals[i], rNumVals[i]);
					rSets[i] = posRecommendations[chosenIdx];
				} else
					rNumVals[i] = 0;
			} else {	// Recom. Techniques die EncSzs vereisen
				uint32 minEncSzIdx = 0;
				double *encSzs = DetermineAugmentedEncodedSizes(rows[i], posRecommendations, numPosRecommendations, ct, minEncSzIdx);
				if(rTechnique == RtShortestEncodedSize) {
					if(encSzs[minEncSzIdx] <= orgEncSzs[i] || rAllowEmpty == false) {
						rcmEncSzs[i] = encSzs[minEncSzIdx];
						DetermineRecommendedItems(rows[i], posRecommendations[minEncSzIdx], rVals[i], rNumVals[i]);
						rSets[i] = posRecommendations[minEncSzIdx];
					} else
						rNumVals[i] = 0;
				} else if(rTechnique == RtWeightedEncodedSize) {
					throw string("snot yet.");
				}  
				delete[] encSzs;
			} 
		}
		delete[] posRecommendations;
	}
	delete ctElems;
}


ItemSet** RecommendTH::DeterminePossibleRecommendations(ItemSet *row, islist *iSets, ItemSet ** abSets, uint32 abSize, uint32 &numPosRecommendations, RecommendationPossibility rStylee) {
	numPosRecommendations = 0;
	uint32 numCTElems = (uint32) iSets->size();
	ItemSet **posCTElems = new ItemSet*[numCTElems + abSize];
	islist::iterator cit = iSets->begin(), cend = iSets->end();
	if(rStylee == RsCTSetOverlap || rStylee == RsCTElemOverlap) {
		for(;cit != cend; ++cit)
			if(!row->IsSubset(*cit) && row->Intersects(*cit))
				posCTElems[numPosRecommendations++] = *cit;
	} else if(rStylee == RsCTSet || rStylee == RsCTElem) {
		for(;cit != cend; ++cit)
			if(!row->IsSubset(*cit))
				posCTElems[numPosRecommendations++] = *cit;
	}
	if(rStylee == RsOneItem || rStylee == RsCTElemOverlap || rStylee == RsCTElem) {
		for(uint32 a=0; a<abSize; a++) {
			if(!row->IsItemInSet(abSets[a]->GetLastItem())) {
				posCTElems[numPosRecommendations++] = abSets[a];
			}
		}
	}
	return posCTElems;
}

double RecommendTH::DetermineAugmentedEncodedSize(ItemSet *row, ItemSet *augment, CodeTable *ct) {
	ItemSet *aug = row->Union(augment);
	double encSz = ct->CalcTransactionCodeLength(aug);
	delete aug;
	return encSz;
}

double* RecommendTH::DetermineAugmentedEncodedSizes(ItemSet *row, ItemSet **augments, uint32 numAugments, CodeTable *ct, uint32 &minEncSzIdx) {
	double *encSzs = new double[numAugments];
	double minEncSz = DOUBLE_MAX_VALUE;
	minEncSzIdx = -1;
	for(uint32 i=0; i<numAugments; i++) {
		ItemSet *aug = row->Union(augments[i]);
		encSzs[i] = ct->CalcTransactionCodeLength(aug);
		if(encSzs[i] < minEncSz) {
			minEncSz = encSzs[i];
			minEncSzIdx = i;
		}
		delete aug;
	}
	return encSzs;
}

void RecommendTH::DetermineRecommendedItems(ItemSet *row, ItemSet *augment, uint16 *rVals, uint16 &numRItems) {
	if(augment->GetLength() == 1) {
		rVals[0] = augment->GetLastItem();
		numRItems = 1;
	} else {
		ItemSet *remainder = augment->Remainder(row);
		numRItems = remainder->GetLength();
		remainder->GetValuesIn(rVals);
		delete remainder;
	}
	//numRItems = augment->GetLength();
	//augment->GetValuesIn(rVals);
}


double RecommendTH::CalculateRecommendationAccuracy(Database *dmgdb, uint16 **estVals, uint16 **misVals, uint16 *numMisVals) {
	ItemSet **rows = dmgdb->GetRows();
	uint32 numRows = dmgdb->GetNumRows();
	uint32 numRight = 0;
	uint32 numGuesses = 0;
	for(uint32 i=0; i<numRows; i++) {
		for(uint32 m=0; m<numMisVals[i]; m++) {
			for(uint32 e=0; e<numMisVals[i]; e++) {
				if(estVals[i][e] == misVals[i][m]) {
					numRight++;
					break;
				}
			}
			numGuesses++;
		}
	}
	return (double)numRight / numGuesses;
}


RecommendationTechnique RecommendTH::StringToRt(string rt) {
	if(rt.compare("minEncSz") == 0 || rt.compare("encSz") == 0)
		return RtShortestEncodedSize;
	else if(rt.compare("rndEncSz") == 0 || rt.compare("weightedEncSz") == 0)
		return RtWeightedEncodedSize;
	else if(rt.compare("random") == 0 || rt.compare("rnd") == 0)
		return RtRandomChoice;
	else if(rt.compare("mostFrequentItem") == 0 || rt.compare("mfi") == 0)
		return RtMostFrequentItem;
	throw string("RecommendTH::StringToRt - Unknown RecommendationTechnique: " + rt);
}
RecommendationPossibility RecommendTH::StringToRp(string rp) {
	if(rp.compare("abItem")==0) 
		return RsOneItem;
	else if(rp.compare("ctElemOverlap")==0)
		return  RsCTElemOverlap;
	else if(rp.compare("ctSetOverlap")==0)
		return RsCTSetOverlap;
	else if(rp.compare("ctSet")==0)
		return RsCTSet;
	else if(rp.compare("ctElem")==0)
		return RsCTElem;
	throw string("RecommendTH::StringToRp - Unknown RecommendationPossibility: " + rp);
}
RecommendableDatabaseDamageTechnique RecommendTH::StringToRDBDt(string dt) {
	if(dt.compare("none")==0)
		return RDBDtNone;
	else if(dt.compare("snipe")==0)
		return RDBDtSnipe;
	throw string("RecommendTH::StringToRDBDt - Unknown RecommendableDatabaseDamageTechnique: " + dt);
}

Database* RecommendTH::DamageDatabase(Database * const db, RecommendableDatabaseDamageTechnique weapon, uint16 **dmgVals, uint16 *dmgNumVals) {
	if(weapon == RDBDtNone) {
		return DamageDBNone(db, dmgVals, dmgNumVals);
	} else if(weapon == RDBDtSnipe) {
		return DamageDBSnipe(db, dmgVals, dmgNumVals);
	}
	throw string("RecommendTH::DamageDatabase - Unknown DatabaseDamageTechnique");
}
Database* RecommendTH::DamageDBNone(Database * const db, uint16 **dmgVals, uint16 *dmgNumVals) {
	Database *rdb = new Database(db);
	memset(dmgNumVals, 0, sizeof(uint16)*db->GetNumRows());
	return rdb;
}
Database* RecommendTH::DamageDBSnipe(Database * const db, uint16 **dmgVals, uint16 *dmgNumVals) {
	Database *rdb = new Database(db);
	ItemSet **rows = rdb->GetRows();
	size_t abSize = db->GetAlphabetSize();
	uint16 *auxValArr = new uint16[abSize];
	uint32 numRows = db->GetNumRows();

	for(uint64 i=0; i<numRows; i++) {
		if(rows[i]->GetLength() > 0) {
			rows[i]->GetValuesIn(auxValArr);
			uint16 misVal = RandomUtils::UniformChoose(auxValArr, rows[i]->GetLength());
			rows[i]->RemoveItemFromSet(misVal);
			dmgVals[i][dmgNumVals[i]] = misVal;
			dmgNumVals[i]++;
		}
	}

	delete[] auxValArr;

	return rdb;
}
