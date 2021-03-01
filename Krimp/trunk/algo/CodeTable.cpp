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
#include "../global.h"

#include <logger/Log.h>

#include "CodeTable.h"

#include <db/Database.h>
#include <isc/ItemSetCollection.h>
#include <itemstructs/ItemSet.h>

#include "CTFile.h"
#include "coverfull/CFCodeTable.h"
#include "coverpartial/CPCodeTable.h"
#include "coverpartial/CPACodeTable.h"
#include "coverpartial/CCPCodeTable.h"		// jilles original cccp
#include "coverpartial/CCCPCodeTable.h"		// sanderised ccp
#include "coverpartial/CPOCodeTable.h"
#include "coverpartial/CPROCodeTable.h"

CodeTable::CodeTable() {
	mDB = NULL;
	mStdLengths = NULL;
	mLaplace = 0;
	mSTSpecialCode = false;

	mCTLogFile = NULL;
	mCoverSetType = NoneItemSetType;

	mCurStats.usgCountSum = 0;
	mCurStats.encDbSize = 0;
	mCurStats.encCTSize = 0;
	mCurStats.encSize = 0;
	mCurStats.numSetsUsed = 0;
	mCurStats.numCandidates = 0;
	mCurStats.alphItemsUsed = 0;
	mPrunedStats = mPrevStats = mCurStats;

	mCurNumSets = 0;
}

CodeTable::CodeTable(const CodeTable &ct) {
	mDB = ct.mDB;
	mStdLengths = ct.mStdLengths;
	mLaplace = ct.mLaplace;
	mSTSpecialCode = ct.mSTSpecialCode;
	mCTLogFile = ct.mCTLogFile;
	mCoverSetType = ct.mCoverSetType;
	mCurStats = ct.mCurStats;
	mPrevStats = ct.mPrevStats;
	mPrunedStats = ct.mPrunedStats;
	mCurNumSets = ct.mCurNumSets;
}

CodeTable::~CodeTable() {
}

void CodeTable::UseThisStuff(Database *db, ItemSetType type, CTInitType ctinit, uint32 maxCTElemLength, uint32 toMinSup) { 
	mDB = db;
	mStdLengths = mDB->GetStdLengths();
	mCoverSetType = type;
}
void CodeTable::SetDatabase(Database *db) {
	mDB = db;
}
void CodeTable::SetCTLogFile(FILE* ctLogFile) {
	mCTLogFile = ctLogFile;
}
void CodeTable::BackupStats() {
	mPrevStats = mCurStats;
}
void CodeTable::RollbackStats() {
	mCurStats = mPrevStats;
}
ItemSet **CodeTable::GetItemSetArray() {
	// naive impl
	ItemSet **ctElems = new ItemSet *[mCurNumSets];
	islist *isList = GetItemSetList();
	{
		islist::iterator iter;
		uint32 idx = 0;
		for(iter=isList->begin(); iter!=isList->end(); ++iter, ++idx)
			ctElems[idx] = (ItemSet*)(*iter);
	}
	delete isList;
	return ctElems;
}
ItemSet **CodeTable::GetSingletonArray() {
	// naive impl
	ItemSet **alphElems = new ItemSet *[mDB->GetAlphabetSize()];
	islist *isList = GetSingletonList();
	{
		islist::iterator iter;
		uint32 idx = 0;
		for(iter=isList->begin(); iter!=isList->end(); ++iter, ++idx)
			alphElems[idx] = (ItemSet*)(*iter);
	}
	delete isList;
	return alphElems;
}

CodeTable* CodeTable::Create(const string &name, ItemSetType dataType) {
	if(name.compare(CFCodeTable::GetConfigBaseName()) == 0)			// coverfull
		return new CFCodeTable();
	else if(name.compare(CPCodeTable::GetConfigBaseName()) == 0) {	// coverpartial defaults naar cccoverpartial
		//return new CCPCodeTable();

		if (dataType == BM128ItemSetType) {
			return new CCCPCodeTable<BM128ItemSetType>();
		} else if (dataType == BAI32ItemSetType) {
			return new CCCPCodeTable<BAI32ItemSetType>();
		} else if (dataType == Uint16ItemSetType) {
			return new CCCPCodeTable<Uint16ItemSetType>();
		} else {
			return NULL;
		}

	} else if(name.compare(CCPCodeTable::GetConfigBaseName()) == 0)	// ccoverpartial
		return new CCPCodeTable();
	else if(name.compare(CCCPCodeTable<BM128ItemSetType>::GetConfigBaseName()) == 0) {	// cccoverpartial
		if (dataType == BM128ItemSetType) {
			return new CCCPCodeTable<BM128ItemSetType>();
		} else if (dataType == BAI32ItemSetType) {
			return new CCCPCodeTable<BAI32ItemSetType>();
		} else if (dataType == Uint16ItemSetType) {
			return new CCCPCodeTable<Uint16ItemSetType>();
		} else {
			return NULL;
		}
	}
	else if(name.compare("coverpartial-basic") == 0)				// coverpartial
		return new CPCodeTable();
	else if(name.compare(CPACodeTable::GetConfigBaseName()) == 0)	// coverpartial-alternative
		return new CPACodeTable();

	else if(name.compare(0,CPOCodeTable::GetConfigBaseName().size(),CPOCodeTable::GetConfigBaseName()) == 0)
		return new CPOCodeTable(name);
	else if(name.compare(0,CPROCodeTable::GetConfigBaseName().size(),CPROCodeTable::GetConfigBaseName()) == 0)
		return new CPROCodeTable(name);
	throw string(__FUNCTION__) + " - Unable to create codetable for type " + name;

/// sanderstuff

//	else if(name.compare(ParallelCFCodeTable::GetConfigBaseName()) == 0)
//		return new ParallelCFCodeTable();
//	else if(name.compare(ParallelCCCPCodeTable::GetConfigBaseName()) == 0)
//		return new ParallelCCCPCodeTable();
//	else if(name.compare(ParallelCCCPCodeTableCC<BM128ItemSetType>::GetConfigBaseName()) == 0)
//		if (type == BM128ItemSetType) {
//			return new ParallelCCCPCodeTableCC<BM128ItemSetType>();
//		} else if (type == BAI32ItemSetType) {
//			return new ParallelCCCPCodeTableCC<BAI32ItemSetType>();
//		} else if (type == Uint16ItemSetType) {
//			return new ParallelCCCPCodeTableCC<Uint16ItemSetType>();
//		} else {
//			return NULL;
//		}
	// wat is dynamisch ookalweer?
//	else if(name.compare(Parallel2DynCCCPCodeTableCC<BM128ItemSetType>::GetConfigBaseName()) == 0)
//		if (type == BM128ItemSetType) {
//			return new Parallel2DynCCCPCodeTableCC<BM128ItemSetType>();
//		} else if (type == BAI32ItemSetType) {
//			return new Parallel2DynCCCPCodeTableCC<BAI32ItemSetType>();
//		} else if (type == Uint16ItemSetType) {
//			return new Parallel2DynCCCPCodeTableCC<Uint16ItemSetType>();
//		} else {
//			return NULL;
//		}
}

CodeTable* CodeTable::CreateCTForClassification(const string &name, ItemSetType dataType) {
	if(name.find("coverpartial-reorderly") != string::npos)
		return new CPROCodeTable(name);
	else
		return new CFCodeTable();
	throw "CodeTable::CreateCTForClassification -- Unable to create codetable for type " + name;
}

CodeTable *CodeTable::LoadCodeTable(const string &filename, Database *db, string type) {
	if(!FileUtils::FileExists(filename))
		return NULL;
	ENTER_LEVEL();
	//LOG_MSG(string("Loading CT: ") + filename);
	printf("Loading CT: %s\n", filename.c_str());
	LEAVE_LEVEL();
	CodeTable *ct = CodeTable::Create(type, db->GetDataType());
	ct->UseThisStuff(db, db->GetDataType(), InitCTEmpty);
	ct->ReadFromDisk(filename, true);
	return ct;
}

void CodeTable::ReadFromDisk(const string &filename, const bool needFreqs) {
	CTFile *ctFile = new CTFile(filename, mDB);
	ctFile->SetNeedSupports(needFreqs);

	mCurStats.usgCountSum = 0;

	// ItemSets
	ItemSet *is = NULL;
	uint32 num = ctFile->GetNumSets();
	for(uint32 i=0; i<num; i++) {
		is = ctFile->ReadNextItemSet();
		mCurStats.usgCountSum += is->GetUsageCount();
		Add(is);
	}
	if(num > 0)
		CommitAdd(false);

	// Alphabet
	num = ctFile->GetAlphLen();
	uint32 val[1];
	for(uint32 i=0; i<num; i++) {
		is = ctFile->ReadNextItemSet();
		mCurStats.usgCountSum += is->GetUsageCount();
		is->GetValuesIn(val); // only 1 value
		SetAlphabetCount(val[0], is->GetUsageCount());
		delete is;
	}

	ctFile->Close();
	delete ctFile;
}
void CodeTable::WriteToDisk(const string &filename) {
	FILE *file;
	if(fopen_s(&file, filename.c_str(), "w") != 0) {
		printf("Cannot open CodeTable file for writing!\nFilename: %s\n", filename.c_str());
		if(fopen_s(&file, filename.c_str(), "w") != 0) {
			printf("Second try failed as well..\n");
			return;
		}
	}
	alphabet *alph = mDB->GetAlphabet();
	uint32 alphLen = (uint32) alph->size();
	islist *ctlist = GetItemSetList();
	islist::iterator cit,cend=ctlist->end();
	bool hasBins = mDB->HasBinSizes();

	// Determine max set length
	uint32 maxSetLen = 1;
	for (cit = ctlist->begin(); cit != cend; ++cit)
		if((*cit)->GetLength() > maxSetLen)
			maxSetLen = (*cit)->GetLength();

	// Header
	fprintf(file, "ficct-1.0\n%d %d %d %d\n", mCurNumSets, alphLen, maxSetLen, hasBins?1:0);

	// Sets
	for (cit = ctlist->begin(); cit != cend; ++cit) {
		// print binned size
		//fprintf(file, "%s \n", ((ItemSet*)(*cit))->ToCodeTableString(hasBins).c_str());
		// don't print binned size
		fprintf(file, "%s \n", ((ItemSet*)(*cit))->ToCodeTableString().c_str());
	}
	delete ctlist;

	// Alphabet
	alphabet::iterator ait = alph->begin();
	for(uint32 i=0; i<alphLen; i++) {
		fprintf(file, "%d (%d,%d)\n", i, GetAlphabetCount(i), ait->second);
		ait++;
	}

	fclose(file);
}
void CodeTable::WriteStatsFile(Database *db, ItemSetCollection *isc, const string &outFile) {
	FILE *file = fopen(outFile.c_str(), "w");
	if(!file) {
		printf("Cannot open codetable stats file for writing!\nFilename: %s\n", outFile.c_str());
		return;
	}

	islist *ctlist = GetItemSetList();
	islist::iterator cit,cend=ctlist->end();
	ItemSet *is;
	ItemSet **collection = isc->GetLoadedItemSets();
	uint64 numCands = isc->GetNumItemSets();
	uint64 cand;

	// Write header
	fprintf(file, "ctIndex;candIndex;length;count;support;codeLength\n");

	// Foreach set in the code table
	uint64 countSum = 0;
	uint32 idx = 0, count;
	for(cit = ctlist->begin(); cit != cend; ++cit)
		countSum += ((ItemSet*)(*cit))->GetUsageCount();
	// countSum not complete! alphabet counts missing.

	for(cit = ctlist->begin(); cit != cend; ++cit, ++idx) {
		is = (ItemSet*)(*cit);
		count = is->GetUsageCount();
		for(uint64 i=0; i<numCands; i++)
			if(collection[(size_t)i]->Equals(is)) {
				cand = i;
				break;
			}
		// cand zapped
#if defined (_MSC_VER) && defined (_WINDOWS)
		fprintf(file, "%d;%I64d;%d;%d;%d;%.4f\n", idx, cand, is->GetLength(), count, is->GetSupport(), count==0?0.0:CalcCodeLength(count, countSum));
#elif defined (__GNUC__) && defined (__unix__)
		fprintf(file, "%d;%lu;%d;%d;%d;%.4f\n", idx, cand, is->GetLength(), count, is->GetSupport(), count==0?0.0:CalcCodeLength(count, countSum));
#endif
	}

	delete ctlist;
	fclose(file);
}

void CodeTable::WriteComparisonFile(CodeTable *compareTo, const string &name1, const string &name2, const string &outFile) {
	FILE *file = fopen(outFile.c_str(), "w");
	if(!file) {
		printf("Cannot open codetable stats file for writing!\nFilename: %s\n", outFile.c_str());
		return;
	}

	islist *list1 = GetItemSetList();
	islist *list2 = compareTo->GetItemSetList();
	islist::iterator cit1,cit2,cend1=list1->end(),cend2=list2->end();

	fprintf(file, "** Comparison of code tables:\n\t%s\n\t\t&\n\t%s\n\n", name1.c_str(), name2.c_str());

	// Intersection
	uint32 numIntersect = 0, cnt1, cnt2;
	fprintf(file, "-- Intersection\n\n");
	for(cit1 = list1->begin(); cit1!=cend1; cit1++)
		for(cit2 = list2->begin(); cit2!=cend2; cit2++)
			if((*cit1)->Equals(*cit2)) {
				cnt1 = (*cit1)->GetUsageCount();
				cnt2 = (*cit2)->GetUsageCount();
				fprintf(file, "%d %s %d\t:\t%s\n", cnt1, cnt1==cnt2?"=":(cnt1>cnt2?">":"<"), cnt2, (*cit1)->ToString(false, true).c_str());
				++numIntersect;
				break;
			}

	// Diff 1 \ 2
	uint32 numDiff1min2 = 0;
	bool found;
	fprintf(file, "\n\n-- Difference %s \\ %s\n\n", name1.c_str(), name2.c_str());
	for(cit1 = list1->begin(); cit1!=cend1; cit1++) {
		found = false;
		for(cit2 = list2->begin(); cit2!=cend2; cit2++) {
			if((*cit1)->Equals(*cit2)) {
				found = true;
				break;
			}
		}
		if(!found) {
			numDiff1min2++;
			fprintf(file, "%s\n", (*cit1)->ToString(true, true).c_str());
		}
	}

	// Diff 2 \ 1
	uint32 numDiff2min1 = 0;
	fprintf(file, "\n\n-- Difference %s \\ %s\n\n", name2.c_str(), name1.c_str());
	for(cit2 = list2->begin(); cit2!=cend2; cit2++) {
		found = false;
		for(cit1 = list1->begin(); cit1!=cend1; cit1++) {
			if((*cit1)->Equals(*cit2)) {
				found = true;
				break;
			}
		}
		if(!found) {
			numDiff2min1++;
			fprintf(file, "%s\n", (*cit2)->ToString(true, true).c_str());
		}
	}

	// Alphabet
	fprintf(file, "\n\n-- Alphabet\n\n");
	uint32 abLen = (uint32) mDB->GetAlphabetSize();
	for(uint32 i=0; i<abLen; i++) {
		cnt1 = GetAlphabetCount(i);
		cnt2 = compareTo->GetAlphabetCount(i);
		fprintf(file, "%d:\t%5d %s %5d\n", i, cnt1, cnt1==cnt2?"=":(cnt1>cnt2?">":"<"), cnt2);
	}

	// Round up
	fprintf(file, "\n\n-- Round up\n\n");
	fprintf(file, "# sets: %d <> %d\n", mCurNumSets, compareTo->GetCurNumSets());
	fprintf(file, "Intersection: %d\n\n", numIntersect);
	fprintf(file, "%s \\ %s: %d\n", name1.c_str(), name2.c_str(), numDiff1min2);
	fprintf(file, "%s \\ %s: %d\n", name2.c_str(), name1.c_str(), numDiff2min1);
	fprintf(file, "\n\nThe End.\n\n");

	delete list1;
	delete list2;
	fclose(file);
}
void CodeTable::UpdateCTLog() {
	if(mCTLogFile != NULL)
		fprintf(mCTLogFile, "log\n");
}

void CodeTable::SortPostPruneList(islist *pruneList, const islist::iterator &after) {
	islist::iterator start, a, b, c, end;
	ItemSet *isB, *isC;

	start = after;
	if(after != pruneList->end())
		++start;

	a = start;
	end = pruneList->end();
	if(a == end || ++a == end) // check whether at least 2 items left, otherwise sorting is quite useless
		return;

	--end;
	bool changed = false;
	for(a=start; a!=end; ++a) {
		changed = false;
		for(c=b=a,++c, isB=(ItemSet*)(*b); b!=end; b=c, isB=isC, ++c) {
			isC = (ItemSet*)(*c);
			if(GetUsageCount(isB) > GetUsageCount(isC)) {
				changed = true;
				*b = isC;
				*c = isC = isB;
			}
		}
		if(!changed)
			break;
	}
}

