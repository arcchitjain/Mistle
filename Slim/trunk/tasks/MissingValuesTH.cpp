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

#include "MissingValuesTH.h"

MissingValuesTH::MissingValuesTH(Config *conf) : TaskHandler(conf){
	mEstOutFile = NULL;

}
MissingValuesTH::~MissingValuesTH() {
	// not my Config*
}

void MissingValuesTH::HandleTask() {
	string command = mConfig->Read<string>("command");

	if(command.compare("miss") == 0)				Miss();
	else if(command.compare("damnml") == 0)			DamageForMatlab();
	else if(command.compare("accrml") == 0)			AccurForMatlab();
	else if(command.compare("damnnir") == 0)		DamageForNir();
	else if(command.compare("accrnir") == 0)		AccurFromNir();
	else	throw string("MissTH :: Unable to handle task `" + command + "`");
}

void MissingValuesTH::DamageForMatlab() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);
	uint32 numRows = db->GetNumRows();
	if(!db->HasColumnDefinition()) 
	{ delete db; throw string("Current Missing Value Estimation approach requires a column definition."); }

	// Control Chaos, or not
	uint32 seed = mConfig->Read<uint32>("chaos",0);
	if(seed > 0)	RandomUtils::Init(seed);
	else			RandomUtils::Init();

	// Determine anti-Database Weapon
	string weaponStr = mConfig->Read<string>("weapon", "");
	DatabaseDamageTechnique weapon = StringToDBDt(weaponStr);

	uint16 ** misVals = new uint16*[numRows];
	uint16 * numMisVals = new uint16[numRows];
	for(uint32 i=0; i<numRows; i++) {
		misVals[i] = new uint16[db->GetMaxSetLength()];
		numMisVals[i] = 0;
	}

	// Damage to the Database!
	Database *dmgdb = DamageDatabase(db, weapon, misVals, numMisVals);

	string damnDBName = dbName + "-dmg";
	string damnDBNamStr = Bass::GetWorkingDir() + damnDBName + ".mlnames";
	string damnDBDatStr = Bass::GetWorkingDir() + damnDBName + ".mldata";

	uint16 *auxValArr = new uint16[db->GetAlphabetSize()];

	// Write .names file (column information)
	uint32 numCols = db->GetNumColumns();
	ItemSet **colDef = db->GetColumnDefinition();

	// Write .data file (the real stuff)
	ItemSet **rows = dmgdb->GetRows();
	FILE* datFile = fopen(damnDBDatStr.c_str(), "w");
	for(uint32 r=0; r<numRows; r++) {
		rows[r]->GetValuesIn(auxValArr);
		//fprintf(datFile, "(");
		for(uint32 c=0; c<numCols-1; c++) {
			if(rows[r]->Intersects(colDef[c])) {
				// not missing
				uint32 numVals = rows[r]->GetLength();
				for(uint32 v=0; v<numVals; v++) {
					if(colDef[c]->IsItemInSet(auxValArr[v])) {
						fprintf(datFile, "%d ", auxValArr[v]);
						break;
					}
				}
			} else {
				// missing
				fprintf(datFile, "NaN ");
			}
		}
		if(rows[r]->Intersects(colDef[numCols-1])) {
			// not missing
			uint32 numVals = rows[r]->GetLength();
			for(uint32 v=0; v<numVals; v++) {
				if(colDef[numCols-1]->IsItemInSet(auxValArr[v])) {
					fprintf(datFile, "%d", auxValArr[v]);
					break;
				}
			}
		} else {
			// missing
			fprintf(datFile, "NaN");
		}
		fprintf(datFile, "\n");
	}
	fclose(datFile);

	// delete estimates and missing values
	for(uint32 i=0; i<numRows; i++) {
		delete[] misVals[i];
	}
	delete[] misVals;
	delete[] numMisVals;

	delete[] auxValArr;
	delete dmgdb;
	delete db;
}

void MissingValuesTH::AccurForMatlab() {
	// 
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);
	if(!db->HasColumnDefinition()) 
	{ delete db; throw string("Current Missing Value Estimation approach requires a column definition."); }

	uint32 numRows = db->GetNumRows();
	ItemSet **colDef = db->GetColumnDefinition();
	uint32 numCols = db->GetNumColumns();

	// Control Chaos, or not
	uint32 seed = mConfig->Read<uint32>("chaos",0);
	if(seed > 0)	RandomUtils::Init(seed);
	else			RandomUtils::Init();

	// Determine anti-Database Weapon
	string weaponStr = mConfig->Read<string>("weapon", "");
	DatabaseDamageTechnique weapon = StringToDBDt(weaponStr);

	uint16 ** misVals = new uint16*[numRows];
	uint16 * numMisVals = new uint16[numRows];
	uint16 ** estVals = new uint16*[numRows];
	for(uint32 i=0; i<numRows; i++) {
		misVals[i] = new uint16[db->GetMaxSetLength()];
		estVals[i] = new uint16[db->GetMaxSetLength()];
		numMisVals[i] = 0;
	}

	uint32 eps = mConfig->Read<uint32>("eps", (uint32)ceil(((double)db->GetNumRows() / (double)100)));

	uint16 **colVals = new uint16*[numCols];
	for(uint32 c=0; c<numCols; c++) {
		colVals[c] = colDef[c]->GetValues();
	}

	// Damage to the Database!
	Database *dmgdb = DamageDatabase(db, weapon, misVals, numMisVals);
	ItemSet **dmgRows = dmgdb->GetRows();

	uint32 colWidth = 0;
	uint16 *auxValArr = new uint16[db->GetAlphabetSize()];

	bool guess = mConfig->Read<bool>("guess", true);

	string pfilepath = Bass::GetExpDir() + "/miss/" + dbName + "-probs.txt";
	FILE *pfile = fopen(pfilepath.c_str(), "r");
	if(pfile == NULL)
		throw ("probs file not present");
	uint32 buflen = 1024 * 10;
	char *buf = new char[buflen];
	RandomUtils::Init();
	for(uint32 r=0; r<numRows; r++) {
		if(numMisVals[r] == 0)
			continue;

		uint32 numMisCols = 0;
		ItemSet **misCols = DetermineMissingColumns(dmgRows[r],colDef,numCols,numMisCols);

		// identify the column idx of the current missing column
		uint32 curCol = 0;
		uint32 m = 0;
		for(uint32 c=0; c<numCols; c++) {
			if(misCols[m] == colDef[c]) {
				curCol = c;
				break;
			}
		}

		// wel misvals, dus de probs uitlezen uit probs.txt
		fgets(buf, buflen, pfile);
		uint32 numProbs = 0;
		double *probs = StringUtils::TokenizeDouble(buf, numProbs);
		double rnd = RandomUtils::UniformDouble();
		double maxProb = 0;
		for(uint32 g=0; g<numProbs; g++) {
			if(guess == true) {
				if(rnd <= probs[g]) {
					estVals[r][0] = colVals[curCol][g];
				} else
					rnd -= probs[g];
			} else {
				if(probs[g] > maxProb) {
					estVals[r][0] = colVals[curCol][g];
					maxProb = probs[g];
				}
			}
		}
		delete[] probs;
		delete[] misCols;
	}

	// Calculate Accuracy
	string iscTag = mConfig->Read<string>("iscname");
	bool mined = false;
	ItemSetCollection *origFIS = FicMain::ProvideItemSetCollection(iscTag, db, mined, false, true);
	uint32 *epsd = CalculateEpsDeltaAccuracy(dmgdb, origFIS, estVals, misVals, numMisVals);
	uint32 csum = 0, cnum = 0;
	uint32 nfis = origFIS->GetNumLoadedItemSets();
	for(uint32 e=0; e<eps; e++) {
		if(epsd[e] != 0) {
			cnum += epsd[e];
		} 
	}
	double epsdelta = ((double)nfis - (double)cnum) / (double)nfis;
	delete origFIS;
	delete[] epsd;


	// Calculate Accuracy, print it.
	double accuracy = CalculateEstimationAccuracy(dmgdb, estVals, misVals, numMisVals);
	printf("%f (%d,%.2f)\n", accuracy, eps, epsdelta);

	// delete estimates and missing values
	for(uint32 i=0; i<numRows; i++) {
		delete[] misVals[i];
		delete[] estVals[i];
	}
	delete[] misVals;
	delete[] estVals;
	delete[] numMisVals;
	delete[] colVals;

	// delete tmp variables
	delete[] auxValArr;
	delete[] buf;

	// delete db and dmgdb
	delete dmgdb;
	delete db;
}

void MissingValuesTH::DamageForNir() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);
	uint32 numRows = db->GetNumRows();
	if(!db->HasColumnDefinition()) 
	{ delete db; throw string("Current Missing Value Estimation approach requires a column definition."); }

	// Control Chaos, or not
	uint32 seed = mConfig->Read<uint32>("chaos",0);
	if(seed > 0)	RandomUtils::Init(seed);
	else			RandomUtils::Init();

	// Determine anti-Database Weapon
	string weaponStr = mConfig->Read<string>("weapon", "");
	DatabaseDamageTechnique weapon = StringToDBDt(weaponStr);

	uint16 ** misVals = new uint16*[numRows];
	uint16 * numMisVals = new uint16[numRows];
	for(uint32 i=0; i<numRows; i++) {
		misVals[i] = new uint16[db->GetMaxSetLength()];
		numMisVals[i] = 0;
	}

	// Damage to the Database!
	Database *dmgdb = DamageDatabase(db, weapon, misVals, numMisVals);


	string damnDBName = dbName + "-dmg";
	string damnDBNamStr = Bass::GetWorkingDir() + damnDBName + ".names";
	string damnDBDatStr = Bass::GetWorkingDir() + damnDBName + ".data";

	uint16 *auxValArr = new uint16[db->GetAlphabetSize()];

	// Write .names file (column information)
	uint32 numCols = db->GetNumColumns();
	ItemSet **colDef = db->GetColumnDefinition();
	FILE* namFile = fopen(damnDBNamStr.c_str(), "w");
	for(uint32 i=0; i<numCols; i++) {
		fprintf(namFile, "(var 'COL%d '(", i);
		colDef[i]->GetValuesIn(auxValArr);
		uint32 numVals = colDef[i]->GetLength();
		for(uint32 j=0; j<numVals-1; j++)
			fprintf(namFile, "%d ",auxValArr[j]);
		fprintf(namFile, "%d))\n\n", auxValArr[numVals-1]);
	}
	fclose(namFile);

	// Write .data file (the real stuff)
	ItemSet **rows = dmgdb->GetRows();
	FILE* datFile = fopen(damnDBDatStr.c_str(), "w");
	for(uint32 r=0; r<numRows; r++) {
		rows[r]->GetValuesIn(auxValArr);
		fprintf(datFile, "(");
		for(uint32 c=0; c<numCols-1; c++) {
			if(rows[r]->Intersects(colDef[c])) {
				// not missing
				uint32 numVals = rows[r]->GetLength();
				for(uint32 v=0; v<numVals; v++) {
					if(colDef[c]->IsItemInSet(auxValArr[v])) {
						fprintf(datFile, "%d ", auxValArr[v]);
						break;
					}
				}
			} else {
				// missing
				fprintf(datFile, "? ");
			}
		}
		if(rows[r]->Intersects(colDef[numCols-1])) {
			// not missing
			uint32 numVals = rows[r]->GetLength();
			for(uint32 v=0; v<numVals; v++) {
				if(colDef[numCols-1]->IsItemInSet(auxValArr[v])) {
					fprintf(datFile, "%d", auxValArr[v]);
					break;
				}
			}
		} else {
			// missing
			fprintf(datFile, "?");
		}
		fprintf(datFile, ")\n");
	}
	fclose(datFile);

	// delete estimates and missing values
	for(uint32 i=0; i<numRows; i++) {
		delete[] misVals[i];
	}
	delete[] misVals;
	delete[] numMisVals;

	delete[] auxValArr;
	delete dmgdb;
	delete db;
}

void MissingValuesTH::AccurFromNir() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);
	if(!db->HasColumnDefinition()) 
	{ delete db; throw string("Current Missing Value Estimation approach requires a column definition."); }

	uint32 numRows = db->GetNumRows();
	ItemSet **colDef = db->GetColumnDefinition();
	uint32 numCols = db->GetNumColumns();

	// Control Chaos, or not
	uint32 seed = mConfig->Read<uint32>("chaos",0);
	if(seed > 0)	RandomUtils::Init(seed);
	else			RandomUtils::Init();

	// Determine anti-Database Weapon
	string weaponStr = mConfig->Read<string>("weapon", "");
	DatabaseDamageTechnique weapon = StringToDBDt(weaponStr);

	uint16 ** misVals = new uint16*[numRows];
	uint16 * numMisVals = new uint16[numRows];
	uint16 ** estVals = new uint16*[numRows];
	for(uint32 i=0; i<numRows; i++) {
		misVals[i] = new uint16[db->GetMaxSetLength()];
		estVals[i] = new uint16[db->GetMaxSetLength()];
		numMisVals[i] = 0;
	}

	// Damage to the Database!
	Database *dmgdb = DamageDatabase(db, weapon, misVals, numMisVals);
	ItemSet **dmgRows = dmgdb->GetRows();

	uint32 colWidth = 0;
	uint16 *auxValArr = new uint16[db->GetAlphabetSize()];

	// load network
	ItemSet ***nWorkCnd = new ItemSet**[numCols];
	uint32 *nWorkNumCnd = new uint32[numCols];
	uint32 **parentCols = new uint32*[numCols];
	uint32 *parentNums = new uint32[numCols];
	double ***nWorkProbs = new double**[numCols];

	string nwName = mConfig->Read<string>("nwname") + ".nw";
	string nwFullpath = Bass::GetExpDir() + "miss/" + nwName;
	FILE *nwFile = fopen(nwFullpath.c_str(), "r");
	uint32 buf_len = 128 * 1024;
	char *buf = new char[buf_len];
	char *bp = buf, *bpN;

	uint16 **colVals = new uint16*[numCols];
	for(uint32 c=0; c<numCols; c++) {
		colVals[c] = colDef[c]->GetValues();
		fgets(buf,buf_len,nwFile);
		bp = buf;
		uint32 numParents = strtoul(bp,&bpN,10);
		bp = bpN + 1;	// correct for : 

		parentCols[c] = new uint32[numParents];
		uint32 parentCombis = 1;
		parentNums[c] = numParents;
		for(uint32 p=0; p<numParents; p++) {
			uint32 parentCol = strtoul(bp,&bpN,10);
			bp = bpN + 1; // correct for space
			parentCombis *= colDef[parentCol]->GetLength();
			parentCols[c][p] = parentCol;
		}

		bp++; // correct for (

		if(numParents == 0) {
			nWorkNumCnd[c] = numParents;
			nWorkCnd[c] = new ItemSet*[1];
			nWorkCnd[c][0] = ItemSet::CreateEmpty(db->GetDataType(), db->GetAlphabetSize());
			nWorkProbs[c] = new double*[1];
			nWorkProbs[c][0] = new double[colDef[c]->GetLength()];
			for(uint32 p=0; p<colDef[c]->GetLength(); p++) {
				// read prob
				nWorkProbs[c][0][p] = strtod(bp,&bpN);
				bp = bpN + 1; // correct for ' '
			}
		} else {
			nWorkNumCnd[c] = parentCombis;
			nWorkCnd[c] = new ItemSet*[nWorkNumCnd[c]];
			nWorkProbs[c] = new double*[parentCombis];
			for(uint32 pc=0; pc<parentCombis; pc++) {
				nWorkProbs[c][pc] = new double[colDef[c]->GetLength()];
				nWorkCnd[c][pc] = ItemSet::CreateEmpty(db->GetDataType(), db->GetAlphabetSize());
				// read condition
				bp++; // '('
				bp++; // '('
				for(uint32 nc=0; nc<numParents; nc++) {
					uint16 curCnd = (uint16) strtoul(bp,&bpN,10);
					bp = bpN + 1; // ' ';
					nWorkCnd[c][pc]->AddItemToSet(curCnd);
				}
				bp++; // ')' (or actually, ' ', as last + 1 did ')' )

				// read probs
				for(uint32 p=0; p<colDef[c]->GetLength(); p++) {
					// read prob
					nWorkProbs[c][pc][p] = strtod(bp,&bpN);
					bp = bpN + 1; // correct for ' '
				}
				bp++; // ')' (or actually, ' ', as last + 1 did ')' )
			}
		}
	}
	fclose(nwFile);
	bool guess = mConfig->Read<bool>("guess", true);

	// now use the network to estimate
	for(uint32 r=0; r<numRows; r++) {
		if(numMisVals[r] == 0)
			continue;

		uint32 numMisCols = 0;
		ItemSet **misCols = DetermineMissingColumns(dmgRows[r],colDef,numCols,numMisCols);

		// determine posEstimates
		// determine prob per posEstimate
		// choose estimate
		// // hier was ik. in dmgnir ook nog ff delete[]'en!

		for(uint32 m=0; m<numMisCols; m++)
			estVals[r][m] = UINT16_MAX_VALUE;

		uint32 numMisLeft = numMisCols;
		while(numMisLeft > 0) {

			for(uint32 m=0; m<numMisCols; m++) {
				if(estVals[r][m] != UINT16_MAX_VALUE) {
					continue;
				} else {
					// identify the column idx of the current missing column
					uint32 curCol = 0;
					for(uint32 c=0; c<numCols; c++) {
						if(misCols[m] == colDef[c]) {
							curCol = c;
							break;
						}
					}

					// check if parent cols are missing
					bool missing = false;
					for(uint32 p=0; p<parentNums[curCol]; p++) {
						if(!dmgRows[r]->Intersects(colDef[parentCols[curCol][p]])) {
							missing = true;
							break;
						} 
					}

					// if not missing, estimate it
					if(missing == false) {
						uint32 curColWidth = colDef[curCol]->GetLength();
						uint32 numConditions = nWorkNumCnd[curCol];
						colDef[curCol]->GetValuesIn(auxValArr);
						if(numConditions == 0) {
							if(guess == true) {
								double guess = RandomUtils::UniformDouble();
								for(uint32 i=0; i<curColWidth; i++) {
									if(guess - nWorkProbs[curCol][0][i] <= 0) {
										estVals[r][m] = auxValArr[i];
										dmgRows[r]->AddItemToSet(estVals[r][m]);
										break;
									} else 
										guess -= nWorkProbs[curCol][0][i];
								}
							} else {
								double maxProb = 0;
								uint32 maxProbIdx = 0;
								for(uint32 i=0; i<curColWidth; i++) {
									if(nWorkProbs[curCol][0][i] > maxProb) {
										maxProb = nWorkProbs[curCol][0][i];
										maxProbIdx = i;
									}
								}
								estVals[r][m] = auxValArr[maxProbIdx];
								dmgRows[r]->AddItemToSet(estVals[r][m]);
							}
							
						} else {
							// bepaal de juiste sub-probdistr
							for(uint32 cnd=0; cnd<numConditions; cnd++) {
								if(dmgdb->GetRow(r)->IsSubset(nWorkCnd[curCol][cnd])) {
									if(guess == true) {
										double guess = RandomUtils::UniformDouble();
										for(uint32 cw=0; cw<curColWidth; cw++) {
											if(guess - nWorkProbs[curCol][cnd][cw] <= 0) {
												estVals[r][m] = auxValArr[cw];
												dmgRows[r]->AddItemToSet(estVals[r][m]);
												break;	// gekozen, dus stoppen met kiezen.
											} else 
												guess -= nWorkProbs[curCol][cnd][cw];
										}
									} else {
										double maxProb = 0;
										uint32 maxProbIdx = 0;
										for(uint32 cw=0; cw<curColWidth; cw++) {
											if(nWorkProbs[curCol][cnd][cw] > maxProb) {
												maxProb = nWorkProbs[curCol][cnd][cw];
												maxProbIdx = cw;
											}
										}
										estVals[r][m] = auxValArr[maxProbIdx];
										dmgRows[r]->AddItemToSet(estVals[r][m]);
									}
									break;	// juiste cnd gevonden, dus stoppen.
								}
							}
						}
						break;	// go back into the while loop
					}
				}
			}

			// eentje meer ingevuld
			numMisLeft--;
		}
		delete[] misCols;
	}

	// Calculate Accuracy, print it.
	double accuracy = CalculateEstimationAccuracy(dmgdb, estVals, misVals, numMisVals);
	printf("%f\n", accuracy);

	// delete network
	for(uint32 col=0; col<numCols; col++) {
		delete[] colVals[col];
		if(nWorkNumCnd[col] == 0) { 
			delete nWorkCnd[col][0];
			delete[] nWorkProbs[col][0];
		} else {
			for(uint32 cnd=0; cnd<nWorkNumCnd[col]; cnd++) {
				delete nWorkCnd[col][cnd];
				delete[] nWorkProbs[col][cnd];
			}
		}
		delete[] nWorkCnd[col];
		delete[] nWorkProbs[col];
		delete[] parentCols[col];
	}
	delete[] colVals;
	delete[] nWorkCnd;
	delete[] nWorkNumCnd;
	delete[] nWorkProbs;
	delete[] parentCols;
	delete[] parentNums;

	// delete estimates and missing values
	for(uint32 i=0; i<numRows; i++) {
		delete[] misVals[i];
		delete[] estVals[i];
	}
	delete[] misVals;
	delete[] estVals;
	delete[] numMisVals;

	// delete tmp variables
	delete[] auxValArr;
	delete[] buf;

	// delete db and dmgdb
	delete dmgdb;
	delete db;
}

void MissingValuesTH::Miss() {
	string dbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(dbName);
	if(!db->HasColumnDefinition()) 
		{ delete db; throw string("Current Missing Value Estimation approach requires a column definition."); }

	string dirname = Bass::GetWorkingDir() + dbName + TimeUtils::GetTimeStampString() + "/";
	Bass::SetWorkingDir(dirname);

	// Control Chaos, or not
	uint32 seed = mConfig->Read<uint32>("chaos",0);
	if(seed > 0)	RandomUtils::Init(seed);
	else			RandomUtils::Init();

	// Determine number of Cross-Validation Folds
	uint32 numFolds = mConfig->Read<uint32>("numfolds", 1);

	// Determine anti-Database Weapon
	string weaponStr = mConfig->Read<string>("weapon", "");
	DatabaseDamageTechnique weapon = StringToDBDt(weaponStr);

	// Determine Estimation Techniques
	string methodStr = mConfig->Read<string>("method", "");
	uint32 numMethods = 0;
	string *methodsStr = StringUtils::Tokenize(methodStr, numMethods, ",", " ");
	double *accuracies = new double[numMethods];

	uint32 numeps = 0;
	uint32 *epss = StringUtils::TokenizeUint32(mConfig->Read<string>("eps","1"),numeps,",");
//	uint32 eps = mConfig->Read<uint32>("eps", (uint32)ceil(((double)db->GetNumRows() / (double)100)));

	double **epsds = new double*[numMethods];
	EstimationTechnique *methods = new EstimationTechnique[numMethods];
	for(uint32 m=0; m<numMethods; m++) {
		epsds[m] = new double[numeps];
		for(uint32 e=0; e<numeps; e++)
			epsds[m][e] = 0.0;
		methods[m] = StringToEMVt(methodsStr[m]);
		accuracies[m] = 0.0;
	}

	string estOutFileStr = Bass::GetWorkingDir() + "estOutfile.txt";
	mEstOutFile = fopen(estOutFileStr.c_str(), "w");

	DmgWhen whenDmg = mConfig->Read<string>("dmgwhen","first").compare("first") == 0 ? DmgFirst : DmgLast;

	uint32 numReps = mConfig->Read<uint32>("numReps",1);
	for(uint32 i=0; i<numReps; i++) {
		printf("-- Starting Repetition %d --\n", i);
		if(numFolds == 1) {
			uint32 numRows = db->GetNumRows();

			CodeTable *ct = NULL;
			Database *dmgdb = NULL;
			uint16 ** misVals = new uint16*[numRows];
			uint16 * numMisVals = new uint16[numRows];
			uint16 ** estVals = new uint16*[numRows];
			for(uint32 i=0; i<numRows; i++) {
				misVals[i] = new uint16[db->GetMaxSetLength()];
				estVals[i] = new uint16[db->GetMaxSetLength()];
				numMisVals[i] = 0;
			}

			if(whenDmg == DmgLast) {
				// Ritsel de code table
				// haha, nu eventjes maar!
				ct = NULL; // FicMain::ProvideCodeTable(db, mConfig, false);

				// Damage to the Database!
				dmgdb = DamageDatabase(db, weapon, misVals, numMisVals);
			} else {
				// Damage to the Database!
				dmgdb = DamageDatabase(db, weapon, misVals, numMisVals);

				// Ritsel de code table
				ct = FicMain::CreateCodeTable(dmgdb, mConfig, false);
			}
			uint32 maxMis = 0;
			for(uint32 x=0; x<numRows; x++) {
				if(numMisVals[x] > maxMis)
					maxMis = numMisVals[x];
			} printf("maxMis: %d\n", maxMis);

			bool mined = false;
			string iscTag = mConfig->Read<string>("iscname");
			ItemSetCollection *origFIS = FicMain::ProvideItemSetCollection(iscTag, db, mined, false, true);
			for(uint32 m=0; m<numMethods; m++) {
				// Yo! MVE Raps!
				ColumnBasedEstimation(dmgdb, ct, methods[m], estVals);

				// Calculate Accuracy
				accuracies[m] += CalculateEstimationAccuracy(dmgdb, estVals, misVals, numMisVals);
				uint32 *epsd = CalculateEpsDeltaAccuracy(dmgdb, origFIS, estVals, misVals, numMisVals);
				uint32 nfis = origFIS->GetNumLoadedItemSets();
				for(uint32 ce=0; ce<numeps; ce++) {
					uint32 csum = 0, cnum = 0;
					uint32 eps = epss[ce];
					for(uint32 e=0; e<=eps; e++) {
						cnum += epsd[e];
					}
					for(uint32 e=0; e<numRows; e++) {
						csum += epsd[e];
					}
					if(csum != nfis)
						printf("nfis: %d, but csum: %d\n", nfis, csum);
					epsds[m][ce] += ((double)nfis - (double)cnum) / (double)nfis;
				}
				delete[] epsd;
			}
			delete origFIS;
			for(uint32 i=0; i<numRows; i++) {
				delete[] misVals[i];
				delete[] estVals[i];
			}
			delete[] misVals;
			delete[] estVals;
			delete[] numMisVals;
			delete dmgdb;
			delete ct;
		} else {
			Database **foldsDBs = db->SplitForCrossValidationIntoDBs(numFolds);
			Database **inGroupParts = new Database*[numFolds-1];
			for(uint32 f=0; f<numFolds; f++) {
				uint32 numRows = db->GetNumRows();

				uint16 ** misVals = new uint16*[numRows];
				uint16 * numMisVals = new uint16[numRows];
				uint16 ** estVals = new uint16*[numRows];
				for(uint32 i=0; i<numRows; i++) {
					misVals[i] = new uint16[db->GetMaxSetLength()];
					estVals[i] = new uint16[db->GetMaxSetLength()];
					numMisVals[i] = 0;
				}

				// Group the ingroups together into one database
				uint32 curIn = 0;
				for(uint32 g=0; g<numFolds; g++)
					if(g != f)
						inGroupParts[curIn++] = foldsDBs[g];
				Database *inGroupDB = Database::Merge(inGroupParts, numFolds-1);

				// Damage to the Database!
				Database *dmgdb = DamageDatabase(foldsDBs[f], weapon, misVals, numMisVals);

				// Ritsel de code table
				CodeTable *ct = FicMain::CreateCodeTable(inGroupDB, mConfig, false);

				// Appliceer de correctieve
				if(mConfig->Read<uint32>("lpcOffset",0) == 1)
					ct->AddOneToEachUsageCount();

				bool mined = false;
				string iscTag = mConfig->Read<string>("foldiscname");
				ItemSetCollection *origFIS = FicMain::ProvideItemSetCollection(iscTag, foldsDBs[f], mined, false, true);
				uint32 nfis = origFIS->GetNumLoadedItemSets();
				for(uint32 m=0; m<numMethods; m++) {
					// Yo! MVE Raps!
					ColumnBasedEstimation(dmgdb, ct, methods[m], estVals);

					// Calculate Accuracy
					accuracies[m] += CalculateEstimationAccuracy(dmgdb, estVals, misVals, numMisVals);

					uint32 *epsd = CalculateEpsDeltaAccuracy(dmgdb, origFIS, estVals, misVals, numMisVals);
					for(uint32 cureps=0; cureps<numeps; cureps++) {
						uint32 csum = 0, cnum = 0;
						uint32 eps = epss[cureps];
						for(uint32 e=0; e<eps; e++) {
							if(epsd[e] != 0) {
								cnum += epsd[e];
							} 
						}
						epsds[m][cureps] += ((double)nfis - (double)cnum) / (double)nfis;
					}
					delete[] epsd;
				}
				delete origFIS;

				for(uint32 i=0; i<numRows; i++) {
					delete[] misVals[i];
					delete[] estVals[i];
				}
				delete[] misVals;
				delete[] estVals;
				delete[] numMisVals;
				delete ct;
				delete dmgdb;
				delete inGroupDB;
			}
			delete[] inGroupParts;
			for(uint32 f=0; f<numFolds; f++)
				delete foldsDBs[f];
			delete[] foldsDBs;
		}
	}
	for(uint32 m=0; m<numMethods; m++) {
		accuracies[m] = accuracies[m] / numFolds / numReps;
		printf("%d-CV MVE-Accuracy: %.2f (e,d)", numFolds, accuracies[m]*100);
		for(uint32 ce=0; ce<numeps; ce++) {
			epsds[m][ce] = epsds[m][ce] / numFolds / numReps;
			printf(" (%d,%.2f)",epss[ce],epsds[m][ce]);
		}
		printf(" (%s)\n", methodsStr[m].c_str());
	}
	// fprint accurs
	for(uint32 m=0; m<numMethods; m++) {
		fprintf(mEstOutFile,"\t%.2f", accuracies[m]*100);
	}
	fprintf(mEstOutFile,"\n");
	// fprint epsdeltas
	for(uint32 ce=0; ce<numeps; ce++) {
		fprintf(mEstOutFile, "%d", epss[ce]);
		for(uint32 m=0; m<numMethods; m++) {
			fprintf(mEstOutFile,"\t%.3f", epsds[m][ce]);
		}
		fprintf(mEstOutFile,"\n");
	}
	fclose(mEstOutFile);

	for(uint32 m=0; m<numMethods; m++)
		delete[] epsds[m];
	delete[] methodsStr;
	delete[] methods;
	delete[] accuracies;
	delete[] epsds;
	delete[] epss;

	delete db;
}

Database* MissingValuesTH::DamageDatabase(Database * const db, DatabaseDamageTechnique weapon, uint16 **misVals, uint16 *numMisVals) {
	if(weapon == DBDtShotgun)
		return ShotgunDB(db, misVals, numMisVals);
	else if(weapon == DBDtDepShotgun)
		return DepShotgunDB(db, misVals, numMisVals);
	else if(weapon == DBDtGuts)
		return GutsDB(db, misVals, numMisVals);
	else if(weapon == DBDtSnipe)
		return SnipeDB(db, misVals, numMisVals);
	else if(weapon == DBDtDepSnipe)
		return DepSnipeDB(db, misVals, numMisVals);
	throw string("MissingValueTH::DamageDatabase - Unknown DatabaseDamageTechnique");
}

double MissingValuesTH::CalculateEstimationAccuracy(Database *dmgdb, uint16 **estVals, uint16 **misVals, uint16 *numMisVals) {
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

uint32* MissingValuesTH::CalculateEpsDeltaAccuracy(Database *dmgdb, ItemSetCollection *origFIS, uint16 **estVals, uint16 **misVals, uint16 *numMisVals) {
	Database *edb = new Database(dmgdb);

	ItemSet **rows = edb->GetRows();
	uint32 numRows = edb->GetNumRows();
	uint32 numRight = 0;
	uint32 numGuesses = 0;
	uint32 maxNumDifs = numRows * (mConfig->Read<uint32>("numfolds",1) + 1);
	uint32 *difs = new uint32[maxNumDifs];
	for(uint32 i=0; i<numRows; i++) {
		difs[i] = 0;
		for(uint32 e=0; e<numMisVals[i]; e++) {
			rows[i]->AddItemToSet(estVals[i][e]);
		}
	}
	for(uint32 i=numRows; i<maxNumDifs; i++)
		difs[i] = 0;
	edb->CountAlphabet();
	edb->ComputeStdLengths();
	edb->ComputeEssentials();

	ItemSet ** fis = origFIS->GetLoadedItemSets();
	uint32 numfis = origFIS->GetNumLoadedItemSets();

	for(uint32 i=0; i<numfis; i++) {
		uint32 cnt = 0;
		for(uint32 r=0; r<numRows; r++) {
			if(rows[r]->IsSubset(fis[i]))
				cnt++;
		}
		int32 ldif = fis[i]->GetSupport() - cnt;
		difs[abs(ldif)]++;
	}

	delete edb;
	return difs;
}


EstimationTechnique MissingValuesTH::StringToEMVt(string emvt) {
	//if(emvt.compare("mostuseditemset") == 0)
//		return EMVtMostUsedItemSet;
//	else if(emvt.compare("mostuseditem") == 0)
//		return EMVtMostUsedItem;
	if(emvt.compare("mostfrequentitem") == 0)
		return EMVtMostFrequentItem;
	else if(emvt.compare("encodedlength") == 0)
		return EMVtShortestEncodedLength;
	else if(emvt.compare("krimpcompletion") == 0)
		return EMVtKrimpCompletion;
	else if(emvt.compare("krimpminimisation") == 0)
		return EMVtKrimpMinimisation;
	else if(emvt.compare("random") == 0)
		return EMVtRandomChoice;
	throw string("MissingValuesTH::StringToEMVt - Unknown EstimationTechnique: " + emvt);
}
DatabaseDamageTechnique MissingValuesTH::StringToDBDt(string dbw) {
	if(dbw.compare("shotgun") == 0)
		return DBDtShotgun;
	else if(dbw.compare("depshotgun") == 0)
		return DBDtDepShotgun;
	else if(dbw.compare("snipe") == 0)
		return DBDtSnipe;
	else if(dbw.compare("depsnipe") == 0)
		return DBDtDepSnipe;
	else if(dbw.compare("guts") == 0)
		return DBDtGuts;
	throw string("MissingValuesTH::StringToDBDt - Unknown DatabaseDamageTechnique: " + dbw);
}

void MissingValuesTH::ColumnBasedEstimation(Database *dmgdb, CodeTable *ct, EstimationTechnique method, uint16 **estVals) {
	ItemSet **rows = dmgdb->GetRows();
	uint32 numRows = dmgdb->GetNumRows();
	size_t abSize = dmgdb->GetAlphabetSize();

	ItemSet **colDef = dmgdb->GetColumnDefinition();
	uint32 numCols = dmgdb->GetNumColumns();

	ItemSet **abElems = new ItemSet*[abSize];
	islist *ctElems = NULL;
	if(ct != NULL) {
		islist *ctElems = ct->GetItemSetList();
		for(uint32 i=0; i<abSize; i++)
			abElems[i] = ItemSet::CreateSingleton(dmgdb->GetDataType(), i, (uint32) abSize, ct->GetAlphabetCount(i));
	} else {
		for(uint32 i=0; i<abSize; i++)
			abElems[i] = NULL;
	}

	uint16 *auxValArr = new uint16[abSize];

	if(method == EMVtKrimpMinimisation) {
		// guess values using KC
		Database *gdb = new Database(dmgdb);
		ItemSet **gdbRows = gdb->GetRows();

		// init-KC
		for(uint32 r=0; r<numRows; r++) {
			// determine missing columns
			uint32 numMisCols = 0;
			ItemSet **missingCols = DetermineMissingColumns(rows[r], colDef, numCols, numMisCols);

			uint32 numPosEstimates = 0;
			uint16 **posEstimates = DeterminePosEstimates(numMisCols, missingCols, numPosEstimates);

			double *encSzs = new double[numPosEstimates];
			double *powSzs = new double[numPosEstimates];

			double sumPowSzs = 0;
			for(uint32 i=0; i<numPosEstimates; i++) {
				for(uint32 c=0; c<numMisCols; c++)
					rows[r]->AddItemToSet(posEstimates[i][c]);
				encSzs[i] = ct->CalcTransactionCodeLength(rows[r]);
				for(uint32 c=0; c<numMisCols; c++)
					rows[r]->RemoveItemFromSet(posEstimates[i][c]);
				powSzs[i] = pow(2, (-encSzs[i]));
				sumPowSzs += powSzs[i];
			}

			double rnd = RandomUtils::UniformDouble(sumPowSzs);
			uint32 estIdx = 0; 
			for(uint32 i=0; i<numPosEstimates; i++) {
				if(rnd > powSzs[i]) {
					rnd -= powSzs[i];
				} else {
					estIdx = i;
					break;
				}
			}

			for(uint32 c=0; c<numMisCols; c++) {
				gdbRows[r]->AddItemToSet(posEstimates[estIdx][c]);
				estVals[r][c] = posEstimates[estIdx][c];
			}

			delete[] encSzs;
			delete[] powSzs;
			for(uint32 i=0; i<numPosEstimates; i++)
				delete[] posEstimates[i];
			delete[] posEstimates;
			delete[] missingCols;
		}
		gdb->CountAlphabet();
		gdb->ComputeStdLengths();
		gdb->ComputeEssentials();
		// and now KM

		double prevEncSz = DOUBLE_MAX_VALUE;
		CodeTable *kmct = NULL;

		while(true) {
			kmct = FicMain::CreateCodeTable(gdb, mConfig, false);
			double curEncSz = kmct->GetCurSize();
			if(curEncSz >= prevEncSz) {
				break;
			} else {
				// better fit of the data, so re-guess.
				prevEncSz = curEncSz;
				kmct->AddOneToEachUsageCount();

				Database *ngdb = new Database(dmgdb);
				ItemSet **ngdbRows = ngdb->GetRows();

				for(uint32 r=0; r<numRows; r++) {
					uint32 numMisCols = 0;
					ItemSet **missingCols = DetermineMissingColumns(rows[r], colDef, numCols, numMisCols);

					uint32 numPosEstimates = 0;
					uint16 **posEstimates = DeterminePosEstimates(numMisCols, missingCols, numPosEstimates);

					double *encSzs = new double[numPosEstimates];
					double *powSzs = new double[numPosEstimates];

					double sumPowSzs = 0;
					for(uint32 i=0; i<numPosEstimates; i++) {
						for(uint32 c=0; c<numMisCols; c++)
							ngdbRows[r]->AddItemToSet(posEstimates[i][c]);
						encSzs[i] = kmct->CalcTransactionCodeLength(ngdbRows[r]);
						for(uint32 c=0; c<numMisCols; c++)
							ngdbRows[r]->RemoveItemFromSet(posEstimates[i][c]);
						powSzs[i] = pow(2, (-encSzs[i]));
						sumPowSzs += powSzs[i];
					}

					double rnd = RandomUtils::UniformDouble(sumPowSzs);
					uint32 estIdx = 0; 
					for(uint32 i=0; i<numPosEstimates; i++) {
						if(rnd > powSzs[i]) {
							rnd -= powSzs[i];
						} else {
							estIdx = i;
							break;
						}
					}

					for(uint32 c=0; c<numMisCols; c++) {
						ngdbRows[r]->AddItemToSet(posEstimates[estIdx][c]);
						estVals[r][c] = posEstimates[estIdx][c];
					}

					delete[] missingCols;
					delete[] encSzs;
					delete[] powSzs;
					for(uint32 i=0; i<numPosEstimates; i++)
						delete[] posEstimates[i];
					delete[] posEstimates;
				}

				delete kmct;
				delete gdb;
				gdb = ngdb;
				gdb->CountAlphabet();
				gdb->ComputeStdLengths();
				gdb->ComputeEssentials();
			}
		}
		delete gdb;
		delete kmct;
	} else {
		// not krimp-minimisation, but row-based

		for(uint32 r=0; r<numRows; r++) {
			uint32 numMisCols = 0;
			ItemSet **missingCols = DetermineMissingColumns(rows[r], colDef, numCols, numMisCols);

			if(method == EMVtShortestEncodedLength) {
				double minEncLen = DOUBLE_MAX_VALUE;
				uint32 minEncIdx = 0;

				uint32 numPosEstimates = 0;
				uint16 **posEstimates = DeterminePosEstimates(numMisCols, missingCols, numPosEstimates);
				for(uint32 i=0; i<numPosEstimates; i++) {
					for(uint32 c=0; c<numMisCols; c++)
						rows[r]->AddItemToSet(posEstimates[i][c]);
					double curEncLen = ct->CalcTransactionCodeLength(rows[r]);
					for(uint32 c=0; c<numMisCols; c++)
						rows[r]->RemoveItemFromSet(posEstimates[i][c]);
					if(curEncLen < minEncLen) {
						minEncLen = curEncLen;
						minEncIdx = i;
					}
				}

				for(uint32 c=0; c<numMisCols; c++) {
					estVals[r][c] = posEstimates[minEncIdx][c];
				}

				for(uint32 i=0; i<numPosEstimates; i++)
					delete[] posEstimates[i];
				delete[] posEstimates;

			} else if(method == EMVtKrimpCompletion) {
				uint32 numPosEstimates = 0;
				uint16 **posEstimates = DeterminePosEstimates(numMisCols, missingCols, numPosEstimates);

				double *encSzs = new double[numPosEstimates];
				double *powSzs = new double[numPosEstimates];

				double sumPowSzs = 0;
				for(uint32 i=0; i<numPosEstimates; i++) {
					for(uint32 c=0; c<numMisCols; c++)
						rows[r]->AddItemToSet(posEstimates[i][c]);
					encSzs[i] = ct->CalcTransactionCodeLength(rows[r]);
					for(uint32 c=0; c<numMisCols; c++)
						rows[r]->RemoveItemFromSet(posEstimates[i][c]);
					powSzs[i] = pow(2, (-encSzs[i]));
					sumPowSzs += powSzs[i];
				}

				double rnd = RandomUtils::UniformDouble(sumPowSzs);
				uint32 estIdx = 0; 
				for(uint32 i=0; i<numPosEstimates; i++) {
					if(rnd > powSzs[i]) {
						rnd -= powSzs[i];
					} else {
						estIdx = i;
						break;
					}
				}

				for(uint32 c=0; c<numMisCols; c++) {
					estVals[r][c] = posEstimates[estIdx][c];
				}

				delete[] encSzs;
				delete[] powSzs;
				for(uint32 i=0; i<numPosEstimates; i++)
					delete[] posEstimates[i];
				delete[] posEstimates;

			} else if(method == EMVtRandomChoice) {
				for(uint32 c=0; c<numMisCols; c++) {
					missingCols[c]->GetValuesIn(auxValArr);
					estVals[r][c] = RandomUtils::UniformChoose(auxValArr, missingCols[c]->GetLength());
				}
			} else if(method == EMVtMostFrequentItem) {
				for(uint32 c=0; c<numMisCols; c++) {
					missingCols[c]->GetValuesIn(auxValArr);
					uint32 maxCnt = 0;
					uint16 maxItem = -1;
					alphabet *ab = dmgdb->GetAlphabet();
					for(uint32 v=0; v<missingCols[c]->GetLength(); v++) {
						if(ab->find(auxValArr[v])->second > maxCnt) {
							maxCnt = ab->find(auxValArr[v])->second;
							maxItem = auxValArr[v];
						}						
					}
					estVals[r][c] = maxItem;
				}
			}

			delete[] missingCols;
		}


	}


	delete[] auxValArr;


	for(uint32 i=0; i<abSize; i++)
		delete abElems[i];
	delete[] abElems;
	delete ctElems;
}

ItemSet** MissingValuesTH::DetermineMissingColumns(ItemSet *row, ItemSet** colDef, uint32 numCols, uint32 &numMisCols) {
	ItemSet **misCols = new ItemSet*[numCols];
	numMisCols = 0;
	for(uint32 c=0; c<numCols; c++) {
		if(!row->Intersects(colDef[c])) {
			misCols[numMisCols] = colDef[c];
			numMisCols++;
		}
	}
	return misCols;
}

ItemSet* MissingValuesTH::ChooseMostUsedItemSet(ItemSet **ctElems, uint32 numCTElems) {
	uint32 maxCount = ctElems[0]->GetUsageCount();
	ItemSet *maxElem = ctElems[0];
	for(uint32 i=1; i<numCTElems; i++) {
		if(ctElems[i]->GetUsageCount() > maxCount) {
			maxCount = ctElems[i]->GetUsageCount();
			maxElem = ctElems[i];
		}
	}
	return maxElem;
}

uint16 MissingValuesTH::DetermineMostUsedItem(uint16 *colItems, uint32 numColItems, ItemSet **ctElems, uint32 numCTElems) {
	uint32 *itemCounts = new uint32[numColItems];
	for(uint32 i=0; i<numColItems; i++)
		itemCounts[i] = 0;

	for(uint32 i=0; i<numCTElems; i++) {
		for(uint32 c=0; c<numColItems; c++) {
			if(ctElems[i]->IsItemInSet(colItems[c])) {
				itemCounts[c] += ctElems[i]->GetUsageCount();
				break;
			}
		}
	}

	uint16 maxItem = colItems[0];
	uint32 maxItemCount = itemCounts[0];
	for(uint32 c=1; c<numColItems; c++) {
		if(itemCounts[c] > maxItemCount) {
			maxItemCount = itemCounts[c];
			maxItem = colItems[c];
			break;
		}
	}
	delete[] itemCounts;

	return maxItem;
}

Database* MissingValuesTH::ShotgunDB(Database *db, uint16 **misVals, uint16 *numMisVals) {
	Database *dmgdb = new Database(db);
	size_t abSize = db->GetAlphabetSize();
	ItemSet **rows = dmgdb->GetRows();
	uint32 numRows = dmgdb->GetNumRows();
	uint32 numCols = dmgdb->GetNumColumns();

	uint32 numShots = mConfig->Read<uint32>("numshots",1);
	if(numShots > numCols)
		throw string("cannot create more damage than there are columns");
	uint32 numHoles =  numShots * numRows;

	uint16 *auxValArr = new uint16[abSize];
	for(uint32 i=0; i<numHoles; i++) {
		uint32 rndRowIdx = RandomUtils::UniformUint32(numRows);
		rows[rndRowIdx]->GetValuesIn(auxValArr);

		uint16 misVal = RandomUtils::UniformChoose(auxValArr, rows[rndRowIdx]->GetLength());
		rows[rndRowIdx]->RemoveItemFromSet(misVal);
		misVals[rndRowIdx][numMisVals[rndRowIdx]] = misVal;
		numMisVals[rndRowIdx]++;
	}

	delete[] auxValArr;

	return dmgdb;
}



Database* MissingValuesTH::DepShotgunDB(Database *db, uint16** misVals, uint16* numMisVals) {
	Database *dmgdb = new Database(db);
	size_t abSize = db->GetAlphabetSize();
	ItemSet **rows = dmgdb->GetRows();
	uint32 numRows = dmgdb->GetNumRows();

	uint32 numHoles = mConfig->Read<uint32>("numshots",1) * numRows;

	uint16 *classDef = dmgdb->GetClassDefinition();
	uint32 numClasses = dmgdb->GetNumClasses();
	ItemSet **colDef = dmgdb->GetColumnDefinition();
	uint32 numCols = dmgdb->GetNumColumns();
	double **colZapProbs = new double*[numClasses];
	for(uint32 cl=0; cl<numClasses; cl++) {
		colZapProbs[cl] = new double[numCols];
		double chanceSum = 0.0;
		for (uint32 i=0; i<numCols; i++) {
			if(!colDef[i]->IsItemInSet(classDef[0])) {
				colZapProbs[cl][i] = RandomUtils::UniformDouble();
			} else {
				colZapProbs[cl][i] = 0.0;
			}
			chanceSum += colZapProbs[cl][i];
		}
		for (uint32 i=0; i<numCols; i++) {
			colZapProbs[cl][i] /= chanceSum;
		}
	}

	uint16 *auxValArr = new uint16[abSize];
	for(uint32 i=0; i<numHoles; i++) {
		uint32 rndRowIdx = RandomUtils::UniformUint32(numRows);

		uint32 curClass = 0;
		for(uint32 cl=0; cl<numClasses; cl++) {
			if(rows[rndRowIdx]->IsItemInSet(classDef[cl])) {
				curClass = cl;
				break;
			}
		}

		while(true) {
			double rndZap = RandomUtils::UniformDouble();
			uint32 curCol = 0;
			for(uint32 co=0; co<numCols; co++) {
				rndZap -= colZapProbs[curClass][co];
				if(rndZap <= 0 && colZapProbs[curClass][co] > 0) {
					curCol = co;
					break;
				}
			}

			if(rows[rndRowIdx]->Intersects(colDef[curCol])) {
				// this column still exists
				colDef[curCol]->GetValuesIn(auxValArr);
				uint32 colLen = colDef[curCol]->GetLength();
				for(uint32 j=0; j<colLen; j++) {
					if(rows[rndRowIdx]->IsItemInSet(auxValArr[j])) {
						rows[rndRowIdx]->RemoveItemFromSet(auxValArr[j]);
						misVals[rndRowIdx][numMisVals[rndRowIdx]] = auxValArr[j];
						numMisVals[rndRowIdx]++;
						break;
					}
				}
				break;
			}
		}
	}

	delete[] auxValArr;

	for(uint32 cl=0; cl<numClasses; cl++) {
		delete[] colZapProbs[cl];
	} delete[] colZapProbs;

	return dmgdb;
}

Database* MissingValuesTH::SnipeDB(Database *db, uint16 **misVals, uint16 *numMisVals) {
	Database *dmgdb = new Database(db);
	size_t abSize = db->GetAlphabetSize();
	ItemSet **rows = dmgdb->GetRows();
	uint32 numRows = dmgdb->GetNumRows();

	uint32 numShots = mConfig->Read<uint32>("numShots",1);

	uint16 *auxValArr = new uint16[abSize];

	for(uint32 s=0; s<numShots; s++) {
		for(uint64 i=0; i<numRows; i++) {
			rows[i]->GetValuesIn(auxValArr);
			uint16 misVal = RandomUtils::UniformChoose(auxValArr, rows[i]->GetLength());
			rows[i]->RemoveItemFromSet(misVal);
			misVals[i][numMisVals[i]] = misVal;
			numMisVals[i]++;
		}
	}

	delete[] auxValArr;

	return dmgdb;
}

Database* MissingValuesTH::DepSnipeDB(Database *db, uint16** misVals, uint16* numMisVals) {
	Database *dmgdb = new Database(db);
	size_t abSize = dmgdb->GetAlphabetSize();
	ItemSet **rows = dmgdb->GetRows();
	uint32 numRows = dmgdb->GetNumRows();
	uint16 *classDef = dmgdb->GetClassDefinition();
	uint32 numClasses = dmgdb->GetNumClasses();
	ItemSet **colDef = dmgdb->GetColumnDefinition();
	uint32 numCols = dmgdb->GetNumColumns();

	uint32 numShots = mConfig->Read<uint32>("numShots",1);
	if(numShots > numCols)
		throw string("removing more than 100% data is impossible");

	uint16 *auxValArr = new uint16[abSize];

	uint32 clsColIdx = UINT32_MAX_VALUE;
	for(uint32 col=0;col<numCols; col++) {
		if(colDef[col]->IsItemInSet(classDef[0])) {
			clsColIdx = col;
			break;
		}
	}

	double **zapProbs = new double*[numClasses];
	for(uint32 cls=0; cls<numClasses; cls++) {
		zapProbs[cls] = new double[numCols];
		double prbSum = 0;
		for(uint32 col=0; col<numCols; col++) {
			if(col != clsColIdx) {
				zapProbs[cls][col] = RandomUtils::UniformDouble();
				prbSum += zapProbs[cls][col];
			} else {
				zapProbs[cls][col] = 0.0;
			}
		}
		// initiele probs uitgedeeld, nu normalisersn
		for(uint32 col=0; col<numCols; col++) {
			zapProbs[cls][col] /= prbSum;
		}
	}

	for(uint32 r=0; r<numRows; r++) {
		// figure out class of current row
		uint32 curClassIdx = UINT32_MAX_VALUE;
		for(uint32 cls=0; cls<numClasses; cls++) {
			if(rows[r]->IsItemInSet(classDef[cls])) {
				curClassIdx = cls;
				break;
			}
		}
		double maxProb = 1.0;
		for(uint32 s=0; s<numShots; s++) {
			// choose an available column to zap from
			uint32 chosenCol = UINT32_MAX_VALUE;
			while(true) {
				double rndZap = RandomUtils::UniformDouble(maxProb);
				chosenCol = UINT32_MAX_VALUE;
				for(uint32 col=0; col<numCols; col++) {
					if(zapProbs[curClassIdx][col] > 0) {
						// not the class label itself.
						rndZap -= zapProbs[curClassIdx][col];
						if(rndZap <= 0) {
							chosenCol = col;
							break;
						}
					}
				}
				if(chosenCol != UINT32_MAX_VALUE && colDef[chosenCol]->Intersects(rows[r]))	// found, and not yet zapped
					break;
			}

			// now zap that column
			colDef[chosenCol]->GetValuesIn(auxValArr);
			uint32 chosenColLen = colDef[chosenCol]->GetLength();
			for(uint32 v=0; v<chosenColLen; v++) {
				if(rows[r]->IsItemInSet(auxValArr[v])) {
					rows[r]->RemoveItemFromSet(auxValArr[v]);
					misVals[r][numMisVals[r]] = auxValArr[v];
					numMisVals[r]++;
					break;
				}
			}
		}
	}

	for(uint32 cls=0; cls<numClasses; cls++)
		delete[] zapProbs[cls];
	delete[] zapProbs;
	delete[] auxValArr;

	return dmgdb;
}


Database* MissingValuesTH::GutsDB(Database *db, uint16 **misVals, uint16 *numMisVals) {
	Database *dmgdb = new Database(db);
	ItemSet **rows = dmgdb->GetRows();
	uint32 numRows = dmgdb->GetNumRows();

	// Briefing on Targets
	uint32 numTargets = 0;
	string gutsStr = mConfig->Read<string>("gutstargets","");
	uint16 *gutsArr = StringUtils::TokenizeUint16(gutsStr, numTargets, ",");

	for(uint64 i=0; i<numRows; i++) {
		for(uint32 t=0; t<numTargets; t++) {
			if(rows[i]->IsItemInSet(gutsArr[t])) {
				misVals[i][numMisVals[i]] = gutsArr[t];
				numMisVals[i]++;
				rows[i]->RemoveItemFromSet(gutsArr[t]);
				break;
			}
		}
	}

	return dmgdb;
}

uint16** MissingValuesTH::DeterminePosEstimates(uint32 numMisCols, ItemSet** misCols, uint32 &numPosEstimates) {
	if(numMisCols == 0) {
		numPosEstimates = 0;
		return NULL;
	} else {
		numPosEstimates = 1;
		uint16 **mcVals = new uint16*[numMisCols];
		for(uint32 mc=0; mc<numMisCols; mc++) {
			numPosEstimates *= misCols[mc]->GetLength();
			mcVals[mc] = misCols[mc]->GetValues();
		}
		uint16 **posEstimates = new uint16*[numPosEstimates];
		uint32 curPosEstimateIdx = 0;
		uint16 *curChoices = new uint16[numMisCols];
		RecursiveDeterminePosEstimates(0,curPosEstimateIdx, posEstimates, curChoices, numMisCols, misCols, mcVals);

		delete[] curChoices;
		for(uint32 mc=0; mc<numMisCols; mc++)
			delete[] mcVals[mc];
		delete[] mcVals;

		// cleanup
		return posEstimates;
	}
}

void MissingValuesTH::RecursiveDeterminePosEstimates(uint32 curMisCol, uint32 &curPosEstimateIdx, uint16 **posEstimates, uint16* curChoices, uint32 numMisCols, ItemSet** misCols, uint16 **mcVals) {
	if(curMisCol < numMisCols) {
		uint32 numVals = misCols[curMisCol]->GetLength();
		for(uint32 v=0; v<numVals; v++) {
			curChoices[curMisCol] = mcVals[curMisCol][v];
			RecursiveDeterminePosEstimates(curMisCol+1,curPosEstimateIdx, posEstimates, curChoices, numMisCols, misCols, mcVals);
		}
	} else {
		posEstimates[curPosEstimateIdx] = new uint16[numMisCols];
		for(uint32 c=0; c<numMisCols; c++) {
			posEstimates[curPosEstimateIdx][c] = curChoices[c];
		}
		curPosEstimateIdx++;
	}
}

