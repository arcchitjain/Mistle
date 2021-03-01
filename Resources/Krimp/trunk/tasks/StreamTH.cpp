//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
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
#ifdef ENABLE_DGEN
#include "../blocks/dgen/DGen.h"
#endif
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

	else	throw "StreamTH :: Unable to handle task `" + command + "`";
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
