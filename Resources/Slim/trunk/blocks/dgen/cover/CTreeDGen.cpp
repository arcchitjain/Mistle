#ifdef BLOCK_DATAGEN

#include "../../../global.h"

#include <Config.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>

#include "../../../algo/CTFile.h"
#include "CTreeSet.h"
#include "CTreeNode.h"

#include "../DGSet.h"
#include "../DGen.h"
#include "CTreeDGen.h"

CTreeDGen::CTreeDGen() {
	mRoot = new CTreeNode();
	mRootSet = NULL;
}

CTreeDGen::~CTreeDGen() {
	delete mRoot;
	delete mRootSet;
}


void CTreeDGen::BuildCodingSets(const string &ctfname) {
	alphabet *a = mDB->GetAlphabet();

	mNumCodingSets = mAlphabetLen; //0
	uint32 curCodingSet = 0;
	if(!ctfname.empty()) {
		CTFile *ctFile = new CTFile(ctfname, mDB);
		uint32 numCTSets = ctFile->GetNumSets();
		mNumCodingSets += numCTSets;
		mCodingSets = (DGSet**) new CTreeSet*[mNumCodingSets];	//mCodingSets = new DGSet*[mNumCodingSets];
		ItemSet *is;
		for(uint32 i=0; i<numCTSets; i++) {
			is = ctFile->ReadNextItemSet();
			//is->SetCount(0);		// in CTree wel op 0 ??
			mCodingSets[curCodingSet] = new CTreeSet(is, curCodingSet+1);
			++curCodingSet;
		}

		mCSABStart = curCodingSet;

		uint32 numABItems = ctFile->GetAlphLen();
		if(numABItems != mAlphabetLen)
			throw string("DGen::BuildCodingSets() -- Alphabet sizes in database an code table unequal.");
		//uint32 val[1];
		for(uint32 i=0; i<numABItems; i++) {
			is = ctFile->ReadNextItemSet();
			//is->GetValuesIn(val); // only 1 value
			mCodingSets[curCodingSet] = new CTreeSet(is, curCodingSet+1);
			++curCodingSet;
		}

		delete ctFile;
	} else  {
		mCodingSets = (DGSet**) new CTreeSet*[mNumCodingSets];	//mCodingSets = new DGSet*[mNumCodingSets];
		mCSABStart = curCodingSet;

		uint16 val = 0;
		uint32 cnt = 0;
		alphabet::iterator ait, aend=a->end();
		for(ait = a->begin(); ait != aend; ++ait) {
			val = ait->first;
			cnt = ait->second;
			mCodingSets[curCodingSet] = new CTreeSet(ItemSet::Create(mDataType, &val, 1, mAlphabetLen, cnt), curCodingSet+1); //mCodingSets[curCodingSet] = new DGSet(ItemSet::Create(type, &val, 1, ablen, cnt), curCodingSet+1);
			++curCodingSet;
		}
	}
}

void CTreeDGen::BuildModel(Database *db, Config *config, const string &ctfname) {
	// eerst coding set maken (laden, in case ct meegeleverd)
	BuildCodingSets(ctfname);

	uint32 lpcFactor = config->Read<uint32>("lpcfactor");
	uint32 lpcOffset = config->Read<uint32>("lpcoffset");

	mRootSet = new CTreeSet(ItemSet::CreateEmpty(mDataType, mAlphabetLen, 0), 0);
	mRoot->SetCTreeSet(mRootSet,0);

	// covertree
	uint32 numrows = db->GetNumRows();
	ItemSet *row, *iset;
	uint32 *vals = new uint32[db->GetMaxSetLength()];
	CoverSet *cset = CoverSet::Create(mDataType, mAlphabetLen);
	CTreeNode *node;
	for(uint32 r=0; r<numrows; r++) {
		row = db->GetRow(r);
		cset->InitSet(row);
		node = mRoot;
		for(uint32 c=0; c<mNumCodingSets; c++) {
			iset = mCodingSets[c]->GetItemSet();
			if(cset->Cover(iset->GetLength(), iset)) {
				node = node->AddCodingSet((CTreeSet*)mCodingSets[c], (lpcFactor * row->GetUsageCount()) + lpcOffset);
			}
		}
	}
	// mogelijke covers
	//{{node->AddGhostSet(...)}}

	FILE *fp = fopen("lalalana.txt", "w");
	if(!fp)
		throw string("Could not write analysis file.");

	mRoot->Print(fp, 0);
	fclose(fp);


	delete[] vals;
	delete cset;
}

Database* CTreeDGen::GenerateDatabase(uint32 numRows) {
	return NULL;
}

#endif // BLOCK_DATAGEN
