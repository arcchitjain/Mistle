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
#include "../Bass.h"
#include "../itemstructs/ItemSet.h"
#include "../isc/ItemTranslator.h"

#include "ClassedDatabase.h"

ClassedDatabase::ClassedDatabase() : Database() {
	mClassLabels = NULL;
	mClassSupports = NULL;
}

ClassedDatabase::ClassedDatabase(ItemSet **itemsets, uint32 numRows, bool binSizes, uint32 numTransactions, uint64 numItems) 
	: Database(itemsets, numRows, binSizes, numTransactions, numItems) {
	mClassLabels = NULL;
	mClassSupports = NULL;
}

ClassedDatabase::ClassedDatabase(Database *db, bool extractClassLabels /* = true */) : Database(db) {
	mClassSupports = NULL;
	mClassLabels = NULL;

	if(extractClassLabels)
		ExtractClassLabels();
}

ClassedDatabase::ClassedDatabase(ClassedDatabase *db) : Database(db) {
	mClassLabels = new uint16[mNumRows];
	memcpy(mClassLabels, db->GetClassLabels(), sizeof(uint16) * mNumRows);
	mClassSupports = new uint32[mNumClasses];
	memcpy(mClassSupports, db->mClassSupports, sizeof(uint32) * mNumClasses);
}

ClassedDatabase::~ClassedDatabase() {
	delete[] mClassLabels;
	delete[] mClassSupports;
}
void ClassedDatabase::SetClassLabels(uint16 *classLabels) {
	if(mClassLabels != NULL)
		throw "ClassedDatabase::SetClasslabels() -- Cannot reset class info.";
	if(mNumRows == 0)
		throw "ClassedDatabase::SetClasslabels() -- Set NumRows before setting class labels.";

	mClassLabels = classLabels;
	mClassSupports = new uint32[mNumClasses];
	for(uint32 j=0; j<mNumClasses; j++)
		mClassSupports[j] = 0;
	for(uint32 i=0; i<mNumRows; i++)
		for(uint32 j=0; j<mNumClasses; j++)
			if(mClassLabels[i] == mClassDefinition[j]) {
				mClassSupports[j]++;
				break;
			}
}
void ClassedDatabase::SwapRows(uint32 row1, uint32 row2) {
	ItemSet *is = mRows[row1];
	mRows[row1] = mRows[row2];
	mRows[row2] = is;

	uint16 cl = mClassLabels[row1];
	mClassLabels[row1] = mClassLabels[row2];
	mClassLabels[row2] = cl;
}

Database** ClassedDatabase::SplitOnClassLabels() {
	Database **dbs = new Database *[mNumClasses];
	ItemSet **iss;
	uint32 classSize, idx;

	for(uint32 c=0; c<mNumClasses; c++) {
		classSize = mClassSupports[c];
		iss = new ItemSet *[classSize];
		idx = 0;

		for(uint32 i=0; i<mNumRows; i++)
			if(mClassLabels[i] == mClassDefinition[c])
				iss[idx++] = mRows[i]->Clone();

		Database *db = new Database(iss, classSize, mHasBinSizes);
		db->UseAlphabetFrom(this);
		db->CountAlphabet();
		db->ComputeEssentials();
		dbs[c] = db;
	}

	return dbs;
}
Database **ClassedDatabase::SplitOnItemSet(ItemSet *is) {
	uint8 outbak = Bass::GetOutputLevel(); Bass::SetOutputLevel(0);

	bool *mask = new bool[mNumRows];
	uint32 numPos = 0;

	for(uint32 i=0; i<mNumRows; i++) {
		if(mRows[i]->IsSubset(is)) {
			mask[i] = true;
			numPos++;
		} else {
			mask[i] = false;
		}
	}

	uint32 numNeg = mNumRows - numPos;
	ItemSet **issPos = new ItemSet *[numPos];
	ItemSet **issNeg = new ItemSet *[numNeg];
	uint16 *clsPos = new uint16[numPos];
	uint16 *clsNeg = new uint16[numNeg];

	uint32 idxPos = 0, idxNeg = 0;
	for(uint32 i=0; i<mNumRows; i++) {
		if(mask[i]) {
			issPos[idxPos] = mRows[i]->Clone();
			clsPos[idxPos++] = mClassLabels[i];
		} else {
			issNeg[idxNeg] = mRows[i]->Clone();
			clsNeg[idxNeg++] = mClassLabels[i];
		}
	}
	delete[] mask;

	ClassedDatabase **dbs = new ClassedDatabase *[2];

	dbs[0] = new ClassedDatabase(issNeg, numNeg, mHasBinSizes);
	dbs[0]->SetAlphabet(new alphabet(*mAlphabet));
	dbs[0]->SetItemTranslator(new ItemTranslator(mItemTranslator));
	dbs[0]->CountAlphabet();
	dbs[0]->ComputeEssentials();
	{
		uint16 *cls = new uint16[mNumClasses];
		memcpy(cls, mClassDefinition, sizeof(uint16)*mNumClasses);
		dbs[0]->SetClassDefinition(mNumClasses, cls);
	}
	dbs[0]->SetClassLabels(clsNeg);

	dbs[1] = new ClassedDatabase(issPos, numPos, mHasBinSizes);
	dbs[1]->SetAlphabet(new alphabet(*mAlphabet));
	dbs[1]->SetItemTranslator(new ItemTranslator(mItemTranslator));
	dbs[1]->CountAlphabet();
	dbs[1]->ComputeEssentials();
	{
		uint16 *cls = new uint16[mNumClasses];
		memcpy(cls, mClassDefinition, sizeof(uint16)*mNumClasses);
		dbs[1]->SetClassDefinition(mNumClasses, cls);
	}
	dbs[1]->SetClassLabels(clsPos);

	Bass::SetOutputLevel(outbak);
	return (Database**)dbs;
}
void ClassedDatabase::ExtractClassLabels() {
	mClassSupports = new uint32[mNumClasses];
	mClassLabels = new uint16[mNumRows];

	for(uint32 c=0; c<mNumClasses; c++) {
		mClassSupports[c] = 0;
		uint16 curClass = mClassDefinition[c];
		for(uint32 i=0; i<mNumRows; i++) {
			if(mRows[i]->IsItemInSet(curClass)) {
				mRows[i]->RemoveItemFromSet(curClass);
				mClassLabels[i] = curClass;
				mClassSupports[c]++;
			}
		}
	}
	CountAlphabet();
	ComputeEssentials();
}
