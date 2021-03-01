#ifdef BLOCK_DATAGEN

#include "../../../global.h"

#include <RandomUtils.h>
#include <Config.h>

#include <db/Database.h>
#include <isc/ItemTranslator.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/CoverSet.h>

#include "../../../algo/CTFile.h"
#include "../DGSet.h"
#include "../DGen.h"
#include "ColumnBasedDGen.h"

ColumnBasedDGen::ColumnBasedDGen() {
	mHMMMatrix = NULL;
}

ColumnBasedDGen::~ColumnBasedDGen() {
	// Zap HMMMatrix
	for(uint32 i=0; i<mNumCols; i++)
		delete[] mHMMMatrix[i];
	delete[] mHMMMatrix;
}

void ColumnBasedDGen::BuildModel(Database *db, Config *config, const string &ctfname) {
	// eerst coding set maken (laden, in case ct meegeleverd)
	BuildCodingSets(ctfname);
	BuildModelAf(db, config);
}

void ColumnBasedDGen::BuildModel(Database *db, Config *config, CodeTable *ct) {
	// eerst coding set maken (laden, in case ct meegeleverd)
	BuildCodingSets(ct);
	BuildModelAf(db, config);
}

void ColumnBasedDGen::BuildModelAf(Database *db, Config *config) {
	ItemSetType type = mDB->GetDataType();
	uint32 ablen = mDB->GetAlphabetSize();

	uint32 lpcFactor = config->Read<uint32>("lpcfactor");
	uint32 lpcOffset = config->Read<uint32>("lpcoffset");

	// ColDefHMMMatrix Initialiseren
	uint32 cnt;
	mHMMMatrix = new uint32*[mNumCols];				// zoveel rijen als dat er kolommen zijn
	for(uint32 i=0; i<mNumCols; i++) {
		mHMMMatrix[i] = new uint32[mNumCodingSets+1];		// 1 kolom extra voor kans-som
		mHMMMatrix[i][0] = 0;
		for(uint32 j=0; j<mNumCodingSets; j++) {
			mHMMMatrix[i][j+1] = 0;
			if(mColDef[i]->Intersects(mCodingSets[j]->GetItemSet())) {
				cnt = mCodingSets[j]->GetItemSet()->GetUsageCount();
				mHMMMatrix[i][j+1] = (cnt * lpcFactor) + lpcOffset;
				mHMMMatrix[i][0] += (cnt * lpcFactor) + lpcOffset;
			} 			
		}
	}

	FILE *hmmfile = fopen("hmm-cpc-file.txt", "w");
	if(hmmfile) {
		for(uint32 i=0; i<mNumCols; i++) {
			fprintf(hmmfile,"col %2d (%12s) : ",i,mColDef[i]->ToString(false,false).c_str());
			for(uint32 j=0; j<mNumCodingSets+1; j++) {
				fprintf(hmmfile,"%d ",mHMMMatrix[i][j]);
			}
			fprintf(hmmfile,"\n");
		}
	}
	fclose(hmmfile);
}

Database* ColumnBasedDGen::GenerateDatabase(uint32 numRows) {
	ItemSet *emptyiset = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
	ItemSet *fulliset = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
	fulliset->FillHerUp(mDB->GetAlphabetSize());

	CoverSet *cset = CoverSet::Create(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize(), fulliset);

	ItemSet *iset;
	uint32 numGenRows = numRows, idx, sum, draw, p;
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
		iset = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());

		for(uint32 c=0; c<mNumCols; c++) {
			if(cset->CanCover(mColDef[c])) {	// deze kolom moet nog!
				// kanssom fixen
				sum = mHMMMatrix[c][0];

				for(uint32 j=0; j<mNumCodingSets; j++) {
					if(mHMMMatrix[c][j+1] > 0) {
						if(!cset->CanCover(mCodingSets[j]->GetItemSet())) {
							sum -= mHMMMatrix[c][j+1];
						}
					}
				}

				if(sum >  0) {		// er iets te kiezen
					// coding element kiezen
					draw = RandomUtils::UniformUint32(sum);
					draw++;
					idx = mNumCodingSets + 1;
					p = 0;
					for(uint32 j=0; j<mNumCodingSets; j++) {
						if(mHMMMatrix[c][j+1] > 0) {
							if(cset->CanCover(mCodingSets[j]->GetItemSet())) {
								p += mHMMMatrix[c][j+1];
								if(draw <= p) {
									idx = j;
									break;
								}
							}
						}
					}

					// CodingSet[idx] toevoegen aan masks en itemset
					iset->Unite(mCodingSets[idx]->GetItemSet());
					ECHO(3, printf("%s + ", mCodingSets[idx]->GetItemSet()->ToString(false,false).c_str()));
					mCodingSets[idx]->GetItemSet()->GetValuesIn(valar);
					mCodingSets[idx]->GetItemSet()->AddOneToUsageCount();
					uint32 numItems = mCodingSets[idx]->GetItemSet()->GetLength();
					for(uint32 j=0; j<numItems; j++) {
						cset->Cover(mColDef[mItemToCol[valar[j]]]);
					}
				} else {
					// niets te kiezen, dit is een smerige fix
					ECHO(2, printf("%s + ", iset->ToString(false,false).c_str()));
					ECHO(2, printf("%s + ", mCodingSets[idx]->GetItemSet()->ToString(false,false).c_str()));
					cset->ResetMask();
					delete iset;
					iset = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
					c=0;
					c--; // en dit is Extra Smerig! :)
					continue;
				}

			}
		}
		ECHO(3, printf(" = %s\n", iset->ToString(false,false).c_str()));

		iss[i] = iset;
	}
	delete emptyiset;
	delete fulliset;
	delete[] valar;
	delete cset;

	/*FILE *fU = fopen("../data/ctuse.txt", "w");
	for(uint32 j=0; j<mNumCodingSets; j++) {
		fprintf(fU, "%s\n", mCodingSets[j]->GetItemSet()->ToString().c_str());
	}
	fclose(fU);*/

	Database *db = new Database(iss, numGenRows, false);
	alphabet *a = new alphabet(*mDB->GetAlphabet());
	db->SetAlphabet(a);
	if(mDB->GetItemTranslator() != NULL)
		db->SetItemTranslator(new ItemTranslator(mDB->GetItemTranslator()));
	db->CountAlphabet();
	db->ComputeStdLengths();
	db->ComputeEssentials();
	return db;
}

#endif // BLOCK_DATAGEN
