#ifdef ENABLE_STREAM

#include "../global.h"

#if defined (_WINDOWS)
	#include <direct.h>
#endif

// qtils
#include <Config.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>
#include <logger/Log.h>
#include <RandomUtils.h>

// bass
#include <Bass.h>
#include <db/ClassedDatabase.h>

// fic
#include "../FicMain.h"
#include "../algo/CodeTable.h"
#include "../blocks/dgen/DGen.h"
#include "../blocks/stream/StreamAlgo.h"
#include "../blocks/stream/Histogram.h"

#include "StreamTH.h"

StreamTH::StreamTH(Config *conf) : TaskHandler(conf){

}

StreamTH::~StreamTH() {
	// not my Config*
}
void StreamTH::HandleTask() {
	string command = mConfig->Read<string>("command");

			if(command.compare("streamkrimp") == 0)		FindTheCTs();
	else	if(command.compare("encodestream") == 0)	EncodeStream();

#ifndef _PUBLIC_RELEASE
	else	if(command.compare("findstreamct") == 0)	FindCT();
	else	if(command.compare("checkcts") == 0)		CheckCTs();
	else	if(command.compare("genstream") == 0)		GenerateStream();
#endif

	else	throw string("StreamTH :: Unable to handle task `" + command + "`");
}

string StreamTH::BuildWorkingDir() {
	string command = mConfig->Read<string>("command");

			if(command.compare("findstreamct") == 0)
				return mConfig->Read<string>("command") + "/" + mConfig->Read<string>("iscname") + "-" + TimeUtils::GetTimeStampString() + "/";
	else	if(command.compare("streamkrimp") == 0)
			return mConfig->Read<string>("command") + "/" + mConfig->Read<string>("iscname") + "-" + TimeUtils::GetTimeStampString() + "/";
	else	if(command.compare("encodestream") == 0)
				return "streamkrimp/" + mConfig->Read<string>("expname") + "/";
	return "";
}


/* ------------------------- Commands -------------------------- */

void StreamTH::CheckCTs() {
	char temp[200]; string s;

	uint32 blockSize = mConfig->Read<uint32>("streamblocksize");
	uint32 offsetBase = mConfig->Read<uint32>("streamoffsetbase",0);
	uint32 randomizeDB = mConfig->Read<uint32>("randomizedb", 0);
	uint32 classedDbAsStream = mConfig->Read<uint32>("classeddbasstream", 0);

	// Retrieve DB
	string iscName = mConfig->Read<string>("iscname");
	string dbName = mConfig->Read<string>("dbname", iscName.substr(0,iscName.find_first_of('-')));
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("dbindatatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));
	Database *db;
	uint32 numClasses = 0;
	uint32 **classSizes = new uint32 *[1];
	uint32 *classCounters = NULL;
	if(classedDbAsStream) {
		db = LoadClassedDbAsStream(dbName, dataType, randomizeDB, numClasses, classSizes);
	} else {
		db = Database::RetrieveDatabase(dbName, dataType, dbType);
		if(randomizeDB != 0)
			db->RandomizeRowOrder(randomizeDB);
	}
	if(db->HasBinSizes())
		throw string("StreamTH::FindCT() -- Binned databases cannot be used.");

	// Build StreamAlgo
	if(blockSize == 0) blockSize = (uint32) db->GetAlphabetSize();
	s = Bass::GetExpDir() + "streamkrimp/" + iscName + "/";
	if(randomizeDB != 0) {
		sprintf_s(temp, 200, "rnd%d/", randomizeDB); 
		s += temp;
	}
	string storageDir = s;

	// Start log
	s = storageDir + "_missingCTs.csv";
	FILE *file;
	fopen_s(&file, s.c_str(), "w");

	sprintf_s(temp, 200, "Block size:\t%d", blockSize); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "Offset:\t%d", offsetBase); s = temp; LOG_MSG(s);
	if(randomizeDB != 0)	{ sprintf_s(temp, 200, "Randomize DB:\t%d", randomizeDB); s = temp; LOG_MSG(s); }
	else					{ s = "Randomize DB:\tno"; LOG_MSG(s); }
	s = classedDbAsStream ? "Classed DB as stream:\tyes" : "Classed DB as stream:\tno"; LOG_MSG(s);
	s = ""; LOG_MSG(s);

	// Do it
	for(uint32 offset = offsetBase; offset + blockSize < db->GetNumRows(); offset += blockSize) {
		sprintf_s(temp, 200, "db%d-%d.ct", offset, offset+blockSize-1);
		if(!FileUtils::FileExists(storageDir + temp))
			fprintf(file, "%d\n", offset);
	}
	fclose(file);

	delete[] classSizes[0];
	delete[] classSizes;
	delete db; // entire DB
}

void StreamTH::FindTheCTs() {
	char temp[200]; string s;

	uint32 blockSize = mConfig->Read<uint32>("streamblocksize");
	uint32 maxIterations = mConfig->Read<uint32>("streammaxiterations");
	uint32 offsetBase = mConfig->Read<uint32>("streamoffsetbase",0);
	uint32 stopAtRow = mConfig->Read<uint32>("streamstopatrow",0);
	uint32 randomizeDB = mConfig->Read<uint32>("randomizedb", 0);
	uint32 classedDbAsStream = mConfig->Read<uint32>("classeddbasstream", 0);
	float maxIR = mConfig->Read<float>("streammaxir", 0.0f);
	float minCTD = mConfig->Read<float>("streamminctd", 0.0f);
	uint32 histNumBins = mConfig->Read<uint32>("histnumbins", 100);
	uint32 blockHistNumSamples = mConfig->Read<uint32>("blocknumsamples", 10000);
	float blockHistCutOff = mConfig->Read<float>("blocksamplesdiscard", 0.0f);
	bool dbSizeOnly = mConfig->Read<bool>("dbsizeonly", false);

	// Start a custom log
	s = Bass::GetWorkingDir() + "_splits.csv";
	FILE *spFile;
	fopen_s(&spFile, s.c_str(), "w");

	// Retrieve DB
	string iscName = mConfig->Read<string>("iscname");
	string dbName = mConfig->Read<string>("dbname", iscName.substr(0,iscName.find_first_of('-')));
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));
	Database *db;
	uint32 numClasses = 0;
	uint32 **classSizes = new uint32 *[1];
	uint32 *classCounters = NULL;
	if(classedDbAsStream) {
		db = LoadClassedDbAsStream(dbName, dataType, randomizeDB, numClasses, classSizes);
		uint32 sp = 0;
		fprintf_s(spFile, "Actual class boundaries\nPosition;Segment length\n");
		fprintf_s(spFile, "0;\n", sp);
		for(uint32 i=0; i<numClasses; i++) { 
			sprintf_s(temp, 200, "Class #%d ==> %d rows", i+1, (*classSizes)[i]); s = temp; LOG_MSG(s);
			sp += (*classSizes)[i];
			fprintf_s(spFile, "%d;%d\n", sp, (*classSizes)[i]);
		}
		fprintf_s(spFile, "\n", sp);
		sprintf_s(temp, 200, "Total # rows: %d\n", db->GetNumRows()); s = temp; LOG_MSG(s);
		classCounters = new uint32[numClasses];
	} else {
		db = Database::RetrieveDatabase(dbName, dataType, dbType);
		if(randomizeDB != 0)
			db->RandomizeRowOrder(randomizeDB);
	}
	if(db->HasBinSizes())
		throw string("StreamKrimp -- Binned databases cannot be used.");

	// Build StreamAlgo
	if(blockSize == 0) blockSize = (uint32) db->GetAlphabetSize();
	StreamAlgo *streamAlgo = new StreamAlgo(blockSize, maxIterations, dbSizeOnly);
	s = Bass::GetExpDir() + mConfig->Read<string>("command") + "/" + iscName + "/";
	if(randomizeDB != 0) {
		sprintf_s(temp, 200, "rnd%d/", randomizeDB); 
		s += temp;
	}
	streamAlgo->SetStorageDir(s);
	sprintf_s(temp, 200, "Block size:\t\t\t\t\t%d", blockSize); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "Max iterations:\t\t\t\t%d", maxIterations); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "Offset:\t\t\t\t\t\t%d", offsetBase); s = temp; LOG_MSG(s);
	if(stopAtRow == 0) {}
	else { sprintf_s(temp, 200, "Stop at row:\t\t\t\t%d", stopAtRow); s = temp; LOG_MSG(s); }
	if(maxIR != 0.0) { sprintf_s(temp, 200, "Maximum IR:\t\t\t\t\t%.03f", maxIR); s = temp; LOG_MSG(s); }
	else					{ throw(string("StreamKrimp needs maximum IR.")); }
	if(minCTD != 0.0) { sprintf_s(temp, 200, "Minimum CTD:\t\t\t\t%.03f", minCTD); s = temp; LOG_MSG(s); }
	else					{ throw(string("StreamKrimp needs minimum CTD.")); }
	sprintf_s(temp, 200, "Block samples discarded:\t%.03f%%", blockHistCutOff); s = temp; LOG_MSG(s);
	if(randomizeDB != 0)	{ sprintf_s(temp, 200, "Randomize DB:\t\t\t\t%d", randomizeDB); s = temp; LOG_MSG(s); }
	else					{ s = "Randomize DB:\t\t\t\tno"; LOG_MSG(s); }
	s = classedDbAsStream ? "Classed DB as stream:\t\tyes" : "Classed DB as stream:\t\tno"; LOG_MSG(s);
	s = ""; LOG_MSG(s);

	if(stopAtRow != 0) {
		Database *newDb = db->Subset(0, stopAtRow);
		delete db;
		db = newDb;
	}

	// Do it
	uint32 offset = offsetBase, prevOffset = offsetBase;
	float purity = 0.0f;
	CodeTable *ct = streamAlgo->FindGoodEnoughCT(mConfig, db, offset, maxIR);
	CodeTable *newCT = NULL;

	// Check if converged
	if(ct == NULL) {
		delete[] classSizes[0];
		delete[] classSizes;
		delete db; // entire DB
		delete streamAlgo;
		return;
	}

	fprintf_s(spFile, "Splits found\nPosition;Segment length\n%d\n", offset);

	uint32 to = offset+ct->GetDatabase()->GetNumRows()-1;
	sprintf_s(temp, 200, "db%d-%d", offset, to);
	// CT->Disk
	s = Bass::GetWorkingDir() + temp + ".ct";
	ct->WriteToDisk(s);
	// StdLens->disk
	s = Bass::GetWorkingDir() + temp + ".stdlen";
	ct->GetDatabase()->WriteStdLengths(s);

	// Build Histogram on CT
	Histogram *hist = streamAlgo->BuildBlockSizeHistogram(ct, blockSize, histNumBins, blockHistNumSamples);
	hist->SetCutOff(blockHistCutOff);
	sprintf_s(temp, 200, "hist%d-%d.csv", offset, to); s = Bass::GetWorkingDir() + temp;
	hist->WriteToDisk(s);

	// Skip rows covered by current CT
	offset += ct->GetDatabase()->GetNumRows();

	// Keep track of some numbers
	uint32 numCTsBuilt = 1;
	uint32 numCTsAccepted = 1;
	uint32 numBlocksSkipped = 0;
	uint32 numBlocksForAcceptedCTs = (offset+ct->GetDatabase()->GetNumRows() - offset) / blockSize;
	float  acldSum = 0.0f;

	while(true) {
		// Skip rows belonging to same distribution
		while(streamAlgo->DistributionIsTheSame(db, ct, hist, offset, blockSize)) {
			offset += blockSize;
			++numBlocksSkipped;
		}

		// Break if at end of database
		if(offset + blockSize > db->GetNumRows())
			break;

		// Find candidate CT
		newCT = streamAlgo->FindGoodEnoughCT(mConfig, db, offset, maxIR);

		// Check whether process converged
		if(newCT == NULL)
			break;

		++numCTsBuilt;

		if(streamAlgo->AcceptNewCT(ct, newCT, minCTD, acldSum)) {			// -- Accept candidate CT
			delete ct->GetDatabase();
			delete ct;
			delete hist;
			ct = newCT;

			if(classedDbAsStream) {
				// So, all records in s(prevOffset,offset-1) should now belong to the same class
				uint32 idx = prevOffset, tmp = 0, val = 0, max = 0;
				for(uint32 i=0; i<numClasses; i++) {
					tmp += (*classSizes)[i];
					tmp = min(offset, tmp);
					if(idx < tmp) {
						val = tmp - idx;
						if(val > max) max = val;
						idx += val;
					}
					if(idx == offset)
						break;
				}
				purity += max;
			}

			fprintf_s(spFile, "%d;%d\n", offset, offset-prevOffset);

			++numCTsAccepted;
			numBlocksForAcceptedCTs += (offset+ct->GetDatabase()->GetNumRows() - offset) / blockSize;

			sprintf_s(temp, 200, "db%d-%d", offset, offset+ct->GetDatabase()->GetNumRows()-1);
			// CT->Disk
			s = Bass::GetWorkingDir() + temp + ".ct";
			ct->WriteToDisk(s);
			// StdLens->disk
			s = Bass::GetWorkingDir() + temp + ".stdlen";
			ct->GetDatabase()->WriteStdLengths(s);

			// Build Histogram on CT
			hist = streamAlgo->BuildBlockSizeHistogram(ct, blockSize, histNumBins, blockHistNumSamples);
			hist->SetCutOff(blockHistCutOff);
			sprintf_s(temp, 200, "hist%d.csv", offset); s = Bass::GetWorkingDir() + temp;
			hist->WriteToDisk(s);

			// Skip rows covered by current CT
			prevOffset = offset;
			offset += ct->GetDatabase()->GetNumRows();
		} else {																// -- Refuse candidate CT
			delete newCT->GetDatabase();
			delete newCT;

			// Skip rows that triggered this candidate
			offset += blockSize;
		}
	}

	if(classedDbAsStream) {
		// So, all records in s(prevOffset,offset-1) should now belong to the same class
		uint32 idx = prevOffset, tmp = 0, val = 0, max = 0;
		for(uint32 i=0; i<numClasses; i++) {
			tmp += (*classSizes)[i];
			if(idx < tmp) {
				val = tmp - idx;
				if(val > max) max = val;
				idx += val;
			}
		}
		purity += max;
		purity /= db->GetNumRows();
	}

	sprintf_s(temp, 200, "\n# CTs built: %d", numCTsBuilt); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "# CTs accepted: %d", numCTsAccepted); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "# CTs rejected: %d", numCTsBuilt-numCTsAccepted); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "\n# blocks: %d", db->GetNumRows() / blockSize); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "# blocks skipped: %d", numBlocksSkipped); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "avg # blocks for CT: %.02f", numBlocksForAcceptedCTs / (float)numCTsAccepted); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "avg CTD for accepts: %.02f", acldSum / (numCTsAccepted-1)); s = temp; LOG_MSG(s);

	if(classedDbAsStream)
		sprintf_s(temp, 200, "\nPurity: %.03f", purity); s = temp; LOG_MSG(s);

	sprintf_s(temp, 200, "\n%d", numCTsBuilt); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "%d", numCTsAccepted); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "%d", numCTsBuilt-numCTsAccepted); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "\n%d", db->GetNumRows() / blockSize); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "%d", numBlocksSkipped); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "%.02f", numBlocksForAcceptedCTs / (float)numCTsAccepted); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "%.02f", acldSum / (numCTsAccepted-1)); s = temp; LOG_MSG(s);

	if(classedDbAsStream)
		sprintf_s(temp, 200, "\n%.03f", purity); s = temp; LOG_MSG(s);

	fclose(spFile);

	delete ct->GetDatabase();
	delete ct;
	delete hist;
	if(classedDbAsStream)
		delete[] classSizes[0];
	delete[] classSizes;
	if(classCounters != NULL) delete[] classCounters;
	delete db; // entire DB
	delete streamAlgo;
}

void StreamTH::FindCT() {
	uint32 blockSize = mConfig->Read<uint32>("streamblocksize"), maxIterations = mConfig->Read<uint32>("streammaxiterations");
	uint32 offsetBase = mConfig->Read<uint32>("streamoffsetbase",0), offsetDelta = mConfig->Read<uint32>("streamoffsetdelta",0);
	uint32 offsetTimes = mConfig->Read<uint32>("streamoffsettimes",0);
	uint32 randomizeDB = mConfig->Read<uint32>("randomizedb", 0);
	uint32 classedDbAsStream = mConfig->Read<uint32>("classeddbasstream", 0);
	float acldThreshold = mConfig->Read<float>("streamacldthresh", 0.0);
	bool dbSizeOnly = mConfig->Read<bool>("dbsizeonly", false);

	StreamAlgo *streamAlgo = new StreamAlgo(blockSize, maxIterations, dbSizeOnly);
	char temp[200]; string s;
	sprintf_s(temp, 200, "Block size:\t%d", blockSize); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "Max iterations:\t%d", maxIterations); s = temp; LOG_MSG(s);
	if(acldThreshold != 0.0) { sprintf_s(temp, 200, "Auto-stop when ACLD less than:\t%.03f", acldThreshold); s = temp; LOG_MSG(s); }
	else					{ s = "Auto-stop when good enough:\tno"; LOG_MSG(s); }
	if(randomizeDB != 0)	{ sprintf_s(temp, 200, "Randomize DB:\t%d", randomizeDB); s = temp; LOG_MSG(s); }
	else					{ s = "Randomize DB:\tno"; LOG_MSG(s); }
	s = classedDbAsStream ? "Classed DB as stream:\tyes" : "Classed DB as stream:\tno"; LOG_MSG(s);
	s = dbSizeOnly ? "Use DB size only:\tyes" : "Use DB size only:\tno"; LOG_MSG(s);
	s = ""; LOG_MSG(s);

	// Retrieve DB
	string iscName = mConfig->Read<string>("iscname");
	string dbName = mConfig->Read<string>("dbname", iscName.substr(0,iscName.find_first_of('-')));
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));
	Database *db;
	uint32 numClasses = 0;
	uint32 **classSizes = new uint32 *[1];
	if(classedDbAsStream) {
		db = LoadClassedDbAsStream(dbName, dataType, randomizeDB, numClasses, classSizes);
		for(uint32 i=0; i<numClasses; i++)
			{ sprintf_s(temp, 200, "Class #%d ==> %d rows", i+1, (*classSizes)[i]); s = temp; LOG_MSG(s); }
		sprintf_s(temp, 200, "Total # rows: %d", db->GetNumRows()); s = temp; LOG_MSG(s);
	} else {
		db = Database::RetrieveDatabase(dbName, dataType, dbType);
		if(randomizeDB != 0)
			db->RandomizeRowOrder(randomizeDB);
	}
	if(db->HasBinSizes())
		throw string("StreamTH::FindCT() -- Binned databases cannot be used.");
	offsetTimes = offsetDelta==0 ? 0 : min(offsetTimes, (db->GetNumRows()-offsetBase)/offsetDelta);
	
	// Do it
	uint32 offset;
	if(acldThreshold != 0.0) {	// Adaptive: stop when CT quality doesn't increase (much)
		try {
			for(uint32 i=0; i<=offsetTimes; i++) {
				offset = offsetBase+offsetDelta*i;
				sprintf_s(temp, 200, "\nOffset:\t%d\n", offset); s = temp; LOG_MSG(s);
				streamAlgo->FindGoodEnoughCT(mConfig, db, offset, acldThreshold);
			}
		} catch(string msg) {
			delete db;
			delete streamAlgo;
			throw msg;
		}
	} else {			// Non-adaptive Krimp on series of database subsets
		try {
			for(uint32 i=0; i<=offsetTimes; i++) {
				offset = offsetBase+offsetDelta*i;
				sprintf_s(temp, 200, "\nOffset:\t%d\n", offset); s = temp; LOG_MSG(s);
				streamAlgo->SerialKrimp(mConfig, db, offset);
			}
		} catch(string msg) {
			delete db;
			delete streamAlgo;
			throw msg;
		}
	}

	delete[] classSizes[0];
	delete[] classSizes;
	delete db;
	delete streamAlgo;
}

void StreamTH::EncodeStream() {
	string expDir = Bass::GetExpDir() + "streamkrimp/" + mConfig->Read<string>("expname") + "/";
	if(!FileUtils::DirectoryExists(expDir))
		throw string("StreamTH::EncodeStream() -- Experiment directory not found.");
	uint32 blockSize = mConfig->Read<uint32>("streamblocksize", 0), numBlocks = mConfig->Read<uint32>("streamnumblocks", 0);
	uint32 randomizeDB = mConfig->Read<uint32>("randomizedb", 0);
	uint32 classedDbAsStream = mConfig->Read<uint32>("classeddbasstream", 0);
	uint32 offset = mConfig->Read<uint32>("streamoffsetbase", 0);
	bool bitsPerItem = mConfig->Read<bool>("bitsperitem", false);
	char temp[200]; string s;

	sprintf_s(temp, 200, "Experiment dir:\t%s", expDir.c_str()); s = temp; LOG_MSG(s);
	if(randomizeDB != 0)	{ sprintf_s(temp, 200, "Randomize DB:\t%d", randomizeDB); s = temp; LOG_MSG(s); }
	else					{ s = "Randomize DB:\tno"; LOG_MSG(s); }
	s = classedDbAsStream ? "Classed DB as stream:\tyes" : "Classed DB as stream:\tno"; LOG_MSG(s);

	// Retrieve DB
	string dbName = mConfig->Read<string>("dbname");
	ItemSetType dataType = ItemSet::StringToType(mConfig->Read<string>("datatype",""));
	DbFileType dbType = DbFile::StringToType(mConfig->Read<string>("dbinencoding",""));
	Database *db;
	uint32 numClasses = 0;
	uint32 **classSizes = new uint32 *[1];
	if(classedDbAsStream) {
		db = LoadClassedDbAsStream(dbName, dataType, randomizeDB, numClasses, classSizes);
		for(uint32 i=0; i<numClasses; i++)
		{ sprintf_s(temp, 200, "Class #%d ==> %d rows", i+1, (*classSizes)[i]); s = temp; LOG_MSG(s); }
		sprintf_s(temp, 200, "Total # rows: %d", db->GetNumRows()); s = temp; LOG_MSG(s);
	} else {
		db = Database::RetrieveDatabase(dbName, dataType, dbType);
		if(randomizeDB != 0)
			db->RandomizeRowOrder(randomizeDB);
	}
	if(db->HasBinSizes())
		throw string("StreamTH::EncodeStream() -- Binned databases cannot be used.");

	if(blockSize == 0) blockSize = (uint32) db->GetAlphabetSize();
	if(numBlocks == 0)
		numBlocks = (db->GetNumRows()-offset) / blockSize;
	else
		numBlocks = min(numBlocks, (db->GetNumRows()-offset) / blockSize);

	sprintf_s(temp, 200, "Offset:\t\t%d", offset); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "Block size:\t%d", blockSize); s = temp; LOG_MSG(s);
	sprintf_s(temp, 200, "# blocks:\t%d", numBlocks); s = temp; LOG_MSG(s);
	s = ""; LOG_MSG(s);

	// Build blocks
	s = ".. Building blocks .."; LOG_MSG(s);
	Database **blocks = new Database *[numBlocks];
	uint32 start;
	for(uint32 b=0; b<numBlocks; b++) {
		start = offset + b * blockSize;
		ItemSet **iss = new ItemSet *[blockSize];
		for(uint32 r=0; r<blockSize; r++)
			iss[r] = db->GetRow(start+r)->Clone();
		blocks[b] = new Database(iss, blockSize, false);
		blocks[b]->UseAlphabetFrom(db);
		blocks[b]->SetMaxSetLength(db->GetMaxSetLength());
	}

	// Find & sort CT files
	uint32 numCTs = FileUtils::CountFiles(expDir, "*.ct");
	directory_iterator dirit(expDir, "*.ct");
	string *filenames = new string[numCTs];
	uint32 *from = new uint32[numCTs];
	uint32 *to = new uint32[numCTs];
	uint32 *idx = new uint32[numCTs];
	for(uint32 i=0; i<numCTs; i++) {
		idx[i] = i;
		dirit.next();
		filenames[i] = dirit.filename();
		if(sscanf_s(filenames[i].c_str(), "db%d-%d.ct", &from[i], &to[i]) != 2)
			throw string("StreamTH::EncodeStream() -- Found CT with incorrect filename (not db%d-%d.ct).");
	}
	uint32 t;
	for(uint32 i=1; i<numCTs; i++)  { // Do some bubblesorting
		for(uint32 j=0; j<numCTs-i; j++)
			if(from[idx[j]] == from[idx[j+1]] ? to[idx[j]] > to[idx[j+1]] : from[idx[j]] > from[idx[j+1]]) {
				t = idx[j];
				idx[j] = idx[j+1];
				idx[j+1] = t;
			}
	}

	// Start log
	FILE *csvFile; //, *integratedFile;
	s = "Output to:"; LOG_MSG(s);
	sprintf_s(temp, 200, "encoded_%dx%d%s.csv", numBlocks, blockSize, bitsPerItem?"perItem":""); s = expDir + temp; LOG_MSG(s);
	fopen_s(&csvFile, s.c_str(), "w");
	fprintf(csvFile, "Encoded sizes of blocks encoded with the specified code tables\n\n");
	fprintf(csvFile, bitsPerItem ? "Avg #bits per item\n" : "Avg #bits per transaction\n");
	fprintf(csvFile, "Offset:;%d\nNumBlocks:;%d\nBlockSize:;%d\n\n",offset,numBlocks,blockSize);
	/*sprintf_s(temp, 200, "encoded_InclCT_%dx%d%s.csv", numBlocks, blockSize, bitsPerItem?"perItem":""); s = expDir + temp; LOG_MSG(s);
	fopen_s(&integratedFile, s.c_str(), "w");
	fprintf(integratedFile, "Stream blocks encoded with the specified code tables, INCLuding code table size.\n");
	fprintf(integratedFile, bitsPerItem ? "Avg #bits per item\n" : "Avg #bits per transaction\n");
	fprintf(integratedFile, "Offset:;%d\nNumBlocks:;%d\nBlockSize:;%d\n\n",offset,numBlocks,blockSize);
	*/

	fprintf(csvFile, ";Block\nCodeTable;");
	//fprintf(integratedFile, ";Block\nCodeTable;");
	for(uint32 b=0; b<numBlocks; b++) {
		fprintf(csvFile, "%d;", b+1);
	//	fprintf(integratedFile, "%d;", b+1);
	}
	fprintf(csvFile, "\n");
	//fprintf(integratedFile, "\n");

	// Foreach CT found
	double size, sum;
	uint64 numItems, sumItems;
	for(uint32 i=0; i<numCTs; i++) {
		sprintf_s(temp, 200, "db%d-%d.stdlen", from[idx[i]], to[idx[i]]); s = Bass::GetWorkingDir() + temp;
		db->ReadStdLengths(s);
		CodeTable *ct = CodeTable::LoadCodeTable(expDir + filenames[idx[i]], db);
		sum = ct->CalcCodeTableSize();
		fprintf(csvFile, "%s;", filenames[idx[i]].c_str());
		//fprintf(integratedFile, "%s;%.0f;", filenames[idx[i]].c_str(), sum);
		sumItems = 0;
		for(uint32 b=0; b<numBlocks; b++) {
			size = ct->CalcEncodedDbSize(blocks[b]);
			sum += size;
			if(bitsPerItem) {
				numItems = blocks[b]->GetNumItems();
				sumItems += numItems;
				fprintf(csvFile, "%.2f;", size/numItems);
				//fprintf(integratedFile, "%.2f;", sum/sumItems);
			} else {
				fprintf(csvFile, "%.2f;", size/blockSize);
				//fprintf(integratedFile, "%.2f;", sum/(blockSize*(b+1)));
			}
		}
		fprintf(csvFile, "\n");
		//fprintf(integratedFile, "\n");
		delete ct;
	}
	fclose(csvFile);
	//fclose(integratedFile);

	// Cleanup
	delete[] classSizes[0];
	delete[] classSizes;
	for(uint32 b=0; b<numBlocks; b++)
		delete blocks[b];
	delete[] blocks;
	delete[] filenames;
	delete[] from;
	delete[] to;
	delete[] idx;
	delete db;
}

void StreamTH::GenerateStream() {
	string ctPath = Bass::GetExpDir() + mConfig->Read<string>("command") + "/" + mConfig->Read<string>("dbname") + "/";

	// Build list with code tables
	uint32 numCTs = 0;
	{
		directory_iterator itr(ctPath, "*.ct");
		while(itr.next())
			++numCTs;
	}
	string *ctFilenames = new string[numCTs];
	{
		directory_iterator itr(ctPath, "*.ct");
		for(uint32 i=0; i<numCTs; i++) {
			itr.next();
			ctFilenames[i] = itr.filename();
		}
	}

	string genType = mConfig->Read<string>("gentype");
	string genPath = ctPath + genType + "-" + TimeUtils::GetTimeStampString() + "/";
	FileUtils::CreateDirIfNotExists(genPath);

	uint32 alphSize = mConfig->Read<uint32>("alphSize");
	ItemSet **iss = new ItemSet *[1];
	uint16 *set = new uint16[alphSize];
	for(uint32 i=0; i<alphSize; i++)
		set[i] = i;
	iss[0] = ItemSet::Create(Uint16ItemSetType, set, alphSize, alphSize);
	delete[] set;
	Database *bogusDb = new Database(iss, 1, false);
	bogusDb->ComputeEssentials();

	uint32 numRowsPerCT = mConfig->Read<uint32>("numrowsperct");
	uint32 numDBs = mConfig->Read<uint32>("numdbs");
	string dbName = mConfig->Read<string>("dbname");
	char temp[200];

	// DGen foreach CT
	DGen **dgs = new DGen *[numCTs];
	for(uint32 i=0; i<numCTs; i++) {
		dgs[i] = DGen::Create(genType);
		dgs[i]->SetInputDatabase(bogusDb);
		dgs[i]->SetColumnDefinition(mConfig->Read<string>("coldef"));
		dgs[i]->BuildModel(bogusDb, mConfig, ctPath + ctFilenames[i]);
	}

	Database **dbs = new Database *[numCTs];

	// Foreach DB to be generated
	for(uint32 d=0; d<numDBs; d++) {
		for(uint32 i=0; i<numCTs; i++)
			dbs[i] = dgs[i]->GenerateDatabase(numRowsPerCT);

		// Merge the segments
		Database *db = Database::Merge(dbs, numCTs);

		// Add classlabels
		ClassedDatabase *cdb = new ClassedDatabase(db, false);
		uint16 *classLabels = new uint16[db->GetNumRows()];
		uint16 *classes = new uint16[numCTs];
		for(uint32 i=0; i<numCTs; i++) {
			uint32 label = alphSize + i;
			classes[i] = label;
			for(uint32 j=0; j<numRowsPerCT; j++)
				classLabels[i*numRowsPerCT+j] = label;
		}
		cdb->SetClassLabels(classLabels);
		cdb->SetClassDefinition(numCTs, classes);

		// Save
		sprintf_s(temp, 200, "_%d", d+1);
		Database::WriteDatabase(cdb, dbName+temp, genPath);

		// Cleanup
		for(uint32 i=0; i<numCTs; i++)
			delete dbs[i];
		delete db;
	}

	for(uint32 i=0; i<numCTs; i++)
		delete dgs[i];
	delete[] dgs;
	delete[] dbs;
	delete[] ctFilenames;
	delete bogusDb;
}

/* ------------------------- Helpers -------------------------- */

Database* StreamTH::LoadClassedDbAsStream(const string dbName, const ItemSetType dataType, const uint32 randomizeClasses, uint32 &numClasses, uint32 **classSizes) {
	Database *oriDb = Database::RetrieveDatabase(dbName, dataType);
	ClassedDatabase *classedDb;
	if(oriDb->GetType() == DatabaseClassed) {
		classedDb = (ClassedDatabase *)oriDb;
	} else {
		if(oriDb->GetNumClasses() == 0) {
			delete oriDb;
			throw string("StreamTH -- Specified database contains no class info.");
		}
		classedDb = new ClassedDatabase(oriDb);
		delete oriDb;
		oriDb = classedDb;
	}
	Database **dbs = classedDb->SplitOnClassLabels();
	numClasses = classedDb->GetNumClasses();
	*classSizes = new uint32[numClasses];
	if(randomizeClasses != 0) {
		uint32 *rndClass = new uint32[numClasses];
		for(uint32 i=0; i<numClasses; i++)
			rndClass[i] = i;
		RandomUtils::Init(randomizeClasses);
		RandomUtils::PermuteArray(rndClass, numClasses);

		Database **dbsRnd = new Database *[numClasses];
		for(uint32 i=0; i<numClasses; i++)
			dbsRnd[i] = dbs[rndClass[i]];
		delete[] dbs;
		delete[] rndClass;
		dbs = dbsRnd;
	}

	for(uint32 i=0; i<numClasses; i++) {
		if(randomizeClasses != 0)
			dbs[i]->RandomizeRowOrder(randomizeClasses);
		(*classSizes)[i] = dbs[i]->GetNumTransactions();
	}
	Database *db = Database::Merge(dbs, numClasses);

	for(uint32 i=0; i<numClasses; i++)
		delete dbs[i];
	delete[] dbs;
	delete oriDb;
	return db;
}

#endif // ENABLE_STREAM
