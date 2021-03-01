#if defined(BLOCK_DATAGEN) || defined (BLOCK_CLASSIFIER)

#include "../../../global.h"

#include <RandomUtils.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>

#include "../../krimp/codetable/CTFile.h"
#include "CTree.h"
#include "CTreeSet.h"
#include "CTreeNode.h"

CTree::CTree() {
	mRoot = new CTreeNode();
	mRootSet = NULL;
	mCodingSets = NULL;
	mNumCodingSets = 0;
	mCSABStart = 0;
}

CTree::~CTree() {
	delete mRoot;
	delete mRootSet;

	for(uint32 i=0; i<mNumCodingSets; i++)
		delete mCodingSets[i];
	delete[] mCodingSets;
}

void CTree::BuildCodingSets(const string &ctfname) {
	ItemSetType type = mDB->GetDataType();
	alphabet* a = mDB->GetAlphabet();
	uint32 ablen = a->size();

	mNumCodingSets = ablen; //0
	uint32 curCodingSet = 0;
	if(!ctfname.empty()) {
		CTFile *ctFile = new CTFile(ctfname, mDB);
		uint32 numCTSets = ctFile->GetNumSets();
		mNumCodingSets += numCTSets;
		mCodingSets = new CTreeSet*[mNumCodingSets];
		ItemSet *is;
		for(uint32 i=0; i<numCTSets; i++) {
			is = ctFile->ReadNextItemSet();
			is->SetUsageCount(0);
			mCodingSets[curCodingSet] = new CTreeSet(is, curCodingSet+1);
			++curCodingSet;
		}
		delete ctFile;
	} else 
		mCodingSets = new CTreeSet*[mNumCodingSets];

	mCSABStart = curCodingSet;

	uint16 val = 0;
	alphabet::iterator ait, aend=a->end();
	for(ait = a->begin(); ait != aend; ++ait) {
		val = ait->first;
		mCodingSets[curCodingSet] = new CTreeSet(ItemSet::Create(type, &val, 1, ablen), curCodingSet+1);
		++curCodingSet;
	}
}

// zet db om in itemset p-tree, alphabet items ook als itemsets
void CTree::BuildCoverTree(Database *db, const string &ctfname) {
	mDB = db;
	ItemSetType type = mDB->GetDataType();
	uint32 ablen = mDB->GetAlphabetSize();

	// eerst coding set maken (laden, in case ct meegeleverd)
	BuildCodingSets(ctfname);

	mRootSet = new CTreeSet(ItemSet::CreateEmpty(type, ablen, 0), 0);
	mRoot->SetCTreeSet(mRootSet,0);

	uint32 numrows = db->GetNumRows();
	ItemSet *row, *iset;
	uint32 *vals = new uint32[db->GetMaxSetLength()];
	CoverSet *cset = CoverSet::Create(type, ablen);
	CTreeNode *node;
	for(uint32 r=0; r<numrows; r++) {
		row = db->GetRow(r);
		cset->InitSet(row);
		node = mRoot;
		for(uint32 c=0; c<mNumCodingSets; c++) {
			iset = mCodingSets[c]->GetItemSet();
			if(cset->Cover(iset->GetLength(), iset)) {
				node = node->AddCodingSet(mCodingSets[c], row->GetUsageCount());
			}
		}
	}

	delete[] vals;
	delete cset;
}
void CTree::CalcProbabilities() {
	mRoot->CalcProbabilities();
}
void CTree::AddOneToCounts() {
	mRoot->AddOneToCounts();
}
void CTree::WriteAnalysisFile(const string &analysisFile) {
	FILE *fp = fopen(analysisFile.c_str(), "w");
	if(!fp)
		throw string("Could not write analysis file.");

	mRoot->Print(fp, 0);
	fclose(fp);
}

void CTree::BDTRecurse(CoverSet *cset, CTreeNode *node, uint32 from) {
	ItemSet *iset;
	CTreeNode *newnode;
	for(uint32 i=from; i<mNumCodingSets; i++) {
		iset = mCodingSets[i]->GetItemSet();
		if(cset->Cover(iset->GetLength(),iset)) {
			// als CTreeset toevoegen
			newnode = node->AddCodingSet(mCodingSets[i],0);	// basiscount
			if(newnode->GetUseCount() > 0) {
				// recurse into CTreeset, maar alleen als het geen onbekende tak is
				BDTRecurse(cset, newnode, i+1);
			} else
				newnode->SetVirtual(true);	// alleen hier introduceren we zero-sets

			// weer van de cover afhalen
			cset->Uncover(iset->GetLength(), iset);
		}
	}
}

void CTree::BDTCTPathsRecurse(CoverSet *cset, CTreeNode *node, uint32 from) {
	ItemSet *iset;
	CTreeNode *newnode;
	uint32 *valar = new uint32[mDB->GetAlphabetSize()];
	for(uint32 i=from; i<mCSABStart; i++) {
		if(from == 0) {
			printf(" Current base-set: %d/%d\n", i, mCSABStart);
		}
		iset = mCodingSets[i]->GetItemSet();
		if(cset->CanCover(iset->GetLength(),iset)) {
			// -> Niet gebruiken
			// want een van mijn elementen 'zit niet' in cset
			iset->GetValuesIn(valar);
			uint32 numvals = iset->GetLength();
			for(uint32 j=0; j<numvals; j++) {
				cset->UnSetItem(valar[j]);
				BDTCTPathsRecurse(cset, node, i+1);
				cset->SetItem(valar[j]);
			}

			// -> Wel gebruiken
			cset->Cover(iset->GetLength(),iset);
			newnode = node->AddCodingSet(mCodingSets[i],0);	// basiscount
			BDTCTPathsRecurse(cset, newnode, i+1);
			cset->Uncover(iset->GetLength(), iset);
		}
	}
	delete[] valar;
}

void CTree::BDTGhostChildrenRecurse(CoverSet *cset, CTreeNode *node, uint32 from) {
	ItemSet *iset;
	uint32 numghost = 0;
	for(uint32 i=from; i<mNumCodingSets; i++) {
		iset = mCodingSets[i]->GetItemSet();
		if(cset->Cover(iset->GetLength(),iset)) {
			if(!node->HasCodingSet(mCodingSets[i])) {
				numghost++;
			} else {
				// recurse into CTreeset
				BDTGhostChildrenRecurse(cset, node->GetCodingSet(mCodingSets[i]), i+1);
			}
			// weer van de cover afhalen
			cset->Uncover(iset->GetLength(), iset);
		}
	}
	node->SetNumGhostChildren(numghost);
}

Database *CTree::BuildHMMMatrix(Database *db, const string &ctfname) {
	mDB = db;
	ItemSetType type = mDB->GetDataType();
	uint32 ablen = (uint32) mDB->GetAlphabetSize();

	// eerst coding set maken (laden, in case ct meegeleverd)
	BuildCodingSets(ctfname);

	uint32 **hmmmatrix = new uint32*[mNumCodingSets+1];		// 1 rij extra voor startkansen
	for(uint32 i=0; i<mNumCodingSets+1; i++) {
		hmmmatrix[i] = new uint32[mNumCodingSets+2];		// 1 kolom extra voor kans-som, 1 voor stopkans
		for(uint32 j=0; j<mNumCodingSets+2; j++)			
			hmmmatrix[i][j] = 0;
	}

	uint16 *abetset = new uint16[ablen];
	for(uint32 i=0; i<ablen; i++) {
		abetset[i] = i;
	}
	ItemSet *abetiset = ItemSet::Create(type,abetset,ablen,ablen);
	delete[] abetset;
	CoverSet *abetcset = CoverSet::Create(type,ablen,abetiset);

	for(uint32 i=0; i<mNumCodingSets; i++) {
		abetcset->ResetMask();
		abetcset->Cover(mCodingSets[i]->GetItemSet()->GetLength(),mCodingSets[i]->GetItemSet());
		hmmmatrix[0][i+2]++;
		hmmmatrix[0][0]++;
		hmmmatrix[i+1][1]++;	// stopkans voor deze set op 1 zetten
		hmmmatrix[i+1][0]++;	// kanssom fixen

		for(uint32 j=i+1; j<mNumCodingSets; j++) {
			if(abetcset->CanCover(mCodingSets[j]->GetItemSet()->GetLength(),mCodingSets[j]->GetItemSet())) {
				hmmmatrix[i+1][j+2]++;
				hmmmatrix[i+1][0]++;
			}
		}
	}

	uint32 numrows = db->GetNumRows();
	ItemSet *row, *iset;
	CoverSet *cset = CoverSet::Create(type, ablen);
	int32 lastIdx = 0;
	for(uint32 r=0; r<numrows; r++) {
		lastIdx = 0;
		row = db->GetRow(r);
		cset->InitSet(row);
		for(uint32 c=0; c<mNumCodingSets; c++) {
			iset = mCodingSets[c]->GetItemSet();
			if(cset->Cover(iset->GetLength(), iset)) {
				hmmmatrix[lastIdx][c+2]++;
				hmmmatrix[lastIdx][0]++;
				lastIdx = c+1;
			}
		}
		// stopkans fixen
		hmmmatrix[lastIdx][0]++;
		hmmmatrix[lastIdx][1]++;
	}
	delete cset;

	FILE *hmmfile = fopen("hmmfile.txt", "w");
	for(uint32 i=0; i<mNumCodingSets+1; i++) {
		for(uint32 j=0; j<mNumCodingSets+2; j++) {
			fprintf(hmmfile,"%d ",hmmmatrix[i][j]);
		}
		fprintf(hmmfile,"\n");
	}
	fclose(hmmfile);

	uint32 numGenRows = 3196;
	ItemSet **iss = new ItemSet *[numGenRows];
	uint16 *emptyset = new uint16[ablen];
	double draw = 0, p=0;
	for(uint32 i=0; i<numGenRows; i++) {
		abetcset->ResetMask();
		iss[i] = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
		iss[i]->SetUsageCount(1);

		int32 lastIdx = 0;
		int32 pos;
		bool stop = false;
		while(stop == false) {
			pos = hmmmatrix[lastIdx][0];
			draw = RandomUtils::UniformDouble();
			p = 0.0;

			for(uint32 j=lastIdx; j<mNumCodingSets+1; j++) {		// elke kolom
				p += (double)(hmmmatrix[lastIdx][j+1]/(double)pos);
				if(draw <= p) {
					if(j == lastIdx) {
						stop = true;
						break;
					}
					iss[i]->Unite(mCodingSets[j-1]->GetItemSet());
					lastIdx = j;
					break;
				}
			}
		}
	}
	delete abetcset;
	delete abetiset;
	delete emptyset;

	for(uint32 i=0; i<mNumCodingSets+1; i++)
		delete[] hmmmatrix[i];
	delete[] hmmmatrix;

#if defined (__GNUC__)
	{
#endif
	Database *db = new Database(iss, numGenRows, false);
	db->ComputeEssentials();
	return db;	
#if defined (__GNUC__)
	}
#endif
}

void CTree::BuildDistributionTree(Database *db, const string &ctfname) {
	/*
		ohkay, eerst BuildCoverTree, dan voor elke node uitrekenen hoeveel 0-count 
		alternatieven er zijn. Evt. aanmaken als children.
	*/

	mDB = db;
	ItemSetType type = mDB->GetDataType();
	uint32 ablen = (uint32) mDB->GetAlphabetSize();

	BuildCoverTree(db,ctfname);

	/*
	// eerst coding set maken (laden, in case ct meegeleverd)
	BuildCodingSets(ctfname);

	mRootSet = new CTreeSet(ItemSet::CreateEmpty(type, ablen, 0), 0);
	mRoot->SetCTreeSet(mRootSet,0);
	*/

	// nu ahv fullitemset mogelijke coverpaden in de tree dumpen
	uint16 *abetset = new uint16[ablen];
	for(uint32 i=0; i<ablen; i++) {
		abetset[i] = i;
	}
	ItemSet *abetiset = ItemSet::Create(type,abetset,ablen,ablen);
	delete[] abetset;
	CoverSet *abetcset = CoverSet::Create(type,ablen,abetiset);

	//BDTRecurse(abetcset, mRoot, 0);
	//BDTGhostChildrenRecurse(abetcset, mRoot, 0);
	BDTCTPathsRecurse(abetcset, mRoot, 0);

	delete abetcset;
	delete abetiset;


}

void CTree::CoverProbabilities(Database *db, string filename) {
	uint32 numRows = db->GetNumRows();
	ItemSet **iss = db->GetRows();
	double probability;

	CoverSet *cs = CoverSet::Create(iss[0]->GetType(), (uint32) db->GetAlphabetSize());
	FILE *fp = fopen(filename.c_str(), "w");
	if(!fp)
		throw string("Could not write file.");

	for(uint32 i=0; i<numRows; i++) {
		cs->InitSet(iss[i]);
		//printf("Covering %s\n", iss[i]->ToString().c_str());
		probability = mRoot->CalcCoverProbability(cs);
		fprintf(fp, "%.08f :: %s\n", probability, iss[i]->ToString().c_str());
	}

	fclose(fp);
	delete cs;
}

Database* CTree::GenerateDatabase(uint32 numRows) {
	uint16 *abetset = new uint16[mDB->GetAlphabetSize()];
	for(uint32 i=0; i<mDB->GetAlphabetSize(); i++) {
		abetset[i] = i;
	}
	ItemSet *abetiset = ItemSet::Create(mDB->GetDataType(),abetset,mDB->GetAlphabetSize(),mDB->GetAlphabetSize());
	delete[] abetset;
	CoverSet *abetcset = CoverSet::Create(mDB->GetDataType(),mDB->GetAlphabetSize(),abetiset);

	ItemSet **iss = new ItemSet *[numRows];
	uint32 fromCodingSet = 0, useCount = 0, numChildren = 0;
	uint16 *emptyset = new uint16[0];
	for(uint32 i=0; i<numRows; i++) {
		abetcset->ResetMask();
		fromCodingSet = 0; useCount = 0; numChildren = 0;

		iss[i] = ItemSet::CreateEmpty(mDB->GetDataType(), mDB->GetAlphabetSize());
		iss[i]->SetUsageCount(1);

		mRoot->GenerateItemSet(abetcset,iss[i],fromCodingSet,useCount,numChildren);
		/*if(mRoot->GenerateItemSet(abetcset,iss[i],fromCodingSet,useCount,numChildren))
			FantasizeItemSet(abetcset,iss[i],fromCodingSet);*/
	}
	delete abetcset;
	delete abetiset;
	delete emptyset;

	Database *db = new Database(iss, numRows, false);
	db->ComputeEssentials();
	return db;
}

void CTree::FantasizeItemSet(CoverSet *cset, ItemSet *gset, uint32 fromCodingSet) {
	ItemSet *iset;

	// we mogen kiezen uit sets mCodingSets[fromCodingSets,mNumCodingSets]
	// eerst tellen hoeveel dat er zijn
	uint32 numPossible = 1; // 1 want stopcodon
	for(uint32 i=fromCodingSet; i<mNumCodingSets; i++) {
		if(cset->CanCover(mCodingSets[i]->GetItemSet()->GetLength(),mCodingSets[i]->GetItemSet())) {
			numPossible++;
		}
	}
	if(numPossible == 1) {
		// nokken
		return;
	}

	float draw = RandomUtils::UniformFloat();
	float p = 0.0f;
	float q = 1 / (float) numPossible;
	for(uint32 i=fromCodingSet; i<mNumCodingSets; i++) {
		if(cset->CanCover(mCodingSets[i]->GetItemSet()->GetLength(),mCodingSets[i]->GetItemSet())) {
			if(draw < p + q) {
				// deze is leuk, deze nemen we!
				iset = mCodingSets[i]->GetItemSet();
				cset->Cover(iset->GetLength(),iset);
				gset->Unite(iset);
				if(numPossible > 2)
					FantasizeItemSet(cset,gset,i+1);
				return;
			}
			p += q;
		}
	}
	// stop codon gekozen
	return;
}

#endif // BLOCK_DATAGEN
