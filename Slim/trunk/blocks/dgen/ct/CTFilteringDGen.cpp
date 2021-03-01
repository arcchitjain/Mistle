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
#include "CTFilteringDGen.h"

CTFilteringDGen::CTFilteringDGen() {
	mHMM = NULL;
}

CTFilteringDGen::~CTFilteringDGen() {
	delete[] mHMM;
}

void CTFilteringDGen::BuildModel(Database *db, Config *config, const string &ctfname) {
	ItemSetType type = mDB->GetDataType();
	uint32 ablen = (uint32) mDB->GetAlphabetSize();

	// eerst coding set maken (laden, in case ct meegeleverd)
	BuildCodingSets(ctfname);

	uint32 lpcFactor = config->Read<uint32>("lpcfactor");
	uint32 lpcOffset = config->Read<uint32>("lpcoffset");

	ItemSet *abetIset = ItemSet::CreateEmpty(mDataType,mAlphabetLen);
	for(uint32 i=0; i<mNumCols; i++) {
		abetIset->Unite(mColDef[i]);
	}
	CoverSet *abetCset = CoverSet::Create(mDataType,mAlphabetLen,abetIset);

	// HMM initialiseren
	uint32 cnt;
	mHMM = new uint32[mNumCodingSets];
	mSum = 0;
	ECHO(3, printf("BuildModel:\n"));
	for(uint32 j=0; j<mNumCodingSets; j++) {
		if(abetCset->CanCover(mCodingSets[j]->GetItemSet()->GetLength(), mCodingSets[j]->GetItemSet())) {
			cnt = (mCodingSets[j]->GetItemSet()->GetUsageCount() * lpcFactor) + lpcOffset;
		} else cnt = 0;
		mHMM[j] = cnt;
		ECHO(3, printf("%d ", cnt));
		mSum += cnt;
		mCodingSets[j]->GetItemSet()->ResetUsage();
	}
	delete abetIset;
	delete abetCset;
	ECHO(3, printf("\nSum = %d\n\n", mSum));
}

Database* CTFilteringDGen::GenerateDatabase(uint32 numRows) {
	ItemSet *emptyiset = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());

	uint16 *volleset = new uint16[mDB->GetAlphabetSize()];
	for(uint32 i=0; i<mDB->GetAlphabetSize(); i++)
		volleset[i] = i;
	ItemSet *volleiset = ItemSet::Create(mDB->GetDataType(), volleset, mDB->GetAlphabetSize(), (uint32) mDB->GetAlphabetSize());

	CoverSet *cset = CoverSet::Create(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize(), volleiset);

	ItemSet *iset;
	uint32 numGenRows = numRows, idx, sum, draw, p;
	ItemSet **iss = new ItemSet *[numGenRows];
	uint32 *valar = new uint32[mDB->GetMaxSetLength()];
	uint32 *hmm = new uint32[mNumCodingSets];
	uint32 numItems;
	ItemSet *is;

	ECHO(3, printf("gendb: %d rows\n", numRows));

	RandomUtils::Init();
	for(uint32 i=0; i<numGenRows; i++) {
		ECHO(3, printf("row %4d : ",i));
		cset->ResetMask();
		iset = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
		iset->SetUsageCount(1);

		sum = mSum;
		memcpy(hmm, mHMM, sizeof(uint32)*mNumCodingSets);

		while(sum > 0) {
			// coding element kiezen
			draw = RandomUtils::UniformUint32(sum);
			draw++;
			idx = mNumCodingSets;
			p = 0;
			for(uint32 j=0; j<mNumCodingSets; j++) {
				if(hmm[j] > 0) {
					p += hmm[j];
					if(draw <= p) {
						idx = j;
						break;
					}
				}
			}
			if(idx == mNumCodingSets) {
				//throw string("ACBHMDGen::GenerateDatabase() -- Did not find a codingset to add!");
				sum = mSum;
				memcpy(hmm, mHMM, sizeof(uint32)*mNumCodingSets);

				cset->ResetMask();
				delete iset;
				iset = ItemSet::CreateEmpty(mDB->GetDataType(), (uint32) mDB->GetAlphabetSize());
				iset->SetUsageCount(1);
				continue;
			}
			
			// CodingSet[idx] toevoegen aan masks en itemset
			is = mCodingSets[idx]->GetItemSet();
			iset->Unite(is);
			is->AddOneToUsageCount();
			ECHO(3, printf("%s + ", is->ToString(false,false).c_str()));
			is->GetValuesIn(valar);
			numItems = is->GetLength();
			for(uint32 j=0; j<numItems; j++)
				cset->Cover(mColDef[mItemToCol[valar[j]]]->GetLength(), mColDef[mItemToCol[valar[j]]]);
			for(uint32 j=0; j<mNumCodingSets; j++) {
				if(hmm[j] > 0 && !cset->CanCover(mCodingSets[j]->GetItemSet()->GetLength(), mCodingSets[j]->GetItemSet())) {
					sum -= hmm[j];
					hmm[j] = 0;
				}
			}
		}
		ECHO(3, printf(" = %s\n", iset->ToString(false,false).c_str()));
		iss[i] = iset;
	}
	ECHO(3, printf("CodingSet counts:\n", iset->ToString(false,false).c_str()));
	for(uint32 j=0; j<mNumCodingSets; j++) {
		ItemSet *is = mCodingSets[j]->GetItemSet();
		ECHO(3, printf("%s = %d (%d)\n", is->ToString(false,false).c_str(), is->GetUsageCount(), is->GetPrevUsageCount()));
	}
       
	delete emptyiset;
	delete volleiset;
	delete[] volleset;
	delete[] valar;
	delete cset;
	delete[] hmm;

	Database *db = new Database(iss, numGenRows, false);
	alphabet *a = new alphabet(*mDB->GetAlphabet());
	db->SetAlphabet(a);
	db->CountAlphabet();
	db->ComputeStdLengths();
	db->ComputeEssentials();
	return db;
}

#endif // BLOCK_DATAGEN
