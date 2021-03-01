#ifndef __CTANALYSER_H
#define __CTANALYSER_H

class CodeTable;

class CtAnalyser {
public:
	CtAnalyser(ItemSet *targetMask);
	~CtAnalyser();

	void AnalyseCodeTable(const string outFile, CodeTable *ct);
	void AnalyseTrainCoverage(const string outFile, CodeTable *ct, Database *db);

	void InitTestCover(const string outFile, const uint32 numTargets, const uint16 *targets);
	void AddTestCoverRow(const double *codeLens, const uint16 evaluation, const uint16 classified, const uint16 actual);
	void EndTestCover();

protected:
	ItemSet		*mTargetMask;
	uint32		mNumTargets;
	FILE		*mFp;
};

#endif // __CTANALYSER_H
