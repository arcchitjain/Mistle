#ifdef BLOCK_STREAM

#include "../../global.h"
#include <time.h>

// qtils
#include <Config.h>
#include <logger/Log.h>
#include <RandomUtils.h>

// bass
#include <db/Database.h>
#include <isc/ItemSetCollection.h>

// fic
#include "../../FicMain.h"
#include "../../algo/CodeTable.h"
#include "Histogram.h"

#include "StreamAlgo.h"

StreamAlgo::StreamAlgo(const uint32 blockSize, const uint32 maxIterations, bool dbSizeOnly) {
	mBlockSize = blockSize;
	mMaxIterations = maxIterations;
	mDbSizeOnly = dbSizeOnly;
	mStorageDir = Bass::GetWorkingDir();
}

StreamAlgo::~StreamAlgo() {

}
CodeTable* StreamAlgo::FindGoodEnoughCT(Config *config, Database *odb, const uint32 offset, const float acldThresh) {
	char temp[500]; string s;
	sprintf_s(temp, 500, "Finding Good Enough CT @ %d", offset); s = temp; LOG_MSG(s);
	sprintf_s(temp, 500, "#blocks\t#rows\tCTSize\tTotalSize\tIR"); s = temp; LOG_MSG(s);

	string iscName = config->Read<string>("iscname");
	string pruneStrategy = config->Read<string>("prunestrategy","nop");
	string iscFullPath = Bass::GetWorkingDir() + iscName + ".isc";

	uint32 iterations = min(mMaxIterations, (odb->GetNumRows()-offset) / mBlockSize);
	CodeTable *ct, *prevCT = NULL;
	Database *prevDB = NULL;
	uint32 numRows;
	double acld = 0.0;
	bool goodEnough = false;

	for(uint32 i=0; i<iterations&&!goodEnough; i++) {
		// Construct temporary db
		numRows = (i+1) * mBlockSize;
		ItemSet **iss = new ItemSet *[numRows];
		for(uint32 r=0; r<numRows; r++)
			iss[r] = odb->GetRow(r+offset)->Clone();
		Database *db = new Database(iss, numRows, false);
		db->UseAlphabetFrom(odb, false); // computes StdLengths for new database using old alphabet (without Laplace..)
		db->ComputeStdLengths(true);		// but now we add Laplace correction
		db->SetMaxSetLength(odb->GetMaxSetLength());

		// Check whether CT already exists in storage, otherwise: create
		sprintf_s(temp, 500, "db%d-%d.ct", offset, offset+numRows-1); s = mStorageDir + temp;
		if(FileUtils::FileExists(s)) {									// -- Load existing
			DISABLE_LOGS();
			ct = CodeTable::LoadCodeTable(s, db);
			ENABLE_LOGS();
			ct->SetLaplace(1);
			ct->CalcTotalSize(ct->GetCurStats());
		} else {														// -- Create new
			if(i+1 >= iterations) {
				goodEnough = true;
				delete db;
				LOG_MSG("---FALSE PARTIAL SKIP---");
				break;
			}

			ct = FicMain::CreateCodeTable(db, config, true);
			ct->AddOneToEachUsageCount();
			ct->CalcTotalSize(ct->GetCurStats());

			// CT->Disk
			sprintf_s(temp, 500, "db%d-%d.ct", offset, offset+numRows-1);
			s = mStorageDir + temp;
			ct->WriteToDisk(s);

			// StdLens->disk
			sprintf_s(temp, 500, "db%d-%d.stdlen", offset, offset+numRows-1);
			s = mStorageDir + temp;
			db->WriteStdLengths(s);
		}

		// Determine ACLD and see whether we can stop
		if(prevCT != NULL) {
			if(mDbSizeOnly) {
				acld = prevCT->GetCurStats().encDbSize;
				for(uint32 i=numRows-mBlockSize; i<numRows; i++)
					acld += prevCT->CalcTransactionCodeLength(iss[i]);
				acld /= ct->GetCurStats().encDbSize;
			} else {
				acld = prevCT->GetCurSize();
				for(uint32 i=numRows-mBlockSize; i<numRows; i++)
					acld += prevCT->CalcTransactionCodeLength(iss[i]);
				acld /= ct->GetCurSize();
			}
			acld -= 1.0;
			if(acld < acldThresh)
				goodEnough = true;
			delete prevCT;
			delete prevDB;
		}

		// Quick&plain summary stats
		sprintf_s(temp, 500, "%d\t\t%d\t\t%.0f\t%.0f\t%.3f", i+1, numRows, ct->GetCurStats().encCTSize, ct->GetCurSize(), acld);
		s = temp; LOG_MSG(s);

		// Rotate
		prevCT = ct;
		ct = NULL;
		prevDB = db;
		db = NULL;
	}
	if(!goodEnough) {
		LOG_MSG("!! Max # iterations or end of data reached, but not good enough! !!\n");
		return NULL;
	} else {
		sprintf_s(temp, 500, "Built CT @ %d using %d rows\n", offset, numRows); s = temp; LOG_MSG(s);
	}
	return prevCT;
}
bool StreamAlgo::DistributionIsTheSame(Database *db, CodeTable *ct, Histogram *hist, uint32 offset, uint32 blockSize) {
	if(offset+blockSize > db->GetNumRows())
		return false;
	double length = 0.0;
	for(uint32 i=0; i<blockSize; i++)
		length += ct->CalcTransactionCodeLength(db->GetRow(offset+i));
	char temp[200];
	sprintf_s(temp, 200, "Block @ %d -- %f", offset, length); string s = temp; LOG_MSG(s);
	if(hist->IsCutOff(length)) {
		LOG_MSG("\n ** Distribution changed? ** \n");
		return false;
	}
	return true;
}
Histogram* StreamAlgo::BuildBlockSizeHistogram(CodeTable *ct, uint32 blockSize, uint32 histNumBins, uint32 blockHistNumSamples) {
	Database *db = ct->GetDatabase();
	uint32 numRows = db->GetNumRows();
	double* lengths = new double[numRows];
	double* samples = new double[blockHistNumSamples];

	// foreach database row, compute its length
	for(uint32 i=0; i<numRows; i++)
		lengths[i] = ct->CalcTransactionCodeLength(db->GetRow(i));

	// do some block samples
	uint32 rnd;
	for(uint32 i=0; i<blockHistNumSamples; i++) {
		samples[i] = 0.0;
		for(uint32 j=0; j<blockSize; j++) {
			rnd = RandomUtils::UniformUint32(numRows);
			samples[i] += lengths[rnd];
		}
	}

	Histogram *hist = new Histogram(histNumBins, samples, blockHistNumSamples);
	delete[] lengths;
	delete[] samples;
	return hist;
}
bool StreamAlgo::AcceptNewCT(CodeTable *ct, CodeTable *newCT, const float acldThreshold, float &acceptedAcldSum) {
	char temp[200]; string s;
	double acld = ct->CalcEncodedDbSize(newCT->GetDatabase());
	if(mDbSizeOnly) {
		acld /= newCT->GetCurStats().encDbSize;
	} else {
		acld += ct->GetCurStats().encCTSize;
		acld /= newCT->GetCurSize();
	}
	acld -= 1.0;
	if(acld > acldThreshold) {
		acceptedAcldSum += (float)acld;
		sprintf_s(temp, 200, "++ Accept new CT, CTD = %.03f\n\n", acld); s = temp; LOG_MSG(s);
		return true;
	} else {
		sprintf_s(temp, 200, "-- Reject new CT, CTD = %.03f\n\n", acld); s = temp; LOG_MSG(s);
		return false;
	}
}

void StreamAlgo::SerialKrimp(Config *config, Database *odb, const uint32 offset) {
	if(config->Read<bool>("streamonlinestats", false))
		SerialKrimpAndStatistics(config, odb, offset);
	else
		SerialKrimpNoStatistics(config, odb, offset);
}
void StreamAlgo::SerialKrimpNoStatistics(Config *config, Database *odb, const uint32 offset) {
	char temp[500]; string s;

	string iscName = config->Read<string>("iscname");
	string pruneStrategy = config->Read<string>("prunestrategy","nop");
	string iscFullPath = Bass::GetWorkingDir() + iscName + ".isc";

	uint32 iterations = min(mMaxIterations, (odb->GetNumRows()-offset) / mBlockSize);
	CodeTable *ct;
	uint32 numRows;

	for(uint32 i=0; i<iterations; i++) {
		// Construct temporary db
		numRows = (i+1) * mBlockSize;
		ItemSet **iss = new ItemSet *[numRows];
		for(uint32 r=0; r<numRows; r++)
			iss[r] = odb->GetRow(r+offset)->Clone();
		Database *db = new Database(iss, numRows, false);
		db->UseAlphabetFrom(odb, false); // computes StdLengths for new database using old alphabet (without Laplace..)
		db->ComputeStdLengths(true);		// but now we add Laplace correction
		db->SetMaxSetLength(odb->GetMaxSetLength());

		// StdLens->disk
		sprintf_s(temp, 500, "db%d-%d.stdlen", offset, offset+numRows-1); s = Bass::GetWorkingDir() + temp;
		db->WriteStdLengths(s);
/*
		double len = 0.0;
		double *stdlens = db->GetStdLengths();
		for(uint32 j=0; j<db->GetAlphabetSize(); j++)
			len += stdlens[j]*2;

		sprintf_s(temp, 500, "%d\t%d\t%.0f\t%.2f\t%.0f", i+1, numRows, db->GetStdDbSize(), db->GetStdDbSize()/numRows, len);
		s = temp; LOG_MSG(s);
*/
		// Create CT
		ct = FicMain::CreateCodeTable(db, config, true);
		ct->AddOneToEachUsageCount();
		ct->CalcTotalSize(ct->GetCurStats());

		// CT->Disk
		sprintf_s(temp, 500, "db%d-%d.ct", offset, offset+numRows-1); s = Bass::GetWorkingDir() + temp;
		ct->WriteToDisk(s);

		// Quick&plain summary stats
		sprintf_s(temp, 500, "%d\t%.0f\t%.0f\t%.2f", i+1, ct->GetCurStats().encCTSize, ct->GetCurSize(), ct->GetCurSize()/numRows);
		s = temp; LOG_MSG(s);

		// Cleanup
		delete ct;
		delete db;
	}
}

void StreamAlgo::SerialKrimpAndStatistics(Config *config, Database *odb, const uint32 offset) {
	char temp[500]; string s;

	string iscName = config->Read<string>("iscname");
	string pruneStrategy = config->Read<string>("prunestrategy","nop");
	string iscFullPath = Bass::GetWorkingDir() + iscName + ".isc";
	sprintf_s(temp,500,"%d",offset); s = Bass::GetWorkingDir() + "ACLDs-" + temp + ".csv";
	FILE *fp; fopen_s(&fp, s.c_str(), "w");

	uint32 iterations = min(mMaxIterations, (odb->GetNumRows()-offset) / mBlockSize);
	uint32 extraBlocks = min(config->Read<uint32>("streamextrablocks",0), (odb->GetNumRows() - (iterations * mBlockSize)) / mBlockSize);
	double *sizes = new double[iterations];
	double size, prevSize, tempSize;
	double **avgCodeLens = new double *[iterations];
	CodeTable **CTs = new CodeTable *[iterations];
	CodeTable *ct;

	uint32 numRows, idx;
	ItemSet **newBlock = new ItemSet *[mBlockSize];
	
	for(uint32 i=0; i<iterations; i++) {
		avgCodeLens[i] = new double[iterations-i+extraBlocks];

		// Construct temporary db
		numRows = (i+1) * mBlockSize;
		ItemSet **iss = new ItemSet *[numRows];
		for(uint32 r=0; r<numRows-mBlockSize; r++)
			iss[r] = odb->GetRow(r+offset)->Clone();
		idx = 0;
		for(uint32 r=numRows-mBlockSize; r<numRows; r++)
			newBlock[idx++] = iss[r] = odb->GetRow(r+offset)->Clone();
		Database *db = new Database(iss, numRows, false);
		db->UseAlphabetFrom(odb, false);
		db->ComputeStdLengths(true);		// now we add Laplace correction
		db->SetMaxSetLength(odb->GetMaxSetLength());

		// StdLens->disk
		sprintf_s(temp, 500, "db%d-%d.stdlen", offset, offset+numRows-1); s = Bass::GetWorkingDir() + temp;
		db->WriteStdLengths(s);

		// Create CT
		ct = CTs[i] = FicMain::CreateCodeTable(db, config, true);
		ct->AddOneToEachUsageCount();
		ct->CalcTotalSize(ct->GetCurStats());
		size = sizes[i] = ct->GetCurSize();
		sprintf_s(temp, 500, "db%d-%d.ct", offset, offset+numRows-1); s = Bass::GetWorkingDir() + temp;
		ct->WriteToDisk(s);

		// Add current avgCL
		avgCodeLens[i][0] = ct->GetCurStats().encDbSize / numRows;

		// Compare with previous CTs
		for(uint32 j=0; j<i; j++) {
			tempSize = 0.0;
			CTs[j]->SetDatabase(db);
			for(uint32 r=0; r<mBlockSize; r++)
				tempSize += CTs[j]->CalcTransactionCodeLength(newBlock[r]);
			avgCodeLens[j][i-j] = tempSize / mBlockSize;
			sizes[j] += tempSize;
			fprintf(fp, "%.03f;", (sizes[j]-size)/size);
		}
		fprintf(fp, "0\n"); fflush(fp);
		//double t1 = ct->CalcEncodedDbSize(odb);
		//double t2 = t1 + ct->GetCurStats().ctSize;
		prevSize = i>0 ? sizes[i-1] : 0.0;
		//sprintf_s(temp, 500, "%d\t%.0f\t%.0f\t%.0f\t%.3f\t%.2f\t%.0f\t%.0f", i+1, ct->GetCurStats().ctSize, size, prevSize, (prevSize-size)/size, size/numRows, t1, t2);
		sprintf_s(temp, 500, "%d\t%.0f\t%.0f\t%.0f\t%.3f\t%.2f", i+1, ct->GetCurStats().encCTSize, size, prevSize, (prevSize-size)/size, size/numRows);
		s = temp; LOG_MSG(s);

		delete db;
	}
	fclose(fp);

	// Compute avg codelens for some additional blocks
	for(uint32 i=0; i<iterations; i++)
		CTs[i]->SetDatabase(odb);
	for(uint32 i=0; i<extraBlocks; i++) {
		// Get rows to be done
		uint32 start = offset + (i+iterations) * mBlockSize;
		for(uint32 r=0; r<mBlockSize; r++)
			newBlock[r] = odb->GetRow(start+r);

		// Compute for each CT
		for(uint32 j=0; j<iterations; j++) {
			tempSize = 0.0;
			for(uint32 r=0; r<mBlockSize; r++)
				tempSize += CTs[j]->CalcTransactionCodeLength(newBlock[r]);
			avgCodeLens[j][iterations+i-j] = tempSize / mBlockSize;
		}
	}

	// Write average code lengths to file
	sprintf_s(temp,500,"%d",offset); s = Bass::GetWorkingDir() + "avgCLs-" + temp + ".csv";
	FILE *clfp; fopen_s(&clfp, s.c_str(), "w");
	for(uint32 i=0; i<iterations; i++) {
		for(uint32 j=0; j<i; j++)
			fprintf(clfp, ";");
		fprintf(clfp, "%.0f;", avgCodeLens[i][0]);
		for(uint32 j=i+1; j<iterations+extraBlocks; j++)
			fprintf(clfp, "%.03f;", avgCodeLens[i][j-i]);
		fprintf(clfp, "\n");
		delete[] avgCodeLens[i];
	}
	delete[] avgCodeLens;
	fclose(clfp);

	for(uint32 i=0; i<iterations; i++)
		delete CTs[i];
	delete[] CTs;
	delete[] sizes;
	delete[] newBlock;
}

/*
CodeTable* StreamAlgo::FindCodeTable(Config *config, Database *odb, const uint32 offset, uint32 &numBlocks) {
char temp[500]; string s;

string iscName = config->Read<string>("iscname");
string pruneStrategy = config->Read<string>("prunestrategy","nop");
string iscFullPath = Bass::GetWorkingDir() + iscName + ".isc";

uint32 iterations = min(mMaxIterations, odb->GetNumRows() / mBlockSize);
double *sizes = new double[iterations];
//CodeTable **CTs = new CodeTable *[iterations];
CodeTable *prevCT = NULL; Database *prevDB = NULL;
double sizeWithPrevCT;
uint32 numRows;

for(uint32 i=0; i<iterations; i++) {
// Construct temporary db
numRows = (i+1) * mBlockSize;
ItemSet **iss = new ItemSet *[numRows];
for(uint32 r=0; r<numRows; r++)
iss[r] = odb->GetRow(r+offset)->Clone();
Database *db = new Database(iss, numRows, false);
db->UseAlphabetFrom(odb);
db->SetMaxSetLength(odb->GetMaxSetLength());

//sprintf_s(temp, 200, "db%d", i+1); s = temp;
//db->Write(s, Bass::GetWorkingDir());

// Create CT
CodeTable *ct = FicMain::CreateCodeTable(db, config, true);
ct->AddOneToEachCount();
ct->CalcTotalSize(ct->GetCurStats());
sizes[i] = ct->GetCurSize();

// Compare with previous CT
sizeWithPrevCT = 0.0;
if(prevCT != NULL) {
sizeWithPrevCT = prevCT->GetCurSize();
for(uint32 r=numRows-mBlockSize; r<numRows; r++)
sizeWithPrevCT += prevCT->CalcTransactionCodeLength(db->GetRow(r));
}
double t1 = ct->CalcEncodedDbSize(odb); double t2 = t1 + ct->GetCurStats().ctSize;
sprintf_s(temp, 500, "%d\t%.0f\t%.0f\t%.0f\t%.3f\t%.2f\t%.0f\t%.0f", i+1, ct->GetCurStats().ctSize, sizes[i], sizeWithPrevCT, (sizeWithPrevCT-sizes[i])/sizes[i], sizes[i]/numRows, t1, t2);
s = temp; LOG_MSG(s);

// Rotate stuff
//delete prevCT;
prevCT = ct;
delete prevDB;
prevDB = db;
}
delete prevCT;
delete prevDB;
delete[] sizes;
return NULL;
}
*/

#endif // BLOCK_STREAM
