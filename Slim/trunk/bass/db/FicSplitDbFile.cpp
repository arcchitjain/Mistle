#ifdef BROKEN

#include "../global.h"
#include "Database.h"
#include "../itemstructs/Uint16ItemSet.h"
#include "../isc/ItemTranslator.h"

#include "FicSplitDbFile.h"

FicSplitDbFile::FicSplitDbFile() {
	
}
FicSplitDbFile::~FicSplitDbFile() {
	
}

Database *FicSplitDbFile::Read(string filename) {
	printf("** Reading Split FIC database from file\n");
	printf("File: %s\n", filename.c_str());
	string intdt = ItemSet::TypeToFullName(mPreferredDataType);
	printf("Internal datatype: %s\n", intdt.c_str());
	FILE *fp = NULL;
	char *bp = NULL, *buffer = new char[FICSPLITDBFILE_BUFFER_LENGTH + 1];
	fp = fopen(filename.c_str(), "r");

	uint32 numRows = 0, numItems = 0, alphaSize = 0, prunedBelow = 0, maxSetLength = 0;
	double stdDbSize;
	alphabet *a = new alphabet();
	double *stdLengths = NULL;
	uint16 *bitToVal = NULL;
	ItemSet **data = NULL;
	uint32 intVal;
	Database *db = new Database();

	if(fp != NULL) {
		uint32 versionMajor = 0, versionMinor = 0;
		printf("Reading header ... ");
		fscanf(fp, "fic-%d.%d\n", &versionMajor, &versionMinor);
		if(versionMajor != 1 || versionMinor != 2)
			throw string("Can only read fic database version 1.2 files.");
		fscanf(fp, "%d %d %d %lf %d %d\n", &numRows, &numItems, &alphaSize, &stdDbSize, &prunedBelow, &maxSetLength);

		db->SetNumRows(numRows);
		db->SetNumItems(numItems);
		db->SetStdDbSize(stdDbSize);
		db->SetPrunedBelow(prunedBelow);
		db->SetMaxSetLength(maxSetLength);

		// Read alphabet counts
		fgets(buffer, FICSPLITDBFILE_BUFFER_LENGTH, fp);
		bp = buffer;
		for(uint32 i=0; i<alphaSize; i++) {
			intVal = atoi(bp);
			a->insert(alphabetEntry(i, intVal));
			while(*bp != ' ')
				bp++;
			bp++;
		}
		db->SetAlphabet(a);

		// Check alphabet length
		/*if(a->size() > 2048 && mPreferredDataType == BA32ItemSetType )
			throw string("Alphabet too long for bit arrays (max length: 2048).");*/

		// Read standard lengths
		/*stdLengths = new double[alphaSize];
		fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp);
		bp = buffer;
		for(uint32 i=0; i<alphaSize; i++) {
			stdLengths[i] = atof(bp);
			while(*bp != ' ')
				bp++;
			bp++;
		}
		db->SetStdLengths(stdLengths);*/
		db->ComputeStdLengths();

		// Read item translator
		bitToVal = new uint16[alphaSize];
		fgets(buffer, FICSPLITDBFILE_BUFFER_LENGTH, fp);
		bp = buffer;
		for(uint32 i=0; i<alphaSize; i++) {
			bitToVal[i] = atoi(bp);
			while(*bp != ' ')
				bp++;
			bp++;
		}
		db->SetItemTranslator(new ItemTranslator(bitToVal, alphaSize));

		printf("done\n");
		printf("Database: %d rows, %d items, standard size: %.2f\n", numRows, numItems, stdDbSize);
		printf("Database: pruned below support %d, maximum set length %d\n", prunedBelow, maxSetLength);
		printf("Alphabet: %d items\n", alphaSize);

		printf("Reading data ... ");
		data = new ItemSet *[numRows];
		uint16 *row = NULL;
		uint32 idx, rowLen, cnt;
		uint16 val = 0;

		for(idx = 0; idx<numRows; idx++) {
			fgets(buffer, FICSPLITDBFILE_BUFFER_LENGTH, fp);
			if(strlen(buffer) > 0) {
				cnt = 1;
				rowLen = this->CountCols(buffer);
				row = new uint16[rowLen];
				bp = buffer;
				for(uint32 col=0; col<rowLen; col++) {
					val = atoi(bp);
					row[col] = val;
					while(*bp != ' ')
						bp++;
					bp++;
				}
				/*if(*bp == '(') {
					bp++;
					cnt = atoi(bp);
					break;
				}*/
				data[idx] = ItemSet::Create(mPreferredDataType, row, rowLen, a->size(), cnt);
				delete row;
			}
		}
		if(idx != numRows)
			throw string("Did not read numRows rows. Invalid database.");
		if(ferror(fp))
			throw string("Error while reading DB.");
		fclose(fp);
		db->SetItemSets(data);
		db->SetDataType(mPreferredDataType);
		printf("done\n");
	} else {	// File open error!
		delete[] buffer;
		throw string("Could not read database.");
	}

	delete[] buffer;

	printf("** Finished reading database.\n\n");
	return db;
}

bool FicSplitDbFile::Write(Database *db, string filename) {
	printf("** Writing FIC split database to files\n");
	printf("Files: %s.*\n", filename.c_str());

	uint16 targets[] = { 1, 2 };
	uint32 numTargets = 2;

	db->ConvertToType(Uint16ItemSetType);
	db->Optimize();

	Database *partialDb = NULL;
	ItemSet **iss = db->GetItemSets();
	uint32 numIss = db->GetNumRows();
	ItemTranslator *it = db->GetItemTranslator();
	alphabet *alph = db->GetAlphabet();

	ItemSet **classes = new ItemSet *[numTargets];
	uint16 *translated = new uint16[numTargets];
	for(uint16 i=0; i<numTargets; i++) {
		translated[i] = it->ValueToBit(targets[i]);
		classes[i] = new Uint16ItemSet(&(translated[i]), 1, alph->size());
	}

	for(uint16 i=0; i<numTargets; i++) {
		printf("\n* Building database for target class %d\n", targets[i]);
		uint32 cnt = alph->find(translated[i])->second;
		ItemSet **partialIss = new ItemSet *[cnt];
		uint32 idx = 0;
		uint32 numItems = 0;
		for(uint32 j=0; j<numIss; j++) {
			if(iss[j]->IsItemInSet(translated[i])) {
				partialIss[idx] = iss[j]->Clone();
				partialIss[idx]->Remove(classes[i]);
				numItems += partialIss[idx++]->GetLength();
			}
		}
		if(idx != cnt)
			throw string("Not so good!");

		partialDb = new Database(partialIss, cnt, numItems);
		partialDb->SetItemTranslator(new ItemTranslator(it));
		partialDb->SetAlphabet(new alphabet(*alph));
		partialDb->CountAlphabet();
		partialDb->ComputeEssentials();

		char buffer[10];
		sprintf(buffer, ".%d", targets[i]);
		string f = filename + buffer;
		FILE *fp = fopen(f.c_str(), "w");
		if(!fp)
			throw string("Could not write database.");

		// write header
		alphabet *a = partialDb->GetAlphabet();

		fprintf(fp, "fic-1.2\n");
		fprintf(fp, "%d %d %d %.5f %d %d\n", partialDb->GetNumRows(), partialDb->GetNumItems(), a->size(), partialDb->GetStdDbSize(), partialDb->GetPrunedBelow(), partialDb->GetMaxSetLength());
		for(alphabet::iterator iter = a->begin(); iter != a->end(); iter++)
			fprintf(fp, "%d ", iter->second);
		fprintf(fp, "\n");
		partialDb->GetItemTranslator()->WriteToFile(fp);

		// write database itself
		ItemSet *is;
		for(uint32 i=0; i<partialDb->GetNumRows(); i++) {
			is = partialDb->GetItemSet(i);
			if(is->GetLength() == 0)
				continue;
			is->ResetEnumerator();
			for(uint16 j=0; j<is->GetLength(); j++)
				fprintf(fp, "%d ", is->GetNext());
			fprintf(fp, "\n");
			//fprintf(fp, "%s\n", is->ToString().c_str());
		}

		fclose(fp);
		delete partialDb;
	}

	for(uint16 i=0; i<numTargets; i++)
		delete classes[i];
	delete[] classes;
	delete translated;
	printf("** Finished writing databases.\n\n");
	return true;
}

uint16 FicSplitDbFile::CountCols(char *buffer) {
	uint16 count = 0, index = 0;
	while(buffer[index] != '\n') {
		if(buffer[index++] == ' ')
			count++;
	}
	return count;
}

#endif // BROKEN
