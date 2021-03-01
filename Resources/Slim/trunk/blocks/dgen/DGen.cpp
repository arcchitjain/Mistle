#ifdef BLOCK_DATAGEN

#include "../../global.h"

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>

// Ct-inlees dingen
#include "../../algo/CTFile.h"
#include "../../algo/CodeTable.h"

#include "DGSet.h"
#include "DGen.h"

#include "ct/ColumnBasedDGen.h"
#include "ct/CTFilteringDGen.h"
#include "cover/CTElemHMMDGen.h"
//#include "ctree/CTreeDGen.h"

DGen::DGen() {
	mDB = NULL;

	mCodingSets = NULL;
	mNumCodingSets = 0;
	mCSABStart = 0;

	mColDef = NULL;
	mItemToCol = NULL;
	mNumCols = 0;
}

DGen::~DGen() {
	for(uint32 i=0; i<mNumCodingSets; i++)
		delete mCodingSets[i];
	delete[] mCodingSets;

	for(uint32 i=0; i<mNumCols; i++)
		delete mColDef[i];
	delete[] mColDef;
	delete[] mItemToCol;
}

void DGen::BuildCodingSets(const string &ctfname) {
	ItemSetType type = mDB->GetDataType();
	alphabet* a = mDB->GetAlphabet();
	uint32 ablen = (uint32) a->size();

	mNumCodingSets = ablen; //0
	uint32 curCodingSet = 0;
	if(!ctfname.empty()) {
		CTFile *ctFile = new CTFile(ctfname, mDB);
		uint32 numCTSets = ctFile->GetNumSets();
		mNumCodingSets += numCTSets;
		mCodingSets = new DGSet*[mNumCodingSets];
		ItemSet *is;
		for(uint32 i=0; i<numCTSets; i++) {
			is = ctFile->ReadNextItemSet();
			//is->SetCount(0);
			mCodingSets[curCodingSet] = new DGSet(is, curCodingSet+1);
			++curCodingSet;
		}

		mCSABStart = curCodingSet;

		uint32 numABItems = ctFile->GetAlphLen();
		if(numABItems != ablen)
			throw string("DGen::BuildCodingSets() -- Alphabet sizes in database an code table unequal.");
		//uint32 val[1];
		for(uint32 i=0; i<numABItems; i++) {
			is = ctFile->ReadNextItemSet();
			//is->GetValuesIn(val); // only 1 value
			mCodingSets[curCodingSet] = new DGSet(is, curCodingSet+1);
			++curCodingSet;
		}

		delete ctFile;
	} else  {
		mCodingSets = new DGSet*[mNumCodingSets];
		mCSABStart = curCodingSet;

		uint16 val = 0;
		uint32 cnt = 0;
		alphabet::iterator ait, aend=a->end();
		for(ait = a->begin(); ait != aend; ++ait) {
			val = ait->first;
			cnt = ait->second;
			mCodingSets[curCodingSet] = new DGSet(ItemSet::Create(type, &val, 1, ablen, cnt), curCodingSet+1);
			++curCodingSet;
		}

	}
}

void DGen::BuildCodingSets(CodeTable *ct) {
	ItemSetType type = mDB->GetDataType();
	alphabet* a = mDB->GetAlphabet();
	uint32 ablen = (uint32) a->size();

	mNumCodingSets = ablen; //0
	uint32 curCodingSet = 0;
	if(ct == NULL) {
		uint32 numCTSets = ct->GetCurNumSets();
		mNumCodingSets += numCTSets;
		mCodingSets = new DGSet*[mNumCodingSets];
		ItemSet *is;

		islist *ctList = ct->GetItemSetList();
		for(islist::iterator cti=ctList->begin(); cti!=ctList->end(); cti++) {
			is = (*cti);
			mCodingSets[curCodingSet] = new DGSet(is, curCodingSet+1);
			++curCodingSet;
		}

		mCSABStart = curCodingSet;

		uint32 numABItems = (uint32) mDB->GetAlphabetSize();
		if(numABItems != ablen)
			throw string("DGen::BuildCodingSets() -- Alphabet sizes in database an code table unequal.");

		uint16 val = 0;
		uint32 cnt = 0;
		alphabet::iterator ait, aend=a->end();
		for(ait = a->begin(); ait != aend; ++ait) {
			val = ait->first;
			cnt = ct->GetAlphabetCount(val);
			mCodingSets[curCodingSet] = new DGSet(ItemSet::Create(type, &val, 1, ablen, cnt), curCodingSet+1);
			++curCodingSet;
		}

	} else  {
		mCodingSets = new DGSet*[mNumCodingSets];
		mCSABStart = curCodingSet;

		uint16 val = 0;
		uint32 cnt = 0;
		alphabet::iterator ait, aend=a->end();
		for(ait = a->begin(); ait != aend; ++ait) {
			val = ait->first;
			cnt = ait->second;
			mCodingSets[curCodingSet] = new DGSet(ItemSet::Create(type, &val, 1, ablen, cnt), curCodingSet+1);
			++curCodingSet;
		}

	}
}


DGSet** DGen::GetCodingSets() {
	return mCodingSets;
}
uint32 DGen::GetNumCodingSets() {
	return mNumCodingSets;
}

void DGen::SetInputDatabase(Database *db) {
	mDB = db;
	mDataType = mDB->GetDataType();
	mAlphabetLen = (uint32) mDB->GetAlphabetSize();
}

void DGen::SetColumnDefinition(ItemSet **coldef, uint32 numColumns) {
	throw string("nee");

	if(mColDef != NULL) {
		for(uint32 i=0; i<mNumCols; i++)
			delete mColDef[i];
		delete[] mColDef;
	}
	mNumCols = numColumns;
	mColDef = new ItemSet*[mNumCols];
	for(uint32 i=0; i<mNumCols; i++) {
		mColDef[i] = coldef[i]->Clone();
	}
}

void DGen::SetColumnDefinition(string coldefstr) {
	// input string is column - comma separated, item - space separated
	size_t len = coldefstr.length();
	uint32 numcols = 1;
	uint32 i;
	for(i=0; i<len; i++) {
		if(coldefstr[i] == ',')
			numcols++;
	}
	if(mColDef != NULL) {
		for(uint32 i=0; i<mNumCols; i++)
			delete mColDef[i];
		delete[] mColDef;
	}
	mNumCols = numcols;
	mColDef = new ItemSet*[mNumCols];


	/* *** Build Column per Item *** */
	mItemToCol = new uint32[mDB->GetAlphabetSize()];

	i=0;
	uint32 item = 0;
	for(uint32 c=0; c<mNumCols; c++) {
		ItemSet *is = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());

		while(i<len && coldefstr[i] != ',') {		// while we're not finished defining this column
			item = atoi(coldefstr.c_str()+i);		// extract the items in this column
			mItemToCol[item] = c;
			is->AddItemToSet(item);
			while(i<len && coldefstr[i] != ' ' && coldefstr[i] != ',')	// find next item
				i++;
			if(i<len && coldefstr[i] == ' ' )
				i++;								// adjust for " "
		}
		if(i<len && coldefstr[i] == ',')
			i+=2;									// adjust for ", "
		mColDef[c] = is;
		//printf("%d : %s\n",c,is->ToString(false,false).c_str());
	}

	/*printf("ab->size(): %d\n",mDB->GetAlphabetSize());
	printf("mItemToCol [\n");
	for(uint32 i=0; i<mDB->GetAlphabetSize(); i++) {
		printf("%d -> %d,\n",i, mItemToCol[i]);
	}
	printf("]\n");*/
}


// preferred method, moet beter uitgewerkt worden, net als dat met codetable ook moet...
DGen* DGen::Create(string type) {
	if(type == ColumnBasedDGen::GetShortNameStatic())
		return new ColumnBasedDGen();
	else if(type == CTFilteringDGen::GetShortNameStatic())
		return new CTFilteringDGen();
	else if(type == CTElemHMMDGen::GetShortNameStatic())
		return new CTElemHMMDGen();
	//else if(type == CTreeDGen::GetShortNameStatic())
	//	return new CTreeDGen();
	throw string("Dunno DGen Type: ") + type;
}

#endif // BLOCK_DATAGEN
