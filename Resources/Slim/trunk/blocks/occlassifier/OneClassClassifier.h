#ifndef __ONECLASSCLASSIFIER_H
#define __ONECLASSCLASSIFIER_H

#include <unordered_map>

#include <Config.h>

// bass
#include <db/ClassedDatabase.h>

// fic
#include "../algo/CodeTable.h"
#include "../algo/StandardCodeTable.h"

enum Inequality {
	Chebyshev = 0,	// Two-sided Chebyshev's inequality
	Cantelli		// One-sided Chebyshev's inequality, aka Cantelli's inequality
};

struct OCCModel {
	double mean;
	double stdev;
	double k;
	double confidence;
	uint16 label;
};

class OneClassClassifier {
public:
	OneClassClassifier(const Config *config);
	~OneClassClassifier();

	void Analyse();
	void Classify();

protected:
	void train(ClassedDatabase *db, CodeTable *ct, OCCModel &model);
	void predict(ClassedDatabase *db, CodeTable *ct, OCCModel model, uint32 **confusion, const string& filename);

	void WriteConfusionFile(const string &filename, uint32 **confusion);
	void WriteCodeLengthsFile(const string &filename, ClassedDatabase *cdb, CodeTable* ct);

	static Inequality StringToInequality(const string& s);

	void coverTransaction(CodeTable* ct, Database* db, ItemSet* transaction, double& codelength, FILE* fp=0);
	void coverTransaction(CodeTable* ct, Database* db, ItemSet* transaction, double& codelength, unordered_map<uint16,double>* codeLenPerItem);

private:
	const Config *mConfig;

	uint16		*mClasses;
	uint32		mNumClasses;
	uint32		mNumFolds;

	Inequality	mInequality;
};

#endif // __ONECLASSCLASSIFIER_H
