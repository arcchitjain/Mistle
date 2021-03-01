#include "../Bass.h"

#include "Database.h"
#include "../itemstructs/ItemSet.h"
#include "../isc/ItemTranslator.h"

#include "DbAnalyser.h"

DbAnalyser::DbAnalyser() {
	
}

DbAnalyser::~DbAnalyser() {

}

void DbAnalyser::Analyse(Database *db, string &outputFile) {
	ECHO(2,	printf("** Analysing database\n"));
	uint32 pos = (uint32) outputFile.find_last_of("\\/");
	ECHO(2, printf(" * Output File:\t%s\n", pos==string::npos ? outputFile.substr(outputFile.length()-55,outputFile.length()-1).c_str() : outputFile.substr(pos+1,outputFile.length()-1).c_str()));
	FILE *fp;
	if(fopen_s(&fp, outputFile.c_str(), "w") != 0)
		throw string("Could not write analysis file.");

	// !!! db->Optimize();
	db->ComputeEssentials();
	alphabet *a = db->GetAlphabet();
	double *stdLengths = db->GetStdLengths();

	ECHO(2, printf(" * Writing analysis:\tin progress..."));
	// write some basic information
	fprintf(fp, "** KRIMP database analysis **\n\n");
	fprintf(fp, "* General information\n\n");
	fprintf(fp, "Number of rows:\t\t\t%d\n", db->GetNumRows());
	fprintf(fp, "Has bin sizes:\t\t\t%s\n", db->HasBinSizes() ? "yes" : "no");
	if(db->HasBinSizes())
		fprintf(fp, "# transactions:\t\t\t%d\n", db->GetNumTransactions());
	fprintf(fp, "Number of items:\t\t%d\n", db->GetNumItems());
	fprintf(fp, "Average row length:\t\t%.2f\n", db->GetNumItems() / (float)(db->HasBinSizes() ? db->GetNumTransactions() : db->GetNumRows()));
	fprintf(fp, "Alphabet length:\t\t%d\n", a->size());
	fprintf(fp, "Standard DB size:\t\t%.2f\n", db->GetStdDbSize());
	fprintf(fp, "Current data type:\t\t%s\n", db->GetDataType() == Uint16ItemSetType ? "uint16" : "bit");

	// write alphabet, counts, standard lengths
	fprintf(fp, "\n* Alphabet\n\nValue\t\tCount\t\t\t\tStdLength\n");
	uint16 idx=0;
	ItemTranslator *it = db->GetItemTranslator();
	float divider = (float)db->GetNumTransactions();
	if(it==NULL)
		for(alphabet::iterator iter = a->begin(); iter != a->end(); iter++)
			fprintf(fp, "%d\t\t%6d (%.1f%%)\t\t\t%.3f\n", iter->first, iter->second, 100*iter->second/divider, stdLengths[idx++]);
	else
		for(alphabet::iterator iter = a->begin(); iter != a->end(); iter++)
			fprintf(fp, "%d=>%d\t\t%6d (%.1f%%)\t\t\t%.3f\n", iter->first, it->BitToValue(iter->first), iter->second, 100*iter->second/divider, stdLengths[idx++]);

	// row length counts
	uint32 *rowLengths = new uint32[db->GetMaxSetLength()+1];
	uint32 *transLengths = new uint32[db->GetMaxSetLength()+1];
	ItemSet *is;
	for(uint32 i=0; i<=db->GetMaxSetLength(); i++)
		rowLengths[i] = transLengths[i] = 0;
	for(uint32 i=0; i<db->GetNumRows(); i++) {
		is = db->GetRow(i);
		rowLengths[is->GetLength()]++;
		transLengths[is->GetLength()] += is->GetUsageCount();
	}
	fprintf(fp, "\n* Row lengths:\n\n");
	if(db->HasBinSizes())
		for(uint32 i=1; i<=db->GetMaxSetLength(); i++)
			fprintf(fp, " %4d => %5d  |  %5d (%3.1f%%)\n", i, rowLengths[i], transLengths[i], transLengths[i]==0?0.0:100*rowLengths[i]/(float)transLengths[i]);
	else
		for(uint32 i=1; i<=db->GetMaxSetLength(); i++)
			fprintf(fp, " %4d => %5d\n", i, rowLengths[i]);
	delete[] rowLengths;
	delete[] transLengths;

	fclose(fp);
	ECHO(2, printf("\r * Writing analysis:\tdone          \n"));
	ECHO(3, printf("** Finished database analysis.\n"));
	ECHO(2, printf("\n"));
}
