#include "../clobal.h"

// qtils
#include <Config.h>

// bass
#include <Bass.h>
#include <db/Database.h>

#include "LESetMiner.h"

LESetMiner::LESetMiner(Config *conf) : Croupier(conf) {
	// todo: read config thingies into member variables
	mConfig = conf;

	mMaxEntropy = mConfig->Read<float>("entropy", LEMINER_DEFAULT_MAXENTROPY);

	// optional
	if(mConfig->Read<string>("minsetlen", "").compare("auto") == 0)
		mMinSetLen = (uint32) max(floor(mMaxEntropy), 1.0);
	else
		mMinSetLen = mConfig->Read<uint32>("minsetlen", LEMINER_DEFAULT_MINSETLEN);
	mMaxSetLen = mConfig->Read<uint32>("maxsetlen", LEMINER_DEFAULT_MAXSETLEN);
	if(mMinSetLen > mMaxSetLen) {
		throw string("LESetMiner::LESetMiner --- dont be a-kidding me with those set length bounds, boyo.");
	}

	// optional
	mMinFrequency = mConfig->Read<float>("minfreq", LEMINER_DEFAULT_MINFREQ);
	mMaxFrequency = mConfig->Read<float>("maxfreq", LEMINER_DEFAULT_MAXFREQ);
	if(mMinFrequency > mMaxFrequency) {
		throw string("LESetMiner::LESetMiner --- dont be a-kidding me with those frequency bounds, boyo.");
	}

	// optional MaxNumSets
	mMaxNumSets = mConfig->Read<uint32>("maxnumsets", LEMINER_DEFAULT_MAXNUMSETS);

	mUseMinerVersion = mConfig->Read<uint32>("version", LEMINER_DEFAULT_VERSION);

	mOutBuffer = new char[LEMINER_BUFFER_LENGTH];
	mFloatBuffer = 	new char [LEMINER_FLOATBUFFER_LENGTH];
}
LESetMiner::LESetMiner() : Croupier() {
	// Using defaults, adjust using SetVariable(..)

	mMinSetLen = LEMINER_DEFAULT_MINSETLEN;
	mMaxSetLen = LEMINER_DEFAULT_MAXSETLEN;
	mMaxEntropy = LEMINER_DEFAULT_MAXENTROPY;

	mMinFrequency = LEMINER_DEFAULT_MINFREQ;
	mMaxFrequency = LEMINER_DEFAULT_MAXFREQ;

	mMaxNumSets = LEMINER_DEFAULT_MAXNUMSETS;

	mUseMinerVersion = LEMINER_DEFAULT_VERSION;

	mOutBuffer = new char[LEMINER_BUFFER_LENGTH];
	mFloatBuffer = 	new char [LEMINER_FLOATBUFFER_LENGTH];
}
LESetMiner::~LESetMiner() {
	// not my Config*
	delete[] mOutBuffer;
	delete[] mFloatBuffer;
}

string LESetMiner::GenerateOutputFileName(const Database *db) {
//	char buf[200];
	return Bass::GetWorkingDir() + "lesets-" + db->GetName() + "-" + ".txt";
}

bool LESetMiner::MinePatternsToFile(const string &dbFilename, const string &outputFile, Database *db) {
	mDB = db;
	MineLESets();
	return true;
}
bool LESetMiner::MinePatternsToFile(const Database *db, const string &outputFile) {
	mDB = db;
	mOutFileName = outputFile;
	MineLESets();
	return true;
}


void LESetMiner::MineLESets() {
	mNumSetsWritten = 0;

	//string iscName = mConfig->Read<string>("iscName");
	string lescOutName = mOutFileName;
	string lescOutPath = Bass::GetExpDir();
	string lescOutFullpath = lescOutPath + lescOutName;
#ifdef WIN32	
	if(fopen_s(&mOutFile, lescOutFullpath.c_str(), "w") != 0)
#else
	mOutFile = fopen(lescOutFullpath.c_str(), "w");
	if(mOutFile == NULL)
#endif
		throw string("Could not write low-entropy set collection.");

	// Gather Attribute stats and calculate Entropy
	size_t abSize = mDB->GetAlphabetSize();
	uint16 *abItems = new uint16[abSize];
	uint32 *abCount = new uint32[abSize];
	uint32 numRows = mDB->GetNumRows();
	ItemSet **rows = mDB->GetRows();
	double* abEntropy = new double[abSize];
	alphabet *ab = mDB->GetAlphabet();
	alphabet::iterator it = ab->begin(), end = ab->end();
	uint32 numAbItems = 0;
	for(; it != end; ++it) {
		double pOne = ((double) it->second / numRows);
		double pNil = ((double) (numRows - it->second) / numRows);
		double entropy = ((pOne * log2(pOne)) + ((pNil) * log2(pNil)));
		entropy = -(entropy);

		if(pOne != 1.0 && pOne != 0.0 && pOne >= mMinFrequency && pOne <= mMaxFrequency && entropy <= mMaxEntropy) {
			abItems[numAbItems] = it->first;
			abCount[numAbItems] = it->second;
			abEntropy[numAbItems] = entropy;
			numAbItems++;
		}
	}
	if(numAbItems > 0) {

		// order abItems on entropy > (< is really bad idea :-)
		//if(false)
		{
			for(uint32 i=0; i<numAbItems-1; i++) {
				for(uint32 j=i+1; j<numAbItems; j++) {
					if(abEntropy[i] < abEntropy[j]) {
						double tmpEntropy = abEntropy[i];
						uint16 tmpItem = abItems[i];
						uint32 tmpCount = abCount[i];
						abEntropy[i] = abEntropy[j];
						abItems[i] = abItems[j];
						abCount[i] = abCount[j];
						abEntropy[j] = tmpEntropy;
						abItems[j] = tmpItem;
						abCount[j] = tmpCount;
					}
				}
			}
		}

		// do tha search
		uint32 prefixLen = 0;
		uint16 *prefix = new uint16[mMaxSetLen];
		if(mUseMinerVersion == 1) {
			// v1
			MineLESetsDepthFirstV1(mMaxEntropy, abItems, numAbItems, prefix, prefixLen);
		} else if(mUseMinerVersion == 31) {
			// v31 = v3.1
			mInstIdxs = new uint32*[mMaxSetLen+1];
			mInstCnts = new uint32*[mMaxSetLen+1];
			for(uint32 i=0; i<mMaxSetLen+1; i++) {
				mInstIdxs[i] = NULL;
				mInstCnts[i] = NULL;
			}
			mInstIdxs[0] = new uint32[mDB->GetNumRows()];
			memset(mInstIdxs[0], 0, sizeof(uint32) * mDB->GetNumRows());
			mInstCnts[0] = new uint32[1];
			mInstCnts[0][0] = mDB->GetNumRows();
			uint32 *numAbRowIdxs = new uint32[abSize];
			memset(numAbRowIdxs, 0, sizeof(uint32) * abSize);
			uint32 **abRowIdxs = new uint32*[abSize];
			for(uint32 i=0; i<numAbItems; i++) {
				abRowIdxs[i] = new uint32[abCount[i]];
			}
			for(uint32 r=0; r<numRows; r++) {
				ItemSet *row = rows[r];
				for(uint32 a=0; a<numAbItems; a++)		// dit moet veel sneller kunnen 
					if(row->IsItemInSet(abItems[a]))
						abRowIdxs[a][numAbRowIdxs[a]++] = r;
			}
			MineLESetsDepthFirstV31(mMaxEntropy, abItems, numAbItems, abCount, abRowIdxs, prefix, prefixLen);
			for(uint32 a=0; a<numAbItems; a++)
				delete[] abRowIdxs[a];
			delete[] abRowIdxs;
			delete[] numAbRowIdxs;
			for(uint32 i=0; i<mMaxSetLen+1; i++) {
				delete[] mInstIdxs[i];
				delete[] mInstCnts[i];
			} 
			delete[] mInstIdxs;
			delete[] mInstCnts;
		} else if(mUseMinerVersion == 4) {
			// v4
			mInstRowIdxs = new uint32**[mMaxSetLen+1];	// array over setlen, array over combi's, array over row indici
			mInstCnts = new uint32*[mMaxSetLen+1];		// array over setlen, array over combi's 
			for(uint32 i=0; i<mMaxSetLen+1; i++) {
				mInstRowIdxs[i] = NULL;
				mInstCnts[i] = NULL;
			}
			mInstCnts[0] = new uint32[1];
			mInstCnts[0][0] = numRows;
			mInstRowIdxs[0] = new uint32*[1];
			mInstRowIdxs[0][0] = new uint32[numRows];
			for(uint32 i=0; i<numRows; i++)
				mInstRowIdxs[0][0][i] = i;
			MineLESetsDepthFirstV4(mMaxEntropy, abItems, numAbItems, prefix, prefixLen, 0);
			for(uint32 i=0; i<mMaxSetLen+1; i++) {
				if(mInstRowIdxs[i] != NULL) {
					uint32 numCombis = (1 << i);
					for(uint32 j=0; j<numCombis; j++) {
						delete[] mInstRowIdxs[i][j];
					}
				}
				delete[] mInstRowIdxs[i];
				delete[] mInstCnts[i];
			} 
			delete[] mInstRowIdxs;
			delete[] mInstCnts;
		}
		else if(mUseMinerVersion == 41) {
			// v41

			mNumInstUsed = new uint32[mMaxSetLen+1];	// array over len: num used insts
			mInstIdxStart = new uint32*[mMaxSetLen+1];	// array over len, inst idx: idx in mInstIdxs
			mInstIdxs = new uint32*[mMaxSetLen+1];		// array over setlen, array over rows: row idx
			mInstCnts = new uint32*[mMaxSetLen+1];		// array over setlen, array over instantiations: usage

			for(uint32 i=0; i<mMaxSetLen+1; i++) {
				mNumInstUsed[i] = 0;
				mInstIdxStart[i] = NULL;
				mInstIdxs[i] = NULL;
				mInstCnts[i] = NULL;
			}
			mNumInstUsed[0] = 1;						// for the empty set, there's only 1 used inst
			mInstIdxStart[0] = new uint32[1];			
			mInstIdxStart[0][0] = 0;					// and that init starts at row 0
			mInstCnts[0] = new uint32[1];
			mInstCnts[0][0] = numRows;					// and that init is used `numRows' times
			mInstIdxs[0] = new uint32[numRows];		
			for(uint32 i=0; i<numRows; i++)
				mInstIdxs[0][i] = i;					// and its indici are 0...n
			MineLESetsDepthFirstV41(mMaxEntropy, abItems, numAbItems, prefix, prefixLen, 0);

			// cleanup
			for(uint32 i=0; i<mMaxSetLen+1; i++) {
				delete[] mInstIdxStart[i];
				delete[] mInstCnts[i];
				delete[] mInstIdxs[i];
			} 
			delete[] mInstCnts;
			delete[] mInstIdxs;
			delete[] mInstIdxStart;
			delete[] mNumInstUsed;		
		}

		delete[] prefix;
	}

	delete[] abEntropy;
	delete[] abItems;
	delete[] abCount;

	fclose(mOutFile);
}


void LESetMiner::MineLESetsDepthFirstV41(double maxEntropy, uint16* abItems, uint32 numAbItems, uint16* prefix, uint32 prefixLen, double prefixEntropy) {
	if(numAbItems == 0)
		return;

	ItemSet **rows = mDB->GetRows();
	uint32 numRows = mDB->GetNumRows();

	uint32 cndSetLen = prefixLen + 1;
	uint32 numCombis = 1 << cndSetLen;
	if(mInstCnts[cndSetLen] == NULL) {
		mInstCnts[cndSetLen] = new uint32[numCombis];
		mInstIdxs[cndSetLen] = new uint32[numRows];
		mInstIdxStart[cndSetLen] = new uint32[numCombis];
	}
	uint32 *cndInstCnts = mInstCnts[cndSetLen];		// huidige inst usage counts

	for(uint32 a=0; a<numAbItems; a++) {
		mNumInstUsed[cndSetLen] = 0;
		memset(cndInstCnts, 0, sizeof(uint32) * mNumInstUsed[prefixLen]);

		double curEntropy = prefixEntropy;

		uint32 idxOffset = 0;

		// voor elk van de gebruikte insts van prefix
		uint32 numPrefixInstsUsed = mNumInstUsed[prefixLen];
		for(uint32 c=0; c<numPrefixInstsUsed; c++) {
			uint32 curOne = 0;
			uint32 curNil = 0;

			uint32 numInstIdxs = mInstCnts[prefixLen][c];

			// voor elk van de rij-indici van deze prefix-instantiatie
			for(uint32 rIdx=0; rIdx<numInstIdxs; rIdx++) {
				uint32 rowIdx = mInstIdxs[prefixLen][idxOffset + rIdx];
				ItemSet *curRow = rows[rowIdx];
				if(curRow->IsItemInSet(abItems[a])) {
					mInstIdxs[cndSetLen][idxOffset + numInstIdxs-(1+curOne)] = rowIdx;
					curOne++;
				} else {
					mInstIdxs[cndSetLen][idxOffset + curNil] = rowIdx;
					curNil++;
				}
			}
			idxOffset += numInstIdxs;

			double p = ((double) mInstCnts[prefixLen][c] / numRows);
			if(p > 0)
				curEntropy += p * log2(p);	// from total entropy subtract the prefix's instantiation entropy

			double thisEntropy = 0;	
			// ahv curOne en curNil counts de boel ff goed instellen
			if(curNil > 0) {
				uint32 curInst = mNumInstUsed[cndSetLen];
				mInstIdxStart[cndSetLen][curInst] = mInstIdxStart[prefixLen][c];
				mInstCnts[cndSetLen][curInst] = curNil;
				mNumInstUsed[cndSetLen]++;

				p = ((double) curNil / numRows);
				thisEntropy += p * log(p);
			}
			if(curOne > 0) {
				uint32 curInst = mNumInstUsed[cndSetLen];
				mInstIdxStart[cndSetLen][curInst] = mInstIdxStart[prefixLen][c] + numInstIdxs - curOne;
				mInstCnts[cndSetLen][curInst] = curOne;
				mNumInstUsed[cndSetLen]++;

				p = ((double) curOne / numRows);
				thisEntropy += p * log(p);
			}
			
			// calculate the entropy for the new 0 and 1 instantiations
			thisEntropy = -thisEntropy / log(2.0);

			curEntropy = curEntropy + thisEntropy;
			if(curEntropy > maxEntropy)
				break;
		}

		if(curEntropy <= maxEntropy) {
			prefix[prefixLen] = abItems[a];
			if(prefixLen +1 >= mMinSetLen) {
				if(mNumSetsWritten >= mMaxNumSets)
					return;
				OutputEntropySet(prefix, prefixLen+1, curEntropy);
			}
			if(prefixLen +1 < mMaxSetLen)
				MineLESetsDepthFirstV41(maxEntropy, abItems+(a+1), numAbItems-(a+1), prefix, prefixLen+1, curEntropy);
		}
	}
}

void LESetMiner::MineLESetsDepthFirstV4(double maxEntropy, uint16* abItems, uint32 numAbItems, uint16* prefix, uint32 prefixLen, double prefixEntropy) {
	if(numAbItems == 0)
		return;

	ItemSet **rows = mDB->GetRows();
	uint32 numRows = mDB->GetNumRows();

	uint32 cndSetLen = prefixLen + 1;
	uint32 numPrefixCombis = 1 << prefixLen;
	uint32 numCombis = 1 << cndSetLen;
	if(mInstCnts[cndSetLen] == NULL) {
		mInstCnts[cndSetLen] = new uint32[numCombis];
		mInstRowIdxs[cndSetLen] = new uint32*[numCombis];
		for(uint32 i=0; i<numCombis; i++) {
			mInstRowIdxs[cndSetLen][i] = new uint32[numRows];	// evt afh van prefix (1 array van lengte numRows, deze array met indici daarnaartoe!)
		}
	}
	uint32 *cndInstCnts = mInstCnts[cndSetLen];

	for(uint32 a=0; a<numAbItems; a++) {
		memset(cndInstCnts, 0, sizeof(uint32) * numCombis);
		for(uint32 ic=0; ic<numPrefixCombis; ic++) {
			cndInstCnts[ic << 1] = mInstCnts[prefixLen][ic];
		}

		double curEntropy = prefixEntropy;

		// voor elk van deze combis
		for(uint32 c=0; c<numPrefixCombis; c++) {
			uint32 curOne = 0;
			uint32 curNil = 0;

			uint32 curInst = c << 1;

			// voor elk van de rij-indici van deze prefix-instantiatie
			for(uint32 rIdx=0; rIdx<mInstCnts[prefixLen][c]; rIdx++) {
				uint32 rowIdx = mInstRowIdxs[prefixLen][c][rIdx];
				ItemSet *curRow = rows[rowIdx];
				if(curRow->IsItemInSet(abItems[a])) {
					cndInstCnts[curInst]--;	// we're not in the base-case anymore, toto
					cndInstCnts[curInst+1]++;

					mInstRowIdxs[cndSetLen][curInst+1][curOne] = mInstRowIdxs[prefixLen][c][rIdx];
					curOne++;
				} else {
					// cndInstCnts is OK
					mInstRowIdxs[cndSetLen][curInst][curNil] = mInstRowIdxs[prefixLen][c][rIdx];
					curNil++;
				}
			}

			double p = ((double) mInstCnts[prefixLen][c] / numRows);
			if(p > 0)
				curEntropy += p * log2(p);	// from total entropy subtract the prefix's instantiation entropy

			// calculate the entropy for the new 0 and 1 instantiations
			double thisEntropy = 0;	
			if(cndInstCnts[curInst] > 0) {
				p = ((double) cndInstCnts[curInst] / numRows);
				thisEntropy += p * log(p);
			}
			if(cndInstCnts[curInst+1] > 0) {
				p = ((double) cndInstCnts[curInst+1] / numRows);
				thisEntropy += p * log(p);
			}
			thisEntropy = -thisEntropy / log(2.0);

			curEntropy = curEntropy + thisEntropy;
			if(curEntropy > maxEntropy)
				break;
		}

		if(curEntropy <= maxEntropy) {
			prefix[prefixLen] = abItems[a];
			if(prefixLen +1 >= mMinSetLen) {
				if(mNumSetsWritten >= mMaxNumSets)
					return;
				OutputEntropySet(prefix, prefixLen+1, curEntropy);
			}
			if(prefixLen +1 < mMaxSetLen)
				MineLESetsDepthFirstV4(maxEntropy, abItems+(a+1), numAbItems-(a+1), prefix, prefixLen+1, curEntropy);
		}
	}
}
void LESetMiner::MineLESetsDepthFirstV31(double maxEntropy, uint16* abItems, uint32 numAbItems, uint32 *numAbRowIdxs, uint32 **abRowIdxs, uint16* prefix, uint32 prefixLen) {
	if(numAbItems == 0)
		return;

	ItemSet **rows = mDB->GetRows();
	uint32 numRows = mDB->GetNumRows();

	uint32 cndSetLen = prefixLen + 1;
	uint32 numPrefixCombis = 1 << prefixLen;
	uint32 numCombis = 1 << cndSetLen;
	if(mInstCnts[cndSetLen] == NULL) {
		mInstCnts[cndSetLen] = new uint32[numCombis];
		mInstIdxs[cndSetLen] = new uint32[numRows];
	}
	uint32 *cndInstCnts = mInstCnts[cndSetLen];

	for(uint32 a=0; a<numAbItems; a++) {
		memset(cndInstCnts, 0, sizeof(uint32) * numCombis);
		for(uint32 ic=0; ic<numPrefixCombis; ic++) {
			cndInstCnts[ic << 1] = mInstCnts[prefixLen][ic];
		}

		memcpy(mInstIdxs[cndSetLen], mInstIdxs[prefixLen], sizeof(uint32) * numRows);
		for(uint32 r=0; r<numRows; r++) {
			mInstIdxs[cndSetLen][r] = mInstIdxs[cndSetLen][r] << 1;
		}
		// nu heb ik in abRowIdxs[a] een lijst met row indici in rows waarin a '1' is.
		for(uint32 i=0; i<numAbRowIdxs[a]; i++) {
			ItemSet *curRow = rows[abRowIdxs[a][i]];
			uint32 curInst = mInstIdxs[cndSetLen][abRowIdxs[a][i]];
			cndInstCnts[curInst]--;	// we're not in the base-case anymore, toto
			curInst |= 1;
			cndInstCnts[curInst]++;
			mInstIdxs[cndSetLen][abRowIdxs[a][i]] = curInst;
		}

		// calculate Entropy
		double curEntropy = 0;
		for(uint32 c=0; c<numCombis; c++) {
			if(cndInstCnts[c] == 0)
				continue;
			double p = ((double) cndInstCnts[c] / numRows);
			curEntropy += p * log(p);
		}
		curEntropy = - curEntropy / log(2.0);	// Add entropy of a=0 and a=1 instantiations

		if(curEntropy <= maxEntropy) {
			prefix[prefixLen] = abItems[a];

			if(prefixLen +1 >= mMinSetLen) {
				if(mNumSetsWritten >= mMaxNumSets)
					return;
				OutputEntropySet(prefix, prefixLen+1, curEntropy);
			}
			if(prefixLen +1 < mMaxSetLen)
				MineLESetsDepthFirstV31(maxEntropy, abItems+(a+1), numAbItems-(a+1), numAbRowIdxs+(a+1), abRowIdxs+(a+1), prefix, prefixLen+1);
		}
	}
}

void LESetMiner::MineLESetsDepthFirstV3(double maxEntropy, uint16* abItems, uint32 numAbItems, uint16* prefix, uint32 prefixLen) {
	if(numAbItems == 0)
		return;

	ItemSet **rows = mDB->GetRows();
	uint32 numRows = mDB->GetNumRows();

	uint32 cndSetLen = prefixLen + 1;
	uint32 numPrefixCombis = 1 << prefixLen;
	uint32 numCombis = 1 << cndSetLen;
	if(mInstCnts[cndSetLen] == NULL) {
		mInstCnts[cndSetLen] = new uint32[numCombis];
		mInstIdxs[cndSetLen] = new uint32[numRows];
	}
	uint32 *cndInstCnts = mInstCnts[cndSetLen];

	for(uint32 a=0; a<numAbItems; a++) {
		memset(cndInstCnts, 0, sizeof(uint32) * numCombis);
		for(uint32 ic=0; ic<numPrefixCombis; ic++) {
			cndInstCnts[ic << 1] = mInstCnts[prefixLen][ic];
		}

		// nu per rij kijken welke inst het is, en daarvan de count ophogen
		for(uint32 r=0; r<numRows; r++) {
			ItemSet *curRow = rows[r];
			uint32 curInst = mInstIdxs[prefixLen][r] << 1;
			if(curRow->IsItemInSet(abItems[a])) {
				cndInstCnts[curInst]--;	// we're not in the base-case anymore, toto
				curInst |= 1;
				cndInstCnts[curInst]++;
			}
			mInstIdxs[cndSetLen][r] = curInst;
		}

		// calc Entropy
		double curEntropy = 0;
		for(uint32 c=0; c<numCombis; c++) {
			if(cndInstCnts[c] == 0)
				continue;
			double p = ((double) cndInstCnts[c] / numRows);
			curEntropy += p * log(p);
		}
		curEntropy = - curEntropy / log(2.0);

		if(curEntropy <= maxEntropy) {
			prefix[prefixLen] = abItems[a];

			if(prefixLen +1 >= mMinSetLen) {
				if(mNumSetsWritten >= mMaxNumSets)
					return;
				OutputEntropySet(prefix, prefixLen+1, curEntropy);
			}
			if(prefixLen +1 < mMaxSetLen)
				MineLESetsDepthFirstV3(maxEntropy, abItems+(a+1), numAbItems-(a+1), prefix, prefixLen+1);
		}
	}
}
 
void LESetMiner::MineLESetsDepthFirstV1(double maxEntropy, uint16* abItems, uint32 numAbItems, uint16* prefix, uint32 prefixLen) {
	if(numAbItems == 0)
		return;

	// todo: bij juiste abItem beginnen!
	uint32 curAbItem = 0;

	ItemSet **rows = mDB->GetRows();
	uint32 numRows = mDB->GetNumRows();

	uint32 numCombis = 1 << (prefixLen+1);
	uint32 *cntCombis = new uint32[numCombis];

	for(uint32 a=curAbItem; a<numAbItems; a++) {
		memset(cntCombis, 0, sizeof(uint32) * numCombis);

		// nu per rij kijken welke inst het is, en daarvan de count ophogen
		for(uint32 r=0; r<numRows; r++) {
			ItemSet *curRow = rows[r];
			uint32 curInst = 0;
			for(uint32 p=0; p<prefixLen; p++)
				if(curRow->IsItemInSet(prefix[p]))
					curInst |= 1 << p;
			if(curRow->IsItemInSet(abItems[a]))
				curInst |= 1 << prefixLen;

			cntCombis[curInst]++;
		}

		// calc Entropy
		double curEntropy = 0;
		for(uint32 c=0; c<numCombis; c++) {
			if(cntCombis[c] == 0)
				continue;
			double p = ((double) cntCombis[c] / numRows);
			curEntropy += p * log(p);
		}
		curEntropy = - curEntropy / log(2.0);

		if(curEntropy <= maxEntropy) {
			prefix[prefixLen] = abItems[a];

			if(prefixLen +1 >= mMinSetLen) {
				if(mNumSetsWritten >= mMaxNumSets)
					return;
				OutputEntropySet(prefix, prefixLen+1, curEntropy);
			}
			if(prefixLen +1 < mMaxSetLen)
				MineLESetsDepthFirstV1(maxEntropy, abItems+(a+1), numAbItems-(a+1), prefix, prefixLen+1);
		}
	}

	delete[] cntCombis;
}

void inline LESetMiner::OutputEntropySet(uint16 *set, uint32 len, double entropy) {
	mNumSetsWritten++;

	char *bp = mOutBuffer;

	_itoa_s(len, mOutBuffer, LEMINER_BUFFER_LENGTH, 10);
	bp += strlen(mOutBuffer);
	*bp = ':';
	*(bp+1) = ' ';
	bp += 2;
	for(uint32 j=0; j<len; j++) {
		_itoa_s(set[j], bp, LEMINER_BUFFER_LENGTH - (bp - mOutBuffer), 10);
		bp += strlen(bp);
		*bp = ' ';
		bp++;
	}
	*bp = '(';
	bp++;

#ifdef WIN32
	int decimal;
	int sign;
	_fcvt_s(mFloatBuffer, LEMINER_FLOATBUFFER_LENGTH, entropy, 5, &decimal, &sign);
	if(entropy < 1) {
		*bp = '0';
		bp += 1;
	} else {
		memcpy(bp,mFloatBuffer,sizeof(char) * decimal);
		bp += decimal;
	}
	*bp = '.';
	bp += 1;
	uint32 restsize = 4;
	for(; decimal<0; decimal++) {
		*bp = '0';
		bp += 1;
		restsize--;
	}
	memcpy(bp,mFloatBuffer + decimal, sizeof(char) * restsize);
	bp += restsize;
#else
	sprintf(mFloatBuffer, "%.4f", entropy);
	memcpy(bp,mFloatBuffer,strlen(mFloatBuffer) * sizeof(char));
	bp += strlen(mFloatBuffer) * sizeof(char);
#endif
	*bp = ')';
	*(bp+1) = '\n';
	bp+=2;

	fwrite(mOutBuffer,sizeof(char),bp - mOutBuffer, mOutFile);
}
