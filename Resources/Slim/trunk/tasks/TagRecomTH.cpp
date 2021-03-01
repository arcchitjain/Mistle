#ifdef BLOCK_TAGRECOM

#include "../global.h"

#include <time.h>

// qtils
#include <Config.h>
#include <ArrayUtils.h>
#include <FileUtils.h>
#include <StringUtils.h>
#include <TimeUtils.h>
#include <RandomUtils.h>
#include <logger/Log.h>

// bass
#include <Bass.h>
#include <db/Database.h>
#include <isc/ItemSetCollection.h>
#include <isc/IscFile.h>
#include <itemstructs/CoverSet.h>

// croupier
#include <Croupier.h>

#include "../blocks/krimp/codetable/CodeTable.h"
#include "../FicConf.h"
#include "../FicMain.h"

#include "TagRecomTH.h"

TagRecomTH::TagRecomTH(Config *conf) : TaskHandler(conf){

}
TagRecomTH::~TagRecomTH() {
	// not my Config*
}

void TagRecomTH::HandleTask() {
	string command = mConfig->Read<string>("command");

			if(command.compare("recommend") == 0)				TagRecommendation();
	else	if(command.compare("fastrecom") == 0)				FastRecommendation();
	else	if(command.compare("cprecom") == 0)					CondProbRecommendation();
	else	if(command.compare("cprecom2") == 0)				CondProbRecommendation2();
	else	if(command.compare("cprecom3") == 0)				CondProbRecommendation3();
	else	if(command.compare("cprecom4") == 0)				CondProbRecommendation4();
	else	if(command.compare("suppestimation") == 0)			TestSupportEstimation();
	else	if(command.compare("slowrecom") == 0)				SlowRecommendation();
	else	if(command.compare("latre") == 0)					LATRE();
	else	if(command.compare("borkursbaseline") == 0)			BorkursBaseline();
	else	if(command.compare("converttolatre") == 0)			ConvertToLatre();
	else	if(command.compare("groupstolatre") == 0)			ConvertGroupsToLatre();

	else	throw "TagRecomTH :: Unable to handle task " + command;
}

string TagRecomTH::BuildWorkingDir() {
	string command = mConfig->Read<string>("command");
	if(command.compare("groupstolatre") == 0)
		return mConfig->Read<string>("command") + "/" + mConfig->Read<string>("tag") + "-" + TimeUtils::GetTimeStampString() + "/";
	if(command.compare("suppestimation") == 0)
		return mConfig->Read<string>("command") + "/" + mConfig->Read<string>("db") + "-" + TimeUtils::GetTimeStampString() + "/";
	return mConfig->Read<string>("command") + "/" + mConfig->Read<string>("testDB") + "-" + TimeUtils::GetTimeStampString() + "/";
}
void TagRecomTH::TagRecommendation() {
	char temp[500]; string s;

	// Settings
	string ctDir = mConfig->Read<string>("ctDir");
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend");
	uint32 numTotal = numInput + numRecommend;
	bool allowOverlap = mConfig->Read<bool>("allowOverlap", false);
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- CT dir: %s", ctDir.c_str()); s=temp; LOG_MSG(s);
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Allow overlap: %s", allowOverlap?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint32 alphSize = (uint16) testDB->GetAlphabetSize();
	
	// Load CTs
	uint32 numCTs = 0;
	{
		directory_iterator itr(ctDir, "*.ct");
		while(itr.next())
			numCTs++;
	}
	if(numCTs == 0)
		THROW("No code tables found...");

	CodeTable **codeTables = new CodeTable *[numCTs];
	string *ctFilenames = new string[numCTs];
	{
		directory_iterator itr(ctDir, "*.ct");
		uint32 curCT = 0;
		while(itr.next()) {
			ctFilenames[curCT] = itr.filename();
			codeTables[curCT++] = CodeTable::LoadCodeTable(itr.fullpath(), testDB);
		}
	}
	sprintf_s(temp, 500, "#CTs loaded: %u", numCTs); s=temp; LOG_MSG(s);

	// Init structures
	printf_s("\nBuilding structures ... ");
	ItemSet ***ctElemss = new ItemSet **[numCTs];
	uint32 *numElemss = new uint32[numCTs];
	ItemSet ***alphElemss = new ItemSet **[numCTs];
	ItemSet ***mixedElemss = new ItemSet **[numCTs];
	uint32 *numMixeds = new uint32[numCTs];
	for(uint32 i=0; i<numCTs; i++) {
		ctElemss[i] = codeTables[i]->GetItemSetArray();
		numElemss[i] = codeTables[i]->GetCurNumSets();
		alphElemss[i] = codeTables[i]->GetSingletonArray();
		numMixeds[i] = numElemss[i] + alphSize;
		mixedElemss[i] = new ItemSet *[numMixeds[i]];
		memcpy_s(mixedElemss[i], numMixeds[i] * sizeof(ItemSet *), ctElemss[i], numElemss[i]*sizeof(ItemSet *));
		memcpy_s(mixedElemss[i]+numElemss[i], alphSize * sizeof(ItemSet *), alphElemss[i], alphSize*sizeof(ItemSet *));

		// Compute codelens for all CT elems (sing+nonsing) in mixedElemss[i]
		uint32 num = numMixeds[i];
		ItemSet **elems = mixedElemss[i];
		double *codeLens = new double[num];
		uint32 *sortMap = new uint32[num];
		uint64 countSum = codeTables[i]->GetCurStats().usgCountSum;
		for(uint32 j=0; j<num; j++) {
			codeLens[j] = CalcCodeLength(elems[j]->GetUsageCount(), countSum) / elems[j]->GetLength(); // avg codelen per tag
			sortMap[j] = j;
		}

		// Sort avg codelens asc and create sorting map
		ArrayUtils::BSortAsc<double>(codeLens, num, sortMap);

		// Replace mixedElemss[i] with correctly sorted version, saves trouble later on
		ItemSet **newElems = new ItemSet *[num];
		for(uint32 j=0; j<num; j++) {
			codeLens[j] *= elems[j]->GetLength(); // .. and go back to normal codelens
			newElems[j] = elems[sortMap[j]];
			newElems[j]->SetStandardLength(codeLens[j]);		// !-- Warning: Ugly but effective storage of codeLength --!
		}
		delete[] mixedElemss[i];
		mixedElemss[i] = newElems;
		delete[] sortMap;
		delete[] codeLens;
	}
	printf_s("done\n\n");

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	uint32 *ctUsed = new uint32[numCTs];
	for(uint32 i=0; i<numCTs; i++)
		ctUsed[i] = 0;

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recom;correct;P@1;P@3;P@5;S@3;S@5;MRR;usedCT\n"); 
	}

	// For each test row
	printf_s("Recommending");
#ifdef _DEBUG
	printf_s("\n");
#endif
	uint32 valuesLength = 2 * max(testDB->GetMaxSetLength(),numTotal);
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput==0 ? 2*min(numRecommend,5) : numInput+min(numRecommend,5); // p@5 hardcoded here.
	uint32 splitPoint;
	for(uint32 r=0; r<testDB->GetNumRows(); r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		splitPoint = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<splitPoint; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=splitPoint; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;

		// Init best solution found so far
		ItemSet *bestRecom = NULL, *bestBase = NULL;
		double bestSize = DOUBLE_MAX_VALUE;
		uint32 bestCT = 0;

		// Generate recoms using each CT
		for(uint32 selCT = 0; selCT<numCTs; selCT++) {
			// Check for recommendations using selected CT
			ItemSet **elems = mixedElemss[selCT]; // singletons + non-singletons
			uint32 numElems = numMixeds[selCT];

			// Select elems that intersect with input
			islist *baseElems = new islist();
			for(uint32 i=0; i<numElems; i++)
				if(elems[i]->Intersects(inputTags))
					baseElems->push_back(elems[i]);

			// Generate candidates that cover input
			islist *cands = new islist();
			ItemSet *emptySet = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
			emptySet->SetStandardLength(0.0); // abuse stdLen for regular codelen
			GenerateCandidates(cands, baseElems, baseElems->begin(), inputTags, emptySet, allowOverlap);
			delete emptySet;

			// For each generated candidated
			for(islist::iterator it=cands->begin(); it!=cands->end(); it++) {
				ItemSet *cand = *it;
				ItemSet *baseCand = NULL;
#ifdef _DEBUG
				printf_s("Input: %s\tCandidate: %s\t%f\n", inputTags->ToString(false).c_str(), cand->ToString(false).c_str(), cand->GetStandardLength());
#endif

				// Check whether candidate consists of only singletons and if there are other candidates. If so, skip it.
				if(cand->GetLength() == numInput && cands->size() > 1) {
#ifdef _DEBUG
					printf_s("Too bad, only singletons!\n");
#endif
					delete cand;
					continue;
				}

				// Check whether candidate is already too long
/*				if(cand->GetLength() > numTotal) {		-- Not necessary to do this. Minimising code length is sufficient.
#ifdef _DEBUG
					printf_s("Too bad, too long!\n");
#endif
					delete cand;
					continue;
				}*/

				// Greedily complete candidate to minimum total length if necessary
				if(cand->GetLength() < numTotal) {
					baseCand = cand->Clone();
					uint32 toRecommend = numTotal - cand->GetLength();
					for(uint32 idx = 0; toRecommend>0 && idx<numElems; ++idx) {
						ItemSet *elem = elems[idx];
						if(elem->GetLength() <= toRecommend && // allow only if it fits within specified length
													!elem->Intersects(cand)) { // never allow overlap here
							cand->Unite(elem);
							cand->SetStandardLength(cand->GetStandardLength() + elem->GetStandardLength());
							toRecommend = numTotal - cand->GetLength();
						}
					}
				}
#ifdef _DEBUG
				printf_s("Input: %s\tComplete cand: %s\t%f\n", inputTags->ToString(false).c_str(), cand->ToString(false).c_str(), cand->GetStandardLength());
#endif

				// Check if recommendation has required minimum length
				if(cand->GetLength() < numTotal) {
					delete cand;
					continue;
				}

				// Check if better than best so far
				if(cand->GetStandardLength() < bestSize) {
					delete bestRecom;
					delete bestBase;
					bestSize = cand->GetStandardLength();
					bestRecom = cand;
					bestBase = baseCand;
					bestCT = selCT;
				} else {
					// Cleanup
					delete cand;
					delete baseCand;
				}
			}

			// Cleanup lists (not what is in the lists)
			delete cands;
			delete baseElems;
		}

		// Test best recommendation found
		if(bestRecom != NULL) {
#ifdef _DEBUG
			printf_s("** Best: CT=%u\t%s\t%f\n", bestCT, bestRecom->ToString(false).c_str(), bestSize);
#endif

			ctUsed[bestCT]++;
			bestRecom->Remove(inputTags);
			if(bestRecom->GetLength() > valuesLength) {
				valuesLength = 2 * bestRecom->GetLength();
				delete[] values;
				values = new uint16[valuesLength];
			}
			if(bestBase==NULL || bestBase->GetLength()==numInput)	{ // all from 1) overlap or 2) completion
				bestRecom->GetValuesIn(values);
			} else {
				bestBase->Remove(inputTags);
				ItemSet *rest = bestRecom->Clone();
				rest->Remove(bestBase);
				rest->GetValuesIn(values);
				memcpy_s(values+bestBase->GetLength(), rest->GetLength()*sizeof(uint16), values, rest->GetLength()*sizeof(uint16));
				bestBase->GetValuesIn(values);
				delete rest;
			}
			ItemSet *correctTags = bestRecom->Intersection(testTags);
			{
				// Compute stats
				uint32 firstCorrect = UINT32_MAX_VALUE;
				for(uint32 i=0; i<bestRecom->GetLength(); i++) {
					if(correctTags->IsItemInSet(values[i])) {
						if(firstCorrect == UINT32_MAX_VALUE)
							firstCorrect = i;
						if(i<5) {
							precision5 += 0.2;
							if(i<3) {
								precision3 += 0.3333333333333333;
								if(i==0)
									precision1 += 1.0;
							}
						}
					}
				}
				if(firstCorrect < UINT32_MAX_VALUE) {
					mrr += 1.0 / (firstCorrect+1);
					if(firstCorrect < 5) {
						success5 += 1.0;
						if(firstCorrect < 3)
							success3 += 1.0;
					}
				}

				// Save to file, if asked for
				if(saveRecommendations) {
					fprintf_s(recomFP, "%s;%s;%s;%s;%f;%f;%f;%u;%u;%f;%d\n", 
						inputTags->ToString(false).c_str(), testTags->ToString(false).c_str(), bestRecom->ToString(false).c_str(), correctTags->ToString(false).c_str(),
						precision1, precision3, precision5,
						firstCorrect<3?1:0,	// S@3
						firstCorrect<5?1:0,	// S@5
						firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0, // MRR
						bestCT
					);
				}
			}

			// Cleanup a bit
			delete correctTags;
			delete bestRecom;
			delete bestBase;
		} else if(saveRecommendations) {
			fprintf_s(recomFP, "%s;%s;None found!;;%f;%f;%f;0;0;0.0;-\n", inputTags->ToString(false).c_str(), testTags->ToString(false).c_str(),
						precision1, precision3, precision5);
		}

		// Cleanup
		delete inputTags;
		delete testTags;

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	if(numRecommend < 5) {
		precision5 = 0.0; success5 = 0.0;
		if(numRecommend < 3)
			precision3 = 0.0; success3 = 0.0;
	}
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- CT usage:"); s=temp; LOG_MSG(s);
	for(uint32 i=0; i<numCTs; i++) {
		sprintf_s(temp, 500, "\t%u\t%s", ctUsed[i], ctFilenames[i].c_str()); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	fprintf_s(fp, "ctDir;%s\n", ctDir.c_str());
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
	fprintf_s(fp, "numInput;numRecommend;overlap;numCTs;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR\n");
	fprintf_s(fp, "%u;%u;%s;%u;%u;%u;%f;%f;%f;%f;%f;%f\n", numInput, numRecommend, allowOverlap?"yes":"no", numCTs, numRowsTested, testDB->GetNumRows()-numRowsTested, 
								precision1, precision3, precision5, success3, success5, mrr);
	fclose(fp);

	// Cleanup
	delete[] ctUsed;
	delete[] values;
	for(uint32 i=0; i<numCTs; i++) {
		for(uint32 j=0; j<alphSize; j++)
			delete alphElemss[i][j];
		delete[] alphElemss[i];
		delete[] ctElemss[i];
		delete[] mixedElemss[i];
		delete codeTables[i];
	}
	delete[] mixedElemss;
	delete[] alphElemss;
	delete[] ctElemss;
	delete[] numElemss;
	delete[] numMixeds;
	delete[] ctFilenames;
	delete[] codeTables;
	delete testDB;
}

void TagRecomTH::GenerateCandidates(islist *cands, islist *elems, islist::iterator startFrom, ItemSet *uncovered, ItemSet *current, const bool allowOverlap) {
	// Foreach element
	for(islist::iterator it=startFrom; it!=elems->end(); it++) {
		if(!allowOverlap && current->Intersects(*it))
			continue; // no overlap allowed
		if(!uncovered->Intersects(*it))
			continue; // should cover at least one uncovered item
		if((*it)->GetStandardLength() == std::numeric_limits<double>::infinity())
			continue; // we don't want to use zero-usage CT elements, now do we

		// Build new uncovered and current tagsets
		ItemSet *newUncovered = uncovered->Clone();
		newUncovered->Remove(*it);
		ItemSet *newCurrent = current->Union(*it);
		newCurrent->SetStandardLength(current->GetStandardLength() + (*it)->GetStandardLength());
		
		if(newUncovered->GetLength() == 0) {	// all covered, found new candidate!
			cands->push_back(newCurrent);
		} else {								// not yet all covered, try to recurse
			islist::iterator next = it; ++next;
			if(next!=elems->end())
				GenerateCandidates(cands, elems, next, newUncovered, newCurrent, allowOverlap);
			delete newCurrent;
		}

		// Cleanup
		delete newUncovered;
	}
}

void TagRecomTH::FastRecommendation() {
	char temp[500]; string s;
	Timer *timer1 = new Timer();

	// Settings
	string ctDir = mConfig->Read<string>("ctDir", "");
	string iscName = mConfig->Read<string>("iscName", "");
	bool useCT = ctDir.size() > 0 ? true : false;
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend");
	uint32 numTotal = numInput + numRecommend;
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	string sOrder = mConfig->Read<string>("recomOrder", "support");
	IscOrderType recomOrder = NoneIscOrder;
	if(sOrder.compare("support") == 0)
		recomOrder = LengthDescIscOrder;
	else if(sOrder.compare("usage") == 0)
		recomOrder = UsageDescIscOrder;
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Mode: %s", useCT?"code table":"itemset collection"); s=temp; LOG_MSG(s);
	if(useCT) {
		sprintf_s(temp, 500, "- CT dir: %s", ctDir.c_str()); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- ISC: %s", iscName.c_str()); s=temp; LOG_MSG(s);
	}
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Order: %s", sOrder.c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint32 alphSize = (uint16) testDB->GetAlphabetSize();
	
	// Init vars
	ItemSet **ctElems = NULL;
	ItemSet **alphElems = NULL;
	ItemSet **mixedElems = NULL;
	uint32 numElems = 0;
	uint32 numMixed = 0;
	CodeTable *codeTable = NULL;
	string ctFilename;
	ItemSetCollection *isc = NULL;
	
	if(useCT) {										// -------------------- CT mode
		// Load CT
		uint32 numCTs = 0;
		{
			directory_iterator itr(ctDir, "*.ct");
			while(itr.next())
				numCTs++;
		}
		if(numCTs == 0)
			THROW("No code tables found...");
		if(numCTs > 1)
			THROW("Fast recommendation can only handle a single CT.");

		{
			directory_iterator itr(ctDir, "*.ct");
			uint32 curCT = 0;
			while(itr.next()) {
				ctFilename = itr.filename();
				codeTable = CodeTable::LoadCodeTable(itr.fullpath(), testDB);
			}
		}
		LOG_MSG("CT loaded.");

		// Init structures
		printf_s("\nBuilding structures ... ");
		ctElems = codeTable->GetItemSetArray();
		numElems = codeTable->GetCurNumSets();
		alphElems = codeTable->GetSingletonArray();
		numMixed = numElems + alphSize;
		mixedElems = new ItemSet *[numMixed];
		memcpy_s(mixedElems, numMixed * sizeof(ItemSet *), ctElems, numElems*sizeof(ItemSet *));
		memcpy_s(mixedElems+numElems, alphSize * sizeof(ItemSet *), alphElems, alphSize*sizeof(ItemSet *));

		// Sort itemsets
		if(recomOrder == UsageDescIscOrder)	{	// usage(desc) length(desc)
			ItemSet::SortItemSetArray(ctElems, numElems, UsageDescIscOrder);
			ItemSet::SortItemSetArray(mixedElems, numMixed, UsageDescIscOrder);
		} else if(recomOrder == LengthDescIscOrder)	{ // support(desc) length(desc)
			ItemSet::SortItemSetArray(ctElems, numElems, LengthDescIscOrder);
			ItemSet::SortItemSetArray(mixedElems, numMixed, LengthDescIscOrder);
		}
		//else			// CT order
		// do nothing
		printf_s("done\n\n");

	} else {											// -------------- ISC mode
		string dbName = iscName.substr(0,iscName.find_first_of('-'));
		Database *db = Database::RetrieveDatabase(dbName);
		bool beenMined;
		isc = FicMain::ProvideItemSetCollection(iscName, db, beenMined, false, true);
		delete db;

		if(isc->GetNumLoadedItemSets() != isc->GetNumItemSets())
			THROW("Too many itemsets, not all of them are loaded in memory; try a higher minsup/lower maxlen.");

		// Init structures
		printf_s("\nBuilding structures ... ");
		ctElems = isc->GetLoadedItemSets();
		numElems = isc->GetNumLoadedItemSets();
		numMixed = numElems;
		mixedElems = ctElems;

		// Sort itemsets
		/*if(recomOrder == UsageDescIscOrder)	{	// usage(desc) length(desc)			-- Assume isc is already in correct order! (Option d)
			ItemSet::SortItemSetArray(ctElems, numElems, UsageDescIscOrder);
		} else if(recomOrder == LengthDescIscOrder)	{ // support(desc) length(desc)
			ItemSet::SortItemSetArray(ctElems, numElems, LengthDescIscOrder);
		}*/
		//else			// CT order
		// do nothing
		printf_s("done\n\n");
	}

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	Timer *timer2 = new Timer();

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recommendation;correct;P@1;P@3;P@5;S@3;S@5;MRR\n"); 
	}

	// For each test row
	printf_s("Recommending");
#ifdef _DEBUG
	printf_s("\n");
#endif
	uint32 valuesLength = 2 * max(testDB->GetMaxSetLength(),numTotal);
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput==0 ? 2 : numInput+min(numRecommend,5); // half/half-->min 2, other-->numIn+atleast(5)
	uint32 numActualInput;
	for(uint32 r=0; r<testDB->GetNumRows(); r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		numActualInput = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<numActualInput; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=numActualInput; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;

		// Select elems that intersect with input
		islist **firstRankElems = new islist *[numActualInput];
		for(uint32 i=0; i<numActualInput; i++)
			firstRankElems[i] = new islist();
		ItemSet *selUnion = inputTags->Clone();
		for(uint32 i=0; i<numElems; i++) {
			if(ctElems[i]->Intersects(inputTags)) {
				ItemSet *is = ctElems[i]->Intersection(inputTags);
				if(ctElems[i]->GetLength() == is->GetLength()) { // check could be stricter by intersecting with selUnion (but this is needed for ranking)
					delete is; // does not add anything
					continue;
				}
				uint32 idx = is->GetLength() - 1;
				delete is;
				firstRankElems[idx]->push_back(ctElems[i]);
				selUnion->Unite(ctElems[i]);
			}
		}

		// Second rank
		islist *secondRankElems = new islist();
		while(selUnion->GetLength() < numTotal) {
			ItemSet *best = NULL;
			uint32 bestOverlap = 0;
			for(uint32 i=0; i<numElems; i++) {
				if(ctElems[i]->Intersects(selUnion)) {
					ItemSet *is = ctElems[i]->Intersection(selUnion);
					if(is->GetLength() == ctElems[i]->GetLength()) {
						delete is;
						continue; // adds nothing!
					}
					if(is->GetLength() > bestOverlap) {
						bestOverlap = is->GetLength();
						best = ctElems[i];
					}
					delete is;
				}
			}
			if(best == NULL)
				break;
			secondRankElems->push_back(best);
			selUnion->Unite(best);
		}

		// Just add anything in order, if needed
		islist *lastRankElems = new islist();
		for(uint32 i=0; selUnion->GetLength() < numTotal && i<numMixed; i++) {
			ItemSet *is = mixedElems[i]->Intersection(selUnion);
			if(is->GetLength() == mixedElems[i]->GetLength()) {
				delete is;
				continue; // adds nothing!
			}
			delete is;
			lastRankElems->push_back(mixedElems[i]);
			selUnion->Unite(mixedElems[i]);
		}

		// Test recommendation
		selUnion->Remove(inputTags);
#ifdef _DEBUG
		printf_s("** Recom: %s\n", selUnion->ToString(false).c_str());
#endif

		if(selUnion->GetLength() > valuesLength) {
			valuesLength = 2 * selUnion->GetLength();
			delete[] values;
			values = new uint16[valuesLength];
		}

		uint32 curLen = 0;
		ItemSet *soFar = inputTags->Clone();
		for(int32 n=numActualInput-1; n>=0; n--) {	// First rank
			islist *isl = firstRankElems[n];
			for(islist::iterator iter = isl->begin(); iter!=isl->end(); iter++) {
				ItemSet *is = (*iter)->Clone();
				is->Remove(soFar);
				is->GetValuesIn(values+curLen);
				curLen += is->GetLength();
				soFar->Unite(is);
				delete is;
			}
		}
		for(islist::iterator iter = secondRankElems->begin(); iter!=secondRankElems->end(); iter++) { // Second rank
			ItemSet *is = (*iter)->Clone();
			is->Remove(soFar);
			is->GetValuesIn(values+curLen);
			curLen += is->GetLength();
			soFar->Unite(is);
			delete is;
		}
		for(islist::iterator iter = lastRankElems->begin(); iter!=lastRankElems->end(); iter++) { // Last rank
			ItemSet *is = (*iter)->Clone();
			is->Remove(soFar);
			is->GetValuesIn(values+curLen);
			curLen += is->GetLength();
			soFar->Unite(is);
			delete is;
		}
		delete soFar;
		if(curLen != selUnion->GetLength())
			THROW("Strange things happening.");

		ItemSet *correctTags = selUnion->Intersection(testTags);
		{
			// Compute stats
			uint32 firstCorrect = UINT32_MAX_VALUE;
			for(uint32 i=0; i<curLen; i++) {
				if(correctTags->IsItemInSet(values[i])) {
					if(firstCorrect == UINT32_MAX_VALUE)
						firstCorrect = i;
					if(i<5) {
						precision5 += 0.2;
						if(i<3) {
							precision3 += 0.3333333333333333;
							if(i==0)
								precision1 += 1.0;
						}
					}
				}
			}
			if(firstCorrect < UINT32_MAX_VALUE) {
				mrr += 1.0 / (firstCorrect+1);
				if(firstCorrect < 5) {
					success5 += 1.0;
					if(firstCorrect < 3)
						success3 += 1.0;
				}
			}

			// Save to file, if asked for
			if(saveRecommendations) {
				fprintf_s(recomFP, "%s;%s;%s;%s;%f;%f;%f;%u;%u;%f\n", 
					inputTags->ToString(false).c_str(), testTags->ToString(false).c_str(), selUnion->ToString(false).c_str(), correctTags->ToString(false).c_str(),
					precision1, precision3, precision5,
					firstCorrect<3?1:0,	// S@3
					firstCorrect<5?1:0,	// S@5
					firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0 // MRR
				);
			}
		}

		// Cleanup
		delete correctTags;
		delete selUnion;
		for(uint32 i=0; i<numActualInput; i++)
			delete firstRankElems[i];
		delete firstRankElems;
		delete secondRankElems;
		delete lastRankElems;
		delete inputTags;
		delete testTags;

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	if(numRecommend < 5) {
		precision5 = 0.0; success5 = 0.0;
		if(numRecommend < 3)
			precision3 = 0.0; success3 = 0.0;
	}
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	if(useCT) {
		fprintf_s(fp, "ctDir;%s\n", ctDir.c_str());
		fprintf_s(fp, "ctName;%s\n", ctFilename.c_str());
	} else {
		fprintf_s(fp, "iscName;%s\n", iscName.c_str());
	}
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
	fprintf_s(fp, "source;numInput;numRecommend;#sets;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_all (s);time_recom (s)\n");
	fprintf_s(fp, "%s;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%I64u;%I64u\n", useCT?ctFilename.c_str():iscName.c_str(), numInput, numRecommend, numElems, numRowsTested, testDB->GetNumRows()-numRowsTested, 
								precision1, precision3, precision5, success3, success5, mrr, timer1->GetElapsedTime(), timer2->GetElapsedTime());
	fclose(fp);

	// Cleanup
	delete timer1;
	delete timer2;
	delete[] values;
	if(useCT) {
		for(uint32 j=0; j<alphSize; j++)
			delete alphElems[j];
		delete[] alphElems;
		delete[] mixedElems;
		delete[] ctElems;
		delete codeTable;
	} else {
		delete isc;
	}
	delete testDB;
}

void TagRecomTH::CondProbRecommendation() {
	char temp[500]; string s;
	Timer *timer1 = new Timer();

	// Settings
	string ctDir = mConfig->Read<string>("ctDir", "");
	string iscName = mConfig->Read<string>("iscName", "");
	bool useCT = ctDir.size() > 0 ? true : false;
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend");
	uint32 numTotal = numInput + numRecommend;
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	uint32 maxTest = mConfig->Read<uint32>("maxTest", 0);
	string sOrder = mConfig->Read<string>("recomOrder", "support");
	IscOrderType recomOrder = NoneIscOrder;
	if(sOrder.compare("support") == 0)
		recomOrder = LengthDescIscOrder;
	else if(sOrder.compare("usage") == 0)
		recomOrder = UsageDescIscOrder;
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);

	if(numInput == 0)
		THROW("cprecom does not work with half-half split (yet).");

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n");	s=temp;	LOG_MSG(s);
	sprintf_s(temp, 500, "- Mode: %s", useCT?"code table":"itemset collection"); s=temp; LOG_MSG(s);
	if(useCT) {
		sprintf_s(temp, 500, "- CT dir: %s", ctDir.c_str()); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- ISC: %s", iscName.c_str()); s=temp; LOG_MSG(s);
	}
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Max # test instances: %u", maxTest); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Order: %s", sOrder.c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint16 alphSize = (uint16) testDB->GetAlphabetSize();
	Database *trainDB = NULL;

	// Temp CT subsetting test
	islist *delList = new islist();

	// Init vars
	ItemSet **ctElems = NULL;
	ItemSet **alphElems = NULL;
	ItemSet **mixedElems = NULL;
	uint32 numElems = 0;
	uint32 numMixed = 0;
	CodeTable *codeTable = NULL;
	string ctFilename;
	ItemSetCollection *isc = NULL;
	
	if(useCT) {										// -------------------- CT mode
		// Load CT
		uint32 numCTs = 0;
		{
			directory_iterator itr(ctDir, "*.ct");
			while(itr.next())
				numCTs++;
		}
		if(numCTs == 0)
			THROW("No code tables found...");
		if(numCTs > 1)
			THROW("cprecom can only handle a single CT.");

		{
			directory_iterator itr(ctDir, "*.ct");
			uint32 curCT = 0;
			while(itr.next()) {
				ctFilename = itr.filename();
				codeTable = CodeTable::LoadCodeTable(itr.fullpath(), testDB);
			}
		}
		LOG_MSG("CT loaded.");

		// Init structures
		printf_s("\nBuilding structures ... ");
		ctElems = codeTable->GetItemSetArray();
		numElems = codeTable->GetCurNumSets();
		alphElems = codeTable->GetSingletonArray();
		numMixed = numElems + alphSize;
		mixedElems = new ItemSet *[numMixed];
		memcpy_s(mixedElems, numMixed * sizeof(ItemSet *), ctElems, numElems*sizeof(ItemSet *));
		memcpy_s(mixedElems+numElems, alphSize * sizeof(ItemSet *), alphElems, alphSize*sizeof(ItemSet *));

		// Sort itemsets
		if(recomOrder == UsageDescIscOrder)	{	// usage(desc) length(desc)
			ItemSet::SortItemSetArray(ctElems, numElems, UsageDescIscOrder);
			ItemSet::SortItemSetArray(mixedElems, numMixed, UsageDescIscOrder);
		} else if(recomOrder == LengthDescIscOrder)	{ // support(desc) length(desc)
			ItemSet::SortItemSetArray(ctElems, numElems, LengthDescIscOrder);
			ItemSet::SortItemSetArray(mixedElems, numMixed, LengthDescIscOrder);
		}
		//else			// CT order
		// do nothing

		// Count individual item supports
		for(uint16 item=0; item<alphSize; item++) {
			{	// Check if item is the expected one
				uint16 *tmp = alphElems[item]->GetValues();
				if(tmp[0] != item)
					THROW("Alphabet order assumption violated.");
				delete[] tmp;
			}
			// Determine support
			uint32 support = 0;
			for(uint32 i=0; i<numMixed; i++)
				if(mixedElems[i]->IsItemInSet(item))
					support += mixedElems[i]->GetUsageCount(); // item support = sum of all usages
			alphElems[item]->SetSupport(support);
		}


		// Foreach ctElem, estimate its support by summing all usages of itself and its supersets
		for(uint32 e=0; e<numElems; e++) {
			uint32 support = 0;
			ItemSet *elem = ctElems[e];
			for(uint32 d=0; d<numElems; d++) {
				if(ctElems[d]->IsSubset(elem))
					support += ctElems[d]->GetUsageCount(); // support =~ sum of all usages of itself and its supersets
			}
			//ctElems[e]->SetSupport(support); WARNING: REAL SUPPORT IS USED
		}


		printf_s("done\n\n");

	} else {											// -------------- ISC mode
		string dbName = iscName.substr(0,iscName.find_first_of('-'));
		trainDB = Database::RetrieveDatabase(dbName);

		bool beenMined;
		isc = FicMain::ProvideItemSetCollection(iscName, trainDB, beenMined, false, true);

		if(isc->GetNumLoadedItemSets() != isc->GetNumItemSets())
			THROW("Too many itemsets, not all of them are loaded in memory; try a higher minsup/lower maxlen.");

		// Init structures
		printf_s("\nBuilding structures ... ");
		mixedElems = isc->GetLoadedItemSets();
		numMixed = isc->GetNumLoadedItemSets();

		// Alphabet
		alphElems = trainDB->GetAlphabetSets();

		// Non-singleton elements
		ctElems = new ItemSet *[numMixed]; // array is a bit too large, but that's ok
		numElems = 0;
		for(uint32 i=0; i<numMixed; i++)
			if(mixedElems[i]->GetLength() > 1) {
				ctElems[numElems++] = mixedElems[i];
			}

		// Sort itemsets
		//-- Assume isc is already in correct order! (Option d)

		printf_s("done\n\n");
	}

	// Create helper array to cache (at most) all non-empty subsets of input tagsets
	uint32 maxNumCachedSubsets = uint32(pow(2.0, (int)numInput) - numInput - 1); // singletons need not be cached
	ItemSet **cachedSubsets = new ItemSet *[maxNumCachedSubsets];
	for(uint32 i=0; i<maxNumCachedSubsets; i++)
		cachedSubsets[i] = ItemSet::CreateEmpty(ctElems[0]->GetType(), alphSize);
	uint32 numCachedSubsets = 0;

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	Timer *timer2 = new Timer();

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recommendation;correct;P@1;P@3;P@5;S@3;S@5;MRR\n"); 
	}

	// For each test row
	printf_s("Recommending");
#ifdef _DEBUG
	printf_s("\n");
#endif
	uint32 valuesLength = 2 * max(testDB->GetMaxSetLength(), numTotal);
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput==0 ? 2 : numInput+min(numRecommend,5); // half/half-->min 2, other-->numIn+atleast(5)
	uint32 numActualInput;
	uint32 numTests = maxTest>0 ? maxTest : testDB->GetNumRows();

	for(uint32 r=0; r<numTests; r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		numActualInput = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<numActualInput; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=numActualInput; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;

		// Select elems that intersect with input
		islist *candList = new islist();
		for(uint32 i=0; i<numElems; i++) {
			if(ctElems[i]->Intersects(inputTags)) {	// Look for *all* intersecting tagsets
				if(!inputTags->IsSubset(ctElems[i])) {	// that are not a subset of the input tagset
					candList->push_back(ctElems[i]);

					if(false && useCT) {			// Begin: consider subsets of CT elems as candidates
						ItemSet *recom = ctElems[i]->Clone();
						ItemSet *overlap = recom->Intersection(inputTags);
						recom->Remove(overlap);
						if(recom->GetLength() > 1) {
							uint16 *items = recom->GetValues();
							ItemSet *newCand;
							if(recom->GetLength() == 2) {
								newCand = ItemSet::CreateSingleton(recom->GetType(), items[0], alphSize); newCand->Unite(overlap);
								EstimateSupport(newCand, mixedElems, numMixed);
								candList->push_back(newCand); delList->push_back(newCand);
								newCand = ItemSet::CreateSingleton(recom->GetType(), items[1], alphSize); newCand->Unite(overlap);
								EstimateSupport(newCand, mixedElems, numMixed);
								candList->push_back(newCand); delList->push_back(newCand);
							} else if(recom->GetLength() == 3) {
								newCand = ItemSet::CreateSingleton(recom->GetType(), items[0], alphSize); newCand->Unite(overlap);
								EstimateSupport(newCand, mixedElems, numMixed);
								candList->push_back(newCand); delList->push_back(newCand);
								newCand = ItemSet::CreateSingleton(recom->GetType(), items[1], alphSize); newCand->Unite(overlap);
								EstimateSupport(newCand, mixedElems, numMixed);
								candList->push_back(newCand); delList->push_back(newCand);
								newCand = ItemSet::CreateSingleton(recom->GetType(), items[2], alphSize); newCand->Unite(overlap);
								EstimateSupport(newCand, mixedElems, numMixed);
								candList->push_back(newCand); delList->push_back(newCand);
								uint16 *set = new uint16[2]; set[0] = items[0]; set[1] = items[1];
								newCand = ItemSet::Create(recom->GetType(), set, 2, alphSize); newCand->Unite(overlap);
								EstimateSupport(newCand, mixedElems, numMixed);
								candList->push_back(newCand); delList->push_back(newCand);
								set = new uint16[2]; set[0] = items[0]; set[1] = items[2];
								newCand = ItemSet::Create(recom->GetType(), set, 2, alphSize); newCand->Unite(overlap);
								EstimateSupport(newCand, mixedElems, numMixed);
								candList->push_back(newCand); delList->push_back(newCand);
								set = new uint16[2]; set[0] = items[1]; set[1] = items[2];
								newCand = ItemSet::Create(recom->GetType(), set, 2, alphSize); newCand->Unite(overlap);
								EstimateSupport(newCand, mixedElems, numMixed);
								candList->push_back(newCand); delList->push_back(newCand);
							} else {
								THROW("Cannot yet handle itemsets this long.");
							}
							delete[] items;
						}
						delete overlap;
						delete recom;
					}			// END: Consider subsets of CT elems as candidates
				}
			}
		}

		// Sort the candidate elements according to conditional probability
		ItemSet **cands = new ItemSet *[candList->size()];
		float *probs = new float[candList->size()];
		uint32 numCands = 0;
		numCachedSubsets = 0;
	
		for(islist::iterator iter = candList->begin(); iter!=candList->end(); iter++) {
			cands[numCands] = *iter;
			float prob = (float)((*iter)->GetSupport()); // support; either estimated (CTmode) or precise (ISCmode)

			ItemSet *overlapIS = (*iter)->Intersection(inputTags);

			if(overlapIS->GetLength() > 1) {		// |overlap| > 1
				// First check whether already cached
				bool found = false;
				for(uint32 i=0; i<numCachedSubsets; i++) {
					if(overlapIS->Equals(cachedSubsets[i])) {
						prob /= cachedSubsets[i]->GetSupport(); // support stored in cachedSubset
						found = true;
						break;
					}
				}

				// If not yet cached, proceed
				if(!found) {
					// We need overlapIS's support
					if(useCT) {		// Approximate from CT
						uint32 support = 0;
						for(uint32 n=0; n<numElems; n++) {
							if(ctElems[n]->Equals(overlapIS)) {
								support = ctElems[n]->GetSupport();
								break;
							}
							if(ctElems[n]->IsSubset(overlapIS))
								support += ctElems[n]->GetUsageCount(); // support =~ sum of all usages
						}
						/*ItemSet **rows = trainDB->GetRows(); uint32 numRows = trainDB->GetNumRows();
						for(uint32 n=0; n<numRows; n++)
							if(rows[n]->IsSubset(overlapIS))
								support++;*/
						cachedSubsets[numCachedSubsets]->CleanTheSlate();
						cachedSubsets[numCachedSubsets]->Unite(overlapIS);
						cachedSubsets[numCachedSubsets++]->SetSupport(support);
						prob /= support;

					} else {		// Get from ISC and cache
						for(uint32 n=0; n<isc->GetNumLoadedItemSets(); n++)
							if(isc->GetLoadedItemSet(n)->Equals(overlapIS)) {
								ItemSet *is = isc->GetLoadedItemSet(n);
								cachedSubsets[numCachedSubsets]->CleanTheSlate();
								cachedSubsets[numCachedSubsets]->Unite(is);
								cachedSubsets[numCachedSubsets++]->SetSupport(is->GetSupport());
								prob /= is->GetSupport();
								break;
							}
					}
				}
			} else {								// |overlap| == 1, get support from alphabet
				overlapIS->GetValuesIn(values);
				prob /= alphElems[values[0]]->GetSupport();
			}
			delete overlapIS;
				
			probs[numCands++] = prob;
		}
		ArrayUtils::BSortDesc<float,ItemSet*>(probs,numCands,cands);

		// Generate the actual recommendation
		uint32 curLen = 0;
		ItemSet *recomSet = inputTags->Clone();

		for(uint32 i=0; i<numCands && curLen<numRecommend; i++) {
			ItemSet *is = cands[i]->Clone();
			is->Remove(recomSet);
			if(is->GetLength() == 0) {
				delete is;
				continue;
			}
			is->GetValuesIn(values+curLen);
			curLen += is->GetLength();
			recomSet->Unite(is);
			delete is;
		}
		delete[] cands;
		delete[] probs;
		delete candList;

		// While curLen < numRecommend, greedily add overlapping itemsets in standard order
		for(uint32 idx = 0; curLen < numRecommend && idx<numElems; idx++) {
			if(recomSet->Intersects(ctElems[idx]) && !recomSet->IsSubset(ctElems[idx])) {
				ItemSet *is = ctElems[idx]->Clone();
				is->Remove(recomSet);
				is->GetValuesIn(values + curLen);
				curLen += is->GetLength();
				recomSet->Unite(is);
				delete is;
				//idx = 0; // restart from first! iterative overlapsearch. may give minor improvement, but is obviously slower.
			}
		}

		// While curLen < numRecommend, greedily add itemsets in standard order
		for(uint32 idx = 0; curLen < numRecommend && idx<numElems; idx++) {
			if(!recomSet->IsSubset(ctElems[idx])) {
				ItemSet *is = ctElems[idx]->Clone();
				is->Remove(recomSet);
				is->GetValuesIn(values + curLen);
				curLen += is->GetLength();
				recomSet->Unite(is);
				delete is;
			}
		}

		// Determine correctly recommended tags
		recomSet->Remove(inputTags);
		ItemSet *correctTags = recomSet->Intersection(testTags);
		
		// Compute stats
		{
			uint32 firstCorrect = UINT32_MAX_VALUE;
			uint32 p1=0, p3=0, p5=0;
			if(correctTags->GetLength() > 0) {
				for(uint32 i=0; i<curLen; i++) {
					if(correctTags->IsItemInSet(values[i])) {
						if(firstCorrect == UINT32_MAX_VALUE)
							firstCorrect = i;
						if(i<5) {
							precision5 += 0.2;
							p5++;
							if(i<3) {
								precision3 += 0.3333333333333333;
								p3++;
								if(i==0) {
									precision1 += 1.0;
									p1++;
								}
							}
						}
					}
				}
			}
			if(firstCorrect < UINT32_MAX_VALUE) {
				mrr += 1.0 / (firstCorrect+1);
				if(firstCorrect < 5) {
					success5 += 1.0;
					if(firstCorrect < 3)
						success3 += 1.0;
				}
			}

			// Save to file, if asked for
			if(saveRecommendations) {
				fprintf_s(recomFP, "%s;%s;",
					inputTags->ToString(false).c_str(), testTags->ToString(false).c_str());
				
				for(uint32 i=0; i<curLen; i++)
					fprintf_s(recomFP, "%u ", values[i]);

				fprintf_s(recomFP, ";%s;%u;%u;%u;%u;%u;%f\n", 
					correctTags->ToString(false).c_str(),
					p1, p3, p5,
					firstCorrect<3?1:0,	// S@3
					firstCorrect<5?1:0,	// S@5
					firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0 // MRR
				);
			}
		}

		// Cleanup
		delete correctTags;
		delete recomSet;
		delete inputTags;
		delete testTags;

		for(islist::iterator iter=delList->begin(); iter!=delList->end(); iter++)
			delete *iter;
		delList->clear();

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	if(numRecommend < 5) {
		precision5 = 0.0; success5 = 0.0;
		if(numRecommend < 3)
			precision3 = 0.0; success3 = 0.0;
	}
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	if(useCT) {
		fprintf_s(fp, "ctDir;%s\n", ctDir.c_str());
		fprintf_s(fp, "ctName;%s\n", ctFilename.c_str());
	} else {
		fprintf_s(fp, "iscName;%s\n", iscName.c_str());
	}
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
	fprintf_s(fp, "source;numInput;numRecommend;#sets;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_all (s);time_recom (s)\n");
	fprintf_s(fp, "%s;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%I64u;%I64u\n", useCT?ctFilename.c_str():iscName.c_str(), numInput, numRecommend, numElems, numRowsTested, testDB->GetNumRows()-numRowsTested, 
								precision1, precision3, precision5, success3, success5, mrr, timer1->GetElapsedTime(), timer2->GetElapsedTime());
	fclose(fp);

	// Cleanup
	delete delList;
	delete timer1;
	delete timer2;
	delete[] values;
	for(uint32 i=0; i<maxNumCachedSubsets; i++)
		delete cachedSubsets[i];
	delete[] cachedSubsets;

	for(uint32 j=0; j<alphSize; j++)
		delete alphElems[j];
	delete[] alphElems;
	delete[] ctElems;

	if(useCT) {
		delete[] mixedElems;
		delete codeTable;
	} else {
		delete isc;
		delete trainDB;
	}
	delete testDB;
}
void TagRecomTH::EstimateSupport(ItemSet *is, ItemSet **mixedElems, const uint32 numMixed) {
	uint32 support = 0;
	for(uint32 i=0; i<numMixed; i++)
		if(mixedElems[i]->IsSubset(is))
			support += mixedElems[i]->GetUsageCount(); // item support = sum of all usages
	is->SetSupport(support);
}
void TagRecomTH::CondProbRecommendation2() {
	char temp[500]; string s;
	Timer *timer1 = new Timer();

	// Settings
	string ctDir = mConfig->Read<string>("ctDir", "");
	string iscName = mConfig->Read<string>("iscName", "");
	bool useCT = ctDir.size() > 0 ? true : false;
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend");
	uint32 numTotal = numInput + numRecommend;
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	uint32 maxTest = mConfig->Read<uint32>("maxTest", 0);
	string sOrder = mConfig->Read<string>("recomOrder", "support");
	IscOrderType recomOrder = NoneIscOrder;
	if(sOrder.compare("support") == 0)
		recomOrder = LengthDescIscOrder;
	else if(sOrder.compare("usage") == 0)
		recomOrder = UsageDescIscOrder;
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);

	if(numInput == 0)
		THROW("cprecom does not work with half-half split (yet).");

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n");	s=temp;	LOG_MSG(s);
	sprintf_s(temp, 500, "- Mode: %s", useCT?"code table":"itemset collection"); s=temp; LOG_MSG(s);
	if(useCT) {
		sprintf_s(temp, 500, "- CT dir: %s", ctDir.c_str()); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- ISC: %s", iscName.c_str()); s=temp; LOG_MSG(s);
	}
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Max # test instances: %u", maxTest); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Order: %s", sOrder.c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint16 alphSize = (uint16) testDB->GetAlphabetSize();
	Database *trainDB = NULL;

	// Temp CT subsetting test
	islist *delList = new islist();

	// Init vars
	ItemSet ***ctElems = new ItemSet **[numInput]; // one CT for each level, levels [2,numInput+1]
	uint32 *numElems = new uint32[numInput];
	ItemSet **allElems = NULL;
	uint32 numAllElems = 0;
	ItemSet **alphElems = NULL;
	CodeTable **codeTables = new CodeTable *[numInput];
	
	if(true) {										// -------------------- CT mode
		// Load CT
		uint32 numCTsAvailable = 0;
		{
			directory_iterator itr(ctDir, "*.ct");
			while(itr.next())
				numCTsAvailable++;
		}
		if(numCTsAvailable == 0)
			THROW("No code tables found...");
		if(numCTsAvailable < numInput)
			THROW("Not enough code tables found.");

		{
			directory_iterator itr(ctDir, "*.ct");
			uint32 curCT = 0;
			string ctFilename;
			uint32 numLoaded = 0;
			while(itr.next()) {
				ctFilename = itr.filename();
				size_t pos = ctFilename.find("maxlen[");
				if(pos==string::npos)
					THROW("Non-codetable or codetable without maxlen found.");
				string s = ctFilename.substr(pos+7, 1);
				int maxlen = atoi(s.c_str());
				if(maxlen < 2 || maxlen > (int)(numInput+1))
					continue;
				codeTables[maxlen-2] = CodeTable::LoadCodeTable(itr.fullpath(), testDB);
				numLoaded++;
			}
			if(numLoaded < numInput)
				THROW("Did not find suitable code tables.");
		}
		LOG_MSG("CTs loaded.");

		// Init structures
		printf_s("\nBuilding structures ... ");
		alphElems = codeTables[0]->GetSingletonArray();
		for(uint32 i=0; i<numInput; i++) {
			ItemSet **iss = codeTables[i]->GetItemSetArray();
			uint32 num = codeTables[i]->GetCurNumSets();

			numElems[i] = 0;
			for(uint32 n=0; n<num; n++) {
				if(iss[n]->GetLength() == i+2)
					numElems[i]++;
			}

			ctElems[i] = new ItemSet *[numElems[i]];
			uint32 idx = 0;
			for(uint32 n=0; n<num; n++) {
				if(iss[n]->GetLength() == i+2)
					ctElems[i][idx++] = iss[n];
			}

			numAllElems += numElems[i];
			delete[] iss;
		}
		allElems = new ItemSet *[numAllElems];
		uint32 num = 0;
		for(uint32 i=0; i<numInput; i++) {
			memcpy_s(allElems + num, (numAllElems-num)*sizeof(ItemSet *), ctElems[i], numElems[i]*sizeof(ItemSet *));
			num += numElems[i];
		}

		// Sort itemsets
		if(recomOrder == UsageDescIscOrder)	{	// usage(desc) length(desc)
			ItemSet::SortItemSetArray(allElems, numAllElems, UsageDescIscOrder);
		} else if(recomOrder == LengthDescIscOrder)	{ // support(desc) length(desc)
			ItemSet::SortItemSetArray(allElems, numAllElems, LengthDescIscOrder);
		}
		//else			// CT order
		// do nothing

		// Count individual item supports
		{
			ItemSet **iss = codeTables[0]->GetItemSetArray();
			uint32 numIss = codeTables[0]->GetCurNumSets();
			for(uint16 item=0; item<alphSize; item++) {
				{	// Check if item is the expected one
					uint16 *tmp = alphElems[item]->GetValues();
					if(tmp[0] != item)
						THROW("Alphabet order assumption violated.");
					delete[] tmp;
				}
				// Determine support
				uint32 support = 0;
				for(uint32 i=0; i<numIss; i++)
					if(iss[i]->IsItemInSet(item))
						support += iss[i]->GetUsageCount(); // item support = sum of all usages
				support += alphElems[item]->GetUsageCount();
				alphElems[item]->SetSupport(support);
			}
			delete[] iss;
		}

		printf_s("done\n\n");
	}

	// Create helper array to cache (at most) all non-empty subsets of input tagsets
	uint32 maxNumCachedSubsets = uint32(pow(2.0, (int)numInput) - numInput - 1); // singletons need not be cached
	ItemSet **cachedSubsets = new ItemSet *[maxNumCachedSubsets];
	for(uint32 i=0; i<maxNumCachedSubsets; i++)
		cachedSubsets[i] = ItemSet::CreateEmpty(ctElems[0][0]->GetType(), alphSize);
	uint32 numCachedSubsets = 0;

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	Timer *timer2 = new Timer();

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recommendation;correct;P@1;P@3;P@5;S@3;S@5;MRR\n"); 
	}

	// For each test row
	printf_s("Recommending");
#ifdef _DEBUG
	printf_s("\n");
#endif
	uint32 valuesLength = 2 * max(testDB->GetMaxSetLength(), numTotal);
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput==0 ? 2 : numInput+min(numRecommend,5); // half/half-->min 2, other-->numIn+atleast(5)
	uint32 numActualInput;
	uint32 numTests = maxTest>0 ? maxTest : testDB->GetNumRows();

	for(uint32 r=0; r<numTests; r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		numActualInput = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<numActualInput; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=numActualInput; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;
		
#if 0
		// Init recommendation
		uint32 curLen = 0;
		ItemSet *recomSet = inputTags->Clone();

		// Go from longest to shortest itemsets
		//for(int32 i=numInput-1; i>=0 && curLen < numRecommend; i--) {
		for(int32 i=0; i>=0 && curLen < numRecommend; i--) {
			islist *candSetList = new islist();
			ItemSet *candItemSet = ItemSet::CreateEmpty(inputTags->GetType(), alphSize);

			// Select all candidate elements for this level
			for(uint32 n=0; n<numElems[i]; n++) {
				if(ctElems[i][n]->Intersects(inputTags)) {	// Look for all intersecting tagsets
					ItemSet *intersection = ctElems[i][n]->Intersection(inputTags);
					if(intersection->GetLength() == i+1) {	// for which the intersection is equal to len-1 == i+1
						candSetList->push_back(ctElems[i][n]);
						candItemSet->Unite(ctElems[i][n]);
					}
					delete intersection;
				}
			}

			// Compute conditional support of each candElem
			uint32 numCandSets = candSetList->size();
			ItemSet **candSets = new ItemSet *[numCandSets];
			float *candSetProbs = new float[numCandSets];
			uint32 idx = 0;
			numCachedSubsets = 0;
			for(islist::iterator iter = candSetList->begin(); iter!=candSetList->end(); iter++) {
				candSets[idx] = *iter;
				float prob = (float)((*iter)->GetSupport()); // support; either estimated (CTmode) or precise (ISCmode)

				ItemSet *overlapIS = (*iter)->Intersection(inputTags);

				if(overlapIS->GetLength() > 1) {		// |overlap| > 1
					// First check whether already cached
					bool found = false;
					for(uint32 n=0; n<numCachedSubsets; n++) {
						if(overlapIS->Equals(cachedSubsets[n])) {
							prob /= cachedSubsets[n]->GetSupport(); // support stored in cachedSubset
							found = true;
							break;
						}
					}

					// If not yet cached, proceed
					if(!found) {
						uint32 overlapLen = overlapIS->GetLength();

						// We need overlapIS's support
						uint32 support = 0;
						uint32 idx = overlapLen-2;
						for(uint32 n=0; n<numElems[idx]; n++) {
							if(ctElems[idx][n]->Equals(overlapIS)) {
								support = ctElems[idx][n]->GetSupport();
								found = true;
								break;
							}
						}
						if(!found) {
							for(uint32 n=0; n<numAllElems; n++)
								if(allElems[n]->IsSubset(overlapIS))
									support += allElems[n]->GetUsageCount(); // support =~ sum of all usages
						}
						cachedSubsets[numCachedSubsets]->CleanTheSlate();
						cachedSubsets[numCachedSubsets]->Unite(overlapIS);
						cachedSubsets[numCachedSubsets++]->SetSupport(support);
						prob /= support;
					}
				} else {								// |overlap| == 1, get support from alphabet
					overlapIS->GetValuesIn(values);
					prob /= alphElems[values[0]]->GetSupport();
				}
				delete overlapIS;
				
				candSetProbs[idx++] = prob;
			}
			
			if(i < (int)(numInput-1)) {		// Combine conditional probabilities per item if not on top level
				candItemSet->Remove(inputTags);
				uint32 numCands = candItemSet->GetLength();
				uint16 *candItems = candItemSet->GetValues();
				float *itemProbs = new float[numCands];

				for(uint32 c=0; c<numCands; c++) {				// THIS PART PROBABLY DOESN'T GENERALIZE TO NUMINPUT>2 YET
					itemProbs[c] = 0.0;
					for(uint32 n=0; n<numCandSets; n++) {
						if(candSets[n]->IsItemInSet(candItems[c]) && candSets[n]->Intersects(inputTags)) {
							itemProbs[c] += candSetProbs[n];
						}
					}
				}

				// Sort items
				ArrayUtils::BSortDesc<float,uint16>(itemProbs,numCands,candItems);

				// Augment the actual recommendation
				for(uint32 c=0; c<numCands && curLen<numRecommend; c++) {
					values[curLen++] = candItems[c];
					recomSet->AddItemToSet(candItems[c]);
				}

				delete[] candItems;
				delete[] itemProbs;

			} else {					// Use direct ranking (which is conceptually the same for this case, but faster)
				ArrayUtils::BSortDesc<float,ItemSet *>(candSetProbs,numCandSets,candSets);

				// Augment the actual recommendation
				for(uint32 c=0; c<numCandSets && curLen<numRecommend; c++) {
					ItemSet *is = candSets[c]->Clone();
					is->Remove(recomSet); // includes inputTags
					is->GetValuesIn(values + curLen);
					curLen += is->GetLength();
					recomSet->Unite(is);
					delete is;
				}
			}

			delete candItemSet;
			delete candSetList;
			delete[] candSets;
			delete[] candSetProbs;
		}
#endif

		// Select elems that intersect with input
		islist *candList = new islist();
		for(uint32 i=0; i<numAllElems; i++) {
			if(allElems[i]->Intersects(inputTags)) {	// Look for *all* intersecting tagsets
				if(!inputTags->IsSubset(allElems[i])) {	// that are not a subset of the input tagset
					candList->push_back(allElems[i]);
				}
			}
		}

		// Sort the candidate elements according to conditional probability
		ItemSet **cands = new ItemSet *[candList->size()];
		float *probs = new float[candList->size()];
		uint32 numCands = 0;
		numCachedSubsets = 0;
	
		for(islist::iterator iter = candList->begin(); iter!=candList->end(); iter++) {
			cands[numCands] = *iter;
			float prob = (float)((*iter)->GetSupport()); // support; either estimated (CTmode) or precise (ISCmode)

			ItemSet *overlapIS = (*iter)->Intersection(inputTags);

			if(overlapIS->GetLength() > 1) {		// |overlap| > 1
				// First check whether already cached
				bool found = false;
				for(uint32 i=0; i<numCachedSubsets; i++) {
					if(overlapIS->Equals(cachedSubsets[i])) {
						prob /= cachedSubsets[i]->GetSupport(); // support stored in cachedSubset
						found = true;
						break;
					}
				}

				// If not yet cached, proceed
				if(!found) {
					// We need overlapIS's support
					if(useCT) {		// Approximate from CT
						uint32 support = 0;
						for(uint32 n=0; n<numAllElems; n++) {
							if(allElems[n]->Equals(overlapIS)) {
								support = allElems[n]->GetSupport();
								break;
							}
							if(allElems[n]->IsSubset(overlapIS))
								support += allElems[n]->GetUsageCount(); // support =~ sum of all usages
						}
						cachedSubsets[numCachedSubsets]->CleanTheSlate();
						cachedSubsets[numCachedSubsets]->Unite(overlapIS);
						cachedSubsets[numCachedSubsets++]->SetSupport(support);
						prob /= support;

					} 
				}
			} else {								// |overlap| == 1, get support from alphabet
				overlapIS->GetValuesIn(values);
				prob /= alphElems[values[0]]->GetSupport();
			}
			delete overlapIS;
				
			probs[numCands++] = prob;
		}
		ArrayUtils::BSortDesc<float,ItemSet*>(probs,numCands,cands);

		// Generate the actual recommendation
		uint32 curLen = 0;
		ItemSet *recomSet = inputTags->Clone();

		for(uint32 i=0; i<numCands && curLen<numRecommend; i++) {
			ItemSet *is = cands[i]->Clone();
			is->Remove(recomSet);
			if(is->GetLength() == 0) {
				delete is;
				continue;
			}
			is->GetValuesIn(values+curLen);
			curLen += is->GetLength();
			recomSet->Unite(is);
			delete is;
		}
		delete[] cands;
		delete[] probs;
		delete candList;

		// CUT HERE --

		// While curLen < numRecommend, greedily add overlapping itemsets in standard order
		for(uint32 idx = 0; curLen < numRecommend && idx<numAllElems; idx++) {
			if(recomSet->Intersects(allElems[idx]) && !recomSet->IsSubset(allElems[idx])) {
				ItemSet *is = allElems[idx]->Clone();
				is->Remove(recomSet);
				is->GetValuesIn(values + curLen);
				curLen += is->GetLength();
				recomSet->Unite(is);
				delete is;
			}
		}

		// While curLen < numRecommend, greedily add itemsets in standard order
		for(uint32 idx = 0; curLen < numRecommend && idx<numAllElems; idx++) {
			if(!recomSet->IsSubset(allElems[idx])) {
				ItemSet *is = allElems[idx]->Clone();
				is->Remove(recomSet);
				is->GetValuesIn(values + curLen);
				curLen += is->GetLength();
				recomSet->Unite(is);
				delete is;
			}
		}

		// Determine correctly recommended tags
		recomSet->Remove(inputTags);
		ItemSet *correctTags = recomSet->Intersection(testTags);
		
		// Compute stats
		{
			uint32 firstCorrect = UINT32_MAX_VALUE;
			uint32 p1=0, p3=0, p5=0;
			if(correctTags->GetLength() > 0) {
				for(uint32 i=0; i<curLen; i++) {
					if(correctTags->IsItemInSet(values[i])) {
						if(firstCorrect == UINT32_MAX_VALUE)
							firstCorrect = i;
						if(i<5) {
							precision5 += 0.2;
							p5++;
							if(i<3) {
								precision3 += 0.3333333333333333;
								p3++;
								if(i==0) {
									precision1 += 1.0;
									p1++;
								}
							}
						}
					}
				}
			}
			if(firstCorrect < UINT32_MAX_VALUE) {
				mrr += 1.0 / (firstCorrect+1);
				if(firstCorrect < 5) {
					success5 += 1.0;
					if(firstCorrect < 3)
						success3 += 1.0;
				}
			}

			// Save to file, if asked for
			if(saveRecommendations) {
				fprintf_s(recomFP, "%s;%s;",
					inputTags->ToString(false).c_str(), testTags->ToString(false).c_str());
				
				for(uint32 i=0; i<curLen; i++)
					fprintf_s(recomFP, "%u ", values[i]);

				fprintf_s(recomFP, ";%s;%u;%u;%u;%u;%u;%f\n", 
					correctTags->ToString(false).c_str(),
					p1, p3, p5,
					firstCorrect<3?1:0,	// S@3
					firstCorrect<5?1:0,	// S@5
					firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0 // MRR
				);
			}
		}

		// Cleanup
		delete correctTags;
		delete recomSet;
		delete inputTags;
		delete testTags;

		for(islist::iterator iter=delList->begin(); iter!=delList->end(); iter++)
			delete *iter;
		delList->clear();

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	if(numRecommend < 5) {
		precision5 = 0.0; success5 = 0.0;
		if(numRecommend < 3)
			precision3 = 0.0; success3 = 0.0;
	}
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	fprintf_s(fp, "ctDir;%s\n", ctDir.c_str());
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
/*		-- Disable time_all
	fprintf_s(fp, "source;numInput;numRecommend;#sets;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_all (s);time_recom (s)\n");
	fprintf_s(fp, "%s;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%I64u;%I64u\n", ctDir.c_str(), numInput, numRecommend, numAllElems, numRowsTested, testDB->GetNumRows()-numRowsTested, 
								precision1, precision3, precision5, success3, success5, mrr, timer1->GetElapsedTime(), timer2->GetElapsedTime());
*/
	fprintf_s(fp, "source;numInput;numRecommend;#sets;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_recom (s)\n");
	fprintf_s(fp, "%s;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%I64u\n", ctDir.c_str(), numInput, numRecommend, numAllElems, numRowsTested, testDB->GetNumRows()-numRowsTested, 
		precision1, precision3, precision5, success3, success5, mrr, timer2->GetElapsedTime());
	fclose(fp);

	// Cleanup
	delete delList;
	delete timer1;
	delete timer2;
	delete[] values;
	for(uint32 i=0; i<maxNumCachedSubsets; i++)
		delete cachedSubsets[i];
	delete[] cachedSubsets;

	for(uint32 j=0; j<alphSize; j++)
		delete alphElems[j];
	delete[] alphElems;
	delete[] allElems;
	for(uint32 i=0; i<numInput; i++) {
		delete[] ctElems[i];
		delete[] codeTables[i];
	}
	delete[] numElems;
	delete[] ctElems;
	delete[] codeTables;
	delete testDB;
}

/*	**** Some not working modified recommendation orders *****

			if(isl->size() > 0) {
				// re-sort isl
				size_t oldSize = isl->size();
				uint16 *vals = inputTags->GetValues(); // sorting according alphabet num is an approximation! actual rankings of counts in training datasets vary from full dataset
				islist *newisl = new islist();
				for(int32 i=numActualInput-1; i>=0; i--)
					for(islist::iterator iter = isl->begin(); iter!=isl->end(); )
						if((*iter)->IsItemInSet(vals[i])) {
							newisl->push_back(*iter);
							iter = isl->erase(iter);
						} else
							iter++;
				if(oldSize != newisl->size())
					THROW("Impossibru.");
				delete[] vals;
				delete isl;
				isl = newisl;
				firstRankElems[n] = isl;
			}
*/
/*
			if(n == 0) {
				// re-sort isl for which numOverlap=1
				uint16 *vals = inputTags->GetValues(); // sorting according alphabet num is an approximation! actual rankings of counts in training datasets vary from full dataset
				islist *newisl = new islist();
				for(int32 i=numActualInput-1; i>=0; i--)
					for(islist::iterator iter = isl->begin(); iter!=isl->end(); iter++)
						if((*iter)->IsItemInSet(vals[i]))
							newisl->push_back((*iter));
				if(isl->size() != newisl->size())
					THROW("Impossibru.");
				delete[] vals;
				delete isl;
				isl = newisl;
				firstRankElems[n] = isl;
			}
*/
/*
			if(n == 0) {
				// re-sort isl for which numOverlap=1, do some sort of summed conditional probability
				uint16 *vals = inputTags->GetValues(); // sorting according alphabet num is an approximation! actual rankings of counts in training datasets vary from full dataset
				alphabet *alph = testDB->GetAlphabet(); // yet another approximation!

				uint16 *recoms = new uint16[selUnion->GetLength()];
				float *probs = new float[selUnion->GetLength()];
				uint32 idx = 0;
				for(int32 i=numActualInput-1; i>=0; i--) {
					float baseFreq = (float)(alph->find(vals[i])->second);	
					for(islist::iterator iter = isl->begin(); iter!=isl->end(); ) {
						if((*iter)->IsItemInSet(vals[i])) {
							uint16 *tmp = (*iter)->GetValues();
							for(uint32 j=0; j<(*iter)->GetLength(); j++) {
								if(tmp[j] == vals[i])
									continue; // skip overlap item
								bool found = false;
								for(uint32 k=0; k<idx; k++) {
									if(recoms[k] == tmp[j]) {
										probs[k] += (*iter)->GetSupport() / baseFreq;//support not available when using CT? then usage?
										found = true;
										break;
									}
								} 
								if(!found) {
									recoms[idx] = tmp[j];
									probs[idx++] = (*iter)->GetSupport() / baseFreq;//support not available when using CT? then usage?
								}
							}
							delete[] tmp;
							iter = isl->erase(iter);
						} else
							iter++;
					}
				}
				ArrayUtils::BSortDesc<float,uint16>(probs,idx,recoms);

				for(uint32 i=0; i<idx; i++) {
					if(soFar->IsItemInSet(recoms[i]))
						continue;
					soFar->AddItemToSet(recoms[i]);
					values[curLen++] = recoms[i];
				}

				delete recoms;
				delete probs;
				delete[] vals;
			} else {
				for(islist::iterator iter = isl->begin(); iter!=isl->end(); iter++) {
					ItemSet *is = (*iter)->Clone();
					is->Remove(soFar);
					is->GetValuesIn(values+curLen);
					curLen += is->GetLength();
					soFar->Unite(is);
					delete is;
				}
			}
		}
*/

void TagRecomTH::CondProbRecommendation3() {
	char temp[500]; string s;
	Timer *timer1 = new Timer();

	// Settings
	string ctDir = mConfig->Read<string>("ctDir", "");
	string iscName = mConfig->Read<string>("iscName", "");
	bool useCT = ctDir.size() > 0 ? true : false;
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend");
	uint32 numTotal = numInput + numRecommend;
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	uint32 maxTest = mConfig->Read<uint32>("maxTest", 0);
	string sOrder = mConfig->Read<string>("recomOrder", "support");
	IscOrderType recomOrder = NoneIscOrder;
	if(sOrder.compare("support") == 0)
		recomOrder = LengthDescIscOrder;
	else if(sOrder.compare("usage") == 0)
		recomOrder = UsageDescIscOrder;
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);

	bool useOracle = mConfig->Read<string>("cpSupport", "ct").compare("oracle")==0;
	bool useProbSum = mConfig->Read<string>("cpCombine", "max").compare("sum")==0;

	if(numInput == 0)
		THROW("cprecom3 does not work with half-half split (yet).");
	if(!useCT)
		THROW("cprecom3 only has CT-mode.")

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n");	s=temp;	LOG_MSG(s);
	sprintf_s(temp, 500, "- Mode: %s", useCT?"code table":"itemset collection"); s=temp; LOG_MSG(s);
	if(useCT) {
		sprintf_s(temp, 500, "- CT dir: %s", ctDir.c_str()); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- ISC: %s", iscName.c_str()); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- Support mode: %s", useOracle?"oracle":"code table estimation"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Combine probabilities: %s", useProbSum?"sum":"max"); s=temp; LOG_MSG(s);
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Max # test instances: %u", maxTest); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Order: %s", sOrder.c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint16 alphSize = (uint16) testDB->GetAlphabetSize();
	Database *trainDB = NULL;
	if(useOracle) 
		trainDB = Database::RetrieveDatabase(mConfig->Read<string>("traindb"));

	// Init vars
	ItemSet **ctElems = NULL;
	ItemSet **alphElems = NULL;
	ItemSet **mixedElems = NULL;
	uint32 numElems = 0;
	uint32 numMixed = 0;
	CodeTable *codeTable = NULL;
	string ctFilename;
	ItemSetCollection *isc = NULL;
	
	if(useCT) {										// -------------------- CT mode
		// Load CT
		uint32 numCTs = 0;
		{
			directory_iterator itr(ctDir, "*.ct");
			while(itr.next())
				numCTs++;
		}
		if(numCTs == 0)
			THROW("No code tables found...");
		if(numCTs > 1)
			THROW("cprecom3 can only handle a single CT.");

		{
			directory_iterator itr(ctDir, "*.ct");
			uint32 curCT = 0;
			while(itr.next()) {
				ctFilename = itr.filename();
				codeTable = CodeTable::LoadCodeTable(itr.fullpath(), testDB);
			}
		}
		LOG_MSG("CT loaded.");

		// Init structures
		printf_s("\nBuilding structures ... ");
		ctElems = codeTable->GetItemSetArray();
		numElems = codeTable->GetCurNumSets();
		alphElems = codeTable->GetSingletonArray();
		numMixed = numElems + alphSize;
		mixedElems = new ItemSet *[numMixed];
		memcpy_s(mixedElems, numMixed * sizeof(ItemSet *), ctElems, numElems*sizeof(ItemSet *));
		memcpy_s(mixedElems+numElems, alphSize * sizeof(ItemSet *), alphElems, alphSize*sizeof(ItemSet *));

		// Sort itemsets
		if(recomOrder == UsageDescIscOrder)	{	// usage(desc) length(desc)
			ItemSet::SortItemSetArray(ctElems, numElems, UsageDescIscOrder);
			ItemSet::SortItemSetArray(mixedElems, numMixed, UsageDescIscOrder);
		} else if(recomOrder == LengthDescIscOrder)	{ // support(desc) length(desc)
			ItemSet::SortItemSetArray(ctElems, numElems, LengthDescIscOrder);
			ItemSet::SortItemSetArray(mixedElems, numMixed, LengthDescIscOrder);
		}
		//else			// CT order
		// do nothing

		// Count individual item supports
		for(uint16 item=0; item<alphSize; item++) {
			{	// Check if item is the expected one
				uint16 *tmp = alphElems[item]->GetValues();
				if(tmp[0] != item)
					THROW("Alphabet order assumption violated.");
				delete[] tmp;
			}
			// Determine support
			uint32 support = 0;
			for(uint32 i=0; i<numMixed; i++)
				if(mixedElems[i]->IsItemInSet(item))
					support += mixedElems[i]->GetUsageCount(); // item support = sum of all usages
			alphElems[item]->SetSupport(support);
		}

		// Foreach ctElem, estimate its support by summing all usages of itself and its supersets
		/*for(uint32 e=0; e<numElems; e++) {
			uint32 support = 0;
			ItemSet *elem = ctElems[e];
			for(uint32 d=0; d<numElems; d++) {
				if(ctElems[d]->IsSubset(elem))
					support += ctElems[d]->GetUsageCount(); // support =~ sum of all usages of itself and its supersets
			}
			ctElems[e]->SetSupport(support); WARNING: REAL SUPPORT IS USED
		}*/

		printf_s("done\n\n");

	} else {											// -------------- ISC mode
		string dbName = iscName.substr(0,iscName.find_first_of('-'));
		trainDB = Database::RetrieveDatabase(dbName);

		bool beenMined;
		isc = FicMain::ProvideItemSetCollection(iscName, trainDB, beenMined, false, true);

		if(isc->GetNumLoadedItemSets() != isc->GetNumItemSets())
			THROW("Too many itemsets, not all of them are loaded in memory; try a higher minsup/lower maxlen.");

		// Init structures
		printf_s("\nBuilding structures ... ");
		mixedElems = isc->GetLoadedItemSets();
		numMixed = isc->GetNumLoadedItemSets();

		// Alphabet
		alphElems = trainDB->GetAlphabetSets();

		// Non-singleton elements
		ctElems = new ItemSet *[numMixed]; // array is a bit too large, but that's ok
		numElems = 0;
		for(uint32 i=0; i<numMixed; i++)
			if(mixedElems[i]->GetLength() > 1) {
				ctElems[numElems++] = mixedElems[i];
			}

		// Sort itemsets
		//-- Assume isc is already in correct order! (Option d)

		printf_s("done\n\n");
	}

	// Candidate set list foreach overlap len, ie foreach i \in [0,numInput-1]
	islist **candSetLists = new islist *[numInput];
	for(uint32 i=0; i<numInput; i++)
		candSetLists[i] = new islist();
	islist *evidence = new islist();

	// Create helper array to cache (at most) all non-empty subsets of input tagsets
	uint32 maxNumCachedSubsets = uint32(pow(2.0, (int)numInput) - numInput - 1); // singletons need not be cached
	ItemSet **cachedSubsets = new ItemSet *[maxNumCachedSubsets];
	for(uint32 i=0; i<maxNumCachedSubsets; i++)
		cachedSubsets[i] = ItemSet::CreateEmpty(ctElems[0]->GetType(), alphSize);
	uint32 numCachedSubsets = 0;

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	Timer *timer2 = new Timer();

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recommendation;correct;P@1;P@3;P@5;S@3;S@5;MRR\n"); 
	}

	// For each test row
	printf_s("Recommending");
#ifdef _DEBUG
	printf_s("\n");
#endif
	uint32 valuesLength = 2 * max(testDB->GetMaxSetLength(), numTotal);
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput==0 ? 2 : numInput+min(numRecommend,5); // half/half-->min 2, other-->numIn+atleast(5)
	uint32 numActualInput;
	uint32 numTests = maxTest>0 ? maxTest : testDB->GetNumRows();

	for(uint32 r=0; r<numTests; r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		numActualInput = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<numActualInput; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=numActualInput; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;

		// Clear candidate set lists
		for(uint32 i=0; i<numInput; i++)
			candSetLists[i]->clear();

		// Select elems that intersect with input
		ItemSet *candUnion = inputTags->Clone();
		for(uint32 i=0; i<numElems; i++) {
			if(ctElems[i]->Intersects(inputTags)) {	// Look for *all* intersecting tagsets
				if(!inputTags->IsSubset(ctElems[i])) {	// that are not a subset of the input tagset
					candUnion->Unite(ctElems[i]);

					// Compute overlap length with inputTags
					ItemSet *intersection = ctElems[i]->Intersection(inputTags);
					uint32 numOverlap = intersection->GetLength();
					delete intersection;

					// Maintain list for each numOverlap
					candSetLists[numOverlap-1]->push_back(ctElems[i]);
				}
			}
		}
		candUnion->Remove(inputTags);

		// Compute the conditional probability foreach candidate item
		uint32 numCands = candUnion->GetLength();
		uint16 *cands = candUnion->GetValues();
		float *probs = new float[numCands];
		numCachedSubsets = 0;

		for(uint32 c=0; c<numCands; c++) {
			uint16 cand = cands[c];
			probs[c] = 0.0f;

			// Take candidate sets from highest overlap level in which cand occurs
			evidence->clear();
			for(uint32 i=numInput; i>0; i--) {
				for(islist::iterator iter=candSetLists[i-1]->begin(); iter!=candSetLists[i-1]->end(); iter++)
					if((*iter)->IsItemInSet(cand))
						evidence->push_back(*iter);
				//if(!evidence->empty())
				//	break;
			}

			// Test for each itemset in evidence
			for(islist::iterator iter = evidence->begin(); iter!=evidence->end(); iter++) {
				// Determine overlap with inputTags
				ItemSet *overlap = (*iter)->Intersection(inputTags);

				// Get support over overlap
				if(overlap->GetLength() > 1) {		// |overlap| > 1
					// First check whether already cached
					bool found = false;
					for(uint32 i=0; i<numCachedSubsets; i++) {
						if(overlap->Equals(cachedSubsets[i])) {
							overlap->SetSupport(cachedSubsets[i]->GetSupport()); // use support stored in cachedSubset
							found = true;
							break;
						}
					}
					// If not yet cached, proceed
					if(!found) {
						// We need overlapIS's support
						if(useOracle)
							SetOracleSupport(overlap, trainDB);
						else
							SetCTEstimatedSupport(overlap, ctElems, numElems);
						cachedSubsets[numCachedSubsets]->CleanTheSlate();
						cachedSubsets[numCachedSubsets]->Unite(overlap);
						cachedSubsets[numCachedSubsets++]->SetSupport(overlap->GetSupport());
					}
				} else {								// |overlap| == 1, get support from alphabet
					overlap->GetValuesIn(values);
					overlap->SetSupport(alphElems[values[0]]->GetSupport());
				}
				float prob = (float) overlap->GetSupport();

				// Get full itemset's support (in `overlap' for efficiency)
				overlap->AddItemToSet(cands[c]);
				if(useOracle)
					SetOracleSupport(overlap, trainDB);
				else
					SetCTEstimatedSupport(overlap, ctElems, numElems);

				// Compute conditional probability
				prob = (float)(overlap->GetSupport()) / prob;
				if(useProbSum)
					probs[c] += prob;				// sum probabilities when multiple itemsets suggest this candidate
				else
					probs[c] = max(prob, probs[c]); // use maximum probability when multiple itemsets suggest this candidate

				// Cleanup
				delete overlap;
			}
		}
		ArrayUtils::BSortDesc<float,uint16>(probs,numCands,cands);

		// Generate the actual recommendation
		uint32 curLen = 0;
		ItemSet *recomSet = inputTags->Clone();

		for(uint32 c=0; c<numCands && curLen<numRecommend; c++) {
			values[curLen++] = cands[c];
			recomSet->AddItemToSet(cands[c]);
		}
		delete[] cands;
		delete[] probs;
		delete candUnion;

		// While curLen < numRecommend, greedily add overlapping itemsets in standard order
		for(uint32 idx = 0; curLen < numRecommend && idx<numElems; idx++) {
			if(recomSet->Intersects(ctElems[idx]) && !recomSet->IsSubset(ctElems[idx])) {
				ItemSet *is = ctElems[idx]->Clone();
				is->Remove(recomSet);
				is->GetValuesIn(values + curLen);
				curLen += is->GetLength();
				recomSet->Unite(is);
				delete is;
				//idx = 0; // restart from first! iterative overlapsearch. may give minor improvement, but is obviously slower.
			}
		}

		// While curLen < numRecommend, greedily add itemsets in standard order
		for(uint32 idx = 0; curLen < numRecommend && idx<numElems; idx++) {
			if(!recomSet->IsSubset(ctElems[idx])) {
				ItemSet *is = ctElems[idx]->Clone();
				is->Remove(recomSet);
				is->GetValuesIn(values + curLen);
				curLen += is->GetLength();
				recomSet->Unite(is);
				delete is;
			}
		}

		// Determine correctly recommended tags
		recomSet->Remove(inputTags);
		ItemSet *correctTags = recomSet->Intersection(testTags);
		
		// Compute stats
		{
			uint32 firstCorrect = UINT32_MAX_VALUE;
			uint32 p1=0, p3=0, p5=0;
			if(correctTags->GetLength() > 0) {
				for(uint32 i=0; i<curLen; i++) {
					if(correctTags->IsItemInSet(values[i])) {
						if(firstCorrect == UINT32_MAX_VALUE)
							firstCorrect = i;
						if(i<5) {
							precision5 += 0.2;
							p5++;
							if(i<3) {
								precision3 += 0.3333333333333333;
								p3++;
								if(i==0) {
									precision1 += 1.0;
									p1++;
								}
							}
						}
					}
				}
			}
			if(firstCorrect < UINT32_MAX_VALUE) {
				mrr += 1.0 / (firstCorrect+1);
				if(firstCorrect < 5) {
					success5 += 1.0;
					if(firstCorrect < 3)
						success3 += 1.0;
				}
			}

			// Save to file, if asked for
			if(saveRecommendations) {
				fprintf_s(recomFP, "%s;%s;",
					inputTags->ToString(false).c_str(), testTags->ToString(false).c_str());
				
				for(uint32 i=0; i<curLen; i++)
					fprintf_s(recomFP, "%u ", values[i]);

				fprintf_s(recomFP, ";%s;%u;%u;%u;%u;%u;%f\n", 
					correctTags->ToString(false).c_str(),
					p1, p3, p5,
					firstCorrect<3?1:0,	// S@3
					firstCorrect<5?1:0,	// S@5
					firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0 // MRR
				);
			}
		}

		// Cleanup
		delete correctTags;
		delete recomSet;
		delete inputTags;
		delete testTags;

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	if(numRecommend < 5) {
		precision5 = 0.0; success5 = 0.0;
		if(numRecommend < 3)
			precision3 = 0.0; success3 = 0.0;
	}
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	if(useCT) {
		fprintf_s(fp, "ctDir;%s\n", ctDir.c_str());
		fprintf_s(fp, "ctName;%s\n", ctFilename.c_str());
	} else {
		fprintf_s(fp, "iscName;%s\n", iscName.c_str());
	}
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
	fprintf_s(fp, "source;numInput;numRecommend;#sets;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_all (s);time_recom (s)\n");
	fprintf_s(fp, "%s;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%I64u;%I64u\n", useCT?ctFilename.c_str():iscName.c_str(), numInput, numRecommend, numElems, numRowsTested, testDB->GetNumRows()-numRowsTested, 
								precision1, precision3, precision5, success3, success5, mrr, timer1->GetElapsedTime(), timer2->GetElapsedTime());
	fclose(fp);

	// Cleanup
	delete timer1;
	delete timer2;
	delete[] values;
	for(uint32 i=0; i<maxNumCachedSubsets; i++)
		delete cachedSubsets[i];
	delete[] cachedSubsets;

	for(uint32 i=0; i<numInput; i++)
		delete candSetLists[i];
	delete[] candSetLists;
	delete evidence;

	for(uint32 j=0; j<alphSize; j++)
		delete alphElems[j];
	delete[] alphElems;
	delete[] ctElems;

	if(useCT) {
		delete[] mixedElems;
		delete codeTable;
	} else {
		delete isc;
	}
	if(useOracle)
		delete trainDB;
	delete testDB;
}
void TagRecomTH::SetOracleSupport(ItemSet *is, Database *db) {
	ItemSet **rows = db->GetRows();
	uint32 numRows = db->GetNumRows();
	uint32 support = 0;
	for(uint32 i=0; i<numRows; i++)
		if(rows[i]->IsSubset(is))
			support++;
	is->SetSupport(support);
}
void TagRecomTH::SetCTEstimatedSupport(ItemSet *is, ItemSet **ctElems, const uint32 numElems) {
	uint32 support = 0;
	for(uint32 n=0; n<numElems; n++) {
		if(ctElems[n]->IsSubset(is))
			support += ctElems[n]->GetUsageCount(); // support =~ sum of all usages
	}
	is->SetSupport(support);
}
void TagRecomTH::CondProbRecommendation4() {
	char temp[500]; string s;
	Timer *timer1 = new Timer();

	// Settings
	string ctDir = mConfig->Read<string>("ctDir", "");
	string iscName = mConfig->Read<string>("iscName", "");
	bool useCT = ctDir.size() > 0 ? true : false;
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 m = mConfig->Read<uint32>("m");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend");
	uint32 numTotal = numInput + numRecommend;
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	uint32 maxTest = mConfig->Read<uint32>("maxTest", 0);
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);

	bool useOracle = mConfig->Read<string>("cpSupport", "ct").compare("oracle")==0;
	string sMode = mConfig->Read<string>("cpCombine", "max");
	TagRecomCombineMode combineMode = TagRecomMax;
	if(sMode.compare("avg")==0)
		combineMode = TagRecomAvg;
	else if(sMode.compare("min")==0)
		combineMode = TagRecomMin;
	else if(sMode.compare("sum")==0)
		combineMode = TagRecomSum;
	bool useMaxOverlap = mConfig->Read<bool>("cpMaxOverlap", false);

	if(numInput == 0)
		THROW("cprecom4 does not work with half-half split (yet).");

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n");	s=temp;	LOG_MSG(s);
	sprintf_s(temp, 500, "- Mode: %s", useCT?"code table":"itemset collection"); s=temp; LOG_MSG(s);
	if(useCT) {
		sprintf_s(temp, 500, "- CT dir: %s", ctDir.c_str()); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- ISC: %s", iscName.c_str()); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- Support mode: %s", useOracle?"oracle":"code table estimation"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Combine probabilities: %s", sMode.c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Use maximum overlap level: %s", useMaxOverlap?"yes":"no"); s=temp; LOG_MSG(s);
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- m: %u", m); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Max # test instances: %u", maxTest); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint16 alphSize = (uint16) testDB->GetAlphabetSize();
	Database *trainDB = Database::RetrieveDatabase(mConfig->Read<string>("traindb"));
	uint32 numTrainRows = trainDB->GetNumRows();

	// Init baseline
	printf_s("\nBuilding baseline structures ... ");
	uint32 *minCounts = new uint32[alphSize];
	uint32 **counts = new uint32 *[alphSize];
	uint32 **cooccurs = new uint32 *[alphSize];
	float **condProbs = new float *[alphSize];
	uint32 *tempCounts = new uint32[alphSize];
	uint32 *alphCounts = new uint32[alphSize];
	uint32 *alphSizeZeroes = new uint32[alphSize];
	for(uint16 j=0; j<alphSize; j++)
		alphSizeZeroes[j] = 0;
	memcpy_s(minCounts,alphSize*sizeof(uint32),alphSizeZeroes,alphSize*sizeof(uint32));
	memcpy_s(alphCounts,alphSize*sizeof(uint32),alphSizeZeroes,alphSize*sizeof(uint32));
	{
		ItemSet *row;
		uint16 *values = new uint16[trainDB->GetMaxSetLength()];
		for(uint16 i=0; i<alphSize; i++) {
			memcpy_s(tempCounts,alphSize*sizeof(uint32),alphSizeZeroes,alphSize*sizeof(uint32));

			// Enumerate conditional database
			for(uint32 r=0; r<numTrainRows; r++) {
				row = trainDB->GetRow(r);
				if(row->IsItemInSet(i)) {
					alphCounts[i]++;
					row->GetValuesIn(values);
					for(uint32 v=0; v<row->GetLength(); v++)
						tempCounts[values[v]]++; // compute co-occurrence count foreach alphabet element (including i itself, but i is ignored in the next step)
				}
			}

			// Store top-m co-occurrences
			counts[i] = new uint32[m];
			cooccurs[i] = new uint32[m];
			condProbs[i] = new float[m];
			for(uint32 j=0; j<m; j++) {
				counts[i][j] = 0;
				cooccurs[i][j] = 0;
			}
			for(uint16 j=0; j<alphSize; j++) {
				if(i!=j && tempCounts[j] > minCounts[i]) { // add or replace
					if(minCounts[i]==0) { // no m cooccurrences found yet, ADD
						uint16 k=0;
						for(; k<m; k++) // seek to first empty spot
							if(counts[i][k]==0)
								break;
						counts[i][k] = tempCounts[j];
						cooccurs[i][k] = j;
						if(k+1 == m) { // m found now
							// find mincount
							minCounts[i] = counts[i][0];
							for(k=1; k<m; k++) // seek to mincount
								if(counts[i][k]<minCounts[i])
									minCounts[i] = counts[i][k];
						}

					} else { // at least m cooccurrences found yet, REPLACE
						uint16 k=0;
						for(; k<m; k++) // seek to mincount
							if(counts[i][k]==minCounts[i])
								break;
						counts[i][k] = tempCounts[j];
						cooccurs[i][k] = j;

						// update mincount
						minCounts[i] = counts[i][0];
						for(k=1; k<m; k++) // seek to mincount
							if(counts[i][k]<minCounts[i])
								minCounts[i] = counts[i][k];
					}
				}
			}

			// Sort top-m co-occurrences descending on counts
			ArrayUtils::BSortDesc<uint32>(counts[i], m, cooccurs[i]);

			// and pre-compute conditional probabilities
			float norm = (float)(alphCounts[i]);
			for(uint32 k=0; k<m; k++)
				condProbs[i][k] = counts[i][k] / norm;	// norm can never be zero here --> otherwise there wouldn't be any co-occurrences
		}
		delete[] values;
	}

	// Init extension vars
	ItemSet **ctElems = NULL;
	ItemSet **alphElems = NULL;
	ItemSet **mixedElems = NULL;
	uint32 numElems = 0;
	uint32 numMixed = 0;
	CodeTable *codeTable = NULL;
	string ctFilename;
	ItemSetCollection *isc = NULL;
	
	if(useCT) {										// -------------------- CT mode
		// Load CT
		uint32 numCTs = 0;
		{
			directory_iterator itr(ctDir, "*.ct");
			while(itr.next())
				numCTs++;
		}
		if(numCTs == 0)
			THROW("No code tables found...");
		if(numCTs > 1)
			THROW("cprecom4 can only handle a single CT.");

		{
			directory_iterator itr(ctDir, "*.ct");
			uint32 curCT = 0;
			while(itr.next()) {
				ctFilename = itr.filename();
				codeTable = CodeTable::LoadCodeTable(itr.fullpath(), testDB);
			}
		}
		LOG_MSG("CT loaded.");

		// Init structures
		printf_s("\nBuilding CT structures ... ");
		ctElems = codeTable->GetItemSetArray();
		numElems = codeTable->GetCurNumSets();
		alphElems = codeTable->GetSingletonArray();
		numMixed = numElems + alphSize;
		mixedElems = new ItemSet *[numMixed];
		memcpy_s(mixedElems, numMixed * sizeof(ItemSet *), ctElems, numElems*sizeof(ItemSet *));
		memcpy_s(mixedElems+numElems, alphSize * sizeof(ItemSet *), alphElems, alphSize*sizeof(ItemSet *));

		printf_s("done\n\n");

	} else {											// -------------- ISC mode
		bool beenMined;
		isc = FicMain::ProvideItemSetCollection(iscName, trainDB, beenMined, true, true);

		if(isc->GetNumLoadedItemSets() != isc->GetNumItemSets())
			THROW("Too many itemsets, could not load all of them are in memory; try a higher minsup/lower maxlen.");

		// Init structures
		printf_s("\nBuilding ISC structures ... ");
		mixedElems = isc->GetLoadedItemSets();
		numMixed = isc->GetNumLoadedItemSets();

		// Alphabet
		alphElems = trainDB->GetAlphabetSets();

		// Non-singleton elements
		ctElems = new ItemSet *[numMixed]; // array is a bit too large, but that's ok
		numElems = 0;
		for(uint32 i=0; i<numMixed; i++)
			if(mixedElems[i]->GetLength() > 1)
				ctElems[numElems++] = mixedElems[i];

		printf_s("done\n\n");
	}

	// Candidate set list
	islist *candSetList = new islist();
	islist *evidence = new islist();

	// Create helper array to cache (at most) all non-empty subsets of input tagsets
	uint32 maxNumCachedSubsets = uint32(pow(2.0, (int)numInput) - numInput - 1); // singletons need not be cached
	ItemSet **cachedSubsets = new ItemSet *[maxNumCachedSubsets];
	for(uint32 i=0; i<maxNumCachedSubsets; i++)
		cachedSubsets[i] = ItemSet::CreateEmpty(ctElems[0]->GetType(), alphSize);
	uint32 numCachedSubsets = 0;

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	Timer *timer2 = new Timer();

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recommendation;correct;P@1;P@3;P@5;S@3;S@5;MRR\n"); 
	}

	// For each test row
	printf_s("Recommending");
#ifdef _DEBUG
	printf_s("\n");
#endif
	uint32 valuesLength = 2 * max(testDB->GetMaxSetLength(), numTotal);
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput==0 ? 2 : numInput+min(numRecommend,5); // half/half-->min 2, other-->numIn+atleast(5)
	uint32 numActualInput;
	uint32 numTests = maxTest>0 ? maxTest : testDB->GetNumRows();
	uint32 numTestsWithCT= 0;

	for(uint32 r=0; r<numTests; r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		numActualInput = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<numActualInput; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=numActualInput; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;
		inputTags->GetValuesIn(values);

		// Clear candidate set list
		candSetList->clear();

		// Select CT elems that have an intersection with input with length>1
		ItemSet *candUnion = inputTags->Clone();
		for(uint32 i=0; i<numElems; i++) {
			if(ctElems[i]->Intersects(inputTags)) {	// Look for *all* intersecting tagsets
				if(!inputTags->IsSubset(ctElems[i])) {	// that are not a subset of the input tagset
					ItemSet *overlap = ctElems[i]->Intersection(inputTags);
					if(overlap->GetLength() > 1) {
						candUnion->Unite(ctElems[i]);
						candSetList->push_back(ctElems[i]);
					}
					delete overlap;
				}
			}
		}
		if(!candSetList->empty())
			numTestsWithCT++;

		// Select baseline candidate items
		for(uint32 i=0; i<inputTags->GetLength(); i++) {
			uint16 item = values[i];
			for(uint32 j=0; j<m && counts[item][j]>0; j++)
				candUnion->AddItemToSet((uint16)cooccurs[item][j]);
		}

		// Remove input tags -- they're not candidates
		candUnion->Remove(inputTags);

		// Compute the conditional probability foreach candidate item
		uint32 numCands = candUnion->GetLength();
		uint16 *cands = candUnion->GetValues();
		float *probs = new float[numCands];
		numCachedSubsets = 0;

		// For each candidate
		for(uint32 c=0; c<numCands; c++) {
			uint16 cand = cands[c];
			uint32 maxOverlap = 0;
			uint32 numEvidence = 0;
			probs[c] = combineMode==TagRecomMin ? FLOAT_MAX_VALUE : 0.0f;

			// First search for probabilities from baseline (if any)
			for(uint32 i=0; i<inputTags->GetLength(); i++) {
				uint16 item = values[i];
				for(uint32 j=0; j<m && counts[item][j]>0; j++) {
					if(cooccurs[item][j] == cand) {
						if(combineMode==TagRecomMin)
							probs[c] = min(probs[c], condProbs[item][j]);
						else if(combineMode==TagRecomMax)
							probs[c] = max(probs[c], condProbs[item][j]);
						else // sum or avg
							probs[c] += condProbs[item][j];
						maxOverlap = 1;
						numEvidence++;
						break;
					}
				}
			}
			
			// Foreach CT candidate set in which cand occurs
			for(islist::iterator iter=candSetList->begin(); iter!=candSetList->end(); iter++) {
				if(!((*iter)->IsItemInSet(cand)))
					continue;

				++numEvidence;

				// Determine overlap with inputTags
				ItemSet *overlap = (*iter)->Intersection(inputTags);
				// Overlap should always be >1 here

				// First check whether already cached
				bool found = false;
				for(uint32 i=0; i<numCachedSubsets; i++) {
					if(overlap->Equals(cachedSubsets[i])) {
						overlap->SetSupport(cachedSubsets[i]->GetSupport()); // use support stored in cachedSubset
						found = true;
						break;
					}
				}
				// If not yet cached, proceed
				if(!found) {
					// We need overlapIS's support
					if(useCT) {
						if(useOracle)
							SetOracleSupport(overlap, trainDB);
						else
							SetCTEstimatedSupport(overlap, ctElems, numElems);
					} else { // ISC
						uint32 idx=0;
						while(!ctElems[idx]->Equals(overlap) && idx<numElems)
							idx++;
						if(idx==numElems)
							THROW("Itemset not found, impossible dude.");
						overlap->SetSupport(ctElems[idx]->GetSupport());
					}
					cachedSubsets[numCachedSubsets]->CleanTheSlate();
					cachedSubsets[numCachedSubsets]->Unite(overlap);
					cachedSubsets[numCachedSubsets++]->SetSupport(overlap->GetSupport());
				}
				float prob = (float) overlap->GetSupport();

				// Skip when normalisation probability is zero; it could only result in NaNning up the final result..
				if(prob == 0.0f) {
					delete overlap;
					continue;
				}

				// Get full itemset's support (in `overlap' for efficiency)
				overlap->AddItemToSet(cand);
				if(useCT) {
					if(useOracle)
						SetOracleSupport(overlap, trainDB);
					else
						SetCTEstimatedSupport(overlap, ctElems, numElems);
				} else { // ISC
					uint32 idx=0;
					while(!ctElems[idx]->Equals(overlap) && idx<numElems)
						idx++;
					if(idx==numElems)
						THROW("Itemset not found, impossible dude.");
					overlap->SetSupport(ctElems[idx]->GetSupport());
				}

				// Compute conditional probability
				prob = (float)(overlap->GetSupport()) / prob;

				// Combine
				if(useMaxOverlap) {
					if(overlap->GetLength() > maxOverlap) {
						maxOverlap = overlap->GetLength();
						probs[c] = prob; // reset to this prob
						numEvidence = 1; // and reset #evidence
					} else if(overlap->GetLength() == maxOverlap) {
						if(combineMode==TagRecomMin)
							probs[c] = min(probs[c], prob);
						else if(combineMode==TagRecomMax)
							probs[c] = max(probs[c], prob);
						else // sum or avg
							probs[c] += prob;
					} // else shorter overlap->do nothing

				} else {
					if(combineMode==TagRecomMin)
						probs[c] = min(probs[c], prob);
					else if(combineMode==TagRecomMax)
						probs[c] = max(probs[c], prob);
					else // sum or avg
						probs[c] += prob;
				}

				// Cleanup
				delete overlap;
			}
			if(combineMode==TagRecomAvg)
				probs[c] /= numEvidence;
		}
		ArrayUtils::BSortDesc<float,uint16>(probs,numCands,cands);

		// Generate the actual recommendation
		uint32 curLen = 0;
		ItemSet *recomSet = inputTags->Clone();

		for(uint32 c=0; c<numCands && curLen<numRecommend; c++) {
			values[curLen++] = cands[c];
			recomSet->AddItemToSet(cands[c]);
		}
		delete[] cands;
		delete[] probs;
		delete candUnion;

		// Determine correctly recommended tags
		recomSet->Remove(inputTags);
		ItemSet *correctTags = recomSet->Intersection(testTags);
		
		// Compute stats
		{
			uint32 firstCorrect = UINT32_MAX_VALUE;
			uint32 p1=0, p3=0, p5=0;
			if(correctTags->GetLength() > 0) {
				for(uint32 i=0; i<curLen; i++) {
					if(correctTags->IsItemInSet(values[i])) {
						if(firstCorrect == UINT32_MAX_VALUE)
							firstCorrect = i;
						if(i<5) {
							precision5 += 0.2;
							p5++;
							if(i<3) {
								precision3 += 0.3333333333333333;
								p3++;
								if(i==0) {
									precision1 += 1.0;
									p1++;
								}
							}
						}
					}
				}
			}
			if(firstCorrect < UINT32_MAX_VALUE) {
				mrr += 1.0 / (firstCorrect+1);
				if(firstCorrect < 5) {
					success5 += 1.0;
					if(firstCorrect < 3)
						success3 += 1.0;
				}
			}

			// Save to file, if asked for
			if(saveRecommendations) {
				fprintf_s(recomFP, "%s;%s;",
					inputTags->ToString(false).c_str(), testTags->ToString(false).c_str());
				
				for(uint32 i=0; i<curLen; i++)
					fprintf_s(recomFP, "%u ", values[i]);

				fprintf_s(recomFP, ";%s;%u;%u;%u;%u;%u;%f\n", 
					correctTags->ToString(false).c_str(),
					p1, p3, p5,
					firstCorrect<3?1:0,	// S@3
					firstCorrect<5?1:0,	// S@5
					firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0 // MRR
				);
			}
		}

		// Cleanup
		delete correctTags;
		delete recomSet;
		delete inputTags;
		delete testTags;

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	if(numRecommend < 5) {
		precision5 = 0.0; success5 = 0.0;
		if(numRecommend < 3)
			precision3 = 0.0; success3 = 0.0;
	}
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Fraction of tests with CT:\t\t%.01f", (100.0f*numTestsWithCT)/numRowsTested); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	if(useCT) {
		fprintf_s(fp, "ctDir;%s\n", ctDir.c_str());
		fprintf_s(fp, "ctName;%s\n", ctFilename.c_str());
	} else {
		fprintf_s(fp, "iscName;%s\n", iscName.c_str());
	}
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
	fprintf_s(fp, "source;numInput;numRecommend;#sets;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_all (s);time_recom (s)\n");
	fprintf_s(fp, "%s;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%I64u;%I64u\n", useCT?ctFilename.c_str():iscName.c_str(), numInput, numRecommend, numElems, numRowsTested, testDB->GetNumRows()-numRowsTested, 
								precision1, precision3, precision5, success3, success5, mrr, timer1->GetElapsedTime(), timer2->GetElapsedTime());
	fclose(fp);

	// Cleanup
	delete timer1;
	delete timer2;
	delete[] values;
	for(uint32 i=0; i<maxNumCachedSubsets; i++)
		delete cachedSubsets[i];
	delete[] cachedSubsets;

	delete candSetList;
	delete evidence;

	for(uint32 i=0; i<alphSize; i++) {
		delete[] counts[i];
		delete[] cooccurs[i];
		delete[] condProbs[i];
	}
	delete[] counts;
	delete[] cooccurs;
	delete[] condProbs;
	delete[] minCounts;
	delete[] tempCounts;
	delete[] alphCounts;
	delete[] alphSizeZeroes;

	for(uint32 j=0; j<alphSize; j++)
		delete alphElems[j];
	delete[] alphElems;
	delete[] ctElems;

	if(useCT) {
		delete[] mixedElems;
		delete codeTable;
	} else {
		delete isc;
	}
	delete trainDB;
	delete testDB;
}
void TagRecomTH::TestSupportEstimation() {
	char temp[500]; string s;

	// Settings
	string ctDir = mConfig->Read<string>("ctDir");
	uint32 itemSetLength = mConfig->Read<uint32>("itemSetLength");
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	uint32 maxTest = mConfig->Read<uint32>("maxTest", 0);

	if(itemSetLength < 2)
		THROW("Testing doesn't make sense for lengths < 2.");

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n");	s=temp;	LOG_MSG(s);
	sprintf_s(temp, 500, "- CT dir: %s", ctDir.c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Itemset length: %u", itemSetLength); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Oracle database: %s", mConfig->Read<string>("db").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Max # test instances: %u", maxTest); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *db = Database::RetrieveDatabase(mConfig->Read<string>("db"));
	uint32 numRows = db->GetNumRows();
	uint32 alphSize = (uint32) db->GetAlphabetSize();

	// Init extension vars
	ItemSet **ctElems = NULL;
	ItemSet **alphElems = NULL;
	ItemSet **mixedElems = NULL;
	uint32 numElems = 0;
	uint32 numMixed = 0;
	CodeTable *codeTable = NULL;
	string ctFilename;

	// Load CT
	uint32 numCTs = 0;
	{
		directory_iterator itr(ctDir, "*.ct");
		while(itr.next())
			numCTs++;
	}
	if(numCTs == 0)
		THROW("No code tables found...");
	if(numCTs > 1)
		THROW("Can only handle a single CT.");

	{
		directory_iterator itr(ctDir, "*.ct");
		uint32 curCT = 0;
		while(itr.next()) {
			ctFilename = itr.filename();
			codeTable = CodeTable::LoadCodeTable(itr.fullpath(), db);
		}
	}
	LOG_MSG("CT loaded.");

	// Init structures
	printf_s("\nBuilding CT structures ... ");
	ctElems = codeTable->GetItemSetArray();
	numElems = codeTable->GetCurNumSets();
	alphElems = codeTable->GetSingletonArray();
	numMixed = numElems + alphSize;
	mixedElems = new ItemSet *[numMixed];
	memcpy_s(mixedElems, numMixed * sizeof(ItemSet *), ctElems, numElems*sizeof(ItemSet *));
	memcpy_s(mixedElems+numElems, alphSize * sizeof(ItemSet *), alphElems, alphSize*sizeof(ItemSet *));

	printf_s("done\n\n");

	// Init
	uint32 valuesLength = db->GetMaxSetLength();
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = itemSetLength;
	uint32 numTests = maxTest>0 ? maxTest : db->GetNumRows();
	uint32 numTestsWithCT= 0;

	// Stats
	int32* absDiffs = new int32[numTests];
	double* relDiffs = new double[numTests];
	double absMeanDiff = 0.0, relMeanDiff = 0.0;
	double absAbsMeanDiff = 0.0, relAbsMeanDiff = 0.0;

	// For each test row
	printf_s("Testing support estimation ...");
#ifdef _DEBUG
	printf_s("\n");
#endif
	for(uint32 r=0; r<numTests; r++) {
		ItemSet *row = db->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing

		// Generate itemset
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *is = ItemSet::CreateEmpty(db->GetDataType(), alphSize);
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		for(uint32 i=0; i<itemSetLength; i++)
			is->AddItemToSet(values[random[i]]);
		delete[] random;

		// Compute true and estimated supports
		SetOracleSupport(is, db);
		int32 trueSupport = (int32) is->GetSupport();
		SetCTEstimatedSupport(is, ctElems, numElems);
		int32 estimatedSupport = (int32) is->GetSupport();

		// Compute difference
		absDiffs[numRowsTested] = estimatedSupport - trueSupport;
		relDiffs[numRowsTested] = (estimatedSupport - trueSupport) / (double)trueSupport;

		absMeanDiff += absDiffs[numRowsTested];
		relMeanDiff += relDiffs[numRowsTested];
		absAbsMeanDiff += abs(absDiffs[numRowsTested]);
		relAbsMeanDiff += abs(relDiffs[numRowsTested]);

		// Next!
		numRowsTested++;

		// Cleanup
		delete is;

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished testing.\n\n");

	// Stats
	absMeanDiff /= numRowsTested;		relMeanDiff /= numRowsTested;		// Means
	absAbsMeanDiff /= numRowsTested;	relAbsMeanDiff /= numRowsTested;
	double absAbsMeanDiffStdev = 0.0, relAbsMeanDiffStdev = 0.0;			// Stdevs
	for(uint32 i=0; i<numRowsTested; i++) {
		absAbsMeanDiffStdev += pow(abs(absDiffs[i]) - absAbsMeanDiff, 2.0); 
		relAbsMeanDiffStdev += pow(abs(relDiffs[i]) - relAbsMeanDiff, 2.0); 
	}
	absAbsMeanDiffStdev /= numRowsTested;
	relAbsMeanDiffStdev /= numRowsTested;
	absAbsMeanDiffStdev = sqrt(absAbsMeanDiffStdev);
	relAbsMeanDiffStdev = sqrt(relAbsMeanDiffStdev);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	fprintf_s(fp, "ctDir;%s\n", ctDir.c_str());
	fprintf_s(fp, "ctName;%s\n", ctFilename.c_str());
	fprintf_s(fp, "DB;%s\n", db->GetFilename().c_str());
	fprintf_s(fp, "CT;isLen;mean(absDiffs);mean(relDiffs);mean(|absDiffs|);stdev(|absDiffs|);mean(|relDiffs|);stdev(|relDiffs|)\n");
	fprintf_s(fp, "%s;%u;%.1f;%.3f;%.1f;%.1f;%.3f;%.3f\n", ctFilename.c_str(), itemSetLength, absMeanDiff, relMeanDiff,
					absAbsMeanDiff, absAbsMeanDiffStdev, relAbsMeanDiff, relAbsMeanDiffStdev);
	fclose(fp);

	// Cleanup
	delete[] absDiffs;
	delete[] relDiffs;
	delete[] values;
	for(uint32 j=0; j<alphSize; j++)
		delete alphElems[j];
	delete[] alphElems;
	delete[] ctElems;
	delete[] mixedElems;
	delete codeTable;
	delete db;
}

void TagRecomTH::SlowRecommendation() {
	char temp[500]; string s;
	Timer *timer1 = new Timer();

	// Settings
	string ctDir = mConfig->Read<string>("ctDir", "");
	string iscName = mConfig->Read<string>("iscName", "");
	bool useCT = ctDir.size() > 0 ? true : false;
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend");
	uint32 numTotal = numInput + numRecommend;
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	uint32 maxTest = mConfig->Read<uint32>("maxTest", 0);
	string sOrder = mConfig->Read<string>("recomOrder", "support");
	IscOrderType recomOrder = NoneIscOrder;
	if(sOrder.compare("support") == 0)
		recomOrder = LengthDescIscOrder;
	else if(sOrder.compare("usage") == 0)
		recomOrder = UsageDescIscOrder;
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Mode: %s", useCT?"code table":"itemset collection"); s=temp; LOG_MSG(s);
	if(useCT) {
		sprintf_s(temp, 500, "- CT dir: %s", ctDir.c_str()); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- ISC: %s", iscName.c_str()); s=temp; LOG_MSG(s);
	}
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Max # test instances: %u", maxTest); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Order: %s", sOrder.c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint32 alphSize = (uint16) testDB->GetAlphabetSize();
	//if(maxTest > 0)
	//	testDB->RandomizeRowOrder();

	// Init vars
	ItemSet **ctElems = NULL;
	ItemSet **alphElems = NULL;
	ItemSet **mixedElems = NULL;
	uint32 numElems = 0;
	uint32 numMixed = 0;
	CodeTable *codeTable = NULL;
	string ctFilename;
	ItemSetCollection *isc = NULL;

	string dbName = iscName.substr(0,iscName.find_first_of('-'));
	Database *db = Database::RetrieveDatabase(dbName);

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	Timer *timer2 = new Timer();

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recommendation;correct;P@1;P@3;P@5;S@3;S@5;MRR\n"); 
	}

	// For each test row
	printf_s("Recommending");
#ifdef _DEBUG
	printf_s("\n");
#endif
	uint32 valuesLength = 2 * max(testDB->GetMaxSetLength(),numTotal);
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput==0 ? 2 : numInput+min(numRecommend,5); // half/half-->min 2, other-->numIn+atleast(5)
	uint32 numActualInput;
	uint32 numTests = maxTest>0 ? maxTest : testDB->GetNumRows();
	for(uint32 r=0; r<numTests; r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		numActualInput = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<numActualInput; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=numActualInput; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;

		// Select appropriate subset of database
		ItemSet **rows = db->GetRows();
		uint32 trainNumRows = db->GetNumRows();
		ItemSet **iss = new ItemSet *[trainNumRows];
		uint32 idx = 0;
		for(uint32 t=0; t<trainNumRows; t++) {
			if(rows[t]->Intersects(inputTags))
				iss[idx++] = rows[t]->Clone();		// cloning shouldn't be necessary, but delete deletes db's rows
		}
		Database *subsetDb = new Database(iss, idx, false);
		subsetDb->UseAlphabetFrom(db);
		subsetDb->ComputeEssentials();

		// Mine itemset collection
		isc = FicMain::MineItemSetCollection(iscName, subsetDb, false, true);
		if(isc->GetNumLoadedItemSets() != isc->GetNumItemSets())
			THROW("Too many itemsets, not all of them are loaded in memory; try a higher minsup/lower maxlen.");

		// Delete subset database
		delete subsetDb;

		// Init structures
		ctElems = isc->GetLoadedItemSets();
		numElems = isc->GetNumLoadedItemSets();
		numMixed = numElems;
		mixedElems = ctElems;

		// Select elems that intersect with input
		islist **firstRankElems = new islist *[numActualInput];
		for(uint32 i=0; i<numActualInput; i++)
			firstRankElems[i] = new islist();
		ItemSet *selUnion = inputTags->Clone();
		for(uint32 i=0; i<numElems; i++) {
			if(ctElems[i]->Intersects(inputTags)) {
				ItemSet *is = ctElems[i]->Intersection(inputTags);
				if(ctElems[i]->GetLength() == is->GetLength()) { // check could be stricter by intersecting with selUnion (but this is needed for ranking)
					delete is; // does not add anything
					continue;
				}
				uint32 idx = is->GetLength() - 1;
				delete is;
				firstRankElems[idx]->push_back(ctElems[i]);
				selUnion->Unite(ctElems[i]);
			}
		}

		// Second rank
		islist *secondRankElems = new islist();
		while(selUnion->GetLength() < numTotal) {
			ItemSet *best = NULL;
			uint32 bestOverlap = 0;
			for(uint32 i=0; i<numElems; i++) {
				if(ctElems[i]->Intersects(selUnion)) {
					ItemSet *is = ctElems[i]->Intersection(selUnion);
					if(is->GetLength() == ctElems[i]->GetLength()) {
						delete is;
						continue; // adds nothing!
					}
					if(is->GetLength() > bestOverlap) {
						bestOverlap = is->GetLength();
						best = ctElems[i];
					}
					delete is;
				}
			}
			if(best == NULL)
				break;
			secondRankElems->push_back(best);
			selUnion->Unite(best);
		}

		// Just add anything in order, if needed
		islist *lastRankElems = new islist();
		for(uint32 i=0; selUnion->GetLength() < numTotal && i<numMixed; i++) {
			ItemSet *is = mixedElems[i]->Intersection(selUnion);
			if(is->GetLength() == mixedElems[i]->GetLength()) {
				delete is;
				continue; // adds nothing!
			}
			delete is;
			lastRankElems->push_back(mixedElems[i]);
			selUnion->Unite(mixedElems[i]);
		}

		// Test recommendation
		selUnion->Remove(inputTags);
#ifdef _DEBUG
		printf_s("** Recom: %s\n", selUnion->ToString(false).c_str());
#endif

		if(selUnion->GetLength() > valuesLength) {
			valuesLength = 2 * selUnion->GetLength();
			delete[] values;
			values = new uint16[valuesLength];
		}

		uint32 curLen = 0;
		ItemSet *soFar = inputTags->Clone();
		for(int32 n=numActualInput-1; n>=0; n--) {	// First rank
			islist *isl = firstRankElems[n];
			for(islist::iterator iter = isl->begin(); iter!=isl->end(); iter++) {
				ItemSet *is = (*iter)->Clone();
				is->Remove(soFar);
				is->GetValuesIn(values+curLen);
				curLen += is->GetLength();
				soFar->Unite(is);
				delete is;
			}
		}
		for(islist::iterator iter = secondRankElems->begin(); iter!=secondRankElems->end(); iter++) { // Second rank
			ItemSet *is = (*iter)->Clone();
			is->Remove(soFar);
			is->GetValuesIn(values+curLen);
			curLen += is->GetLength();
			soFar->Unite(is);
			delete is;
		}
		for(islist::iterator iter = lastRankElems->begin(); iter!=lastRankElems->end(); iter++) { // Last rank
			ItemSet *is = (*iter)->Clone();
			is->Remove(soFar);
			is->GetValuesIn(values+curLen);
			curLen += is->GetLength();
			soFar->Unite(is);
			delete is;
		}
		delete soFar;
		if(curLen != selUnion->GetLength())
			THROW("Strange things happening.");

		ItemSet *correctTags = selUnion->Intersection(testTags);
		{
			// Compute stats
			uint32 firstCorrect = UINT32_MAX_VALUE;
			for(uint32 i=0; i<curLen; i++) {
				if(correctTags->IsItemInSet(values[i])) {
					if(firstCorrect == UINT32_MAX_VALUE)
						firstCorrect = i;
					if(i<5) {
						precision5 += 0.2;
						if(i<3) {
							precision3 += 0.3333333333333333;
							if(i==0)
								precision1 += 1.0;
						}
					}
				}
			}
			if(firstCorrect < UINT32_MAX_VALUE) {
				mrr += 1.0 / (firstCorrect+1);
				if(firstCorrect < 5) {
					success5 += 1.0;
					if(firstCorrect < 3)
						success3 += 1.0;
				}
			}

			// Save to file, if asked for
			if(saveRecommendations) {
				fprintf_s(recomFP, "%s;%s;%s;%s;%f;%f;%f;%u;%u;%f\n", 
					inputTags->ToString(false).c_str(), testTags->ToString(false).c_str(), selUnion->ToString(false).c_str(), correctTags->ToString(false).c_str(),
					precision1, precision3, precision5,
					firstCorrect<3?1:0,	// S@3
					firstCorrect<5?1:0,	// S@5
					firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0 // MRR
					);
			}
		}

		// Cleanup
		delete isc;
		delete correctTags;
		delete selUnion;
		for(uint32 i=0; i<numActualInput; i++)
			delete firstRankElems[i];
		delete firstRankElems;
		delete secondRankElems;
		delete lastRankElems;
		delete inputTags;
		delete testTags;

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	if(numRecommend < 5) {
		precision5 = 0.0; success5 = 0.0;
		if(numRecommend < 3)
			precision3 = 0.0; success3 = 0.0;
	}
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	if(useCT) {
		fprintf_s(fp, "ctDir;%s\n", ctDir.c_str());
		fprintf_s(fp, "ctName;%s\n", ctFilename.c_str());
	} else {
		fprintf_s(fp, "iscName;%s\n", iscName.c_str());
	}
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
	fprintf_s(fp, "source;numInput;numRecommend;#sets;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_all (s);time_recom (s)\n");
	fprintf_s(fp, "%s;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%I64u;%I64u\n", useCT?ctFilename.c_str():iscName.c_str(), numInput, numRecommend, numElems, numRowsTested, testDB->GetNumRows()-numRowsTested, 
		precision1, precision3, precision5, success3, success5, mrr, timer1->GetElapsedTime(), timer2->GetElapsedTime());
	fclose(fp);

	// Cleanup
	delete db;
	delete timer1;
	delete timer2;
	delete[] values;
	delete testDB;
}

void TagRecomTH::LATRE() {
	char temp[500]; string s;
	Timer *timer1 = new Timer();

	// Settings
	uint32 numInput = mConfig->Read<uint32>("numInput");
	//uint32 maxLen = mConfig->Read<uint32>("maxlen");
	//uint32 minSup = mConfig->Read<uint32>("minsup");
	//float minConf = mConfig->Read<float>("minconf");
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	uint32 maxTest = mConfig->Read<uint32>("maxTest", 0);
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);
	//if(maxLen > 3)
		//THROW("maxLen > 3 not implemented.");
	if(numInput < 2 || numInput > 2)
		THROW("numInput should currently be in range [2,2].");

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	//sprintf_s(temp, 500, "- maxLen: %u", maxLen); s=temp; LOG_MSG(s);
	//sprintf_s(temp, 500, "- minSup: %u", minSup); s=temp; LOG_MSG(s);
	//sprintf_s(temp, 500, "- minConf: %f", minConf); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Train database: %s", mConfig->Read<string>("traindb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Max # test instances: %u", maxTest); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load database
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint16 alphSize = (uint16) testDB->GetAlphabetSize();
	Database *trainDB = Database::RetrieveDatabase(mConfig->Read<string>("traindb"));
	uint32 numTrainRows = trainDB->GetNumRows();
	ItemSet **trainRows = trainDB->GetRows();

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	Timer *timer2 = new Timer();

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recommendation;correct;P@1;P@3;P@5;S@3;S@5;MRR\n"); 
	}

	// For each test row
	printf_s("Recommending");
#ifdef _DEBUG
	printf_s("\n");
#endif
	uint32 valuesLength = 2 * testDB->GetMaxSetLength();
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput+5;
	uint32 numActualInput;
	uint32 numTests = maxTest>0 ? maxTest : testDB->GetNumRows();
	uint32 numTestsWithCT= 0;
	ItemSet **condRows = new ItemSet *[numTrainRows];
	uint32 numCondRows = 0;
	
	for(uint32 r=0; numRowsTested<numTests && r<testDB->GetNumRows(); r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		numActualInput = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<numActualInput; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=numActualInput; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;
		inputTags->GetValuesIn(values);

		// Compute conditional database and candidate tagset
		ItemSet *candUnion = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		numCondRows = 0;
		for(uint32 i=0; i<numTrainRows; i++) {
			if(inputTags->Intersects(trainRows[i])) {
				condRows[numCondRows++] = trainRows[i];
				candUnion->Unite(trainRows[i]);
			}
		}
		candUnion->Remove(inputTags);

		// Compute conditional probabilities
		uint32 numCands = candUnion->GetLength();
		uint16 *cands = candUnion->GetValues();
		float *scores = new float[numCands];
		float *probs = new float[numCands];
		uint32 antecedentCount;
		ItemSet *antecedent = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		for(uint32 c=0; c<numCands; c++)
			scores[c] = 0.0f;

			// singleton antecedent
		for(uint32 i=0; i<numInput; i++) {
			antecedentCount = 0;
			for(uint32 c=0; c<numCands; c++)
				probs[c] = 0.0f;

			for(uint32 j=0; j<numCondRows; j++) {
				if(condRows[j]->IsItemInSet(values[i])) { // antecedent check
					antecedentCount++;
					for(uint32 c=0; c<numCands; c++) {
						if(condRows[j]->IsItemInSet(cands[c]))
							probs[c] += 1.0f;
					}
				}
			}
			for(uint32 c=0; c<numCands; c++) {
				probs[c] /= antecedentCount;
				scores[c] += probs[c];
			}
		}

		// length two antecedent
		antecedent->CleanTheSlate();
		antecedent->AddItemToSet(values[0]);
		antecedent->AddItemToSet(values[1]);
		antecedentCount = 0;
		for(uint32 c=0; c<numCands; c++)
			probs[c] = 0.0f;
		for(uint32 j=0; j<numCondRows; j++) {
			if(condRows[j]->IsSubset(antecedent)) {
				antecedentCount++;
				for(uint32 c=0; c<numCands; c++) {
					if(condRows[j]->IsItemInSet(cands[c]))
						probs[c] += 1.0f;
				}
			}
		}
		for(uint32 c=0; c<numCands; c++) {
			probs[c] /= antecedentCount;
			scores[c] += probs[c];
		}

		// Sort to rank
		ArrayUtils::BSortDesc<float>(scores, numCands, cands);

		// Cleanup	
		delete antecedent;
		delete[] scores;

		// Determine correctly recommended tags
		ItemSet *correctTags = candUnion->Intersection(testTags);

		// Compute stats
		{
			uint32 firstCorrect = UINT32_MAX_VALUE;
			uint32 p1=0, p3=0, p5=0;
			if(correctTags->GetLength() > 0) {
				for(uint32 i=0; i<numCands; i++) {
					if(correctTags->IsItemInSet(cands[i])) {
						if(firstCorrect == UINT32_MAX_VALUE)
							firstCorrect = i;
						if(i<5) {
							precision5 += 0.2;
							p5++;
							if(i<3) {
								precision3 += 0.3333333333333333;
								p3++;
								if(i==0) {
									precision1 += 1.0;
									p1++;
								}
							}
						}
					}
				}
			}
			if(firstCorrect < UINT32_MAX_VALUE) {
				mrr += 1.0 / (firstCorrect+1);
				if(firstCorrect < 5) {
					success5 += 1.0;
					if(firstCorrect < 3)
						success3 += 1.0;
				}
			}

			// Save to file, if asked for
			if(saveRecommendations) {
				fprintf_s(recomFP, "%s;%s;",
					inputTags->ToString(false).c_str(), testTags->ToString(false).c_str());

				for(uint32 i=0; i<numCands; i++)
					fprintf_s(recomFP, "%u ", cands[i]);

				fprintf_s(recomFP, ";%s;%u;%u;%u;%u;%u;%f\n", 
					correctTags->ToString(false).c_str(),
					p1, p3, p5,
					firstCorrect<3?1:0,	// S@3
					firstCorrect<5?1:0,	// S@5
					firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0 // MRR
					);
			}
		}

		// Cleanup
		delete[] cands;
		delete candUnion;
		delete correctTags;
		delete inputTags;
		delete testTags;

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
	fprintf_s(fp, "source;numInput;numRecommend;#sets;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_all (s);time_recom (s)\n");
	fprintf_s(fp, "%s;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%I64u;%I64u\n", trainDB->GetFilename().c_str(), numInput, 5, 0, numRowsTested, testDB->GetNumRows()-numRowsTested, 
		precision1, precision3, precision5, success3, success5, mrr, timer1->GetElapsedTime(), timer2->GetElapsedTime());
	fclose(fp);

	// Cleanup
	delete[] condRows;
	delete timer1;
	delete timer2;
	delete[] values;
	delete trainDB;
	delete testDB;
}


void TagRecomTH::BorkursBaseline() {
	char temp[500]; string s;
	Timer *timer1 = new Timer();

	// Settings
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend", 5);
	uint32 m = mConfig->Read<uint32>("m");
	bool useVoting = mConfig->Read<string>("strategy","vote").compare("vote") == 0;
	bool usePromotion = mConfig->Read<bool>("promotion", false);
	uint32 promoStability = mConfig->Read<uint32>("promoStability", 9);
	uint32 promoDescriptiveness = mConfig->Read<uint32>("promoDescriptiveness", 11);
	uint32 promoRank = mConfig->Read<uint32>("promoRank", 4);
	uint32 seed = mConfig->Read<uint32>("seed", 0);
	bool saveRecommendations = mConfig->Read<bool>("saveRecommendations", false);

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n"); s=temp; LOG_MSG(s);
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- m: %u", m); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Strategy: %s", useVoting?"vote":"sum"); s=temp; LOG_MSG(s);
	if(usePromotion) {
		sprintf_s(temp, 500, "- Promotion: yes (%d, %d, %d)", promoStability, promoDescriptiveness, promoRank); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- Promotion: no"); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- Train database: %s", mConfig->Read<string>("traindb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Save recommendations: %s", saveRecommendations?"yes":"no"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	// Load databases
	Database *trainDB = Database::RetrieveDatabase(mConfig->Read<string>("traindb"));
	Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));
	uint16 alphSize = (uint16) testDB->GetAlphabetSize();
	uint32 numTrainRows = trainDB->GetNumRows();

	// Init structures
	printf_s("\nBuilding structures ... ");
	uint32 *minCounts = new uint32[alphSize];
	uint32 **counts = new uint32 *[alphSize];
	uint32 **cooccurs = new uint32 *[alphSize];
	uint32 *tempCounts = new uint32[alphSize];
	uint32 *alphCounts = new uint32[alphSize];
	uint32 *alphSizeZeroes = new uint32[alphSize];
	for(uint16 j=0; j<alphSize; j++)
		alphSizeZeroes[j] = 0;
	memcpy_s(minCounts,alphSize*sizeof(uint32),alphSizeZeroes,alphSize*sizeof(uint32));
	memcpy_s(alphCounts,alphSize*sizeof(uint32),alphSizeZeroes,alphSize*sizeof(uint32));
	{
		ItemSet *row;
		uint16 *values = new uint16[trainDB->GetMaxSetLength()];
		for(uint16 i=0; i<alphSize; i++) {
			memcpy_s(tempCounts,alphSize*sizeof(uint32),alphSizeZeroes,alphSize*sizeof(uint32));

			// Enumerate conditional database
			for(uint32 r=0; r<numTrainRows; r++) {
				row = trainDB->GetRow(r);
				if(row->IsItemInSet(i)) {
					alphCounts[i]++;
					row->GetValuesIn(values);
					for(uint32 v=0; v<row->GetLength(); v++)
						tempCounts[values[v]]++; // compute co-occurrence count foreach alphabet element (including i itself, but i is ignored in the next step)
				}
			}

			// Store top-m co-occurrences
			counts[i] = new uint32[m];
			cooccurs[i] = new uint32[m];
			for(uint32 j=0; j<m; j++) {
				counts[i][j] = 0;
				cooccurs[i][j] = 0;
			}
			for(uint16 j=0; j<alphSize; j++) {
				if(i!=j && tempCounts[j] > minCounts[i]) { // add or replace
					if(minCounts[i]==0) { // no m cooccurrences found yet, ADD
						uint16 k=0;
						for(; k<m; k++) // seek to first empty spot
							if(counts[i][k]==0)
								break;
						counts[i][k] = tempCounts[j];
						cooccurs[i][k] = j;
						if(k+1 == m) { // m found now
							// find mincount
							minCounts[i] = counts[i][0];
							for(k=1; k<m; k++) // seek to mincount
								if(counts[i][k]<minCounts[i])
									minCounts[i] = counts[i][k];
						}

					} else { // at least m cooccurrences found yet, REPLACE
						uint16 k=0;
						for(; k<m; k++) // seek to mincount
							if(counts[i][k]==minCounts[i])
								break;
						counts[i][k] = tempCounts[j];
						cooccurs[i][k] = j;

						// update mincount
						minCounts[i] = counts[i][0];
						for(k=1; k<m; k++) // seek to mincount
							if(counts[i][k]<minCounts[i])
								minCounts[i] = counts[i][k];
					}
				}
			}

			// Sort top-m co-occurrences descending on counts
			ArrayUtils::BSortDesc<uint32>(counts[i], m, cooccurs[i]);
		}
		delete[] values;
	}
	printf_s("done\n\n");

#if 0
	// Output ISC-file with used associations
	uint32 maxNum = alphSize * m;
	ItemSet **iss = new ItemSet *[maxNum];
	uint32 num = 0;
	ItemSetType itype = trainDB->GetDataType();

	for(uint32 j=0; j<alphSize; j++) {
		for(uint32 k=0; k<m; k++) {
			if(counts[j][k] == 0)
				break; // no more cooccurs for this singleton
			iss[num] = ItemSet::CreateSingleton(itype, j, alphSize);
			iss[num]->AddItemToSet(cooccurs[j][k]);
			iss[num++]->SetSupport(counts[j][k]);
		}
	}

	ItemSetCollection *isc = new ItemSetCollection(trainDB);
	isc->SetLoadedItemSets(iss, num);
	isc->SortLoaded(LengthDescIscOrder);
	
	ItemSet **sets = isc->GetLoadedItemSets();
	iss = new ItemSet *[num];
	uint32 newNum = 0;
	iss[newNum++] = sets[0]->Clone();
	for(uint32 i=1; i<num; i++)
		if(!sets[i]->Equals(sets[i-1]))
			iss[newNum++] = sets[i]->Clone();
	delete isc;

	isc = new ItemSetCollection(trainDB);
	isc->SetIscOrder(LengthDescIscOrder);
	isc->SetLoadedItemSets(iss, newNum);
	isc->DetermineMaxSetLength();
	isc->DetermineMinSupport();
	ItemSetCollection::WriteItemSetCollection(isc, "pairs", Bass::GetWorkingDir());
#endif

	// Stats
	double precision1 = 0.0, precision3 = 0.0, precision5 = 0.0;
	double success3 = 0.0, success5 = 0.0, mrr = 0.0;
	Timer *timer2 = new Timer();

	// Open recommendation csv
	FILE *recomFP = NULL;
	if(saveRecommendations) {
		s = Bass::GetWorkingDir() + "recom-" + TimeUtils::GetTimeStampString() + ".csv";
		fopen_s(&recomFP, s.c_str(), "w");
		fprintf_s(recomFP, "inputTags;testTags;recom;correct;P@1;P@3;P@5;S@3;S@5;MRR\n"); 
	}

	// For each test row
	printf_s("Recommending");
	uint32 valuesLength = 2 * testDB->GetMaxSetLength();
	uint16 *values = new uint16[valuesLength];
	uint32 numRowsTested = 0;
	uint32 minRowLen = numInput==0 ? 2 : numInput+min(numRecommend,5); // half/half-->min 2, other-->numIn+atleast(5)
	uint32 numActualInput;
	for(uint32 r=0; r<testDB->GetNumRows(); r++) {
		ItemSet *row = testDB->GetRow(r);
		if(row->GetLength() < minRowLen)		
			continue;	// skip if too short for testing
		++numRowsTested;

		// Generate input and test sets
		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		ItemSet *inputTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		ItemSet *testTags = ItemSet::CreateEmpty(testDB->GetDataType(), testDB->GetAlphabetSize());
		uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
		numActualInput = numInput==0 ? rowLen/2 : numInput;
		for(uint32 i=0; i<numActualInput; i++)
			inputTags->AddItemToSet(values[random[i]]);
		for(uint32 i=numActualInput; i<rowLen; i++)
			testTags->AddItemToSet(values[random[i]]);
		delete[] random;

		// Generate recommendation
		inputTags->GetValuesIn(values);
		ItemSet *candidates = ItemSet::CreateEmpty(trainDB->GetDataType(), alphSize);

		// Build itemset containing all candidate items that will appear in the ranking
		for(uint32 i=0; i<inputTags->GetLength(); i++) {
			uint16 item = values[i];
			for(uint32 j=0; j<m && counts[item][j]>0; j++)
				candidates->AddItemToSet((uint16)cooccurs[item][j]);
		}
		candidates->Remove(inputTags);
		
		// Build recommendation item array
		uint32 numRecom = candidates->GetLength();
		uint32 *recommended = new uint32[numRecom];
		candidates->GetValuesIn(recommended);
		
		// Score items
		double *scores = new double[numRecom];
		for(uint32 i=0; i<numRecom; i++)
			scores[i] = 0.0;
		if(usePromotion) {				// -- With promotion
			for(uint32 i=0; i<inputTags->GetLength(); i++) {
				uint16 userItem = values[i];
				double stability = promoStability / (promoStability + abs(promoStability - log10((double)alphCounts[userItem])));
				for(uint32 j=0; j<m && counts[userItem][j]>0; j++) {
					uint16 candItem = cooccurs[userItem][j];
					if(!inputTags->IsItemInSet(candItem)) {
						uint32 idx = ArrayUtils::BinarySearchAsc<uint32>(candItem, recommended, numRecom);
						double promo = stability;																									// promotion=stability
						promo *= promoDescriptiveness / (promoDescriptiveness + abs(promoDescriptiveness - log10((double)alphCounts[candItem])));		// * descriptiveness
						promo *= promoRank / (promoRank + j); // assume that cooccurs ordered by counts desc											// * rank
						if(useVoting) {				// -- Voting
							scores[idx] += promo;
						} else {					// -- Summing
							scores[idx] += (counts[userItem][j] / (double)alphCounts[userItem]) * promo;
						}
					}
				}
			}

		} else {						// -- Without promotion (code duplication, but faster)
			for(uint32 i=0; i<inputTags->GetLength(); i++) {
				uint16 userItem = values[i];
				for(uint32 j=0; j<m && counts[userItem][j]>0; j++) {
					uint16 candItem = cooccurs[userItem][j];
					if(!inputTags->IsItemInSet(candItem)) {
						uint32 idx = ArrayUtils::BinarySearchAsc<uint32>(candItem, recommended, numRecom);
						if(useVoting) {				// -- Voting
							scores[idx] += 1.0;
						} else {					// -- Summing
							scores[idx] += counts[userItem][j] / (double)alphCounts[userItem];
						}
					}
				}
			}
		}

		// Rank items according to scores
		ArrayUtils::BSortDesc<double>(scores, numRecom, recommended);

		// Test recommendations found
		if(numRecom != 0) {
#ifdef _DEBUG
			s = "";
			for(uint32 i=0; i<numRecom; i++) {
				sprintf_s(temp, 500, "%u ", recommended[i]);
				s += temp;
			}
			printf_s("** Recom: %s\n", s.c_str());
#endif
			
			ItemSet *correctTags = candidates->Intersection(testTags);
			{
				// Compute stats
				uint32 firstCorrect = UINT32_MAX_VALUE;
				for(uint32 i=0; i<numRecom; i++) {
					if(correctTags->IsItemInSet(recommended[i])) {
						if(firstCorrect == UINT32_MAX_VALUE)
							firstCorrect = i;
						if(i<5) {
							precision5 += 0.2;
							if(i<3) {
								precision3 += 0.3333333333333333;
								if(i==0)
									precision1 += 1.0;
							}
						}
					}
				}
				if(firstCorrect < UINT32_MAX_VALUE) {
					mrr += 1.0 / (firstCorrect+1);
					if(firstCorrect < 5) {
						success5 += 1.0;
						if(firstCorrect < 3)
							success3 += 1.0;
					}
				}

				// Save to file, if asked for
				if(saveRecommendations) {
					s = "";
					for(uint32 i=0; i<numRecom; i++) {
						sprintf_s(temp, 500, "%u ", recommended[i]);
						s += temp;
					}
					fprintf_s(recomFP, "%s;%s;%s;%s;%f;%f;%f;%u;%u;%f\n", 
						inputTags->ToString(false).c_str(), testTags->ToString(false).c_str(), s.c_str(), correctTags->ToString(false).c_str(),
						precision1, precision3, precision5,
						firstCorrect<3?1:0,	// S@3
						firstCorrect<5?1:0,	// S@5
						firstCorrect<UINT32_MAX_VALUE ? (1.0 / (firstCorrect+1)) : 0.0 // MRR
						);
				}
			}
			
			// Cleanup a bit
			delete correctTags;
		} else if(saveRecommendations) {
			fprintf_s(recomFP, "%s;%s;None found!;;%f;%f;%f;0;0;0.0;-\n", inputTags->ToString(false).c_str(), testTags->ToString(false).c_str(),
						precision1, precision3, precision5);
		}

		// Cleanup
		delete[] recommended;
		delete[] scores;
		delete candidates;
		delete inputTags;
		delete testTags;

		if((r+1) % 100 == 0)
			printf_s(".");
	}
	printf_s("\nFinished recommendation.\n\n");

	// Close recommendations file
	if(saveRecommendations) {
		fclose(recomFP);
		recomFP = NULL;
	}

	// Stats
	precision1 /= numRowsTested;	precision3 /= numRowsTested;	precision5 /= numRowsTested;
	success3 /= numRowsTested;		success5 /= numRowsTested;		mrr /= numRowsTested;
	if(numRecommend < 5) {
		precision5 = 0.0; success5 = 0.0;
		if(numRecommend < 3)
			precision3 = 0.0; success3 = 0.0;
	}
	sprintf_s(temp, 500, "\n\n** Results **\n"); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- #tested tagsets: %u (%u skipped)", numRowsTested, testDB->GetNumRows()-numRowsTested); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Precision:\n\tP@1\t\t%f\n\tP@3\t\t%f\n\tP@5\t\t%f", precision1, precision3, precision5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Success:\n\tS@3\t\t%f\n\tS@5\t\t%f", success3, success5); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- MRR:\t\t%f", mrr); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Save to results csv
	s = Bass::GetWorkingDir() + "results-" + TimeUtils::GetTimeStampString() + ".csv";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");
	fprintf_s(fp, "trainDB;%s\n", trainDB->GetFilename().c_str());
	fprintf_s(fp, "testDB;%s\n", testDB->GetFilename().c_str());
	fprintf_s(fp, "numInput;m;strategy;promotion;promoS;promoD;promoR;numRecommend;rowsTested;rowsSkipped;P@1;P@3;P@5;S@3;S@5;MRR;time_all (s);time_recom (s)\n");
	fprintf_s(fp, "%u;%u;%s;%s;%u;%u;%u;%u;%u;%u;%f;%f;%f;%f;%f;%f;%u;%u\n", numInput, m, useVoting?"vote":"sum", usePromotion?"yes":"no", usePromotion?promoStability:0, usePromotion?promoDescriptiveness:0, usePromotion?promoRank:0, numRecommend, numRowsTested, testDB->GetNumRows()-numRowsTested, 
								precision1, precision3, precision5, success3, success5, mrr, timer1->GetElapsedTime(), timer2->GetElapsedTime());
	fclose(fp);

	// Cleanup
	delete timer1;
	delete timer2;
	for(uint32 i=0; i<alphSize; i++) {
		delete[] counts[i];
		delete[] cooccurs[i];
	}
	delete[] counts;
	delete[] cooccurs;
	delete[] minCounts;
	delete[] tempCounts;
	delete[] alphCounts;
	delete[] alphSizeZeroes;
	delete[] values;
	delete testDB;
	delete trainDB;
}

void TagRecomTH::ConvertToLatre() {
	string s; char temp[500];

	// Settings
	uint32 numInput = mConfig->Read<uint32>("numInput");
	uint32 numRecommend = mConfig->Read<uint32>("numRecommend");
	uint32 numTotal = numInput + numRecommend;
	uint32 seed = mConfig->Read<uint32>("seed", 0);

	// Log settings
	sprintf_s(temp, 500, "\n** Settings **\n"); s=temp; LOG_MSG(s);
	if(numInput==0) {
		sprintf_s(temp, 500, "- #input tags: -half-"); s=temp; LOG_MSG(s);
	} else {
		sprintf_s(temp, 500, "- #input tags: %u", numInput); s=temp; LOG_MSG(s);
	}
	sprintf_s(temp, 500, "- #tags to recommend: %u", numRecommend); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Train database: %s", mConfig->Read<string>("traindb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Test database: %s", mConfig->Read<string>("testdb").c_str()); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "- Seed: %u", seed); s=temp; LOG_MSG(s);
	sprintf_s(temp, 500, "\n"); s=temp; LOG_MSG(s);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	if(mConfig->KeyExists("testdb")) {
		// Load test database
		Database *testDB = Database::RetrieveDatabase(mConfig->Read<string>("testdb"));

		// Open .lac test database
		s = Bass::GetWorkingDir() + mConfig->Read<string>("testdb") + ".lac";
		FILE *fp;
		fopen_s(&fp, s.c_str(), "w");

		// For each test row
		printf_s("Converting test DB");
		uint16 *values = new uint16[testDB->GetMaxSetLength()];
		uint32 numRowsTested = 0;
		uint32 minRowLen = numInput==0 ? 2*min(numRecommend,5) : numInput+min(numRecommend,5); // p@5 hardcoded here.
		uint32 splitPoint;
		for(uint32 r=0; r<testDB->GetNumRows(); r++) {
			ItemSet *row = testDB->GetRow(r);
			if(row->GetLength() < minRowLen)		
				continue;	// skip if too short for testing

			// Write to LATRE .lac file
			// 0 CLASS=961 CLASS=3228 w=6338 w=4032
			// Line #, classes are test, w is input
			fprintf_s(fp, "%u", numRowsTested);

			// Generate input and test sets
			uint32 rowLen = row->GetLength();
			row->GetValuesIn(values);
			uint32 *random = RandomUtils::GeneratePermutedList(0, rowLen);
			splitPoint = numInput==0 ? rowLen/2 : numInput;
			for(uint32 i=splitPoint; i<rowLen; i++)
				fprintf_s(fp, " CLASS=%u", values[random[i]]);
			for(uint32 i=0; i<splitPoint; i++)
				fprintf_s(fp, " w=%u", values[random[i]]);
			delete[] random;

			fprintf_s(fp, "\n");

			++numRowsTested;
		}
		printf_s("\nFinished writing test DB.\n\n");
		fclose(fp);
		delete[] values;
		delete testDB;
	}

	if(mConfig->KeyExists("traindb")) {
		// Load train database
		Database *trainDb = Database::RetrieveDatabase(mConfig->Read<string>("traindb"));

		// Open .lac train database
		s = Bass::GetWorkingDir() + mConfig->Read<string>("traindb") + ".lac";
		FILE *fp;
		fopen_s(&fp, s.c_str(), "w");

		// For each test row
		printf_s("Converting train DB");
		uint16 *values = new uint16[trainDb->GetMaxSetLength()];
		for(uint32 r=0; r<trainDb->GetNumRows(); r++) {
			ItemSet *row = trainDb->GetRow(r);

			// Write to LATRE .lac file
			// 2 CLASS=6386 CLASS=3245 CLASS=3552 CLASS=2394 CLASS=1459 w=6386 w=3245 w=3552 w=2394 w=1459
			// Line #, all tags twice
			fprintf_s(fp, "%u", r);

			uint32 rowLen = row->GetLength();
			row->GetValuesIn(values);
			for(uint32 i=0; i<rowLen; i++)
				fprintf_s(fp, " CLASS=%u", values[i]);
			for(uint32 i=0; i<rowLen; i++)
				fprintf_s(fp, " w=%u", values[i]);

			fprintf_s(fp, "\n");
		}
		printf_s("\nFinished writing train DB.\n\n");
		fclose(fp);

		delete trainDb;
		delete[] values;
	}
}
void TagRecomTH::ConvertGroupsToLatre() {
	char temp[500];

	// Settings
	string dbDir = mConfig->Read<string>("dbdir");
	uint32 numGroups = mConfig->Read<uint32>("numGroups");
	uint32 seed = mConfig->Read<uint32>("seed", 0);

	// Seed
	if(seed==0)
		RandomUtils::Init();
	else
		RandomUtils::Init(seed);

	ConvertTrainDbToLatre("rest", dbDir);
	for(uint32 i=0; i<numGroups; i++) {
		sprintf_s(temp, 500, "group%u", i+1);
		ConvertTrainDbToLatre(temp, dbDir);
	}
}

void TagRecomTH::ConvertTrainDbToLatre(string dbName, string path) {
	string s;

	// Load train database
	Database *trainDb = Database::ReadDatabase(dbName, path);

	// Open .lac train database
	s = Bass::GetWorkingDir() + dbName + ".lac";
	FILE *fp;
	fopen_s(&fp, s.c_str(), "w");

	// For each test row
	printf_s("Converting training DB '%s'", dbName.c_str());
	uint16 *values = new uint16[trainDb->GetMaxSetLength()];
	for(uint32 r=0; r<trainDb->GetNumRows(); r++) {
		ItemSet *row = trainDb->GetRow(r);

		// Write to LATRE .lac file
		// 2 CLASS=6386 CLASS=3245 CLASS=3552 CLASS=2394 CLASS=1459 w=6386 w=3245 w=3552 w=2394 w=1459
		// Line #, all tags twice
		fprintf_s(fp, "%u", r);

		uint32 rowLen = row->GetLength();
		row->GetValuesIn(values);
		for(uint32 i=0; i<rowLen; i++)
			fprintf_s(fp, " CLASS=%u", values[i]);
		for(uint32 i=0; i<rowLen; i++)
			fprintf_s(fp, " w=%u", values[i]);

		fprintf_s(fp, "\n");
	}
	printf_s("\nFinished writing .lac file.\n\n");
	fclose(fp);

	delete trainDb;
	delete[] values;
}

#endif // BLOCK_TAGRECOM