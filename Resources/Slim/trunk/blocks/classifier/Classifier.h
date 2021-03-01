#ifndef __CLASSIFIER_H
#define __CLASSIFIER_H

#include <Bass.h>

class Config;
class Database;
class CodeTable;
class DbFile;

class Classifier {
public:
	Classifier(const Config *config);
	~Classifier();

	void Classify();

protected:
	void DoClassification(ClassedDatabase *cdb, CodeTable **CTs, uint32 reportVal, supScoreMap **good, supScoreMap **bad, uint32 **confusion);
	void DoMajorityVoteClassification(ClassedDatabase *cdb, CodeTable **CTs, uint32 reportVal, supScoreMap **good, supScoreMap **bad, uint32 **confusion);

	void WriteTopscoreFile(const string &filename, supScoreMap ***goodT, supScoreMap ***badT, supScoreMap ***good, supScoreMap ***bad, bool percentage);

	void WriteScoreFile(const string &expDir, supScoreMap ***good, supScoreMap ***bad, bool percentage);
	void WriteConfusionFile(const string &filename, uint32 **confusion);
	uint32 LoadCodeTableForSup(Database *db, CodeTable **codeTable, const string &ctType, const string &dir, const uint32 fold, const uint16 classlabel, const uint32 support, string &filename);
		
	const Config *mConfig;

	uint32		mMinSup, mOverallMaxSup, mReportSup;
	uint16		*mClasses;
	uint32		mNumClasses;
	uint32		*mClassSizes;
	uint32		mNumFolds;
	bool		mSanitize;
};

#endif // __CLASSIFIER_H
