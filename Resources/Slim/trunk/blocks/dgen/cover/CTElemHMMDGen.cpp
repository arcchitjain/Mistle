#ifdef BLOCK_DATAGEN

#include "../../../global.h"

#include <RandomUtils.h>
#include <Config.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>

#include "../../../algo/CTFile.h"
#include "../DGSet.h"
#include "../DGen.h"
#include "CTElemHMMDGen.h"

/*
  :: Codetable Element 1e orde HMM - Data generator 
    -: Drie mogelijke matrix invulmogelijkheden, ingesteld door multifp :-

    :: Cover-usage met LaPlace (fillstyle = 1) ::
        tel[vorigeElement][ditElement] = laPlaceOffset + ((# keer in een cover samen met vorigeElement) * laPlaceFactor)

    :: Gewoon CT Counts met LaPlace (fillstyle = 0) ::
        tel[vorigeElement][ditElement] = laPlaceOffset + (ditElement->GetCount() * laPlaceFactor)
						
    :: Alleen laPlace (fillstyle = 2)
        tel[vorigeElement][ditElement] = laPlaceOffset

*/

CTElemHMMDGen::CTElemHMMDGen() {
	mHMMMatrix = NULL;
	mHMMHeight = 0;
	mHMMWidth = 0;
	mAbetIset = NULL;
	mAbetCset = NULL;
}

CTElemHMMDGen::~CTElemHMMDGen() {
	// Zap HMMMatrix
	for(uint32 i=0; i<mHMMHeight; i++)
		delete[] mHMMMatrix[i];
	delete[] mHMMMatrix;
	delete mAbetIset;
	delete mAbetCset;
}

void CTElemHMMDGen::BuildModel(Database *db, Config *config, const string &ctfname) {
	// eerst coding set maken (laden, in case ct meegeleverd)
	BuildCodingSets(ctfname);

	uint32 lpcFactor = config->Read<uint32>("lpcfactor");
	uint32 lpcOffset = config->Read<uint32>("lpcoffset");
	uint32 fillStyle = config->Read<uint32>("multifp");

	// CTElemHMMDGen Initialiseren
	mHMMHeight = mNumCodingSets + 1;		// 1 rij extra voor startkansen
	mHMMWidth = mNumCodingSets + 1;			// 1 kolom extra voor kanssommen

	mHMMMatrix = new uint32*[mHMMHeight];		
	for(uint32 i=0; i<mHMMHeight; i++) {
		mHMMMatrix[i] = new uint32[mHMMWidth];
		for(uint32 j=0; j<mHMMWidth; j++) {
			mHMMMatrix[i][j] = 0;
		}
	}

	mAbetIset = ItemSet::CreateEmpty(mDataType,mAlphabetLen);
	for(uint32 i=0; i<mNumCols; i++) {
		mAbetIset->Unite(mColDef[i]);
	}
	mAbetCset = CoverSet::Create(mDataType,mAlphabetLen,mAbetIset);

	ItemSet *iset, *jset;
	for(uint32 i=0; i<mNumCodingSets; i++) {
		mAbetCset->ResetMask();
		iset = mCodingSets[i]->GetItemSet();
		mAbetCset->Cover(iset->GetLength(),iset);
		mHMMMatrix[0][i+1] += lpcOffset;
		mHMMMatrix[0][0] += lpcOffset;

		if(fillStyle == 1) {
			mHMMMatrix[0][i+1] += lpcFactor * iset->GetUsageCount();
			mHMMMatrix[0][0] += lpcFactor * iset->GetUsageCount();
		}
		for(uint32 j=0; j<mNumCodingSets; j++) {
			jset = mCodingSets[j]->GetItemSet();
			if(mAbetCset->CanCover(jset->GetLength(),jset)) {
				mHMMMatrix[i+1][j+1] += lpcOffset;
				mHMMMatrix[i+1][0] += lpcOffset;

				if(fillStyle == 1) {
					mHMMMatrix[i+1][j+1] += lpcFactor * jset->GetUsageCount();
					mHMMMatrix[i+1][0] += lpcFactor * jset->GetUsageCount();
				}
			}
		}
	}
	
	if(fillStyle == 0) {
		// cover verwerken

		uint32 numRows = mDB->GetNumRows();
		ItemSet *row, *iset;
		CoverSet *cset = CoverSet::Create(mDataType, mAlphabetLen);
		int32 lastIdx = 0;
		for(uint32 r=0; r<numRows; r++) {
			lastIdx = 0;
			row = db->GetRow(r);
			cset->InitSet(row);
			for(uint32 c=0; c<mNumCodingSets; c++) {
				iset = mCodingSets[c]->GetItemSet();
				if(cset->Cover(iset->GetLength(), iset)) {
					mHMMMatrix[lastIdx][c+1] += lpcFactor;
					mHMMMatrix[lastIdx][0] += lpcFactor;
					lastIdx = c+1;
				}
			}
		}
		delete cset;

	} else if(fillStyle == 2) {
		// no fill, just lpc
	}

	FILE *hmmfile = fopen("../data/ctpathhmm-hmm.txt", "w");
	for(uint32 i=0; i<mHMMHeight; i++) {
		fprintf(hmmfile,"%2d : ",i);
		for(uint32 j=0; j<mHMMWidth; j++) {
			fprintf(hmmfile,"%d ",mHMMMatrix[i][j]);
		}
		fprintf(hmmfile,"\n");
	}
	fclose(hmmfile);
}

Database* CTElemHMMDGen::GenerateDatabase(uint32 numRows) {
	ItemSet *emptyiset = ItemSet::CreateEmpty(mDB->GetDataType(), mDB->GetAlphabetSize());

	CoverSet *cset = mAbetCset; //CoverSet::Create(mDB->GetDataType(), mDB->GetAlphabetSize(), volleiset);

	ItemSet *iset;
	uint32 numGenRows = numRows, idx, draw, p;
	ItemSet **iss = new ItemSet *[numGenRows];
	uint32 *valar = new uint32[mDB->GetMaxSetLength()];

	ECHO(3, printf("gendb: %d rows\n", numRows));

	for(uint32 j=0; j<mNumCodingSets; j++) {
		mCodingSets[j]->GetItemSet()->ResetUsage();
	}

	//RandomUtils::Init();
	for(uint32 i=0; i<numGenRows; i++) {
		ECHO(3, printf("row %4d : ",i));
		cset->ResetMask();
		iset = ItemSet::CreateEmpty(mDB->GetDataType(), mDB->GetAlphabetSize());
		iset->SetUsageCount(1);

		uint32 numColsFilled = 0;
		uint32 lastChoice = 0;
		while(numColsFilled < mNumCols) {

			uint32 kansSom = 0;
			for(uint32 j=0; j<mNumCodingSets; j++) {
				if(mHMMMatrix[lastChoice][j+1] > 0) {
					if(cset->CanCover(mCodingSets[j]->GetItemSet()->GetLength(),mCodingSets[j]->GetItemSet())) {
						kansSom += mHMMMatrix[lastChoice][j+1];
					}
				}
			}

			if(kansSom > 0) {
				draw = RandomUtils::UniformUint32(kansSom) + 1;
				idx = mNumCodingSets + 1;
				p = 0;
				for(uint32 j=0; j<mNumCodingSets; j++) {
					if(mHMMMatrix[lastChoice][j+1] > 0) {
						if(cset->CanCover(mCodingSets[j]->GetItemSet()->GetLength(),mCodingSets[j]->GetItemSet())) {
							p += mHMMMatrix[lastChoice][j+1];
							if(draw <= p) {
								idx = j;
								break;
							}
						}
					}
				}

				// CodingSet[idx] toevoegen aan masks en itemset
				iset->Unite(mCodingSets[idx]->GetItemSet());
				mCodingSets[idx]->GetItemSet()->GetValuesIn(valar);
				mCodingSets[idx]->GetItemSet()->AddOneToUsageCount();
				uint32 numItems = mCodingSets[idx]->GetItemSet()->GetLength();
				for(uint32 j=0; j<numItems; j++) {
					cset->Cover(mColDef[mItemToCol[valar[j]]]->GetLength(), mColDef[mItemToCol[valar[j]]]);
				}
				numColsFilled += numItems;
			} else {
				// niets te kiezen, dit is een smerige fix
				ECHO(2, printf("%s + ", iset->ToString(false,false).c_str()));
				ECHO(2, printf("%s + ", mCodingSets[idx]->GetItemSet()->ToString(false,false).c_str()));
				cset->ResetMask();
				delete iset;
				iset = ItemSet::CreateEmpty(mDB->GetDataType(), mDB->GetAlphabetSize());
				iset->SetUsageCount(1);
				lastChoice = 0;	
				numColsFilled = 0;
			}
		}
		iss[i] = iset;
	}
	delete emptyiset;
	//delete volleiset;
	//delete[] volleset;
	delete[] valar;
	//delete cset;

	FILE *fU = fopen("../data/ctuse.txt", "w");
	for(uint32 j=0; j<mNumCodingSets; j++) {
		fprintf(fU, "%s\n", mCodingSets[j]->GetItemSet()->ToString().c_str());
	}
	fclose(fU);

	Database *db = new Database(iss, numGenRows, false);
	alphabet *a = new alphabet(*mDB->GetAlphabet());
	db->SetAlphabet(a);
	db->CountAlphabet();
	db->ComputeStdLengths();
	db->ComputeEssentials();
	return db;
}

#endif 
