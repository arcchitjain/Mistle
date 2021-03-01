#include "../global.h"

#include <logger/Log.h>

#include "CodeTable.h"
#include <db/Database.h>
#include <itemstructs/ItemSet.h>

#include "StandardCodeTable.h"

StandardCodeTable::StandardCodeTable(uint32 numCounts) {
	mNumCounts = numCounts;
	mCounts = new uint32[mNumCounts];
	mCodeLengths = new double[mNumCounts];
	for(uint32 i=0; i<mNumCounts; i++) {
		mCounts[i] = 0;
		mCodeLengths[i] = 0.0;
	}
	mCountSum = 0;
}

StandardCodeTable::StandardCodeTable(Database *db) {
	alphabet *alph = db->GetAlphabet();
	mNumCounts = (uint32) alph->size();
	mCounts = new uint32[mNumCounts];
	mCodeLengths = new double[mNumCounts];

	uint32 idx = 0;
	mCountSum = 0;
	for(alphabet::iterator iter = alph->begin(); iter != alph->end(); iter++, idx++) {
		mCounts[idx] = iter->second;
		mCountSum += mCounts[idx];
	}
	if(idx != mNumCounts)
		throw string("StandardCodeTable -- Number of alphabet elements incorrect?!");

	UpdateCodeLengths();
}

StandardCodeTable::StandardCodeTable(uint32 *counts, uint32 numCounts) {
	mNumCounts = numCounts;
	mCounts = counts;
	mCodeLengths = new double[mNumCounts];

	mCountSum = 0;
	for(uint32 i=0; i<mNumCounts; i++)
		mCountSum += mCounts[i];

	UpdateCodeLengths();
}

StandardCodeTable::~StandardCodeTable() {
	delete[] mCounts;
	delete[] mCodeLengths;
}

void StandardCodeTable::AddLaplace() {
	for(uint32 i=0; i<mNumCounts; i++)
		++mCounts[i];
	mCountSum += mNumCounts;
	++mLaplace;
	UpdateCodeLengths();
}
void StandardCodeTable::RemoveLaplace() {
	if(mLaplace > 0) {
		for(uint32 i=0; i<mNumCounts; i++)
			--mCounts[i];
		mCountSum -= mNumCounts;
		--mLaplace;
		UpdateCodeLengths();
	}
}
double StandardCodeTable::GetCodeLength(uint32 singleton) {
	return mCodeLengths[singleton];
}
double StandardCodeTable::GetCodeLength(ItemSet *is) {
	uint16 *set = is->GetValues();
	double len = GetCodeLength(set, is->GetLength());
	delete[] set;
	return len;
}
double StandardCodeTable::GetCodeLength(uint16 *set, uint32 numItems) {
	double len = 0.0;
	for(uint32 i=0; i<numItems; i++)
		len += mCodeLengths[set[i]];
	return len;
}
void StandardCodeTable::UpdateCodeLengths() {
	for(uint32 i=0; i<mNumCounts; i++)
		mCodeLengths[i] = mCounts[i] == 0 ? 0.0 : CalcCodeLength(mCounts[i], mCountSum);
}
