#include "../Bass.h"

#include "Database.h"
#include "ClassedDatabase.h"
#include "../itemstructs/ItemSet.h"
#include "../isc/ItemTranslator.h"

#include <StringUtils.h>
#include <logger/Log.h>

#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
  #include <glibc_s.h>
  #include <tr1/cinttypes>
#endif
#if defined (__GNUC__)
	#include <cstring>
#endif

#include "FicDbFile.h"


FicDbFile::FicDbFile() {
	
}
FicDbFile::~FicDbFile() {
	
}

Database *FicDbFile::Read(string filename) {
	ECHO(2, printf("** Database :: \n"));
	uint32 pos = (uint32) filename.find_last_of("\\/");

	ECHO(2, printf(" * File:\t\t%s\n", pos==string::npos ? (filename.length()>55 ?	filename.substr(filename.length()-55,filename.length()-1).c_str() : filename.c_str()) : filename.substr(pos+1,filename.length()-1).c_str()));
	string intdt = ItemSet::TypeToFullName(mPreferredDataType);
	FILE *fp = NULL;
	char *bp = NULL, *buffer = new char[FICDBFILE_BUFFER_LENGTH + 1];
	if(fopen_s(&fp, filename.c_str(), "r") != 0)
		throw string("FicDbFile::Read() --- Could not open file for reading.");

	uint32 numRows = 0, alphaSize = 0, prunedBelow = 0, maxSetLength = 0, numColumns = 0, numTransactions = 0, dummyInt;
	uint32 *abNumRows = NULL, *abCounts = NULL;
	uint64 numItems = 0;
	double stdDbSize;
	bool hasColDef = false, hasBinSizes = false, isClassedDB = false, calcAbNumRows = false, hasTransactionIds = false, abWarn = false;
	alphabet *a = new alphabet();
	ItemSet **data = NULL;
	ItemSet **colDef = NULL;
	Database *db = NULL;
	ClassedDatabase *cdb = NULL;

	if(fp != NULL) {
		uint32 versionMajor = 0, versionMinor = 0;
		ECHO(3, printf(" * Reading header:\tin progress..."));
		if(fscanf_s(fp, "fic-%d.%d\n", &versionMajor, &versionMinor) < 2)
			throw string("Can only read fic database files.");
		if(versionMajor!=1 || (versionMinor!=2 && versionMinor!=3 && versionMinor!=4 && versionMinor!=5 && versionMinor!=6))
			throw string("Can only read fic database version 1.2-1.6 files.");
		if(versionMinor == 2)								// 1.2
			fscanf_s(fp, "%d %" I64d " %d %lf %d %d\n", &numRows, &numItems, &alphaSize, &stdDbSize, &prunedBelow, &maxSetLength);
		else if (versionMinor == 3) {						// 1.3
			uint32 uintBinSize;
			fscanf_s(fp, "%d %" I64d " %d %lf %d %d %d\n", &numRows, &numItems, &alphaSize, &stdDbSize, &prunedBelow, &maxSetLength, &uintBinSize);
			hasBinSizes = uintBinSize==0 ? false : true;
		} else if (versionMinor == 4) {						// 1.4
			uint32 uintBinSize, uintClasses;
			fscanf_s(fp, "%d %" I64d " %d %lf %d %d %d %d\n", &numRows, &numItems, &alphaSize, &stdDbSize, &prunedBelow, &maxSetLength, &uintBinSize, &uintClasses);
			hasBinSizes = uintBinSize==0 ? false : true;
			isClassedDB = uintClasses==0 ? false : true;
		} else if (versionMinor == 5) {						// 1.5
			uint32 uintBinSize, uintClasses;
			fscanf_s(fp, "mi: nR=%d nT=%d nI=%" I64d " aS=%d sS=%lf mL=%d b?=%d c?=%d\n", &numRows, &numTransactions, &numItems, &alphaSize, &stdDbSize, &maxSetLength, &uintBinSize, &uintClasses);
			hasBinSizes = uintBinSize==0 ? false : true;
			isClassedDB = uintClasses==0 ? false : true;
		} else {											// 1.6
			uint32 uintBinSize, uintClasses, uintTids;
			fscanf_s(fp, "mi: nR=%d nT=%d nI=%" I64d " aS=%d sS=%lf mL=%d b?=%d c?=%d tid?=%d\n", &numRows, &numTransactions, &numItems, &alphaSize, &stdDbSize, &maxSetLength, &uintBinSize, &uintClasses, &uintTids);
			hasBinSizes = uintBinSize==0 ? false : true;
			isClassedDB = uintClasses==0 ? false : true;
			hasTransactionIds = uintTids==0? false : true;
		}

		if(isClassedDB) {
			cdb = new ClassedDatabase();
			db = cdb;
		} else
			db = new Database();

		// Path & filename
		string path, ext;
		Database::DeterminePathFileExt(path, filename, ext, DbFileTypeFic);
		db->SetPath(path);
		db->SetFilename(filename);

		// Some statistics
		db->SetNumRows(numRows);
		db->SetNumItems(numItems);
		db->SetStdDbSize(stdDbSize);
		db->SetMaxSetLength(maxSetLength);
		db->SetHasBinSizes(hasBinSizes);
		db->SetHasTransactionIds(hasTransactionIds);

		// New Stylee (v1.5+) Header Reading
		if(versionMinor >= 5) {
			fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp);
			while(buffer[0] <= 'z' && buffer[0] >= 'a' && buffer[1] <= 'z' && buffer[1] >= 'a' && buffer[2] == ':') {
				string three(buffer,3);

				// Alphabet -- implicitly assumes the next line to contain the Alphabet Counts 
				if(three.compare("ab:") == 0) {
					uint16* abItems = StringUtils::TokenizeUint16(buffer+4, dummyInt, " ");
					fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp);
					abCounts = StringUtils::TokenizeUint32(buffer+4, dummyInt, " ");
					for(uint32 i=0; i<alphaSize; i++)
						a->insert(alphabetEntry(abItems[i], abCounts[i]));
					db->SetAlphabet(a);
					delete[] abItems;

					if(alphaSize > 128 && mPreferredDataType == BM128ItemSetType) {
						mPreferredDataType = Uint16ItemSetType;
						abWarn = true;
					}
				}

				// Alphabet Row Counts
				else if(three.compare("ar:") == 0) {
//					abNumRows = StringUtils::TokenizeUint32(buffer+4, dummyInt);
//					db->SetAlphabetNumRows(abNumRows);
						;
				}

				// Item Translator
				else if(three.compare("it:") == 0) {
					db->SetItemTranslator(new ItemTranslator(StringUtils::TokenizeUint16(buffer+4, dummyInt), alphaSize));
				}

				// Column Definition
				else if(three.compare("cd:") == 0) {
					hasColDef = true;
					string* colDefStrAr = StringUtils::Tokenize(buffer+4, numColumns, ",", " ");
					colDef = new ItemSet*[numColumns];
					for(uint32 i=0; i<numColumns; i++) {
						uint32 colLength = 0;
						uint16 *iset = StringUtils::TokenizeUint16(colDefStrAr[i], colLength, " ", "");
						colDef[i] = ItemSet::Create(mPreferredDataType, iset, colLength, alphaSize, 1);
						delete[] iset;
					}
					delete[] colDefStrAr;
					db->SetColumnDefinition(colDef, numColumns);
				}

				// Classes
				else if(three.compare("cl:") == 0) {
					uint32 numClasses = 0;
					uint16 *classes = StringUtils::TokenizeUint16(buffer+4, numClasses);
					db->SetClassDefinition(numClasses, classes);
				}

				// Read next line, while-loop exists when no header-element is present
				if(fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp) == NULL)
					break;
			}

			if(abNumRows != NULL) {	// - AbNumRows reeds geladen
				calcAbNumRows = false;
				delete[] abCounts;
			} else {				// - AbNumRows niet geladen
				if(hasBinSizes) {		// - AbNumRows moet uitgerekend worden
					calcAbNumRows = true;
					abNumRows = abCounts;
				} else {				// - AbNumRows kan gekopieerd worden
					calcAbNumRows = false;
					db->SetAlphabetNumRows(abCounts);
				}
			}
		} else {
			// Old Stylee (< v1.5), only for backward compatibility

			// Read alphabet elements
			fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp);
			string ab = StringUtils::Trim(buffer, " ");
			abCounts = StringUtils::TokenizeUint32(ab, dummyInt, " ");
			for(uint32 i=0; i<alphaSize; i++)
				a->insert(alphabetEntry(i, abCounts[i]));
			db->SetAlphabet(a);

			if(hasBinSizes) {
				calcAbNumRows = true;
				abNumRows = abCounts;
			} else {
				db->SetAlphabetNumRows(abCounts);				
			}

			// Read ItemTranslator
			fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp);
			string it = StringUtils::Trim(buffer," ");
			db->SetItemTranslator(new ItemTranslator(StringUtils::TokenizeUint16(it, dummyInt), alphaSize));

			if(isClassedDB) {
				fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp);
				bp = buffer;
				uint32 numClasses = atoi(bp);
				while(*bp != ' ')
					bp++;
				bp++;
				uint16* cls = new uint16[numClasses];
				for(uint32 i=0; i<numClasses; i++) {
					cls[i] = atoi(bp);
					while(*bp != ' ')
						bp++;
					bp++;
				}
				cdb->SetClassDefinition(numClasses, cls);
			}

			fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp);	// first data row
		}

		ECHO(3, printf("\r * Reading header:\tdone.         \n"));

		db->ComputeStdLengths();
		ECHO(2, printf(" * Database:\t\t%dt %dr, %" I64d "i, %.2lfbits\n", numTransactions, numRows, numItems, stdDbSize));
		ECHO(2, printf(" * \t\t\tpruned below support %d, maximum set length %d\n", prunedBelow, maxSetLength));
		if(hasBinSizes) {
			ECHO(2, printf(" * \t\t\tdatabase has bin sizes.\n"));
		}
		if(hasTransactionIds) {
			ECHO(2, printf(" * \t\t\tdatabase has transaction id's.\n"));
		}

		ECHO(2, printf(" * Alphabet:\t\t%d items\n", alphaSize));
		ECHO(2, printf(" * Internal datatype:\t%s\n", intdt.c_str()));
		if(abWarn == true || (alphaSize > 128 && mPreferredDataType == BM128ItemSetType)) {
			LOG_WARNING("|ab| > 128 : won't fit in BM128, using Uint16");
			mPreferredDataType = Uint16ItemSetType;
		}

		if(calcAbNumRows) {
			for(uint32 i=0; i<alphaSize; i++)
				abNumRows[i] = 0;
		}


		ECHO(3, printf(" * Reading data:\tin progress..."));
		data = new ItemSet *[numRows];
		uint16 *row = NULL;
		uint32 curRow=0, rowLen=0, cnt=1, numTransactions = hasBinSizes ? 0 : numRows;
		uint16 val = 0;
		uint64 tid = 0;

		uint16 *classLabels = NULL;
		if(isClassedDB)
			classLabels = new uint16[numRows];

		while(curRow<numRows) {
			if(strlen(buffer) > 0) {
				bp = buffer;
				if(versionMinor >= 5) {
					if(hasTransactionIds) {
						++bp; // '['
#if defined (_WINDOWS)
						tid = _atoi64(bp);
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
						tid = atoll(bp);
#endif
						while(*bp != ' ')
							++bp;
						++bp; // ' '
					}
					rowLen = atoi(bp);
					while(*bp != ':')
						++bp;
					++bp;	// ':'
					++bp;	// ' '
				} else {
					rowLen = this->CountCols(buffer);
					if(hasBinSizes) rowLen--;
					if(isClassedDB) rowLen--;
				}

				row = new uint16[rowLen];
				for(uint32 col=0; col<rowLen; col++) {
					val = atoi(bp);
					row[col] = val;
					while(*bp != ' ')
						++bp;
					++bp;
					if(calcAbNumRows)
						abNumRows[val]++;
				}
				if(hasBinSizes) {
					while(*bp != '(')
						++bp;
					++bp;
					cnt = atoi(bp);
					numTransactions += cnt;
				}
				if(isClassedDB) {
					while(*bp != '[')
						++bp;
					++bp;
					classLabels[curRow] = atoi(bp);
				}
				data[curRow] = ItemSet::Create(mPreferredDataType, row, rowLen, (uint32) a->size(), cnt);
				if(hasTransactionIds) data[curRow]->SetID(tid);
				delete[] row;
			}

			fgets(buffer, FICDBFILE_BUFFER_LENGTH, fp);
			curRow++;
		}
		if(curRow != numRows)
			throw string("Did not read numRows rows. Invalid database.");
		if(ferror(fp))
			throw string("Error while reading DB.");
		fclose(fp);

		if(calcAbNumRows)	// alleen voor oude file versie
			db->SetAlphabetNumRows(abNumRows);

		db->SetRows(data);
		db->SetNumTransactions(numTransactions);
		db->SetDataType(mPreferredDataType);
		if(isClassedDB)
			cdb->SetClassLabels(classLabels);
		ECHO(3, printf("\r * Reading data:\tdone          \n"));
	} else {	// File open error!
		delete[] buffer;
		delete a;
		throw string("Could not read database: ") + filename;
	}

	delete[] buffer;

	ECHO(3, printf("** Finished reading database.\n"));
	ECHO(2, printf("\n"));
	return db;
}

bool FicDbFile::Write(Database *db, const string &fullpath) {
	ECHO(2, printf("** Writing FIC database to file\n"));
	size_t pos = fullpath.find_last_of("\\/");
	// Print max 55 characters of `fullpath`, if that long and includes a directory-separator cut off at that spot.
	ECHO(2, printf(" * File:\t\t%s\n", (fullpath.length() > 55 ? (pos == string::npos ? fullpath.substr(fullpath.length()-55,fullpath.length()-1)  : fullpath.substr(pos+1,fullpath.length()-1)) :	fullpath).c_str() ));
	db->ComputeEssentials();	// zouden al computed moeten zijn, maar soit

	FILE *fp;
	if(fopen_s(&fp, fullpath.c_str(), "w") != 0)
		throw string("Could not write database.");

	// Path & filename
	string path, filename = fullpath, ext;
	Database::DeterminePathFileExt(path, filename, ext, DbFileTypeFic);
	db->SetPath(path);
	db->SetFilename(filename);

	bool isDbClassed = db->GetType() == DatabaseClassed;
	ClassedDatabase *cdb = NULL;
	if(isDbClassed)
		cdb = (ClassedDatabase *)(db);

	//////////////////////////////////////////////////////////////////////////
	// :: Header ::
	//////////////////////////////////////////////////////////////////////////

	// mi: Meta Information (mandatory)
	alphabet *a = db->GetAlphabet();
	bool hasBinSizes = db->HasBinSizes();
	bool hasTransactionIds = db->HasTransactionIds();
	fprintf(fp, "fic-1.6\n");
	uint64 ni = db->GetNumItems();
	fprintf(fp, "mi: nR=%d nT=%d nI=%" I64d " aS=%d sS=%.5lf mL=%d b?=%d c?=%d tid?=%d\n", db->GetNumRows(), db->GetNumTransactions(), ni, a->size(), db->GetStdDbSize(), db->GetMaxSetLength(), hasBinSizes?1:0, isDbClassed?1:0, hasTransactionIds?1:0);	// mi: MetaInfo

	// ab: Alphabet (mandatory)
	size_t abLen = a->size();
	fprintf(fp, "ab: "); uint32 i=0;
	for(alphabet::iterator iter = a->begin(); iter != a->end(); i++, ++iter)
		fprintf(fp, "%d%s", iter->first, i<abLen-1 ? " " : "");
	fprintf(fp, "\n");

	// ac: Alpabet Counts (mandatory)
	fprintf(fp, "ac: "); i=0;
	for(alphabet::iterator iter = a->begin(); iter != a->end(); i++, ++iter)
		fprintf(fp, "%d%s", iter->second, i<abLen-1 ? " " : "");
	fprintf(fp, "\n");

	// ar: Alphabet Row Counts (optional, very much preferred if binned db)
	if(db->GetAlphabetNumRows() != NULL) {
		uint32 *abNuRo = db->GetAlphabetNumRows();
		fprintf(fp, "ar: ");
		for(uint32 i=0; i<abLen; i++)
			fprintf(fp, "%d%s", abNuRo[i], i<abLen-1 ? " " : "");
		fprintf(fp, "\n");
	}

	// it: Item Translator (optional)
	if(db->GetItemTranslator() != NULL)
		fprintf(fp, "it: %s\n", db->GetItemTranslator()->ToString().c_str());

	// cd: Column Definition (optional)
	if(db->HasColumnDefinition() && db->GetNumColumns() > 1) {
		fprintf(fp, "cd: ");
		uint32 numCol = db->GetNumColumns();
		ItemSet **colDef = db->GetColumnDefinition();
		for(uint32 i=0; i<numCol; i++)
			fprintf(fp, "%s%s", colDef[i]->ToString(false,false).c_str(), (i<numCol-1) ? ", " : "");
		fprintf(fp, "\n");
	}

	// cl: Classes (optional)
	if(db->GetNumClasses() > 0) {
		uint32 num = db->GetNumClasses();
		uint16* cls = db->GetClassDefinition();
		fprintf(fp, "cl: ");
		for(uint32 i=0; i<num; i++)
			fprintf(fp, "%d%s", cls[i], (i<num-1 ? " " : ""));
		fprintf(fp, "\n");
	}

	// dc: Data Integrity Code (optional?)
	/*if(db->HasDataIntegrityCode()) {

	}*/
	// 

	//////////////////////////////////////////////////////////////////////////
	// :: Data ::
	//////////////////////////////////////////////////////////////////////////

	ItemSet *is;
	uint32 *vals = new uint32[db->GetMaxSetLength()];
	uint32 islen;
	for(uint32 i=0; i<db->GetNumRows(); i++) {
		is = db->GetRow(i);
		islen = is->GetLength();
		if(islen == 0)
			continue;
#if defined (_MSC_VER) && defined (_WINDOWS)
		if(hasTransactionIds) fprintf(fp, "[%I64u] ", is->GetID());
#elif defined (__GNUC__) && (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
		if(hasTransactionIds) fprintf(fp, "[%lu] ", is->GetID());
#endif
		fprintf(fp, "%d: %s", islen, is->ToString(false,false).c_str());
		if(hasBinSizes)	fprintf(fp, " (%d)", is->GetUsageCount());
		if(isDbClassed)	fprintf(fp, " [%d]", cdb->GetClassLabel(i));
		fprintf(fp, "\n");
	}
	delete[] vals;

	fclose(fp);

	ECHO(2, printf("** Finished writing database.\n\n"));
	return true;
}

uint32 FicDbFile::CountCols(char *buffer) {
	uint32 count = 0, index = 0;
	while(buffer[index] != '\n') {
		if(buffer[index++] == ' ')
			count++;
	} if(index != 0 && buffer[index] == '\n' && buffer[index-1] != ' ')
		count++;
	return count;
}
