#ifdef BLOCK_CLUSTER

// system
#include <time.h>
#if defined (_WINDOWS)
	#include <direct.h>
#endif
#if defined (__GNUC__)
	#include <cmath>
	using std::abs;
#endif
#include <algorithm>

#include "../../global.h"

// -- qtils
#include <Config.h>
#include <FileUtils.h>
#include <TimeUtils.h>
#include <logger/Log.h>

// -- bass
#include <db/Database.h>
#include <db/ClassedDatabase.h>
#include <db/DbFile.h>
#include <isc/ItemSetCollection.h>
#include <itemstructs/ItemSet.h>

#include "../../FicConf.h"
#include "../../FicMain.h"
#include "../../algo/Algo.h"
#include "../../tasks/DissimilarityTH.h"

#include "DBOntwarreraar.h"

DBOntwarreraar::DBOntwarreraar(Config *config) : Clusterer(config) {

}
DBOntwarreraar::~DBOntwarreraar() {

}

void DBOntwarreraar::Cluster() {
	DISABLE_LOGS();

	// Stap 1. Men neme een database
	mDbName = mConfig->Read<string>("dbname");
	Database *db = Database::RetrieveDatabase(mDbName);
	uint32 numRows = db->GetNumRows();
	Bass::SetMineToMemory(false);

	string ontwarLogPath = Bass::GetWorkingDir() + "owl.txt";
	FILE *fp = fopen(ontwarLogPath.c_str(),"w");


	// Stap 2. Men bestudere den opdracht
	uint32 numParts = mConfig->Read<uint32>("numparts",2);
	uint32 initStylee = DBONTWAR_INIT_RANDOM;
	string initStyleeStr = mConfig->Read<string>("initstylee");
	if(initStyleeStr.compare("random") == 0)
		initStylee = DBONTWAR_INIT_RANDOM;
	else if(initStyleeStr.compare("encsz") == 0)
		initStylee = DBONTWAR_INIT_ENCSZ;
	else if(initStyleeStr.compare("eszdif") == 0)
		initStylee = DBONTWAR_INIT_ENCSZDIFMEAN;
	uint32 ontwarStylee = DBONTWAR_STYLEE_KEEPSWAPPING;
	string ontwarStyleeStr = mConfig->Read<string>("ontwarstylee");
	if(ontwarStyleeStr.compare("keeponswappin") == 0)
		ontwarStylee = DBONTWAR_STYLEE_KEEPSWAPPING;
	else if(ontwarStyleeStr.compare("mimimize") == 0)
		ontwarStylee = DBONTWAR_STYLEE_MINIMIZESZ;
	bool useLaplace = mConfig->Read<bool>("uselaplace",true);
	bool directional = mConfig->Read<uint32>("directional") == 1 ? true : false;
	bool bestSzDifOnly = mConfig->Read<uint32>("bestszdif") == 1 ? true : false;


	// Stap 3. Men splitse de database en dele de rijen in
	Database **parts = NULL;
	if(initStylee == DBONTWAR_INIT_RANDOM) {
		// Stap 3a. Men randomizere de volgorde der rijen en splitsen
		time_t seed			= mConfig->Read<uint32>("seed", 0);
		if(seed == 0)		{ time(&seed); mConfig->Set("seed", seed); }
		db->RandomizeRowOrder((uint32)seed);
		parts = db->Split(numParts);
	} else if(initStylee == DBONTWAR_INIT_ENCSZ || initStylee == DBONTWAR_INIT_ENCSZDIFMEAN) {
		printf("Creating Codetable for initial sorting of rows...\n");
		CodeTable *ct = FicMain::RetrieveCodeTable(mConfig, db);
		if(ct == NULL) {
			ct = FicMain::CreateCodeTable(db, mConfig);
			string tag = mDbName + "-" + mConfig->Read<string>("isctype") + "-" + mConfig->Read<string>("iscminsup") + mConfig->Read<string>("iscorder") + "-" + mConfig->Read<string>("prunestrategy");
			string ctPath = Bass::GetCtReposDir() + tag + "/";
			FileUtils::CreateDir(ctPath);
			ct->WriteToDisk(ctPath + "ct-" + tag + "-" + TimeUtils::GetTimeStampString() + "-" + mConfig->Read<string>("iscminsup") + "-1337.ct");
		}
		printf("Done creating Codetable for initial sorting of rows...\n");

		double encSzSum = 0, encRowSz;
		double *enclens = new double[numRows];
		ItemSet **rows = db->GetRows();
		for(uint32 i=0; i<numRows; i++) {
			encRowSz = ct->CalcTransactionCodeLength(rows[i]);
			enclens[i] = encRowSz;
			encSzSum += encRowSz;
		}
		double encSzMean = encSzSum / numRows;

		if(initStylee == DBONTWAR_INIT_ENCSZ) {
			double tf;
			for(uint32 i=0; i<numRows-1; i++)
				for(uint32 j=i+1; j<numRows; j++)
					if(enclens[j] < enclens[i]) {
						db->SwapRows(i,j);
						tf = enclens[j];
						enclens[j] = enclens[i];
						enclens[i] = tf;
					}
		} else if(initStylee == DBONTWAR_INIT_ENCSZDIFMEAN) {
			double tf, di, dj;
			for(uint32 i=0; i<numRows-1; i++) {
				di = abs(enclens[i] - encSzMean);
				for(uint32 j=i+1; j<numRows; j++) {
					dj = abs(enclens[j] - encSzMean);
					if(dj < di) {
						db->SwapRows(i,j);
						tf = enclens[j];
						enclens[j] = enclens[i];
						enclens[i] = tf;
						di = dj;
					}
				}
			}
		}
		delete[] enclens;
		delete ct;

		parts = db->Split(numParts);
	}
	uint32 numReAssignments = db->GetNumRows();
	double *stLengths = db->GetStdLengths();

	ItemSet ***tempArr = new ItemSet**[numParts];
	Database **minParts = new Database*[numParts];
	for(uint32 i=0; i<numParts; i++) {
		tempArr[i] = new ItemSet*[db->GetNumRows()];
		minParts[i] = NULL;
	}


	// Stap 4. Men bereide de reassignatieadministratie voor
	uint32 *numRowsVanAnaarZ = new uint32[numParts];
	uint32 *numRowsVanZnaarA = new uint32[numParts];
	uint32 *numTransVanAnaarZ = new uint32[numParts];
	uint32 *numTransVanZnaarA = new uint32[numParts];
	uint32 *tempArrIdx = new uint32[numParts];
	uint32 **numRowsVanAnaarB = new uint32*[numParts];
	CodeTable **cts = new CodeTable*[numParts];
	CodeTable **minCts = new CodeTable*[numParts];
	for(uint32 i=0; i<numParts; i++) {
		tempArrIdx[i] = 0;
		numRowsVanAnaarZ[i] = 0;
		numRowsVanZnaarA[i] = 0;
		numRowsVanAnaarB[i] = new uint32[numParts];
		for(uint32 j=0; j<numParts; j++)
			numRowsVanAnaarB[i][j] = 0;
		cts[i] = NULL;
		minCts[i] = NULL;
	}
	uint32 noEncSzDif = 0;


	// Stap 5. Kongfuschizzle voorbereijding
	uint32 curNumOntwars = 0, minNumOntwars = 0;
	uint32 maxNumOntWars = mConfig->Read<uint32>("maxnumontwars",24);
	uint32 from = 0;
	uint32 numCRows = 0;
	uint32 numClasses = 0;
	uint32 *clToIdx = NULL;
	string cDbName = mConfig->Read<string>("cdbname","");
	bool doClassification = false;
	ClassedDatabase *cDB = NULL;
	if(cDbName.length() > 0) {
		doClassification = true;
		cDB = (ClassedDatabase*) Database::RetrieveDatabase(cDbName);
		numCRows = cDB->GetNumRows();
		numClasses = cDB->GetNumClasses();
		uint32 maxClass = 0;
		for(uint32 i=0; i<numClasses; i++)
			if(cDB->GetClassDefinition()[i] > maxClass)
				maxClass = cDB->GetClassDefinition()[i];
		clToIdx = new uint32[maxClass+1];
		for(uint32 i=0; i<numClasses; i++)
			clToIdx[cDB->GetClassDefinition()[i]] = i;
	}
	bool *partChanged = new bool[numParts];
	uint32 **curAssign = new uint32*[numParts];
	uint32 **minAssign = new uint32*[numParts];
	ItemSet ***rowsAr = new ItemSet**[numParts];
	uint32 *numRowsAr = new uint32[numParts];
	double curAccur = 0, minAccur = 0, curTotalEncSz = 0, prevTotalEncSz = 0, minTotalEncSz = 0;
	for(uint32 i=0; i<numParts; i++) {
		cts[i] = FicMain::CreateCodeTable(parts[i], mConfig);
		minCts[i] = cts[i];
		if(useLaplace)
			cts[i]->AddOneToEachUsageCount();
		else
			cts[i]->AddSTSpecialCode();
		cts[i]->CalcCodeTableSize();
		curTotalEncSz += cts[i]->GetCurStats().encSize;
		partChanged[i] = false;
		curAssign[i] = new uint32[numClasses];
		minAssign[i] = new uint32[numClasses];
	}
	minTotalEncSz = DOUBLE_MAX_VALUE;
	numReAssignments = 0;

	// Stap 6. Doorgaen zoolange de verdeling der transacties nog niet optimael seit
	while(true) {
		from = curNumOntwars % numParts;
		fprintf(fp, "Ontwarstap %d\n", curNumOntwars);

		prevTotalEncSz = curTotalEncSz;
		curTotalEncSz = 0;
		for(uint32 i=0; i<numParts; i++) {
			tempArrIdx[i] = 0;
			numRowsVanAnaarZ[i] = 0;
			numRowsVanZnaarA[i] = 0;
			numTransVanAnaarZ[i] = 0;
			numTransVanZnaarA[i] = 0;
			rowsAr[i] = parts[i]->GetRows();
			numRowsAr[i] = parts[i]->GetNumRows();
			for(uint32 j=0; j<numParts; j++) {
				numRowsVanAnaarB[i][j] = 0;
			}

			if(partChanged[i] == true) {
				if(minCts[i] != cts[i])
					delete cts[i];
				cts[i] = FicMain::CreateCodeTable(parts[i], mConfig, true);
				if(useLaplace)
					cts[i]->AddOneToEachUsageCount();
				else
					cts[i]->AddSTSpecialCode();
				cts[i]->CalcCodeTableSize();
				partChanged[i] = false;
			}
			curTotalEncSz += cts[i]->GetCurStats().encSize;
		}

		// Bewaer den mimimaalste codering
		if(ontwarStylee != DBONTWAR_STYLEE_MINIMIZESZ && curTotalEncSz < minTotalEncSz) {
			for(uint32 i=0; i<numParts; i++) {
				// CodeTable
				char tmp[256];
				sprintf_s(tmp, 256, "minCT%d.ct", i);	// evt wel numOntwars meenemen, en oude meenemen en oude files zappen?
				cts[i]->WriteToDisk(Bass::GetWorkingDir() + tmp);
				if(minCts[i] != cts[i]) {
					delete minCts[i];
					minCts[i] = cts[i];
				}
				// Database
				sprintf_s(tmp, 256, "minPart%d", i);
				parts[i]->Write(tmp,Bass::GetWorkingDir());
				delete minParts[i];
				minParts[i] = new Database(parts[i]);
			}

			minTotalEncSz = curTotalEncSz;
			minNumOntwars = curNumOntwars;
		}

		// Keep on dubbin?
		if(ontwarStylee == DBONTWAR_STYLEE_KEEPSWAPPING && numReAssignments == 0 && curNumOntwars > 0) {
			break;
		} else if(ontwarStylee == DBONTWAR_STYLEE_MINIMIZESZ && curTotalEncSz > prevTotalEncSz && curNumOntwars > 0) {
			break;
		} else if(curNumOntwars > maxNumOntWars) {
			break;
		}

		// Stap 6b. Bepael of den transacties in die optimaele dele verblijfen

		// J? dit niet slimmer door gewoon over rows te itereren, en ahv kortste code te verdelen ipv twee loops?
		// J? ook niet slimmer om een RemoveRows/AddRows te doen, die statistieken bijhouden?
		numReAssignments = 0;
		double bestSzDif = 0;
		double bestSzDifOrigSz = 0;
		double bestSzDifBestSz = 0;
		uint32 bestSzDifOrigPart = UINT32_MAX_VALUE;
		uint32 bestSzDifOrigRowIdx = UINT32_MAX_VALUE;
		uint32 bestSzDifBestPart = UINT32_MAX_VALUE;
		for(uint32 i=0; i<numParts; i++) {
			if(directional == false || from == i) {
				if(bestSzDifOnly == true) {
					for(uint32 j=0; j<numRowsAr[i]; j++) {
						double origSz = cts[i]->CalcTransactionCodeLength(rowsAr[i][j], stLengths);
						double bestSz = origSz;
						uint32 bestIdx = i;
						for(uint32 k=0; k<numParts; k++) {
							if(k != i) {
								double ctKsz = cts[k]->CalcTransactionCodeLength(rowsAr[i][j], stLengths);
								if(bestSz > ctKsz) {
									bestIdx = k;
									bestSz = ctKsz;
								} else if(ctKsz == origSz) {
									noEncSzDif++;
								}
							}
						}
						if(bestIdx != i) {
							double curSzDif = origSz - bestSz;
							if(curSzDif > bestSzDif) {
								bestSzDif = curSzDif;
								bestSzDifOrigPart = i;
								bestSzDifOrigRowIdx = j;
								bestSzDifBestPart = bestIdx;
								bestSzDifBestSz = bestSz;
								bestSzDifOrigSz = origSz;
							}
						}
					}
					memcpy(tempArr[i] + tempArrIdx[i],rowsAr[i], sizeof(ItemSet*) * numRowsAr[i]);
					tempArrIdx[i] += numRowsAr[i];
				} else {
					fprintf(fp, "Van %d naar Z:\n", i);
					for(uint32 j=0; j<numRowsAr[i]; j++) {
						double origSz = cts[i]->CalcTransactionCodeLength(rowsAr[i][j], stLengths);
						double bestSz = origSz;
						uint32 bestIdx = i;
						for(uint32 k=0; k<numParts; k++) {
							if(k != i) {
								double ctKsz = cts[k]->CalcTransactionCodeLength(rowsAr[i][j], stLengths);
								if(bestSz > ctKsz) {
									bestIdx = k;
									bestSz = ctKsz;
								} else if(ctKsz == origSz) {
									noEncSzDif++;
								}
							}
						}
						if(bestIdx != i) {
							fprintf(fp, "%.2f = %.2f - %.2f (%d) :: %s\n",origSz-bestSz,origSz,bestSz, bestIdx,rowsAr[i][j]->ToString(false,false).c_str());
							tempArr[bestIdx][tempArrIdx[bestIdx]++] = rowsAr[i][j];

							numRowsVanAnaarB[i][bestIdx]++;

							numRowsVanAnaarZ[i]++;
							numRowsVanZnaarA[bestIdx]++;

							numTransVanAnaarZ[i] += rowsAr[i][j]->GetSupport();
							numTransVanZnaarA[bestIdx] += rowsAr[i][j]->GetSupport();
						} else
							tempArr[i][tempArrIdx[i]++] = rowsAr[i][j];
					}
				}
			} else {
				fprintf(fp, "Van %d naar Z:\n", i);
				memcpy(tempArr[i] + tempArrIdx[i],rowsAr[i], sizeof(ItemSet*) * numRowsAr[i]);
				tempArrIdx[i] += numRowsAr[i];
			}
		}
		if(bestSzDifOnly == true && bestSzDifBestPart != bestSzDifOrigPart) {
			fprintf(fp, "%.2f = %.2f - %.2f (%d) :: %s\n",bestSzDif,bestSzDifOrigSz,bestSzDifBestSz, bestSzDifBestPart, rowsAr[bestSzDifOrigPart][bestSzDifOrigRowIdx]->ToString(false,false).c_str());

			// even de beste winst-rij naar het juiste part kopieren
			tempArr[bestSzDifBestPart][tempArrIdx[bestSzDifBestPart]++] = rowsAr[bestSzDifOrigPart][bestSzDifOrigRowIdx];
			numRowsVanAnaarB[bestSzDifOrigPart][bestSzDifBestPart]++;
			numRowsVanAnaarZ[bestSzDifOrigPart]++;
			numRowsVanZnaarA[bestSzDifBestPart]++;
			numTransVanAnaarZ[bestSzDifOrigPart] += rowsAr[bestSzDifOrigPart][bestSzDifOrigRowIdx]->GetSupport();
			numTransVanZnaarA[bestSzDifBestPart] += rowsAr[bestSzDifOrigPart][bestSzDifOrigRowIdx]->GetSupport();			

			// even de beste wint-rij weghalen uit het orig-part
			tempArr[bestSzDifOrigPart][bestSzDifOrigRowIdx] = tempArr[bestSzDifOrigPart][tempArrIdx[bestSzDifOrigPart]-1];	// in slechtste geval heeft ie maar 1 rij, en wordt er hier die dus gekopieerd,  maar omdat aantal rijen afneemt hebben we daar geen last van
			tempArrIdx[bestSzDifOrigPart] -= 1;
		}

		// Stap 6c. Men herdistribueere
		numReAssignments = 0;
		for(uint32 i=0; i<numParts; i++) {
			uint32 numChange = numRowsVanAnaarZ[i] + numRowsVanZnaarA[i];
			if(numChange > 0) {
				uint32 newArrSize = numRowsAr[i] - numRowsVanAnaarZ[i] + numRowsVanZnaarA[i];
				ItemSet **newArr = new ItemSet*[newArrSize];
				memcpy(newArr, tempArr[i], sizeof(ItemSet*) * newArrSize);
				numReAssignments += numRowsVanAnaarZ[i];

				delete[] rowsAr[i];
				parts[i]->SetRows(newArr);
				parts[i]->SetNumRows(newArrSize);
				parts[i]->SetNumTransactions(parts[i]->GetNumTransactions() - numTransVanAnaarZ[i] + numTransVanZnaarA[i]);
				parts[i]->CountAlphabet();
				parts[i]->SetMaxSetLength(0);
				parts[i]->ComputeEssentials();
				partChanged[i] = true;
			} else
				partChanged[i] = false;
		}

		for(uint32 i=0; i<numParts; i++) {
			for(uint32 j=0; j<numParts; j++) {
				if(j == i)
					continue;
				if(numRowsVanAnaarB[i][j] == 0)
					continue;
				printf("%d -> %d = %d\n", i, j, numRowsVanAnaarB[i][j]);
				fprintf(fp, "%d -> %d = %d\n", i, j, numRowsVanAnaarB[i][j]);
			}
		}
		for(uint32 i=0; i<numParts; i++) {
			printf("|%d|=%d  ", i, tempArrIdx[i]);
			fprintf(fp, "|%d|=%d  ", i, tempArrIdx[i]);
		}
		printf("\n");
		fprintf(fp, "\n");

		// Stap 6z. Qualiteitsbepaeling !!?? hier moet minParts nog ff meegenomen worden
		if(doClassification == true) {
			curAccur = 0;
			for(uint32 i=0; i<numParts; i++)
				for(uint32 j=0; j<numClasses; j++) {
					curAssign[i][j] = 0;
					minAssign[i][j] = 0;
				}

			for(uint32 i=0; i<numCRows; i++) {
				ItemSet *curCRow = cDB->GetRow(i);
				
				double curBestSz = DOUBLE_MAX_VALUE, minBestSz = DOUBLE_MAX_VALUE;
				uint32 curBestIdx = UINT32_MAX_VALUE, minBestIdx = UINT32_MAX_VALUE;
				for(uint32 j=0; j<numParts; j++) {
					double ctJsz = cts[j]->CalcTransactionCodeLength(curCRow, stLengths);
					double ctMsz = minCts[j]->CalcTransactionCodeLength(curCRow, stLengths);
					if(ctJsz < curBestSz) {
						curBestSz = ctJsz;
						curBestIdx = j;
					}
					if(ctMsz < minBestSz) {
						minBestSz = ctMsz;
						minBestIdx = j;
					}
				}
				curAssign[curBestIdx][clToIdx[cDB->GetClassLabel(i)]] += curCRow->GetUsageCount();
				minAssign[minBestIdx][clToIdx[cDB->GetClassLabel(i)]] += curCRow->GetUsageCount();
			}

			fprintf(fp, "cl");
			for(uint32 i=0; i<numClasses; i++)
				fprintf(fp, "\t%d", cDB->GetClassDefinition()[i]);
			fprintf(fp, "\n");

			for(uint32 i=0; i<numParts; i++) {
				fprintf(fp, "%d", i);
				uint32 curMax = 0;
				uint32 minMax = 0;
				for(uint32 j=0; j<numClasses; j++) {
					if(curAssign[i][j] > curMax)
						curMax = curAssign[i][j];
					fprintf(fp, "\t%d", curAssign[i][j]);
					if(minAssign[i][j] > minMax)
						minMax = minAssign[i][j];
				}
				curAccur += curMax;
				minAccur += minMax;
				fprintf(fp, "\n");
			}
			curAccur /= cDB->GetNumTransactions();
			minAccur /= cDB->GetNumTransactions();
			fprintf(fp, "\n");
		}

		fflush(fp);

		curNumOntwars++;
	}

	// Stap 7. Men doet het er maar mee.

	printf("database ontwarred in %d parts.\n", numParts);
	for(uint32 i=0; i<numParts; i++) {
		printf("part %d : %d rows, %d trans\n", i, parts[i]->GetNumRows(), parts[i]->GetNumTransactions());
	}
	for(uint32 i=0; i<numParts; i++) {
		char tmp[256];
		sprintf_s(tmp, 256, "ct%d.ct", i);
		cts[i]->WriteToDisk(Bass::GetWorkingDir() + tmp);
	}
	uint32 curNumPartsUsed = 0;
	uint32 minNumPartsUsed = 0;
	for(uint32 i=0; i<numParts; i++) {
		if(parts[i]->GetNumRows() > 0)
			curNumPartsUsed++;
		if(minParts[i] != NULL && minParts[i]->GetNumRows() > 0)
			minNumPartsUsed++;

		char tmp[256];
		sprintf_s(tmp, 256, "part%d", i);
		parts[i]->Write(tmp,Bass::GetWorkingDir());
	}

	// -- Rank transactions --
	/*Database *dbOriginal = Database::RetrieveDatabase(mDbName, Uint16ItemSetType);
	for(uint32 i=0; i<numParts; i++) {
		Database *part = parts[i];
		uint32 nR = part->GetNumRows();
		uint32 *ids = new uint32[nR];
		double *values = new double[nR];
		double value;
		ItemSet *is;
		uint32 s;
		bool *mask = new bool[numRows];
		for(uint32 s=0; s<numRows; s++)
			mask[s] = false;
		for(uint32 r=0; r<nR; r++) {
			is = part->GetRow(r);
			for(s=0; s<numRows; s++) {
				if(!mask[s] && dbOriginal->GetRow(s)->Equals(is))
					break;
			}
			if(s>=numRows)
				throw string("Row not found here!");
			ids[r] = s;
			values[r] = cts[i]->CalcTransactionCodeLength(is, stLengths);
			if(values[r] == 0.0) {
				uint32 la = 0;
				values[r] = cts[i]->CalcTransactionCodeLength(is, stLengths);
			}
			if(mConfig->Read<bool>("sortonspecificness", false)) {
				value = 0.0;
				for(uint32 j=0; j<numParts; j++) {
					if(j==i)
						continue;
					value += cts[j]->CalcTransactionCodeLength(is, stLengths);
				}
				value /= values[r];
				value -= numParts-1;
				value /= numParts-1;
				values[r] = value;
			}
			mask[s] = true;
		}
		delete[] mask;
		
		// bubblesort on value
		double t;
		if(mConfig->Read<bool>("sortonspecificness", false)) {
			for(uint32 k=1; k<nR; k++)										// descending
				for(uint32 j=0; j<nR-k; j++)
					if(values[j] < values[j+1]) {
						t = values[j];
						values[j] = values[j+1];
						values[j+1] = t;
						s = ids[j];
						ids[j] = ids[j+1];
						ids[j+1] = s;
					}

		} else {
			for(uint32 k=1; k<nR; k++)										// ascending
				for(uint32 j=0; j<nR-k; j++)
					if(values[j] > values[j+1]) {
						t = values[j];
						values[j] = values[j+1];
						values[j+1] = t;
						s = ids[j];
						ids[j] = ids[j+1];
						ids[j+1] = s;
					}
		}

		char tmp[256];
		sprintf_s(tmp, 256, "part%dids.txt", i);
		string filename = Bass::GetWorkingDir() + tmp;
		FILE *fp = fopen(filename.c_str(),"w");
		if(mConfig->Read<bool>("sortonspecificness", false))
			for(uint32 r=0; r<nR; r++)
				fprintf_s(fp, "%d\t%.2f\t%.2f\n", ids[r], values[r], cts[i]->CalcTransactionCodeLength(db->GetRow(ids[r]), stLengths));
		else
			for(uint32 r=0; r<nR; r++)
				fprintf_s(fp, "%d\t%.2f\n", ids[r], values[r]);
		fclose(fp);
		delete[] values;
		delete[] ids;
	}

	// -- Rank code table elements --
	for(uint32 i=0; i<numParts; i++) {
		CodeTable *ct = cts[i];
		islist *setList = cts[i]->GetItemSetList();
		islist *alphList = cts[i]->GetSingletonList();
		uint32 numElems = setList->size() + alphList->size();
		ItemSet **elems = new ItemSet *[numElems];
		islist::iterator iter;
		uint32 idx = 0;
		for(iter = setList->begin(); iter != setList->end(); ++iter)
			elems[idx++] = (ItemSet*)(*iter);
		for(iter = alphList->begin(); iter != alphList->end(); ++iter)
			elems[idx++] = (ItemSet*)(*iter);
		double *values = new double[numElems];
		double value;
		ItemSet *is;

		for(uint32 e=0; e<numElems; e++) {
			is = elems[e];
			values[e] = ct->CalcTransactionCodeLength(is, stLengths);
			if(mConfig->Read<bool>("sortonspecificness", false)) {
				value = 0.0;
				for(uint32 j=0; j<numParts; j++) {
					if(j==i)
						continue;
					value += cts[j]->CalcTransactionCodeLength(is, stLengths);
				}
				value /= values[e];
				value -= numParts-1;
				value /= numParts-1;
				values[e] = value;
			}
		}

		// bubblesort on value
		double t;
		if(mConfig->Read<bool>("sortonspecificness", false)) {
			for(uint32 k=1; k<numElems; k++)										// descending
				for(uint32 j=0; j<numElems-k; j++)
					if(values[j] < values[j+1]) {
						t = values[j];
						values[j] = values[j+1];
						values[j+1] = t;
						is = elems[j];
						elems[j] = elems[j+1];
						elems[j+1] = is;
					}

		} else {
			for(uint32 k=1; k<numElems; k++)										// ascending
				for(uint32 j=0; j<numElems-k; j++)
					if(values[j] > values[j+1]) {
						t = values[j];
						values[j] = values[j+1];
						values[j+1] = t;
						is = elems[j];
						elems[j] = elems[j+1];
						elems[j+1] = is;
					}
		}

		char tmp[256];
		sprintf_s(tmp, 256, "ct%dranking.txt", i);
		string filename = Bass::GetWorkingDir() + tmp;
		FILE *fp = fopen(filename.c_str(),"w");
		if(mConfig->Read<bool>("sortonspecificness", false)) {
			for(uint32 r=0; r<numElems; r++)
				if(elems[r]->GetCount() != 0)
					fprintf_s(fp, "%.2f\t%d\t%.2f\t%s\n", values[r], elems[r]->GetCount(), ct->CalcTransactionCodeLength(elems[r]), elems[r]->ToString().c_str());
		} else
			for(uint32 r=0; r<numElems; r++)
				fprintf_s(fp, "%.2f\t%d\t%s\n", values[r], elems[r]->GetCount(), elems[r]->ToString().c_str());
		fclose(fp);

		// cleanup
		delete setList;
		for(iter = alphList->begin(); iter != alphList->end(); ++iter)
			delete *iter;
		delete alphList;
		delete[] elems;
		delete[] values;
	}
	delete dbOriginal;
	*/

	// -- Other stuff(tm)
	char tmp[2048];
	char tmpFN[2048];
	string ps = mConfig->Read<string>("prunestrategy","la");
	sprintf_s(tmp, 2048, "%s\t%d\t%d\t%s\t%s\t%s\t%.2f\t%.3f\t%d",
		ps.c_str(),
		numParts,
		curNumPartsUsed,
		useLaplace ? "Laplace" : "ST",
		initStyleeStr.c_str(),
		directional == 0 ? (bestSzDifOnly == 0 ? "duplex" : "duplex1") : (bestSzDifOnly == 0 ? "directional" : "directional1"),
		curTotalEncSz,
		curAccur,
		curNumOntwars
	);
	sprintf_s(tmpFN, 2048, "_last-%s-%d-%s-%s-%s-%.2f-%.3f-%d.txt",
		ps.c_str(),
		numParts,
		useLaplace ? "lp" : "st",
		initStyleeStr.c_str(),
		directional == 0 ? (bestSzDifOnly == 0 ? "duplex" : "duplex1") : (bestSzDifOnly == 0 ? "directional" : "directional1"),
		curTotalEncSz,
		curAccur,
		curNumOntwars
		);

	FILE *la = fopen((Bass::GetWorkingDir() + tmpFN).c_str(), "w");
	fprintf(la, "%s\n", tmp);
	fclose(la);

	if(ontwarStylee != DBONTWAR_STYLEE_MINIMIZESZ) {
		sprintf_s(tmp, 2048, "%s\t%d\t%d\t%s\t%s\t%s\t%.2f\t%.3f\t%d",
			ps.c_str(),
			numParts,
			minNumPartsUsed,
			useLaplace ? "Laplace" : "ST",
			initStyleeStr.c_str(),
			directional == 0 ? (bestSzDifOnly == 0 ? "duplex" : "duplex1") : (bestSzDifOnly == 0 ? "directional" : "directional1"),
			minTotalEncSz,
			minAccur,
			minNumOntwars
			);
		sprintf_s(tmpFN, 2048, "_min-%s-%d-%s-%s-%s-%.2f-%.3f-%d.txt",
			ps.c_str(),
			numParts,
			useLaplace ? "lp" : "st",
			initStyleeStr.c_str(),
			directional == 0 ? (bestSzDifOnly == 0 ? "duplex" : "duplex1") : (bestSzDifOnly == 0 ? "directional" : "directional1"),
			minTotalEncSz,
			minAccur,
			minNumOntwars
			);

		la = fopen((Bass::GetWorkingDir() + tmpFN).c_str(), "w");
		fprintf(la, "%s\n", tmp);
		fclose(la);
	}

	fprintf(fp, "\n%s", tmp); 
	fclose(fp);

	for(uint32 i=0; i<numParts; i++) {
		if(cts[i] == minCts[i])
			minCts[i] = NULL;
		delete cts[i];
		delete minCts[i];

		delete parts[i];
		delete minParts[i];
		delete[] curAssign[i];
		delete[] minAssign[i];
		delete[] tempArr[i];
		delete[] numRowsVanAnaarB[i];
	}
	delete[] numRowsVanAnaarB;
	delete[] numRowsVanAnaarZ;
	delete[] numRowsVanZnaarA;
	delete[] numTransVanAnaarZ;
	delete[] numTransVanZnaarA;

	delete[] numRowsAr;
	delete[] rowsAr;

	delete[] cts;
	delete[] minCts;
	delete[] partChanged;
	delete[] parts;
	delete[] minParts;
	delete[] tempArr;
	delete[] tempArrIdx;
	delete[] curAssign;
	delete[] minAssign;
	delete[] clToIdx;

	delete cDB;
	delete db;
}

void DBOntwarreraar::Dissimilarity(const string &ontwarDir, bool minCTs) {
	Logger::SetLogToDisk(false);

	LOG_MSG(string("Computing dissimilarities with DBOntwar experiment found in directory:"));
	LOG_MSG("\t" + ontwarDir);
	char temp[200];

	// Determine the compression tag of the original compression run
	string tag = ontwarDir;
	if(tag.find_last_of("\\/") == tag.length()-1)			// remove trailing slash or backslash
		tag = tag.substr(0, tag.length()-1);
	tag = tag.substr(tag.find_last_of("\\/")+1);	// remove path before tag

	// Find and read the appropiate database
	mDbName = tag.substr(0, tag.find_first_of("-"));
	Database *db = Database::RetrieveDatabase(mDbName);

	// Find k
	size_t pos = 0;
	for(uint32 i=0; i<4; i++) {
		pos = tag.find_first_of("-", pos+1);
		if(pos == string::npos)
			throw string("DBOntwarreraar::Dissimilarity() -- Cannot find k in ontwarDir.");
	}
	pos++;
	uint32 k = atoi(tag.c_str() + pos);

	// Read CTs defining the clusters
	CodeTable **CTs = new CodeTable *[k];
	string filename;
	for(uint32 i=0; i<k; i++) {
		sprintf_s(temp, 100, "%d", i);
		filename = ontwarDir + (minCTs == true ? "minCT": "ct") + temp + ".ct";
		CTs[i] = CodeTable::LoadCodeTable(filename, db);
		CTs[i]->AddOneToEachUsageCount();
		CTs[i]->CalcCodeTableSize();
	}

	// Read parts
	Database **dbs = new Database *[k];
	for(uint32 i=0; i<k; i++) {
		sprintf_s(temp, 100, "%d", i);
		filename = ontwarDir + "part" + temp + ".db";
		dbs[i] = Database::ReadDatabase(filename);
	}

	double dbSize = 0.0, ctSize = 0.0, totalSize = 0.0;
	string summaryFile = Bass::GetWorkingDir() + (minCTs == true ? "min-": "end-") + "dissimilarity.txt";
	FILE *file;
	if(fopen_s(&file, summaryFile.c_str(), "w+") != 0)
		throw string("Cannot write DS file.");

	// Compute distances
	double avg = 0.0, ds = 0, minDS = DOUBLE_MAX_VALUE, maxDS = 0.0;
	uint32 nr = 0;
	for(uint32 i=0; i<k-1; i++) {
		if(dbs[i]->GetNumRows() == 0)
			continue;
		for(uint32 j=i+1; j<k; j++) {
			if(dbs[j]->GetNumRows() == 0)
				continue;
			ds = DissimilarityTH::BerekenAfstandTussen(dbs[i], dbs[j], CTs[i], CTs[j]);
			avg += ds;
			if(ds < minDS) minDS = ds;
			if(ds > maxDS) maxDS = ds;
			nr++;
			sprintf_s(temp, 100, "%.02f\t", ds);
			fputs(temp, file);
		}
		fputs("\n", file);
	}
	avg /= nr;
	sprintf_s(temp, 200, "%.02f\t%.02f\t%.02f\n", minDS, avg, maxDS);
	fputs(temp, file);

	// Cleanup
	fclose(file);
	for(uint32 i=0; i<k; i++) {
		delete dbs[i];
		delete CTs[i];
	}
	delete[] dbs;
	delete[] CTs;
	delete db;
}

#endif // BLOCK_CLUSTER
