#ifdef BLOCK_CLASSIFIER

#include "../../global.h"

// -- bass
#include <db/Database.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <itemstructs/ItemSet.h>

#include "../../FicConf.h"
#include "../../FicMain.h"
#include "../krimp/codetable/coverfull/CFCodeTable.h"

#include "CtAnalyser.h"

CtAnalyser::CtAnalyser(ItemSet *targetMask) {
	mTargetMask = targetMask;
}
CtAnalyser::~CtAnalyser() {

}

void CtAnalyser::AnalyseCodeTable(const string outFile, CodeTable *ct) {
	ct->WriteCodeLengthsToFile(outFile);
}

void CtAnalyser::AnalyseTrainCoverage(const string outFile, CodeTable *ct, Database *db) {
	string s = outFile + ".traincover";
	FILE *fpAll = fopen(s.c_str(), "w");
	if(!fpAll)
		throw string("CtAnalyser could not write file: ") + s;

	/*s = outFile + ".dat";
	FILE *fp = fopen(s.c_str(), "w");
	if(!fp)
		throw string("CtAnalyser could not write file: ") + s;*/

	uint32 numRows = db->GetNumRows(); //, len;
	ItemSet *is;
	//double codeLen;
	//uint16 *temp = new uint16[ct->GetCurNumSets() + db->GetAlphabetSize()];
	for(uint32 r=0; r<numRows; r++) {
		is = db->GetRow(r)->Clone();
		is->Remove(mTargetMask);
		fprintf(fpAll, "%f,", ct->CalcTransactionCodeLength(is));
		/*len = ct->EncodeRow(is, codeLen, temp);
		fprintf(fpAll, "%f,", codeLen);
		for(uint32 i=0; i<len; i++)
			fprintf(fp, "%d ", temp[i]);
		fprintf(fp, "\n");*/
		delete is;
	}
	fseek(fpAll, -1, SEEK_CUR); /* use return value ? */
	fprintf(fpAll, "\n");
	fclose(fpAll);
	//fclose(fp);
	//delete[] temp;

#if 0
	string exec = FicMain::gExecDir + "fim_closed.exe \"" + s + "\" 2 \"" + s + ".fi\"";
	system(exec.c_str());	// !!! exit code checken
	if(!FicConf::FileExists(s + ".fi"))
		throw string("CtAnalyser::AnalyseTrainCoverage -- Frequent ItemSet generation failed.");

	/* <DirtyHack> */	// !!! lellukke fix, ooit 'ns opruimen
	ItemSetType ist = db->GetDataType();
	db->SetDataType(Uint16ItemSetType); 
	/* </DirtyHack> */
	ItemSetCollection::ConvertIscFile(db, s + ".fi", s + ".isc", IscTextFileType, false, FOLengthDesc);
	/* <CleanUpDirtyHack> */ 
	db->SetDataType(ist);
	/* </CleanUpDirtyHack> */

	string t = s + ".fi";
	remove( t.c_str() );
	t += "s";
	if(!FicConf::FileExists(t))
		throw string("CtAnalyser::AnalyseTrainCoverage -- Frequent ItemSet conversion failed.");

	// Read ItemSetCollection
	ItemSetType istBak = db->GetDataType(); db->SetDataType(Uint16ItemSetType);
	ItemSetCollection *isc = new ItemSetCollection(db);
	isc->OpenFile(t);
	uint32 num = isc->GetLength();

	// Write back-translated frequent ct elems
	ItemSet **map = ct->GetEncodingMap();
	string u = outFile + ".combis";
	FILE *fpCombis = fopen(u.c_str(), "w");
	if(!fpCombis)
		throw string("CtAnalyser could not write file: ") + u;
	uint32 ctNumSets = ct->GetCurNumSets();
	uint32 *values = new uint32[ctNumSets + db->GetAlphabetSize()];
	uint32 val;
	for(uint32 i=0; i<num; i++) {
		is = isc->ReadItemSet();
		fprintf(fpCombis, "%dx\n", is->GetCount());
		is->GetValuesIn(values);
		len = is->GetLength();
		for(uint32 i=0; i<len; i++) {
			val = values[i];
			if(val >= ctNumSets) // alphabet element
				fprintf(fpCombis, "\t%d\n", val - ctNumSets);
			else				// itemset
				fprintf(fpCombis, "\t%s\n", map[val]->ToString().c_str());
		}
		delete is;
	}
	fclose(fpCombis);
	delete[] map;
	delete[] values;
	db->SetDataType(istBak);

	// Delete Frequent Itemsets
	remove(t.c_str());
#endif
}

void CtAnalyser::InitTestCover(const string outFile, const uint32 numTargets, const uint16 *targets) {
	mFp = fopen(outFile.c_str(), "w");
	if(!mFp)
		throw string("CtAnalyser could not write file: ") + outFile;
	mNumTargets = numTargets;
	for(uint32 i=0; i<mNumTargets; i++)
		fprintf(mFp, "lenClass%d;", targets[i]);
	fprintf(mFp, "mWinCorrect;mPosWin2nd;mNegWin2nd;evaluation;classifiedClass;actualClass\n");
}
void CtAnalyser::AddTestCoverRow(const double *codeLens, const uint16 evaluation, const uint16 winner, const uint16 actual) {
	uint32 runnerUp = winner == 0 ? 1 : 0;
	for(uint32 i=0; i<mNumTargets; i++) {
		if(codeLens[i] < codeLens[runnerUp] && codeLens[i] > codeLens[winner])
			runnerUp = i;
		fprintf(mFp, "%f;", codeLens[i]);
	}
	if(winner == actual)
		fprintf(mFp, "NA;%f;NA;%d;%d;%d\n", codeLens[runnerUp] - codeLens[winner], evaluation, winner, actual);
	else
		fprintf(mFp, "%f;NA;%f;%d;%d;%d\n", codeLens[actual]-codeLens[winner], codeLens[runnerUp] - codeLens[winner], evaluation, winner, actual);
}
void CtAnalyser::EndTestCover() {
	fclose(mFp);
	mFp = NULL;
}
#endif // BLOCK_CLASSIFIER
