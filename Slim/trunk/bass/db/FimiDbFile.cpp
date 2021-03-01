#include "../Bass.h"

#include "Database.h"
#include "../itemstructs/Uint16ItemSet.h"

#include "FimiDbFile.h"

FimiDbFile::FimiDbFile() {

}
FimiDbFile::~FimiDbFile() {

}

Database *FimiDbFile::Read(string filename) {
	ECHO(2, printf("** Database :: \n"));
	ECHO(2,printf(" * Type:\t\tFIMI database\n"));
	uint32 pos = (uint32) filename.find_last_of("\\/");
	ECHO(2, printf(" * File:\t\t%s\n", pos==string::npos ? (filename.length()>55 ?	filename.substr(filename.length()-55,filename.length()-1).c_str() : filename.c_str()) : filename.substr(pos+1,filename.length()-1).c_str()));
	FILE *fp = NULL;
	char *bp = NULL, *buffer = new char[FIMIDBFILE_BUFFER_LENGTH + 1];
	if(fopen_s(&fp, filename.c_str(), "r") != 0)
		throw string("FimiDbFile::Read() --- Could not read file.");

	uint32 numRows = 0;
	uint16 maxCols = 0;
	ItemSet **data = NULL;
	uint16 *rowLengths = NULL;
	bool hasBinSizes = false;
	bool hasTransactionIds = false;		// accepted data format: '[tid] item_0 item_1 .. item_n'
	bool isRectangular = true;
	ItemSet **colDef = NULL;
	uint32 numColumns = 0, colCount;
	bool calcColDef = false;
	uint32 numTransactions = 0; 
	uint64 transactionId = 0; 

	if(fp != NULL) {
		ECHO(2,printf(" * Scanning file:\tin progress..."));
		if(fgets(buffer, FIMIDBFILE_BUFFER_LENGTH, fp) == NULL || strlen(buffer) == 0) {
			delete[] buffer;
			throw string("Error while scanning DB (at first line).");
		}
		size_t bufferLen = strlen(buffer);
		if(bufferLen == FIMIDBFILE_BUFFER_LENGTH-1)
			throw string("FimiDbFile::Read() -- Line does not fit in buffer.");
		uint32 index = 0;
		while(buffer[index] != '\n') {
			if(buffer[index++] == '(') {
				hasBinSizes = true;
				break;
			}
		}
		
		maxCols = this->CountCols(buffer);
		if(buffer[0] == '[')
			hasTransactionIds = true;
		do {
			colCount = this->CountCols(buffer);
			if(colCount > 0)
				numRows++;
			if(colCount != maxCols) {
				isRectangular = false;
				if(colCount > maxCols)
					maxCols = colCount;
			}
		} while(fgets(buffer, FIMIDBFILE_BUFFER_LENGTH, fp) != NULL && strlen(buffer) > 0);
		
		if(hasBinSizes) // substract one for bin size column
			--maxCols;
		if(hasTransactionIds) // and the same for transaction ids
			--maxCols;
		ECHO(2,printf("\r * Scanning file: done                       \n"));
		ECHO(2,printf(" * Database:\t%d rows, maximal %d columns\n", numRows, maxCols));
		ECHO(2,printf(" * \t\thas bin sizes: %s\n", hasBinSizes ? "yes" : "no"));
		ECHO(2,printf(" * \t\thas transaction id's: %s\n", hasTransactionIds ? "yes" : "no"));
		if(ferror(fp)) {
			delete[] buffer;
			throw string("Error while scanning DB.");
		}
		rewind(fp);

		data = new ItemSet *[numRows];
		rowLengths = new uint16[numRows];
		uint16 *row = NULL;
		uint32 idx = 0;
		uint32 cnt = 1;
		numTransactions = hasBinSizes ? 0 : numRows;

		if(isRectangular) {
			calcColDef = true;
			numColumns = maxCols;
			uint16 *emptySet = new uint16[0];
			colDef = new ItemSet*[numColumns];
			for(uint32 i=0; i<numColumns; i++)
				colDef[i] = ItemSet::CreateEmpty(mPreferredDataType, 0, 1);
			delete[] emptySet;
		}

		ECHO(2,printf(" * Reading database:\tin progress...\r"));
		while(fgets(buffer, FIMIDBFILE_BUFFER_LENGTH, fp) != NULL) {
			if(strlen(buffer) > 0 && buffer[0] != '\n') {
				rowLengths[idx] = this->CountCols(buffer);
				if(hasBinSizes) --rowLengths[idx];
				if(hasTransactionIds) --rowLengths[idx];
				row = new uint16[rowLengths[idx]];
				bp = buffer;
				if(hasTransactionIds) {
					++bp; // skip '['
#if defined (_WINDOWS)
					transactionId = _atoi64(bp);
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
					transactionId = atoll(bp);
#endif
						while(*bp != ' ')
					while(*bp != ' ')
						++bp;
					++bp;
				}
				for(uint32 col=0; col<rowLengths[idx]; col++) {
					uint16 val = atoi(bp);
					if(calcColDef) {
						colDef[col]->AddItemToSet(val);
					}
					row[col] = val;
					while(*bp != ' ' && *bp != '\n')
						bp++;
					bp++;
				}
				if(hasBinSizes) {
					while(*bp != '(')
						++bp;
					++bp;
					cnt = atoi(bp);
					numTransactions += cnt;
				}
				data[idx] = ItemSet::Create(mPreferredDataType, row, rowLengths[idx], 0, cnt);
				data[idx]->SetID(transactionId);
				delete[] row;
				idx++;
			}
		}
		if(ferror(fp))
			throw string("FimiDbFile::Read() --- Error while reading DB.");
		fclose(fp);
		ECHO(2,printf(" * Reading database:\tdone          \n"));
	} else {	// File open error!
		delete[] buffer;
		throw string("FimiDbFile::Read() --- Could not read database.");
	}

	delete[] rowLengths;
	delete[] buffer;

	Database *db = new Database(data, numRows, hasBinSizes, numTransactions);
	db->SetHasTransactionIds(hasTransactionIds);
	db->ComputeEssentials();
	if(calcColDef)
		db->SetColumnDefinition(colDef, numColumns);

	// Path & filename
	string path, ext;
	Database::DeterminePathFileExt(path, filename, ext, DbFileTypeFimi);
	db->SetPath(path);
	db->SetFilename(filename);

	ECHO(3,printf("** Finished reading database.\n"));
	ECHO(2,printf("\n"));
	return db;
}

uint16 FimiDbFile::CountCols(char *buffer) {
	uint32 count = 0, index = 0, find = 0;
	while(buffer[index] != '\n')
		if(buffer[index++] == ' ')
			count++;
	if(buffer[index] == '\n' && index > 0 && buffer[index-1] != ' ')
		count++;
	return count;
}

bool FimiDbFile::Write(Database *db, const string &fullpath) {
	printf("Cannot write FIMI database. (Not useful anyway...)\n");
	return false;
}
