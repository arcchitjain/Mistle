#ifdef BLOCK_OCCLASSIFIER

#include <cassert>
#include <cmath>
#include <unordered_map>

#include "../global.h"

// -- qtils
#include <logger/Log.h>
#include <RandomUtils.h>

// -- bass
#include <db/ClassedDatabase.h>
#include <itemstructs/CoverSet.h>

// -- fic
#include "../FicMain.h"
#include "../algo/CodeTable.h"
#include "../dgen/cover/CTree.h"

#include "OneClassClassifier.h"

OneClassClassifier::OneClassClassifier(const Config *config) {
	mConfig = config;

	mClasses = NULL;
	mNumClasses = 0;
	mNumFolds = 0;
}
OneClassClassifier::~OneClassClassifier() {
	if (mClasses)
		delete mClasses;
}

Inequality OneClassClassifier::StringToInequality(const string& s)
{
	if (s == "chebyshev") return Chebyshev;
	else if (s == "cantelli") return Cantelli;
	else throw string("Unknown inequality");
}

void OneClassClassifier::Analyse() {
	char temp[200];
	string s;

	// Some settings
	string expDir = Bass::GetWorkingDir();
	mNumFolds = mConfig->Read<uint32>("numfolds");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype"));
	uint32 lpcFactor = mConfig->Read<uint32>("lpcfactor", 1);
	uint32 lpcOffset = mConfig->Read<uint32>("lpcoffset", 1);
	bool firstCT = mConfig->Read<bool>("firstCT", false);
	bool lastCT = mConfig->Read<bool>("lastCT", false);
	uint32 numPrototypeOutliers = mConfig->Read<uint32>("numPrototypeOutliers", 0);
	uint32 numOutliers = mConfig->Read<uint32>("numOutliers", 0);
	string algoname = mConfig->Read<string>("algo");

	// Determine classes
	uint8 levelBak = Bass::GetOutputLevel(); Bass::SetOutputLevel(0);
	Database *db = ClassedDatabase::ReadDatabase(string("test"), expDir + "f1/", dataType);
	Bass::SetOutputLevel(levelBak);
	mNumClasses = db->GetNumClasses();
	mClasses = new uint16[mNumClasses];
	memcpy(mClasses, db->GetClassDefinition(), sizeof(uint16) * mNumClasses);
	delete db;

	// Init some variables
	string **strClasses = new string *[mNumClasses];
	ClassedDatabase *testDb, *trainingDb;
	for(uint16 i=0; i<mNumClasses; i++) {
		_itoa(mClasses[i], temp, 10);
		strClasses[i] = new string(temp);
	}
	string dbNames[2] = { "target" , "outlier" };
	_itoa(lpcFactor, temp, 10);
	string lpcFactorStr(temp);
	_itoa(lpcOffset, temp, 10);
	string lpcOffsetStr(temp);
	_itoa(numPrototypeOutliers, temp, 10);
	string numPrototypeOutliersStr(temp);
	_itoa(numOutliers, temp, 10);
	string numOutliersStr(temp);

	// For each fold
	for(uint32 f=1; f<=mNumFolds; f++) {
		sprintf_s(temp, 200, " * Processing Fold:\t%d", f); s = temp; LOG_MSG(s);

		_itoa(f, temp, 10);
		string foldDir = expDir + "f" + string(temp) + "/";

		// Load train database
		trainingDb = (ClassedDatabase *) Database::ReadDatabase(string("train"), foldDir, dataType);
		if(!trainingDb)
			throw("OneClassClassifier::Analyse -- Could not read training database!");
		uint32 numTrainingSamples = trainingDb->GetNumTransactions();

/*
		assert(trainingDb->HasColumnDefinition()); // TODO: find out why cd is not written to train.db file!

		ItemSet** columnDefinition = trainingDb->GetColumnDefinition();
		uint32 numColumns = trainingDb->GetNumColumns();

		uint16** columnValues = new uint16 *[numColumns];
		uint32* value2column = new uint32[trainingDb->GetAlphabetSize()];
		for (uint32 c = 0; c < numColumns; ++c) {
			columnValues[c] = columnDefinition[c]->GetValues();
			for (uint32 v = 0; v < columnDefinition[c]->GetLength(); ++v) {
				value2column[columnValues[c][v]] = c;
			}
		}

		double** codelengths = new double *[numPrototypeOutliers];
		for (uint32 o = 0; o < numPrototypeOutliers; ++o)
			codelengths[o] = new double[trainingDb->GetAlphabetSize()];

		uint16** prototypeValues = new uint16 *[numPrototypeOutliers];
		double** prototypeDistribution = new double *[numPrototypeOutliers];
		uint32* prototypeLength = new uint32[numPrototypeOutliers];

		double*** outlierDistribution = new double **[numPrototypeOutliers];
		for (uint32 oid = 0; oid < numPrototypeOutliers; ++oid) {
			outlierDistribution[oid] = new double *[numColumns];
			for (uint32 cid = 0; cid < numColumns; ++cid) {
				outlierDistribution[oid][cid] = new double[columnDefinition[cid]->GetLength()];
			}
		}

		unordered_map<uint16, double>* codeLenPerItem = new unordered_map<uint16, double>();
*/

		// Load test database
		testDb = (ClassedDatabase *) Database::ReadDatabase("test", foldDir, dataType);
		if(!testDb)
			throw("OneClassClassifier::Analyse -- Could not read test database!");

		// Foreach class
		for(uint16 c=0; c<mNumClasses; c++) {
			sprintf_s(temp, 200, " * Processing Class:\t%d", mClasses[c]); s = temp; LOG_MSG(s);

			// For target and outlier
			for(int j = 0; j < 2; ++j) { 
				string dbName = dbNames[j] + *strClasses[c];
				Database *db =  Database::ReadDatabase(dbName, foldDir, dataType);
				if(!db)
					throw(" OneClassClassifier::Analyse -- Could not read database!");
				string ctDir = foldDir + '/' + dbName + '/';

				uint32 numCTs;
				CTFileInfo **ctFiles = FicMain::LoadCodeTableFileList("min-max", db, numCTs, ctDir);

				uint32 start_i = 0;
				if (firstCT) // Load only the first codetable
					numCTs = 1;
				else if (lastCT) {// Load only the last codetable
					start_i = numCTs - 1;
					if(algoname.compare(0, 4, "slim") == 0) {
						ctFiles[start_i]->filename = "ct-latest.ct";
					}
				}



				for(uint32 i=start_i; i<numCTs; i++) {
					CodeTable *ct = CodeTable::LoadCodeTable(ctDir + ctFiles[i]->filename, db);
					ct->ApplyLaplaceCorrection(lpcFactor, lpcOffset);

					string filename;
					string suffix = ctFiles[i]->filename + "_" + lpcFactorStr + "_" + lpcOffsetStr + "_lengths.txt";
					filename = ctDir + "train_" + suffix;
					WriteCodeLengthsFile(filename, trainingDb, ct);
					filename = ctDir + "test_" + suffix;
					WriteCodeLengthsFile(filename, testDb, ct);
/*
					if (! FileUtils::Exists(ctDir + "train_" + ctFiles[i]->filename + "_ctree.txt")) {
						CTree* ctree = new CTree();
						ctree->BuildCoverTree(trainingDb, ctDir + ctFiles[i]->filename);
						ctree->WriteAnalysisFile(ctDir + "train_" + ctFiles[i]->filename + "_ctree.txt");
						delete ctree;
					}
					if (! FileUtils::Exists(ctDir + "test_" + ctFiles[i]->filename + "_ctree.txt")) {
						CTree* ctree = new CTree();
						ctree->BuildCoverTree(testDb, ctDir + ctFiles[i]->filename);
						ctree->WriteAnalysisFile(ctDir + "test_" + ctFiles[i]->filename + "_ctree.txt");
						delete ctree;
					}
*/

/* TODO: uncomment
					// Check stability of classification decisions...
					string filename;
					string suffix = ctFiles[i]->filename + "_" + lpcFactorStr + "_" + lpcOffsetStr + "_lengths_modifications.txt";
					filename = ctDir + "test_" + suffix;

					FILE *fp = fopen(filename.c_str(), "w");
					for(uint32 r=0; r<testDb->GetNumRows(); ++r) {

						ItemSet* is = testDb->GetRow(r)->Clone();
						uint16* values = is->GetValues();

						double codeLen;
						coverTransaction(ct, testDb, is, codeLen, fp);
						fprintf(fp, " [%d]\n", testDb->GetClassLabel(r));

						for (uint32 v = 0; v < is->GetLength(); ++v) {
							uint32 cid = value2column[values[v]];
							is->RemoveItemFromSet(values[v]);
							double sumOutlierDistribution = 0.0;
							for (uint32 i = 0; i < columnDefinition[cid]->GetLength(); ++i) {
								if (values[v] != columnValues[cid][i]) {
									is->AddItemToSet(columnValues[cid][i]);
									coverTransaction(ct, testDb, is, codeLen, fp);
									fprintf(fp, " [%d -> %d]\n", values[v], columnValues[cid][i]);
									is->RemoveItemFromSet(columnValues[cid][i]);
								}
							}
							is->AddItemToSet(values[v]);
						}
						fprintf(fp,"\n");

						delete values;
						delete is;
					}
					fclose(fp);
*/
/* TODO: uncomment
					// Search for a good example that if presence of one (discretization error) is miss-classified.
					for(uint32 r=0; r<trainingDb->GetNumRows(); ++r) {
						if (trainingDb->GetClassLabel(r) != mClasses[c])
							continue;

						ItemSet* is = trainingDb->GetRow(r)->Clone();
						uint16* values = is->GetValues();
						uint16 bestV;
						uint16 bestI;

						double codeLen = ct->CalcTransactionCodeLength(is);
						double maxCodeLen = codeLen;
						fprintf(stdout, "\n%f %s\n", codeLen, is->ToString().c_str());

						for (uint32 v = 0; v < is->GetLength(); ++v) {
							uint32 cid = value2column[values[v]];
							is->RemoveItemFromSet(values[v]);
							double sumOutlierDistribution = 0.0;
							for (uint32 i = 0; i < columnDefinition[cid]->GetLength(); ++i) {
								is->AddItemToSet(columnValues[cid][i]);
								codeLen = ct->CalcTransactionCodeLength(is);
								if (codeLen > maxCodeLen) {
									fprintf(stdout, "%f %s\n", codeLen, is->ToString().c_str());
									maxCodeLen = codeLen;
									bestV = values[v];
									bestI = columnValues[cid][i];
								}
								is->RemoveItemFromSet(columnValues[cid][i]);
							}
							is->AddItemToSet(values[v]);
						}

						fprintf(stdout, "\n");
						coverTransaction(ct, trainingDb, is, codeLen);
						fprintf(stdout, "\n");
						is->RemoveItemFromSet(bestV);
						is->AddItemToSet(bestI);
						coverTransaction(ct, trainingDb, is, codeLen);
						is->RemoveItemFromSet(bestI);
						is->AddItemToSet(bestV);


						delete values;
					}

*/
/* comment out
					// Choose outliers randomly
					set<uint32> tids;
					while (tids.size() != numPrototypeOutliers) {
						uint32 tid = RandomUtils::UniformUint32(numTrainingSamples);
						if (j == 0 && trainingDb->GetClassLabel(tid) != mClasses[c])
							tids.insert(tid);
						if (j == 1 && trainingDb->GetClassLabel(tid) == mClasses[c])
							tids.insert(tid);
					}

					// Precompute code lengths of generated outliers and probability distribution boundaries
					uint32 oid = 0;
					for (set<uint32>::iterator it = tids.begin(); it != tids.end(); ++it, ++oid) {
						ItemSet* is = trainingDb->GetRow(*it)->Clone();
						double codeLen;
						codeLenPerItem->clear();
						coverTransaction(ct, trainingDb, is, codeLen, codeLenPerItem);
						ct->CalcTransactionCodeLength(is);
						assert(codeLen == ct->CalcTransactionCodeLength(is));
						prototypeLength[oid] = is->GetLength();
						prototypeValues[oid] = is->GetValues();
						prototypeDistribution[oid] = new double[is->GetLength()];

						for (uint32 v = 0; v < is->GetLength(); ++v) {
							uint32 cid = value2column[prototypeValues[oid][v]];
							is->RemoveItemFromSet(prototypeValues[oid][v]);
							double sumOutlierDistribution = 0.0;
							for (uint32 i = 0; i < columnDefinition[cid]->GetLength(); ++i) {
								is->AddItemToSet(columnValues[cid][i]);
								codelengths[oid][columnValues[cid][i]] = ct->CalcTransactionCodeLength(is);
								outlierDistribution[oid][cid][i] = pow(2,-codelengths[oid][columnValues[cid][i]]);
								sumOutlierDistribution += outlierDistribution[oid][cid][i];
								is->RemoveItemFromSet(columnValues[cid][i]);
							}
							outlierDistribution[oid][cid][0] /= sumOutlierDistribution;
							for (uint32 i = 1; i < columnDefinition[cid]->GetLength()-1; ++i) {
								outlierDistribution[oid][cid][i] /= sumOutlierDistribution;
								outlierDistribution[oid][cid][i] += outlierDistribution[oid][cid][i-1];
							}
							outlierDistribution[oid][cid][columnDefinition[cid]->GetLength()-1] = 1.0;
							is->AddItemToSet(prototypeValues[oid][v]);
						}

						double sumPrototypeDistribution = 0.0;
						for (uint32 v = 0; v < is->GetLength(); ++v) {
							prototypeDistribution[oid][v] = pow(2,1/-(*codeLenPerItem)[prototypeValues[oid][v]]);
							sumPrototypeDistribution += prototypeDistribution[oid][v];
						}
						prototypeDistribution[oid][0] /= sumPrototypeDistribution;
						for (uint32 v = 1; v < is->GetLength()-1; ++v) {
							prototypeDistribution[oid][v] /= sumPrototypeDistribution;
							prototypeDistribution[oid][v] += prototypeDistribution[oid][v-1];
						}
						prototypeDistribution[oid][is->GetLength()-1] = 1.0;

						delete is;
					}


					string suffix = ctFiles[i]->filename + "_" + lpcFactorStr + "_" + lpcOffsetStr + "_" + numPrototypeOutliersStr + "_" + numOutliersStr + "_outliers_lengths.txt";
					string filename = ctDir + suffix;

					FILE *fp = fopen(filename.c_str(), "w");
					double n = 0.0;
					double len = 0.0;
					double delta = 0.0;
					double mean = 0.0;
					double M2 = 0.0;
					// Generate outliers randomly
					for (uint32 o = 0; o < numOutliers; ++o) {
						// Uniformly choose outlier prototype
						uint32 oid = RandomUtils::UniformUint32(numPrototypeOutliers);

						double p = RandomUtils::UniformDouble();
						uint32 v = 0;
						while (prototypeDistribution[oid][v] < p) v++;
						uint32 cid = value2column[prototypeValues[oid][v]];

						p = RandomUtils::UniformDouble();
						v = 0;
						while (outlierDistribution[oid][cid][v] < p) v++;

						// @see http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
						n += 1.0;
						len = codelengths[oid][columnValues[cid][v]];
						fprintf_s(fp, "%f\n", len);
						delta = len - mean;
						mean += delta/n;
						M2 += delta * (len - mean);
					}
					double var = M2 / (n - 1.0);
					double stdev = sqrt(var);
					fclose(fp);

					suffix = ctFiles[i]->filename + "_" + lpcFactorStr + "_" + lpcOffsetStr + "_" + numPrototypeOutliersStr + "_" + numOutliersStr + "_outliersmodel.txt";
					filename = ctDir + suffix;

					fp = fopen(filename.c_str(), "w");
					fprintf(fp, "mean: %lf\nstdev: %lf", mean, stdev);
					fclose(fp);

					for (uint32 i = 0; i < numPrototypeOutliers; ++i) {
						delete prototypeValues[i];
						delete prototypeDistribution[i];
					}

*/
				}

				// Cleanup
				for(uint32 i=0; i<numCTs; i++) {
					delete ctFiles[i];
				}
				delete[] ctFiles;
			}
		}

		// Cleanup
/*
		for (uint32 oid = 0; oid < numPrototypeOutliers; ++oid) {
			for (uint32 cid = 0; cid < numColumns; ++cid)
				delete[] outlierDistribution[oid][cid];
			delete[] outlierDistribution[oid];
		}
		delete[] outlierDistribution;

		for (uint32 o = 0; o < numPrototypeOutliers; ++o)
			delete codelengths[o];
		delete[] codelengths;

		delete prototypeLength;
		delete[] prototypeValues;
		delete[] prototypeDistribution;

		delete value2column;

		for (uint32 c = 0; c < numColumns; ++c)
			delete columnValues[c];
		delete[] columnValues;

		delete codeLenPerItem;
*/

		delete trainingDb;
		delete testDb;
	}

	// Cleanup
	for(uint16 i=0; i<mNumClasses; i++) {
		delete strClasses[i];
	}
	delete[] strClasses;
}

void OneClassClassifier::Classify() {
	char temp[200]; string s;

	// All variables with suffix T are meant for maintaining info on training data, without suffix is for test data.

	// Some settings
	string expDir = Bass::GetWorkingDir();
	mNumFolds = mConfig->Read<uint32>("numfolds");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype"));
	uint32 lpcFactor = mConfig->Read<uint32>("lpcfactor", 1);
	uint32 lpcOffset = mConfig->Read<uint32>("lpcoffset", 1);
	mInequality = StringToInequality(mConfig->Read<string>("inequality"));

	// Determine classes
	uint8 levelBak = Bass::GetOutputLevel(); Bass::SetOutputLevel(0);
	Database *db = ClassedDatabase::ReadDatabase(string("test"), expDir + "f1/", dataType);
	Bass::SetOutputLevel(levelBak);
	uint32 alphLen = db->GetAlphabetSize();
	mNumClasses = db->GetNumClasses();
	mClasses = new uint16[mNumClasses];
	memcpy(mClasses, db->GetClassDefinition(), sizeof(uint16) * mNumClasses);
	delete db;

	// Init some variables
	string **strClasses = new string *[mNumClasses];
	ClassedDatabase *testDb, *trainDb;
	uint32 **confusion = new uint32 *[mNumClasses];
	uint32 **confusionT = new uint32 *[mNumClasses];
	for(uint16 i=0; i<mNumClasses; i++) {
		_itoa(mClasses[i], temp, 10);
		strClasses[i] = new string(temp);
		confusion[i] = new uint32[2];
		confusionT[i] = new uint32[2];
	}
	string foldDir;

	_itoa(lpcFactor, temp, 10);
	string lpcFactorStr(temp);
	_itoa(lpcOffset, temp, 10);
	string lpcOffsetStr(temp);
	string interfix = "_" + lpcFactorStr + "_" + lpcOffsetStr;

	// Foreach class
	for(uint16 c=0; c<mNumClasses; c++) {
		sprintf_s(temp, 200, " * Processing Class:\t%d", mClasses[c]); s = temp; LOG_MSG(s);

		// (re-)initialize confusion matrices
		for(uint16 i=0; i<mNumClasses; i++) {
			confusion[i][0] = confusion[i][1] = 0;
			confusionT[i][0] = confusionT[i][1] = 0;
		}

		// Foreach fold
		for(uint32 f=1; f<=mNumFolds; f++) {

			sprintf_s(temp, 200, " * Processing Fold:\t%d", f); s = temp; LOG_MSG(s);

			_itoa(f, temp, 10);
			foldDir = expDir + "f" + string(temp) + "/";

			// Load train database
			trainDb = (ClassedDatabase *) Database::ReadDatabase(string("train") + *strClasses[c], foldDir, dataType);
			if(!trainDb)
				throw("OneClassClassifier::Classify -- Could not read train database!");
			// Load test database
			testDb = (ClassedDatabase *) Database::ReadDatabase("test", foldDir, dataType);
			if(!testDb)
				throw("OneClassClassifier::Classify -- Could not read test database!");

			// Load codetable
			CodeTable* ct = FicMain::LoadFirstCodeTable(trainDb, foldDir + string("train") + *strClasses[c] + string("/"));
			ct->ApplyLaplaceCorrection(lpcFactor, lpcOffset);

			// Build one-class model
			OCCModel model;
			model.label = c;
			train(trainDb, ct, model);

			string modelFile = foldDir + "model-" + *strClasses[c] + interfix + ".txt";
			FILE *fp = fopen(modelFile.c_str(), "w");
			fprintf(fp, "mean: %.2lf\nstdev: %.2lf\nconfidence: %.5lf\n", model.mean, model.stdev, model.confidence);
			fclose(fp);

			// Load test & train databases
			testDb = (ClassedDatabase *) Database::ReadDatabase("test", foldDir, dataType);
			if(!testDb)
				throw("Classifier::Classify -- Could not read test database!");
			trainDb = (ClassedDatabase *) Database::ReadDatabase("train", foldDir, dataType);
			if(!trainDb)
				throw("Classifier::Classify -- Could not read train database!");

			string resultsFile = foldDir + "results-" + *strClasses[c] + interfix + ".csv";
			predict(testDb, ct, model, confusion, resultsFile);
			resultsFile = foldDir + "Tresults-" + *strClasses[c] + interfix + ".csv";
			predict(trainDb, ct, model, confusionT, resultsFile);

		}


		string confusionFile = expDir + "confusion-" + *strClasses[c] + interfix + ".txt";
		WriteConfusionFile(confusionFile, confusion);
		confusionFile = expDir + "Tconfusion-" + *strClasses[c] + interfix + ".txt";
		WriteConfusionFile(confusionFile, confusionT);
	}

	for(uint16 c=0; c<mNumClasses; c++)
		delete[] confusion[c];
	delete[] confusion;
}

// Build one-class model
void OneClassClassifier::train(ClassedDatabase *db, CodeTable *ct, OCCModel& model)
{
	double n = 0.0;
	double len = 0.0;
	double delta = 0.0;
	double mean = 0.0;
	double M2 = 0.0;
	double m = DBL_MAX;
	double M = DBL_MIN;
	ItemSet **iss = db->GetRows();
	for(uint32 i=0; i<db->GetNumRows(); i++) { // @see http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
		n += 1.0;
		len = ct->CalcTransactionCodeLength(iss[i]);
		delta = len - mean;
		mean += delta/n;
		M2 += delta * (len - mean);
		if (len < m)
			m = len;
		else if (len > M)
			M = len;
	}
	double var = M2 / (n - 1.0);
	double stdev = sqrt(var);

	double k = 0.0;
	double confidence = 0.0;
	switch (mInequality) {
	case Chebyshev:
		k = max(abs(m - mean), abs(M - mean)) / stdev;
		confidence = 1.0 / (k*k);
		break;
	case Cantelli:
		k = (M - mean) / stdev;
		confidence = 1.0 / (1.0 + k*k);
		break;
	}

	model.mean = mean;
	model.stdev = stdev;
	model.k = k;
	model.confidence = confidence;
}

// Evaluate one-class model on dataset
void OneClassClassifier::predict(ClassedDatabase *db, CodeTable *ct, OCCModel model, uint32 **confusion, const string &filename)
{
	FILE *fp = fopen(filename.c_str(), "w");

	double len = 0.0;
	double k = 0.0;
	double confidence = 0.0;
	ItemSet **iss = db->GetRows();
	for(uint32 i=0; i<db->GetNumRows(); i++) {
		len = ct->CalcTransactionCodeLength(iss[i]);
		switch (mInequality) {
		case Chebyshev:
			k = abs(len - model.mean) / model.stdev;
			confidence = 1.0 / (k*k);
			//printf("P(|%.2lf - %.2lf| >= %.2lf %.2lf) <= %.2lf\n", len, mean, k, stdev, confidence);
			break;
		case Cantelli:
			k = (len - model.mean) / model.stdev;
			confidence = 1.0 / (1.0 + k*k);
			//printf("P( %.2lf - %.2lf  >= %.2lf %.2lf) <= %.2lf\n", len, mean, k, stdev, confidence);
			break;
		}

		uint16 actual;
		bool prediction = false;
		for(uint16 c=0; c<mNumClasses; c++) {
			if(db->GetClassLabel(i) == mClasses[c]) {
				actual = c;
				break;
			}
		}
		if (confidence < model.confidence)
			prediction = true;
		confusion[actual][prediction]++;
		fprintf(fp, "%d %lf\n", actual == model.label, confidence);
	}

	fclose(fp);
}

void OneClassClassifier::WriteConfusionFile(const string &filename, uint32 **confusion) {
	FILE *fp = fopen(filename.c_str(), "w");
	uint32 *sums = new uint32[mNumClasses];

	// To confuse zhe classes
	fprintf(fp, "Confusion matrix\n\n\t");
	for(uint32 t=0; t<mNumClasses; t++) {
		fprintf(fp, "%5d\t", mClasses[t]);
		sums[t] = 0;
	}
	fprintf(fp, "actual class\n");
	for(uint32 t=0; t<2; t++) {
		fprintf(fp, "%d\t", t);
		for(uint32 i=0; i<mNumClasses; i++) {
			sums[i] += confusion[i][t];
			fprintf(fp, "%5d\t",
				confusion[i][t]
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

void OneClassClassifier::WriteCodeLengthsFile(const string &filename, ClassedDatabase *cdb, CodeTable* ct) {
	FILE *fp;
	if(fopen_s(&fp, filename.c_str(), "w"))
		throw string("Cannot write code lengths to " + filename);
	ItemSet **iss = cdb->GetRows();
	for(uint32 i=0; i<cdb->GetNumRows(); i++) {
		fprintf_s(fp, "%f\n", ct->CalcTransactionCodeLength(iss[i]));
	}
	fclose(fp);
}

void OneClassClassifier::coverTransaction(CodeTable* ct, Database* db, ItemSet* transaction, double& codeLen, FILE* fp) {
	uint16* values = new uint16[transaction->GetLength()];
	CoverSet* cs = CoverSet::Create(db->GetDataType(), db->GetAlphabetSize());
	cs->InitSet(transaction);

	codeLen = 0.0;

	// item sets (length > 1)
	islist* isl = ct->GetItemSetList();
	for (islist::iterator cit = isl->begin(); cit != isl->end(); ++cit) {
		if(cs->Cover(*cit)) {
			double cl = CalcCodeLength((*cit)->GetUsageCount(), ct->GetCurStats().usgCountSum);
			if (fp) fprintf(fp, "(%s,%f) ", (*cit)->ToString(false, false).c_str(), cl);
			codeLen += cl;
		}
	}


	// alphabet items
	islist* sl = ct->GetSingletonList();
	uint16 i = 0;
	for (islist::iterator cit = sl->begin(); cit != sl->end(); ++cit, ++i)
		if(cs->IsItemUncovered(i)) {
			double cl = CalcCodeLength((*cit)->GetUsageCount(), ct->GetCurStats().usgCountSum);
			if (fp) fprintf(fp, "(%s,%f) ", (*cit)->ToString(false, false).c_str(), cl);
			codeLen += cl;
		}

	if (fp) fprintf(fp, ": %f", codeLen);

	delete cs;
	delete values;
	isl->clear();
	delete isl;
	sl->clear();
	delete sl;
}

void OneClassClassifier::coverTransaction(CodeTable* ct, Database* db, ItemSet* transaction, double& codeLen, unordered_map<uint16,double>* codeLenPerItem) {
	uint16* values = new uint16[transaction->GetLength()];
	CoverSet* cs = CoverSet::Create(db->GetDataType(), db->GetAlphabetSize());
	cs->InitSet(transaction);

	codeLen = 0.0;

	// item sets (length > 1)
	islist* isl = ct->GetItemSetList();
	for (islist::iterator cit = isl->begin(); cit != isl->end(); ++cit) {
		if(cs->Cover(*cit)) {
			double cl = CalcCodeLength((*cit)->GetUsageCount(), ct->GetCurStats().usgCountSum);
			codeLen += cl;
			cl /= (*cit)->GetLength();
			(*cit)->GetValuesIn(values);
			for (uint32 i=0; i < (*cit)->GetLength(); ++i) {
				(*codeLenPerItem)[values[i]] = cl;
			}
		}
	}

	// alphabet items
	islist* sl = ct->GetSingletonList();
	uint16 i = 0;
	for (islist::iterator cit = sl->begin(); cit != sl->end(); ++cit, ++i)
		if(cs->IsItemUncovered(i)) {
			double cl = CalcCodeLength((*cit)->GetUsageCount(), ct->GetCurStats().usgCountSum);
			codeLen += cl;
			(*codeLenPerItem)[i] = cl;
		}

	delete cs;
	delete values;
}

#endif // BLOCK_OCCLASSIFIER
