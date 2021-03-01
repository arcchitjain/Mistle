#ifdef BLOCK_CLASSIFIER

#include <float.h>

#include "../../global.h"

// -- qtils
#include <Config.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <RandomUtils.h>
#include <logger/Log.h>

// -- bass
#include <db/DbFile.h>
#include <db/ClassedDatabase.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "../../FicConf.h"
#include "../../FicMain.h"
#include "../krimp/codetable/coverfull/CFCodeTable.h"
#include "../krimp/codetable/coverpartial/CPACodeTable.h"
#include "CtAnalyser.h"

#include "Classifier.h"

Classifier::Classifier(const Config *config) {
	mConfig = config;

	mClasses = NULL;
	mNumClasses = 0;
	mNumFolds = 0;
}
Classifier::~Classifier() {
	
}

void Classifier::Classify() {
	char temp[200]; string s;

	// All variables with postfix T are meant for maintaining info on training data, without prefix is for test data.

	// Some settings
	string expDir = Bass::GetWorkingDir();
	mNumFolds = mConfig->Read<uint32>("numfolds");
	mReportSup = mConfig->Read<uint32>("reportsup");
	bool relativeMatching = mConfig->Read<bool>("classifypercentage");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype"));
	mSanitize = mConfig->Read<bool>("sanitize", false);
	bool majorityVoting = mConfig->Read<bool>("majorityvoting", false);

	// Determine classes
	uint8 levelBak = Bass::GetOutputLevel(); Bass::SetOutputLevel(0);
	Database *db = ClassedDatabase::ReadDatabase(string("test"), expDir + "f1/", dataType);
	Bass::SetOutputLevel(levelBak);
	uint32 alphLen = (uint32) db->GetAlphabetSize();
	mNumClasses = db->GetNumClasses();
	mClasses = new uint16[mNumClasses];
	memcpy(mClasses, db->GetClassDefinition(), sizeof(uint16) * mNumClasses);
	mClassSizes = new uint32[mNumClasses];
	delete db;

	// Init some variables
	string **strClasses = new string *[mNumClasses];
	ClassedDatabase *testDb, *trainDb;
	CodeTable **CTs = new CodeTable *[mNumClasses];
	CodeTable **CTsT = new CodeTable *[mNumClasses];
	uint32 *numCTs = new uint32[mNumClasses];
	CTFileInfo ***ctFiles = new CTFileInfo **[mNumClasses];
	uint32 **confusion = new uint32 *[mNumClasses];
	uint32 **confusionT = new uint32 *[mNumClasses];
	ItemSet **masks = new ItemSet *[mNumClasses];
	for(uint32 i=0; i<mNumClasses; i++) {
		CTs[i] = NULL;
		CTsT[i] = NULL;
		_itoa(mClasses[i], temp, 10); strClasses[i] = new string(temp);
		masks[i] = ItemSet::Create(dataType, mClasses+i, 1, alphLen);
		confusionT[i] = new uint32[mNumClasses];
		confusion[i] = new uint32[mNumClasses];
		for(uint32 c=0; c<mNumClasses; c++) {
			confusion[i][c] = 0;
			confusionT[i][c] = 0;
		}
	}
	ItemSet *combinedMask = ItemSet::Create(dataType, mClasses, mNumClasses, alphLen);

	uint32 **maxSups = new uint32 *[mNumFolds];
	uint32 minMaxSup, curSup;
	mOverallMaxSup = 0;
	mMinSup = UINT32_MAX_VALUE;

	supScoreMap ***good = new supScoreMap **[mNumFolds];
	supScoreMap ***bad = new supScoreMap **[mNumFolds];
	supScoreMap ***goodT = new supScoreMap **[mNumFolds];
	supScoreMap ***badT = new supScoreMap **[mNumFolds];

	string foldDir, filename;

	// Foreach fold
	for(uint32 f=1; f<=mNumFolds; f++) {
		sprintf_s(temp, 200, " * Processing Fold:\t%d", f); s = temp; LOG_MSG(s);

		good[f-1] = new supScoreMap *[mNumClasses];
		bad[f-1] = new supScoreMap *[mNumClasses];
		goodT[f-1] = new supScoreMap *[mNumClasses];
		badT[f-1] = new supScoreMap *[mNumClasses];
		maxSups[f-1] = new uint32[mNumClasses];
		minMaxSup = UINT32_MAX_VALUE;

		_itoa(f, temp, 10);
		foldDir = expDir + "f" + string(temp) + "/";

		// Load train & test databases
		{
			uint8 levelBak = Bass::GetOutputLevel(); Bass::SetOutputLevel(0);
			testDb = (ClassedDatabase *) Database::ReadDatabase("test", foldDir, dataType);
			if(!testDb)
				throw("Classifier::Classify -- Could not read test database!");

			// MvL for ACMK Financial haxor -- provide train.db from merged trainX.db and trainX.db ////
			/*LOG_WARNING("Hack :: Merging Train0 and Train15 to obtain Train.");
			Database *trainA = Database::ReadDatabase("train0", foldDir, dataType);
			Database *trainB = Database::ReadDatabase("train15", foldDir, dataType);
			Database *train = Database::Merge(trainA, trainB);
			trainDb = new ClassedDatabase(train, false);
			uint16 *classLabels = new uint16[trainDb->GetNumRows()];
			for(uint32 i=0; i<trainA->GetNumRows(); i++)
				classLabels[i] = 0;
			for(uint32 i=trainA->GetNumRows(); i<train->GetNumRows(); i++)
				classLabels[i] = 15;
			trainDb->SetClassLabels(classLabels);
			delete trainA;
			delete trainB;
			delete train;*/
			// End of hack. Don't forget to dis-/enable default: */
			trainDb = (ClassedDatabase *) Database::ReadDatabase("train", foldDir, dataType);
			// End of default.

			if(!trainDb)
				throw("Classifier::Classify -- Could not read train database!");
			testDb->SetMaxSetLength(trainDb->GetMaxSetLength());

			if(majorityVoting) {
				if(!testDb->HasTransactionIds())
					throw("Classifier::Classify -- Majority voting without transaction id's in testDb, let's not do that.");
				if(!trainDb->HasTransactionIds())
					throw("Classifier::Classify -- Majority voting without transaction id's in trainDb, let's not do that.");

				// --- Cleanup this hack later, it's Friday evening now.. ---
				ItemSet **rows = testDb->GetRows(); uint32 numRows = testDb->GetNumRows();
				for(uint32 r=0; r<numRows; r++)
					rows[r]->SetSupport(testDb->GetClassLabel(r)); // abuse support...
				ItemSet::SortItemSetArray(rows, numRows, IdAscLexAscIscOrder);
				for(uint32 r=0; r<numRows; r++)
					testDb->SetClassLabel(r, rows[r]->GetSupport());

				rows = trainDb->GetRows(); numRows = trainDb->GetNumRows();
				for(uint32 r=0; r<numRows; r++)
					rows[r]->SetSupport(trainDb->GetClassLabel(r)); // abuse support...
				ItemSet::SortItemSetArray(rows, numRows, IdAscLexAscIscOrder);
				for(uint32 r=0; r<numRows; r++)
					trainDb->SetClassLabel(r, rows[r]->GetSupport());

				// Reset & count class sizes
				for(uint32 c=0; c<mNumClasses; c++)
					mClassSizes[c] = 0;
				uint64 prevTid = UINT64_MAX_VALUE;
				uint16 classLabel;
				for(uint32 r=0; r<trainDb->GetNumRows(); r++) {
					if(trainDb->GetRow(r)->GetID() != prevTid) {
						classLabel = trainDb->GetClassLabel(r);
						for(uint32 c=0; c<mNumClasses; c++) {
							if(mClasses[c] == classLabel) {
								++mClassSizes[c];
								break;
							}
						}
						prevTid = trainDb->GetRow(r)->GetID();
					}
				}
			} else {
				memcpy(mClassSizes, trainDb->GetClassSupports(), sizeof(uint32) * mNumClasses);
			}

			Bass::SetOutputLevel(levelBak);
		}

		// Check common max report sup
		for(uint32 c=0; c<mNumClasses; c++) {
			ctFiles[c] = FicMain::LoadCodeTableFileList("min-999999999", testDb, numCTs[c], foldDir + string("train") + *strClasses[c] + string("/"));
			if(numCTs[c] < 1)
				throw string("Classifier::Classify -- No codetables found where they should be.");

			good[f-1][c] = new supScoreMap();	// !!! max en min zoeken, dan weet je undecided
			bad[f-1][c] = new supScoreMap();
			goodT[f-1][c] = new supScoreMap();	// !!! max en min zoeken, dan weet je undecided
			badT[f-1][c] = new supScoreMap();
			mMinSup = min(mMinSup, ctFiles[c][0]->minsup);
			maxSups[f-1][c] = ctFiles[c][numCTs[c]-1]->minsup;
			minMaxSup = min(minMaxSup, maxSups[f-1][c]);
		}

		// Determine reportValues (fixed sup or percentage)
		uint32 reportVal, numReports, reportDelta;
		if(relativeMatching) {
			//LOG_WARNING("Doing only 25-1% right now!");
			numReports = 100;
			reportDelta = 1;
			reportVal = 100;
			mOverallMaxSup = max(mOverallMaxSup, minMaxSup);
		} else {
			reportVal = minMaxSup - ((minMaxSup-mMinSup) % mReportSup);
			numReports = (reportVal - mMinSup) / mReportSup + 1;
			reportDelta = mReportSup;
			mOverallMaxSup = max(mOverallMaxSup, reportVal);
		}

		// Foreach sup
		LOG_MSG("Starting classification");
		for(uint32 rep=0; rep<numReports; rep++, reportVal -= reportDelta) {
			if(relativeMatching) {
				ECHO(2, printf("\r * Classifying ct %%:\t%-6d",reportVal));
			} else {
				ECHO(2, printf("\r * Classifying ct sup:\t%-6d",reportVal));
			}

			for(uint32 c=0; c<mNumClasses; c++) {
				if(relativeMatching) {
					if(reportVal == 100)
						curSup = maxSups[f-1][c];
					else {
						curSup = (uint32)(reportVal / 100.0f * maxSups[f-1][c]);
						if(curSup < mMinSup)
							curSup = mMinSup;
						else
							curSup -= (curSup-mMinSup) % mReportSup;
					}
				} else 
					curSup = reportVal;
				delete CTs[c];
				delete CTsT[c];
				CTs[c] = CodeTable::CreateCTForClassification("coverfull", dataType);
				CTsT[c] = CodeTable::CreateCTForClassification("coverfull", dataType);

				// StdLens don't matter in classification!
				CTs[c]->UseThisStuff(testDb, testDb->GetDataType(), InitCTEmpty);
				CTsT[c]->UseThisStuff(trainDb, trainDb->GetDataType(), InitCTEmpty);
				{
					uint8 levelBak = Bass::GetOutputLevel(); Bass::SetOutputLevel(0);
					bool found = false;
					for(uint32 i=0; i<numCTs[c]; i++)
						if(ctFiles[c][i]->minsup >= curSup) {
							filename = foldDir + string("train") + *strClasses[c] + string("/") + ctFiles[c][i]->filename;
							CTs[c]->ReadFromDisk(filename, false);
							CTsT[c]->ReadFromDisk(filename, false);
							found = true;
							break;
						}
					if(!found)
						throw string("Classifier::Classify -- Could not find the right code table.");
					Bass::SetOutputLevel(levelBak);
				}

				if(mSanitize) {
					islist *pruneList = CTs[c]->GetSanitizePruneList();
					islist::iterator iter;
					for(iter = pruneList->begin(); iter != pruneList->end(); ++iter) {
						ItemSet *is = (ItemSet*)(*iter);
						CTs[c]->Del(is, true, false); // zap immediately
					}
					delete pruneList;
					pruneList = CTsT[c]->GetSanitizePruneList();
					for(iter = pruneList->begin(); iter != pruneList->end(); ++iter) {
						ItemSet *is = (ItemSet*)(*iter);
						CTsT[c]->Del(is, true, false); // zap immediately
					}
					delete pruneList;
				}

				// Apply Laplace correction
				CTs[c]->AddOneToEachUsageCount();
				CTsT[c]->AddOneToEachUsageCount();

				// Remove Laplace for class items (effectively remove these from the alphabet)
				for(uint32 j=0; j<mNumClasses; j++) {
					CTs[c]->SetAlphabetCount(mClasses[j], 0);
					CTsT[c]->SetAlphabetCount(mClasses[j], 0);
				}
				CTs[c]->GetCurStats().usgCountSum -= mNumClasses;
				CTsT[c]->GetCurStats().usgCountSum -= mNumClasses;

				// Init score map for current reportVal
				good[f-1][c]->insert(supScoreMapEntry(reportVal, 0));
				bad[f-1][c]->insert(supScoreMapEntry(reportVal, 0));
				goodT[f-1][c]->insert(supScoreMapEntry(reportVal, 0));
				badT[f-1][c]->insert(supScoreMapEntry(reportVal, 0));
			}

			if(!majorityVoting) {					/// ------------------- Normal operation -----------------------
				// on Trainingsdata
				DoClassification(trainDb, CTsT, reportVal, goodT[f-1], badT[f-1], rep==numReports-1?confusionT:NULL);

				// on Testdata
				DoClassification(testDb, CTs, reportVal, good[f-1], bad[f-1], rep==numReports-1?confusion:NULL);
			} else {								/// ------------------- Majority voting -----------------------

				// on Trainingsdata
				DoMajorityVoteClassification(trainDb, CTsT, reportVal, goodT[f-1], badT[f-1], rep==numReports-1?confusionT:NULL);

				// on Testdata
				DoMajorityVoteClassification(testDb, CTs, reportVal, good[f-1], bad[f-1], rep==numReports-1?confusion:NULL);
			}
		}	// EndOf reportVal loop

		for(uint32 c=0; c<mNumClasses; c++) {
			for(uint32 i=0; i<numCTs[c]; i++)
				delete ctFiles[c][i];
			delete[] ctFiles[c];
		}

		delete testDb;
		delete trainDb;
		ECHO(2, printf("\r * Classifying:\t\tdone\n"));
	} // EndOf Fold loop

	WriteScoreFile(expDir + "T", goodT, badT, relativeMatching);
	WriteScoreFile(expDir, good, bad, relativeMatching);

	string scoreFile = expDir + "confusion-" + (relativeMatching?"rel":"abs") + ".txt";
	WriteConfusionFile(scoreFile, confusion);
	scoreFile = expDir + "Tconfusion-" + (relativeMatching?"rel":"abs") + ".txt";
	WriteConfusionFile(scoreFile, confusionT);

	// find best in T, report score in test
	string topscoreFile = expDir + "topscore.txt";
	WriteTopscoreFile(topscoreFile, goodT, badT, good, bad, relativeMatching);

	for(uint32 c=0; c<mNumClasses; c++) {
		delete strClasses[c];
		delete masks[c];
		delete CTs[c];
		delete[] confusion[c];
		delete[] confusionT[c];
	}
	delete[] masks;
	delete[] mClasses;
	delete[] mClassSizes;
	delete[] CTs;
	delete[] CTsT;
	delete[] numCTs;
	delete[] ctFiles;
	delete[] confusion;
	delete[] confusionT;

	for(uint32 f=0; f<mNumFolds; f++) {
		for(uint32 c=0; c<mNumClasses; c++) {
			delete good[f][c];
			delete bad[f][c];
			delete goodT[f][c];
			delete badT[f][c];
		}
		delete[] good[f];
		delete[] bad[f];
		delete[] goodT[f];
		delete[] badT[f];
		delete[] maxSups[f];
	}
	delete[] strClasses;
	delete[] maxSups;
	delete[] good;
	delete[] bad;
	delete[] goodT;
	delete[] badT;
	delete combinedMask;
}
 
void Classifier::DoClassification(ClassedDatabase *cdb, CodeTable **CTs, uint32 reportVal, supScoreMap **good, supScoreMap **bad, uint32 **confusion) {
	uint32 numRows = cdb->GetNumRows();
	ItemSet *is;
	uint32 bestIdx, actualIdx, numBestIds;
	double bestVal;
	double *codeLens = new double[mNumClasses];
	uint32 *bestIds = new uint32[mNumClasses];

	for(uint32 r=0; r<numRows; r++) {
		is = cdb->GetRow(r);
		for(uint32 c=0; c<mNumClasses; c++) {
			if(cdb->GetClassLabel(r) == mClasses[c]) {
				actualIdx = c;
				break;
			}
		}
		bestVal = DOUBLE_MAX_VALUE;
		for(uint32 c=0; c<mNumClasses; c++) {
			codeLens[c] = CTs[c]->CalcTransactionCodeLength(is);
#if defined (_WINDOWS)
			if(!_finite(codeLens[c]))
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
			if(!finite(codeLens[c]))
#endif
				ECHO(0, printf("Infinite codelength in classification, not good."));
			if(codeLens[c] < bestVal) bestVal = codeLens[c];
		}
		numBestIds = 0;
		for(uint32 c=0; c<mNumClasses; c++) {
			if(codeLens[c] == bestVal)
				bestIds[numBestIds++] = c;
		}

		bestIdx = bestIds[0];
		if(numBestIds > 1) { 	// tie, pick majority class from tied classes
			uint32 largest = mClassSizes[bestIdx];
			for(uint32 i=1; i<numBestIds; i++) {
				if(mClassSizes[bestIds[i]] > largest) {
					largest = mClassSizes[bestIds[i]];
					bestIdx = bestIds[i];
				}				
			}
		}

		if(bestIdx == actualIdx) {			// good!
			good[actualIdx]->find(reportVal)->second += is->GetUsageCount();
		} else {							// bad.
			bad[bestIdx]->find(reportVal)->second += is->GetUsageCount();
		}
		if(confusion != NULL) {				// if provided: maintain confusion matrix
			confusion[bestIdx][actualIdx]++;
		}
	}
	
	delete[] codeLens;
	delete[] bestIds;
}

void Classifier::DoMajorityVoteClassification(ClassedDatabase *cdb, CodeTable **CTs, uint32 reportVal, supScoreMap **good, supScoreMap **bad, uint32 **confusion) {
	double *codeLens = new double[mNumClasses];
	uint32 *votes = new uint32[mNumClasses];
	uint32 *bestIds = new uint32[mNumClasses];
	uint32 numBestIds;

	uint32 numRows = cdb->GetNumRows();
	ItemSet *is;
	uint64 curTid;
	uint32 bestIdx, secondIdx, actualIdx;
	double bestVal, secondVal;
	uint32 bestVotes;

	// Init first row
	for(uint32 c=0; c<mNumClasses; c++)
		votes[c] = 0;
	curTid = cdb->GetRow(0)->GetID();

	for(uint32 r=0; r<numRows; r++) {
		is = cdb->GetRow(r);
		//uint64 id = is->GetID();
		if(is->GetID() != curTid) {	// Decide for previous tid
			bestVotes = votes[0];
			for(uint32 c=1; c<mNumClasses; c++) {
				if(votes[c] > bestVotes)
					bestVotes = votes[c];
			}
			numBestIds = 0;
			for(uint32 c=0; c<mNumClasses; c++) {
				if(votes[c] == bestVotes)
					bestIds[numBestIds++] = c;
			}
			if(numBestIds == 1)
				bestIdx = bestIds[0];
			else {
				//LOG_WARNING("Random class picked because of tie -- largest class would be better but ACMK does not want this right now. :)");
				//bestIdx = bestIds[RandomUtils::UniformUint16(numBestIds)];
				
				uint32 largest = mClassSizes[bestIdx];
				for(uint32 i=1; i<numBestIds; i++) {
					if(mClassSizes[bestIds[i]] > largest) {
						largest = mClassSizes[bestIds[i]];
						bestIdx = bestIds[i];
					}				
				}// */
			}

			if(bestIdx == actualIdx) {
				good[bestIdx]->find(reportVal)->second++;
			} else {
				bad[bestIdx]->find(reportVal)->second++;
			}
			if(confusion != NULL) {
				confusion[bestIdx][actualIdx]++;
			}

			// Start new vote
			for(uint32 c=0; c<mNumClasses; c++)
				votes[c] = 0;
			curTid = is->GetID();
		}
		for(uint32 c=0; c<mNumClasses; c++)
			if(cdb->GetClassLabel(r) == mClasses[c]) {
				actualIdx = c;
				break;
			}
		for(uint32 c=0; c<mNumClasses; c++) {
			codeLens[c] = CTs[c]->CalcTransactionCodeLength(is);
#if defined (_WINDOWS)
			if(!_finite(codeLens[c]))
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
			if(!finite(codeLens[c]))
#endif
				ECHO(0, printf("Infinite codelength in classification, not good."));
		}
		bestIdx = secondIdx = 0;
		bestVal = secondVal = codeLens[0];
		for(uint32 c=1; c<mNumClasses; c++) {
			if(codeLens[c] < bestVal) {
				secondIdx = bestIdx;
				secondVal = bestVal;
				bestIdx = c;
				bestVal = codeLens[c];
			} else if(codeLens[c] > bestVal) {
				if(bestIdx == secondIdx || secondVal > codeLens[c]) {
					secondIdx = c;
					secondVal = codeLens[c];
				} 
			}
		}
		
		if(bestVal < secondVal) // in case of a tie, cast no vote
			votes[bestIdx] += is->GetUsageCount();
	}
	// Decide for last tid
	bestVal = votes[0];
	for(uint32 c=1; c<mNumClasses; c++) {
		if(votes[c] > bestVal)
			bestVal = votes[c];
	}
	numBestIds = 0;
	for(uint32 c=0; c<mNumClasses; c++) {
		if(votes[c] == bestVal)
			bestIds[numBestIds++] = c;
	}
	if(numBestIds == 1)
		bestIdx = bestIds[0];
	else { // pick random best class (actually, we may want to pick the majority class here as baseline -- random for now, for 'fair' comparison to ACMK)
		//LOG_WARNING("Random class picked because of tie -- largest class would be better but ACMK does not want this right now. :)");
		//bestIdx = bestIds[RandomUtils::UniformUint16(numBestIds)];
		uint32 largest = mClassSizes[bestIdx];
		for(uint32 i=1; i<numBestIds; i++) {
			if(mClassSizes[bestIds[i]] > largest) {
				largest = mClassSizes[bestIds[i]];
				bestIdx = bestIds[i];
			}				
		}// */
	}
	if(bestIdx == actualIdx) {
		good[bestIdx]->find(reportVal)->second++;
	} else {
		bad[bestIdx]->find(reportVal)->second++;
	}
	if(confusion != NULL) {
		confusion[bestIdx][actualIdx]++;
	}

	delete[] codeLens;
	delete[] votes;
	delete[] bestIds;
}

void Classifier::WriteTopscoreFile(const string &filename, supScoreMap ***goodT, supScoreMap ***badT, supScoreMap ***good, supScoreMap ***bad, bool relativeMatching) {
#ifndef _PUBLIC_RELEASE
	uint32 reportVal, numReports, reportDelta, plus, notPlus;

	FILE *fp = fopen(filename.c_str(), "a");
	if(!fp)
		throw string("Could not write topscore file.");

	if(relativeMatching) {
		numReports = 100;
		reportDelta = 1;
	} else {
		numReports = (mOverallMaxSup - mMinSup) / mReportSup + 1;
		reportDelta = mReportSup;
	}

	float avgTrainAcc = 0.0f, avgTestAcc = 0.0f;
	uint32 numTransactions = 0;

	for(uint32 f=1; f<=mNumFolds; f++) {
		//fprintf(fp, "\n\nFold %d\n\n", f);

		// find best training result
		float accuracy, bestAcc = 0.0f;
		uint32 bestReportVal = 0;
		reportVal = relativeMatching ? 100 : mOverallMaxSup;
		for(uint32 rep=0; rep<numReports; rep++, reportVal -= reportDelta) {
			if(goodT[f-1][0]->find(reportVal) != goodT[f-1][0]->end()) {
				plus = notPlus = 0;
				for(uint32 t=0; t<mNumClasses; t++) {
					plus += goodT[f-1][t]->find(reportVal)->second;
					notPlus += badT[f-1][t]->find(reportVal)->second;
				}
				accuracy = plus / (plus + (float)notPlus);
				if(accuracy > bestAcc) {
					bestAcc = accuracy;
					bestReportVal = reportVal;
					//fprintf(fp, "%d;%.4f;%d;%d;BEST\n", reportVal, accuracy, plus, notPlus);
				} else {
					//fprintf(fp, "%d;%.4f;%d;%d\n", reportVal, accuracy, plus, notPlus);
				}
			}
		}
		avgTrainAcc += bestAcc;

		// compute result on test result
		plus = notPlus = 0;
		for(uint32 t=0; t<mNumClasses; t++) {
			plus += good[f-1][t]->find(bestReportVal)->second;
			notPlus += bad[f-1][t]->find(bestReportVal)->second;
		}
		accuracy = plus / (plus + (float)notPlus);
		avgTestAcc += (plus + notPlus) * accuracy;
		numTransactions += plus + notPlus;
		//fprintf(fp, "Test\n%d;%.4f;%d;%d\n", bestReportVal, accuracy, plus, notPlus);
	}
	avgTrainAcc /= mNumFolds;
	avgTestAcc /= numTransactions;

	string::size_type pos;
	string expTag = mConfig->Read<string>("exptag");
	while((pos = expTag.find("-")) != string::npos)
		expTag.replace(pos, 1, ";");
	pos = 0;
	for(uint32 i=0; i<2; i++) {
		while(expTag[pos] != ';')
			++pos;
		++pos;
	}
	while(expTag[pos] >= '0' && expTag[pos] <= '9')
		++pos;
	expTag.insert(pos, ";");
	if(mSanitize) {
		pos = expTag.find("nop");
		if(pos != string::npos)
			expTag.insert(pos+3, "post");
	}
	fprintf_s(fp, "%s;%s;%.4f;%.4f\n", expTag.c_str(), relativeMatching?"rel":"abs", avgTrainAcc, avgTestAcc);
	fclose(fp);
#endif
}


void Classifier::WriteScoreFile(const string &expDir, supScoreMap ***good, supScoreMap ***bad, bool relativeMatching) {
	string classificationType = relativeMatching ? "rel" : "abs";
	string scoreFile = expDir + "classify-" + classificationType + ".csv";
	FILE *fp = fopen(scoreFile.c_str(), "w");
	if(!fp)
		throw string("Could not write result file.");
	fprintf(fp, "Support;");
	for(uint32 f=1; f<=mNumFolds; f++) {
		fprintf(fp, "Fold %d;;", f);
		for(uint32 t=1; t<mNumClasses; t++)
			fprintf(fp, ";;");
	}
	fprintf(fp, "Overall\n;");
	for(uint32 f=0; f<=mNumFolds; f++) {
		for(uint32 t=0; t<mNumClasses; t++)
			fprintf(fp, "%d+;%d-;", mClasses[t], mClasses[t]);
	}
	fprintf(fp, "AvgAccuracy;StdDev;MaxAccuracy\n");
	uint32 *overall = new uint32[2 * mNumClasses];
	uint32 total, totalPlus, totalMin, val, reportVal, numReports, reportDelta, plus, notPlus;
	if(relativeMatching) {
		numReports = 100;
		reportDelta = 1;
		reportVal = 100;
	} else {
		numReports = (mOverallMaxSup - mMinSup) / mReportSup + 1;
		reportDelta = mReportSup;
		reportVal = mOverallMaxSup;
	}
	float *accuracies = new float[mNumFolds];
	float avgOfReport, maxOfReport, stdDev;
	float bestReportMax = 0.0f, bestReportAvg = 0.0f, bestReportStdDev = 0.0f;
	uint32 bestReportVal = 0;
	uint32 numAccs;

	uint32 foldCount = 0;	// number of folds used at reportlevel

	for(uint32 rep=0; rep<numReports; rep++, reportVal -= reportDelta) {
		for(uint32 i=0; i<2*mNumClasses; i++)
			overall[i] = 0;
		numAccs = total = totalPlus = totalMin = 0;
		fprintf(fp, "%d;", reportVal);

		foldCount = 0;

		for(uint32 f=1; f<=mNumFolds; f++) {
			if(good[f-1][0]->find(reportVal) != good[f-1][0]->end()) {
				plus = notPlus = 0;
				for(uint32 t=0; t<mNumClasses; t++) {
					val = good[f-1][t]->find(reportVal)->second;
					fprintf(fp, "%d;", val);
					overall[t*2] += val;
					total += val;
					totalPlus += val;
					plus += val;
					val = bad[f-1][t]->find(reportVal)->second;
					fprintf(fp, "%d;", val);
					overall[t*2+1] += val;
					total += val;
					totalMin += val;
					notPlus += val;
				}
				accuracies[numAccs++] = plus / (plus + (float)notPlus);
			} else {
				for(uint32 t=0; t<mNumClasses; t++)
					fprintf(fp, ";;");
			}
		}

		float t = (float)total;
		for(uint32 i=0; i<mNumClasses; i++)
			fprintf(fp, "%.06f;%.06f;", overall[i*2] / t, overall[i*2+1] / t);
		stdDev = 0.0f;
		maxOfReport = 0.0f;
		avgOfReport = totalPlus / t;
		for(uint32 i=0; i<numAccs; i++) {
			if(accuracies[i] > maxOfReport)
				maxOfReport = accuracies[i];
			stdDev += (avgOfReport - accuracies[i]) * (avgOfReport - accuracies[i]);
		}
		stdDev /= numAccs;
		stdDev = sqrt(stdDev);

		if(avgOfReport > bestReportAvg) {
			bestReportAvg = avgOfReport;
			bestReportMax = maxOfReport;
			bestReportStdDev = stdDev;
			bestReportVal = reportVal;
		}
		fprintf(fp, "%.04f;%.04f;%.04f\n", avgOfReport, stdDev, maxOfReport);
	}
	for(uint32 f=0; f<mNumFolds; f++) {
		for(uint32 t=0; t<mNumClasses; t++)
			fprintf(fp, ";;");
	}
	fprintf(fp, ";;;;;%.04f;%.04f;%.04f\n", bestReportAvg, bestReportStdDev, bestReportMax);
	delete[] overall;
	delete[] accuracies;
	fclose(fp);

	scoreFile = expDir + "summary.csv";
	fp = fopen(scoreFile.c_str(), "a");
	if(!fp)
		throw string("Could not write summary file.");
	string expTag = mConfig->Read<string>("exptag");
	fprintf(fp, ":: %s\n%s matching\n\n", expTag.c_str(), relativeMatching ? "Relative" : "Absolute");
	fprintf(fp, ";%s;AvgAccuracy;StdDev;MaxAccuracy\n", relativeMatching ? "%" : "minsup");
	fprintf(fp, "Best:;%d", bestReportVal);
	if(relativeMatching)
		fprintf(fp, "%%");
	fprintf(fp, ";%.04f;%.04f;%.04f\n", bestReportAvg, bestReportStdDev, bestReportMax);
	fprintf(fp, "Last:;");
	if(relativeMatching)
		fprintf(fp, "1%%");
	else
		fprintf(fp, "%d", mMinSup);
	fprintf(fp, ";%.04f;%.04f;%.04f\n\n", avgOfReport, stdDev, maxOfReport);
	fclose(fp);
	scoreFile = expDir + "singleline.txt";
	fp = fopen(scoreFile.c_str(), "a");
	if(!fp)
		throw string("Could not write singleline file.");
	string::size_type pos;
	while((pos = expTag.find("-")) != string::npos)
		expTag.replace(pos, 1, ";");
	pos = 0;
	for(uint32 i=0; i<2; i++) {
		while(expTag[pos] != ';')
			++pos;
		++pos;
	}
	while(expTag[pos] >= '0' && expTag[pos] <= '9')
		++pos;
	expTag.insert(pos, ";");
	if(mSanitize) {
		pos = expTag.find("nop");
		if(pos != string::npos)
			expTag.insert(pos+3, "post");
	}
	fprintf(fp, "%s;%s;%d;%.04f;%.04f;%.04f;%d;%.04f;%.04f;%.04f\n", expTag.c_str(), relativeMatching?"rel":"abs", bestReportVal, bestReportAvg, bestReportStdDev, bestReportMax, relativeMatching?1:mMinSup, avgOfReport, stdDev, maxOfReport);
	fclose(fp);
}


void Classifier::WriteConfusionFile(const string &filename, uint32 **confusion) {
	FILE *fp = fopen(filename.c_str(), "w");
	uint32 *sums = new uint32[mNumClasses];

	// To confuse zhe classes
	fprintf(fp, "Confusion matrix for last reported CT match\n\n\t");
	for(uint32 t=0; t<mNumClasses; t++) {
		fprintf(fp, "%5d\t", mClasses[t]);
		sums[t] = 0;
	}
	fprintf(fp, "actual class\n");
	for(uint32 t=0; t<mNumClasses; t++) {
		fprintf(fp, "%d\t", mClasses[t]);
		for(uint32 i=0; i<mNumClasses; i++) {
			sums[i] += confusion[t][i];
			fprintf(fp, "%5d\t",
				confusion[t][i]
				);
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "\t");
	uint32 total = 0;
	for(uint32 t=0; t<mNumClasses; t++) {
		total += sums[t];
		fprintf(fp, "%5d\t", sums[t]);
	}
	fprintf(fp, "\t%d\nclassified\n", total);

	fclose(fp);
	delete[] sums;
}

#endif // BLOCK_CLASSIFIER
