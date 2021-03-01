#include "../Bass.h"

#include <cstring>

#include <FileUtils.h>
#include <RandomUtils.h>
#include <logger/Log.h>

#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
#include <glibc_s.h>
#endif

#include "../isc/ItemSetCollection.h"
#include "../itemstructs/Uint16ItemSet.h"
#include "../isc/ItemTranslator.h"
#include "DbFile.h"
#include "ClassedDatabase.h"

#include "Database.h"

Database::Database() {
	mDbName = "unknown";
	mRows = NULL;
	mNumRows = 0;
	mNumTransactions = 0;
	mNumItems = 0;
	mDataType = NoneItemSetType;
	mHasBinSizes = false;
	mHasTransactionIds = false;
	mFastGenerateDBOccs = false;

	mPath = "";
	mFilename = "";

	// Essentials(tm)
	mAlphabet = NULL;
	mStdLengths = NULL;
	mStdLaplace = false;
	mStdDbSize = 0.0;
	mMaxSetLength = 0;

	// Not per-se Essential (tm)
	mAlphabetNumRows = NULL;
	mAlphabetDbOccs = NULL;
	mItemTranslator = NULL;

	// Column Definition (c)
	mHasColumnDefinition = false;
	mColumnDefinition = NULL;
	mNumColumns = 0;

	// Class Labels (.)
	mNumClasses = 0;
	mClassDefinition = NULL;
}

Database::Database(Database *db) {
	mDbName = db->mDbName;
	mNumRows = db->GetNumRows();
	mNumTransactions = db->GetNumTransactions();
	ItemSet **iss = db->GetRows();
	mRows = new ItemSet *[mNumRows];
	for(uint32 i=0; i<mNumRows; i++)
		mRows[i] = iss[i]->Clone();
	mNumItems = db->GetNumItems();
	mDataType = db->GetDataType();

	mPath = "";
	mFilename = "";

	mHasBinSizes = db->HasBinSizes();
	mHasTransactionIds = db->HasTransactionIds();

	// Essentials(tm)
	mAlphabet = new alphabet(*db->GetAlphabet());
	mStdLengths = new double[mAlphabet->size()];
	memcpy(mStdLengths, db->GetStdLengths(), sizeof(double) * mAlphabet->size());
	mStdLaplace = db->GetStdLaplace();
	mStdDbSize = db->GetStdDbSize();
	mStdCtSize = db->GetStdCtSize();
	mMaxSetLength = db->GetMaxSetLength();

	// Not per-se Essential (tm)
	mAlphabetNumRows = NULL;
	if(db->mAlphabetNumRows != NULL) {
		mAlphabetNumRows = new uint32[mAlphabet->size()];
		memcpy(mAlphabetNumRows, db->mAlphabetNumRows, sizeof(uint32) * mAlphabet->size());
	}
	mAlphabetDbOccs = NULL;
	if(db->mAlphabetDbOccs != NULL) {
		EnableFastDBOccurences();
	}
	mFastGenerateDBOccs = db->mFastGenerateDBOccs;
	mItemTranslator = db->GetItemTranslator() != NULL ? new ItemTranslator(db->GetItemTranslator()) : NULL;

	// Column Definition (c)
	mHasColumnDefinition = db->HasColumnDefinition();
	mNumColumns = mHasColumnDefinition ? db->GetNumColumns() : 0;
	mColumnDefinition = new ItemSet*[mNumColumns];
	ItemSet **colDef = db->GetColumnDefinition();
	for(uint32 i=0; i<mNumColumns; i++)
		mColumnDefinition[i] = colDef[i]->Clone();

	// Class Labels (.)
	mNumClasses = db->mNumClasses;
	mClassDefinition = new uint16[mNumClasses];
	memcpy(mClassDefinition, db->mClassDefinition, sizeof(uint16) * mNumClasses);
}

Database::Database(ItemSet **itemsets, uint32 numRows, bool binSizes, uint32 numTransactions, uint64 numItems) {
	mDbName = "unknown";
	mRows = itemsets;
	mNumRows = numRows;
	mNumItems = numItems;
	mNumTransactions = numTransactions;

	mPath = "";
	mFilename = "";

	bool computeNumItems = false;
	bool computeNumTrans = false;

	if(mNumItems == 0) 
		computeNumItems = true;

	mDataType = numRows > 0 ? itemsets[0]->GetType() : NoneItemSetType;
	mHasBinSizes = binSizes;
	mHasTransactionIds = false;
	mFastGenerateDBOccs = false;

	if(!mHasBinSizes) {
		mNumTransactions = mNumRows;
	} else if(mHasBinSizes && numTransactions == 0) {
		computeNumTrans = true;
	}
		
	if(computeNumTrans && computeNumItems) {
		for(uint32 i=0; i<mNumRows; i++) {
			mNumItems += mRows[i]->GetLength() * mRows[i]->GetUsageCount();
			mNumTransactions += mRows[i]->GetUsageCount();
		}
	} else if(computeNumItems) {
		for(uint32 i=0; i<mNumRows; i++)
			mNumItems += mRows[i]->GetLength() * mRows[i]->GetUsageCount();
	} else if(computeNumTrans) {
		for(uint32 i=0; i<mNumRows; i++) 
			mNumTransactions += mRows[i]->GetUsageCount();
	}


	// Essentials(tm)
	mAlphabet = NULL;
	mStdLengths = NULL;
	mStdLaplace = false;
	mStdDbSize = 0.0;
	mMaxSetLength = 0;

	// Not per-se Essential (tm)
	mAlphabetNumRows = NULL;
	mAlphabetDbOccs = NULL;
	mItemTranslator = NULL;

	// Column Definition (c)
	mHasColumnDefinition = false;
	mColumnDefinition = NULL;
	mNumColumns = 0;

	// Class Labels (.)
	mNumClasses = 0;
	mClassDefinition = NULL;
}

Database::~Database() {

	CleanUpStaticMess();
	for(uint32 i=0; i<mNumRows; i++)
		delete mRows[i];
	delete[] mRows;
	for(uint32 i=0; i<mNumColumns; i++)
		delete mColumnDefinition[i];
	delete[] mColumnDefinition;

	delete[] mStdLengths;
	delete mItemTranslator;
	delete[] mAlphabetNumRows;
	if(mAlphabetDbOccs != NULL) {
		uint32 abSize = (uint32) mAlphabet->size();
		for(uint32 i=0; i<abSize; i++)
			delete[] mAlphabetDbOccs[i];
	}
	delete[] mAlphabetDbOccs;
	delete mAlphabet;

	delete[] mClassDefinition;

}

Database* Database::Clone() {
	return new Database(this);
}

void Database::CleanUpStaticMess() {
	// ~~~ if(ItemSet::HasStaticMess())
	if(mNumRows > 0)
		mRows[0]->CleanUpStaticMess();
}

//////////////////////////////////////////////////////////////////////////
//	Alphabet Functions
//////////////////////////////////////////////////////////////////////////

void Database::UseAlphabetFrom(Database *db, bool originalStdLengths) {
	if(mAlphabet) delete mAlphabet;
	mAlphabet = new alphabet(*db->GetAlphabet());
	CountAlphabet();
	if(originalStdLengths) {
		mStdLengths = new double[mAlphabet->size()];
		memcpy(mStdLengths, db->GetStdLengths(), sizeof(double)*mAlphabet->size());
	} else
		ComputeStdLengths();
}
void Database::SetAlphabetSize(const uint32 newSize) {
	if(mAlphabet) delete mAlphabet;
	mAlphabet = new alphabet();
	for(uint32 i=0; i<newSize; i++)
		mAlphabet->insert(alphabetEntry(i, 0));
	CountAlphabet();
	ComputeStdLengths();
}
ItemSet** Database::GetAlphabetSets() const {
	ItemSet **abSets = new ItemSet*[mAlphabet->size()];
	alphabet::iterator it = mAlphabet->begin(), end = mAlphabet->end();
	uint32 curIdx = 0;
	uint32 abSize = (uint32) mAlphabet->size();
	for(; it != end; ++it) {
		abSets[curIdx] = ItemSet::CreateSingleton(mDataType, it->first, abSize, it->second, mAlphabetNumRows[curIdx]);
		curIdx++;
	}
	return abSets;
}

//////////////////////////////////////////////////////////////////////////
//	Internal Statistics Functions
//////////////////////////////////////////////////////////////////////////

bool Database::ComputeEssentials() {
	if(!mAlphabet && !this->DetermineAlphabet())
		return false;
	if(!mStdLengths && !this->ComputeStdLengths())
		return false;
	/*if(!mItemTranslator) {
		// Old impl: 
		//mItemTranslator = new ItemTranslator(mAlphabet);

		// New impl:
		std::multimap<uint32,uint16> reverse;
		for(alphabet::iterator iter = mAlphabet->begin(); iter!=mAlphabet->end(); iter++)
			reverse.insert(std::pair<uint32,uint16>(iter->second, iter->first));
		uint16 *bitToVal = new uint16[mAlphabet->size()];
		uint16 idx = (uint16)mAlphabet->size()-1;
		for(std::multimap<uint32,uint16>::iterator iter = reverse.begin(); iter!=reverse.end(); iter++)
			bitToVal[idx--] = iter->second;
		mItemTranslator = new ItemTranslator(bitToVal, (uint16)mAlphabet->size());
	}*/
	if(mMaxSetLength == 0)
		this->DetermineMaxSetLength();

	return true;
}

bool Database::DetermineAlphabet() {
	if(mAlphabet)
		return false;
	ECHO(3, printf("Database :: Determining alphabet ... "));
	ItemSet *is;
	uint16 val;
	mAlphabet = new alphabet();
	alphabet *abNR = new alphabet();
	alphabet::iterator iter;
	uint32 cnt;
	mNumItems = 0;

	uint32 maxLen = 0;
	for(uint32 row=0; row<mNumRows; row++)
		if(mRows[row]->GetLength() > maxLen)
			maxLen = mRows[row]->GetLength();
	uint32 *values = new uint32[maxLen];

	for(uint32 row=0; row<mNumRows; row++) {
		is = mRows[row];
		is->GetValuesIn(values);
		cnt = is->GetUsageCount();
		for(uint16 col=0; col<is->GetLength(); col++) {
			val = values[col];
			iter = mAlphabet->find(val);
			if(iter == mAlphabet->end()) {
				mAlphabet->insert(alphabetEntry(val, cnt));
				abNR->insert(alphabetEntry(val, 1));
			} else {
				iter->second += cnt;
				iter = abNR->find(val);
				iter->second += 1;
			}
		}
		mNumItems += is->GetLength() * cnt;
	}
	delete[] values;

	delete[] mAlphabetNumRows;
	mAlphabetNumRows = new uint32[abNR->size()];
	uint32 idx = 0;
	for(iter = abNR->begin(); iter != abNR->end(); iter++, idx++)
		mAlphabetNumRows[idx] = iter->second;
	delete abNR;

	return true;
}
void Database::CountAlphabet(bool countAlphabetNumRows) {
	ECHO(3,printf(" * Counting alphabet:\tin progress..."));
	uint32 *vals = new uint32[mAlphabet->size()];
	ItemSet *is;
	uint32 len, cnt;
	alphabet::iterator iter;
	for(iter = mAlphabet->begin(); iter != mAlphabet->end(); iter++)
		iter->second = 0;

	for(uint32 row=0; row<mNumRows; row++) {
		is = mRows[row];
		is->GetValuesIn(vals);
		len = is->GetLength();
		cnt = is->GetUsageCount();
		for(uint32 col=0; col<len; col++) {
			iter = mAlphabet->find(vals[col]);
			if(iter == mAlphabet->end())
				throw string("Item not in alphabet!");
			else
				iter->second += cnt;
		}
	}
	delete[] vals;

	mNumItems = 0;
	for(iter = mAlphabet->begin(); iter != mAlphabet->end(); iter++)
		mNumItems += iter->second;

	if (countAlphabetNumRows) {
		delete[] mAlphabetNumRows;
		mAlphabetNumRows = new uint32[mAlphabet->size()];
		uint32 idx = 0;
		for(iter = mAlphabet->begin(); iter != mAlphabet->end(); iter++, idx++)
			mAlphabetNumRows[idx] = iter->second;
	}


	ECHO(3,printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\bdone            \n"));
}
bool Database::ComputeStdLengths(const bool laplace /* = false */) {
	ECHO(3,	printf(" * Standard lengths:\tcomputing..."));
	if(mStdLengths)
		delete[] mStdLengths;
	mStdLengths = new double[mAlphabet->size()];
	mStdDbSize = 0.0;
	mStdCtSize = 0.0;
	uint32 idx=0;
	mStdLaplace = laplace;

	if(mStdLaplace) {
		uint64 num = mNumItems + mAlphabet->size();
		for(alphabet::iterator iter = mAlphabet->begin(); iter != mAlphabet->end(); iter++, idx++) {
			mStdLengths[idx] = CalcCodeLength(iter->second + 1, num);
			mStdDbSize += iter->second * mStdLengths[idx];
			mStdCtSize += mStdLengths[idx];
		}
	} else {
		for(alphabet::iterator iter = mAlphabet->begin(); iter != mAlphabet->end(); iter++, idx++) {
			mStdLengths[idx] = iter->second == 0 ? 0.0 : CalcCodeLength(iter->second, mNumItems);
			mStdDbSize += iter->second * mStdLengths[idx];
			mStdCtSize += mStdLengths[idx];
		}
	}
	mStdCtSize *= 2;
	ECHO(3,	printf("\r * Standard lengths:\tcomputed.   \n * Standard db size:\t%f\n", mStdDbSize));
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Simple Database Operations
//////////////////////////////////////////////////////////////////////////

uint32 Database::GetSupport(ItemSet *is, uint32 &numOccs) {
	uint32 freq = 0;
	numOccs = 0;
	for(uint32 i=0; i<mNumRows; i++) {
		if(mRows[i]->IsSubset(is)) {
			freq += mRows[i]->GetUsageCount();
			++numOccs;
		}
	}
	return freq;
}

void Database::CalcSupport(ItemSet *is) {
	uint32 freq = 0;
	uint32 numOcc = 0;
	for(uint32 i=0; i<mNumRows; i++) {
		if(mRows[i]->IsSubset(is)) {
			freq += mRows[i]->GetUsageCount();
			++numOcc;
		}
	}
	is->SetSupport(freq);
	is->SetNumDBOccurs(numOcc);
}

void Database::CalcStdLength(ItemSet *is) {
	float stdLen = 0.0f;
	uint16* values = is->GetValues();
	uint32 length = is->GetLength();
	for(uint32 i=0; i<length; i++)
		stdLen += (float)mStdLengths[values[i]];
	is->SetStandardLength(stdLen);
	delete[] values;
}

void Database::EnableFastDBOccurences() {
	if(mFastGenerateDBOccs == false) {
		mFastGenerateDBOccs = true;

		// bepaal alphabet-dbOcc-array
		uint32 abSize = (uint32) mAlphabet->size();

		delete[] mAlphabetDbOccs;
		mAlphabetDbOccs = new uint32 *[abSize];

		uint32 *tempIdx = new uint32[abSize];

		if(mAlphabetNumRows == NULL)
			THROW("geen alphabet num rows available\n");

		for(uint32 i=0; i<abSize; i++) {
			tempIdx[i] = 0;
			uint32 tstNum = mAlphabetNumRows[i];
			mAlphabetDbOccs[i] = new uint32[tstNum];
		}
		uint32 *vals = new uint32[mMaxSetLength];
		uint32 numVals = 0, curIdx = 0, curVal = 0;;

		for(uint32 i=0; i<mNumRows; i++) {
			numVals = mRows[i]->GetLength();
			mRows[i]->GetValuesIn(vals);
			for(uint32 j=0; j<numVals; j++) {
				curVal = vals[j];
				curIdx = tempIdx[curVal];
				mAlphabetDbOccs[curVal][curIdx] = i;
				tempIdx[curVal]++;
			}
		}
		delete[] tempIdx;
		delete[] vals;
	}
}
void Database::DetermineOccurrences(ItemSet *is, bool calcNumOcc) {
	uint32 *occs = NULL;
	uint32 numOcc = 0;
	uint32 idx = 0;

	if(calcNumOcc == false) {
		numOcc = is->GetNumOccs();
		occs = new uint32[numOcc];

		/*	-- SNELSTE VERSIE DIE GEBRUIK MAAKT VAN CORRECTE NumDBOccurs in ItemSet, EN DIE BEPAALD DOOR SUBSET VAN DB ROWS VIA GetLastItem() */
		if(mFastGenerateDBOccs == true) {
			uint16 valLastItem = is->GetLastItem();
			uint32 numLastItem = mAlphabetNumRows[valLastItem];
			if(numOcc == numLastItem) {
				memcpy(occs, mAlphabetDbOccs[valLastItem], sizeof(uint32) * numOcc);
			} else {
				uint32 *abOccs = mAlphabetDbOccs[valLastItem];
				for(uint32 i=0; i<numLastItem; i++) {
					if(mRows[abOccs[i]]->IsSubset(is)) {
						occs[idx++] = abOccs[i];
						if(idx>=numOcc)
							break;
					}
				}
			}
		} 
		/*	-- SNELLERE VERSIE DIE GEBRUIK MAAKT VAN CORRECTE NumDBOccurs IN ItemSet. */
		else {
			for(uint32 i=0; i<mNumRows; i++) {
				if(mRows[i]->IsSubset(is)) {
					occs[idx++] = i;
					if(numOcc == idx)
						break;
				}
			}

		}
	} else {
		// number of occurences are not up2date
		uint32 *tmpArr = NULL;
		uint32 supp = 0;
		if(mFastGenerateDBOccs == true) {
			uint16 lastItem = is->GetLastItem();
			uint32 maxNumOccs = mAlphabetNumRows[lastItem];
			tmpArr = new uint32[maxNumOccs];
			for(uint32 i=0; i<maxNumOccs; i++) {
				if(mRows[mAlphabetDbOccs[lastItem][i]]->IsSubset(is)) {
					supp += mRows[mAlphabetDbOccs[lastItem][i]]->GetUsageCount();
					tmpArr[numOcc++] = mAlphabetDbOccs[lastItem][i];
				}
			}
		} else {
			tmpArr = new uint32[mNumRows];
			for(uint32 i=0; i<mNumRows; i++) {
				if(mRows[i]->IsSubset(is)) {
					supp += mRows[i]->GetUsageCount();
					tmpArr[numOcc++] = i;
				}
			}
		}
		occs = new uint32[numOcc];
		memcpy(occs, tmpArr, numOcc * sizeof(uint32));
		delete[] tmpArr;
		is->SetNumDBOccurs(numOcc);
		is->SetSupport(supp);
	}

	is->SetDBOccurrence(occs, numOcc);

		/*	-- LANGZAME VERSIE WAARBIJ COUNTS NIET CORRECT HOEVEN TE ZIJN (EN BINSIZE PROOF): */
		/*	
		uint32 numOcc = 0, count = 0;
		uint32 numdb = mDatabase->GetNumRows();
		ItemSet *transaction;
		for(uint32 i=0; i<numdb; i++) { // First loop to determine numOcc (unfortunately required...)
		transaction = mDatabase->GetItemSet(i);
		if(transaction->IsSubset(is)) {
		++numOcc;
		count += transaction->GetCount();
		}
		}
		uint32 *occur = new uint32[numOcc];
		numOcc = 0;
		for(uint32 i=0; i<numdb; i++)
		if(mDatabase->GetItemSet(i)->IsSubset(is))
		occur[numOcc++] = i;
		if(is->GetCount() != count) {
		ECHO(2,printf(" * ItemSet count incorrect:\t%s ==> (%d)\n",is->ToString().c_str(), count));
		is->SetCount(count);
		}
		//	-- SNELLERE VERSIE WAARBIJ COUNTS CORRECT MOETEN ZIJN (NIET BINSIZE PROOF):
		// we gaan er maar even vanuit dat de cnt up-to-date is
		uint32 numOcc = is->GetCount();
		uint32 *occur = new uint32[numOcc];
		uint32 cur = 0;
		uint32 numdb = mDatabase->GetNumRows();
		for(uint32 i=0; i<numdb; i++) {
		if(mDatabase->GetItemSet(i)->IsSubset(is))
		occur[cur++] = i;
		}
		*/
}

bool Database::IsItemSetInDatabase(ItemSet *is) {
	for(uint32 i=0; i<mNumRows; i++)
		if(mRows[i]->IsSubset(is))
			return true;
	return false;
}


//////////////////////////////////////////////////////////////////////////
// Database Transformation Operations
//////////////////////////////////////////////////////////////////////////

void Database::Transform(string commandlist) {
	// Randomize
	// TranslateValToBit
	// TranslateBitToVal
	// Transpose
	// SplitOn(..)
	// Bin
	// UnBin
	for(uint32 i=0; i<commandlist.length(); i++) {
		string curCommand = commandlist.substr(i,min(commandlist.find_first_of(','),commandlist.length()));
		//curCommand.erase(' ');
		if(curCommand.compare("bin") == 0)
			Bin();
		else if(curCommand.compare("unbin") == 0)
			UnBin();
	}
}

void Database::RandomizeRowOrder(uint32 seed /* = 0 */) {
	if(seed != 0)
		RandomUtils::Init(seed);
	uint32 row2;
	for(uint32 i=0; i<5; i++)
		for(uint32 row1=0; row1<mNumRows; row1++) {
			row2 = RandomUtils::UniformUint32(mNumRows);
			if(row1 != row2)
				this->SwapRows(row1, row2);
		}
}

void Database::MultiplyCounts(uint32 multiplier) {
	mHasBinSizes = true;
	mNumTransactions = mNumTransactions * multiplier;
	for(uint32 i=0; i<mNumRows; i++)
		mRows[i]->SetUsageCount(mRows[i]->GetUsageCount() * multiplier);
}

// Bins the (presumably unbinned) database, grouping duplicate rows
void Database::Bin() {
	if(mNumRows == 0)
		return;

	ItemSet **bins = new ItemSet*[mNumRows];	// cannot grow larger, so no need for mNumTransactions as max
	uint32 numBins = 0, curBin = 0, setLen = 0, zappedRowCount = 0, pos = 0;
	uint32 *vals = new uint32[mMaxSetLength];

	if(true) {
		// New Stylee Binnin'

		// Order mRows
		ItemSet::SortItemSetArray(mRows, mNumRows, LexAscIscOrder);

		bins[0] = mRows[0];
		mRows[0] = NULL;

		numBins = 0;
		delete[] mAlphabetNumRows;
		size_t abLen = mAlphabet->size();
		mAlphabetNumRows = new uint32[abLen];
		for(uint32 i=0; i<abLen; i++)
			mAlphabetNumRows[i] = 0;

		uint32 startCount = bins[0]->GetUsageCount();
		uint32 numGrouped = 1;
		for(uint32 i=1; i<mNumRows; i++) {
			if(mRows[i]->Equals(bins[numBins])) {
				bins[numBins]->AddUsageCount(mRows[i]->GetUsageCount());
				numGrouped++;
				delete mRows[i];
				mRows[i] = NULL;
			} else {
				setLen = bins[numBins]->GetLength();
				bins[numBins]->GetValuesIn(vals);
				for(uint32 j=0; j<setLen; j++)
					mAlphabetNumRows[vals[j]] += numGrouped;

				// Fix mAlphabetNumRows
				numGrouped = 1;
				numBins++;
				bins[numBins] = mRows[i];
				mRows[i] = NULL;
			}
		}
		numBins++;	// always 1 row, always 1 bin.

		delete[] mRows;
		if(numBins != mNumRows) {
			mRows = new ItemSet*[numBins];
			memcpy(mRows, bins, sizeof(ItemSet*)*numBins);
			delete[] bins;
		} else
			mRows = bins;
		mHasBinSizes = true;
		mNumRows = numBins;		// we didn't fuck with mNumTransactions, so leave it be

	} else if(false) {
		// Old-Fashioned Binnin'

		uint32 numBins = mNumRows;
		size_t abLen = mAlphabet->size();
		mAlphabetNumRows = new uint32[abLen];
		for(uint32 i=0; i<abLen; i++)
			mAlphabetNumRows[i] = mAlphabet->find(i)->second;
		for(uint32 i=0; i<mNumRows; i++) {		// last row cannot be binned by any rows beneath it
			if(mRows[i] != NULL) {
				uint32 prevCount = mRows[i]->GetUsageCount();
				for(uint32 j=i+1; j<mNumRows; j++) {
					if(mRows[j] != NULL && mRows[i]->Equals(mRows[j])) {
						mRows[i]->AddUsageCount(mRows[j]->GetUsageCount());
						delete mRows[j];
						mRows[j] = NULL;
						numBins--;
					}
				}
				bins[curBin++] = mRows[i];

				setLen = mRows[i]->GetLength();
				mRows[i]->GetValuesIn(vals);
				zappedRowCount = mRows[i]->GetUsageCount() - prevCount;
				for(uint32 j=0; j<setLen; j++)
					mAlphabetNumRows[vals[j]] -= zappedRowCount;
			}
		}
		delete[] mRows;
		if(numBins != mNumRows) {
			// pas grootte van bins-array aan
			mRows = new ItemSet*[curBin];
			memcpy(mRows, bins, sizeof(ItemSet*)*numBins);
			delete[] bins;
		} else
			mRows = bins;
		mHasBinSizes = true;
		mNumRows = numBins;		// we didn't fuck with mNumTransactions, so leave it be
	}
	delete[] vals;
}
// Unbins the (presumed binned) database, ungrouping duplicate rows
void Database::UnBin() {
	if(mHasBinSizes) {
		ItemSet **transactions = new ItemSet*[mNumTransactions];
		uint32 curTrans = 0;
		for(uint32 i=0; i<mNumRows; i++) {
			uint32 rowCount = mRows[i]->GetUsageCount();
			for(uint32 t=0; t<rowCount; t++) {
				transactions[curTrans] = mRows[i]->Clone();
				transactions[curTrans]->SetUsageCount(1);
				curTrans++;
			}
			delete mRows[i];
		}
		delete[] mRows;
		mRows = transactions;
		mNumRows = mNumTransactions;
		mHasBinSizes = false;
		if(curTrans != mNumTransactions)
			throw string("Database::UnBin - transaction counts pre- and post-unbinning don't match.");
	} else
		printf("Warning: Attempt at unbinning a not-binned database.");
}
void Database::RemoveDuplicateTransactions() {
	ECHO(1,	printf(" * Removing duplicate transactions:\t\tworking on it..."));
	Bin();
	for(uint32 i=0; i<mNumRows; i++)
		mRows[i]->SetUsageCount(1);
	mHasBinSizes = false;
	mNumTransactions = mNumRows;
	delete[] mAlphabetNumRows; mAlphabetNumRows = NULL;
	CountAlphabet();
	ComputeStdLengths();
	ECHO(1,	printf("\r * Removing duplicate transactions:\t\tdone\t\t\t\t\n\n"));
}
void Database::RemoveTransactionsLongerThan(uint32 maxLen) {
	ECHO(1,	printf(" * Removing transactions longer than %d items:\t\tworking on it...", maxLen));
	uint32 numRows = 0;
	ItemSet **newRows = new ItemSet *[mNumRows];
	for(uint32 i=0; i<mNumRows; i++)
		if(mRows[i]->GetLength() <= maxLen)
			newRows[numRows++] = mRows[i];
		else {
			mNumTransactions -= mRows[i]->GetUsageCount();
			delete mRows[i];
		}
	delete[] mRows;
	mRows = newRows;
	mNumRows = numRows;
	DetermineMaxSetLength();
	CountAlphabet();
	ComputeStdLengths();
	ECHO(1,	printf("\r * Removing transactions longer than %d items:\t\tdone\t\t\t\t\n\n", maxLen));
}
void Database::RemoveDoubleItems() {
	if(mDataType == Uint16ItemSetType) {
		ECHO(1,	printf(" * Removing double items:\t\tworking on it..."));
		for(uint32 i=0; i<mNumRows; i++)
			((Uint16ItemSet *)mRows[i])->RemoveDoubleItems();
		ECHO(1,	printf("\r * Removing double items:\t\tdone\t\t\t\t\n\n"));
	} else {
		ECHO(1,	printf("\r * Removing double items:\t\tnot needed with bitmask\t\t\n\n"));
	}
}
void Database::RemoveFrequentItems(float frequencyThreshold) {
	ItemSet *zapItems = ItemSet::CreateEmpty(mDataType, (uint32)mAlphabet->size());
	uint32 absFreqThresh = (uint32)ceil(frequencyThreshold * mNumRows);
	for(alphabet::iterator iter = mAlphabet->begin(); iter != mAlphabet->end(); iter++) {
		if(iter->second >= absFreqThresh)
			zapItems->AddItemToSet(iter->first);
	}
	RemoveItems(zapItems);
}
void Database::RemoveItems(ItemSet *zapItems) {
	uint16 *auxArr = zapItems->GetValues();
	uint32 numValues = zapItems->GetLength();
	RemoveItems(auxArr, numValues);
	delete[] auxArr;
}
void Database::RemoveItems(uint16 *zapItems, uint32 numItems) {
	uint32 newNumRows = 0;
	for(uint32 i=0; i<mNumRows; i++) {
		for(uint32 j=0; j<numItems; j++) {
			if(mRows[i]->IsItemInSet(zapItems[j]))
				mRows[i]->RemoveItemFromSet(zapItems[j]);
		}
		if(mRows[i]->GetLength() > 0)
			mRows[newNumRows++] = mRows[i];
		else {
			mNumTransactions -= mRows[i]->GetUsageCount();
			delete mRows[i];
		}
	}
	mNumRows = newNumRows;
	CountAlphabet();
	ComputeStdLengths();
}


Database* Database::Transpose() {
	if(mNumRows > UINT16_MAX_VALUE)
		throw string("Database::Transpose -- Number of rows too large (>65k) to be transposed");

	ECHO(1,	printf(" * Transposing:\t\tworking on it..."));
	uint32 idx=0;
	uint32 len=0;
	uint16 *emptySet = new uint16[0], *set = NULL;

	uint32 alphLen = mNumRows;
	uint32 numRows = (uint32) mAlphabet->size();
	ItemSet **iss = new ItemSet *[numRows];
	for(uint32 i=0; i<numRows; i++)
		iss[i] = ItemSet::CreateEmpty(Uint16ItemSetType, alphLen);

	for(uint32 i=0; i<alphLen; i++) {
		set = mRows[i]->GetValues();
		len = mRows[i]->GetLength();
		for(uint32 j=0; j<len; j++)
			iss[set[j]]->AddItemToSet(i);
		delete[] set;
	}

	delete[] emptySet;

	ECHO(1,	printf("\r * Transposing:\t\tdone\t\t\t\t\n\n"));
	return new Database(iss, numRows, false, 0, mNumItems);
}

// Applies ItemTranslator in a backward fashion, back to the original values thus
void Database::TranslateBackward() {
	if(mItemTranslator == NULL)
		throw string("Unable to translate backward without valid ItemTranslator");

	for(uint32 row=0; row<mNumRows; row++)
		mRows[row]->TranslateBackward(mItemTranslator);
	alphabet *a = new alphabet();
	for(alphabet::iterator iter=mAlphabet->begin(); iter!=mAlphabet->end(); iter++)
		a->insert(alphabetEntry(mItemTranslator->BitToValue(iter->first), iter->second));
	delete mAlphabet;
	mAlphabet = a;
}

/* Applies ItemTranslator in a forward fashion, away from the original values, towards the Fic-optimized ones */
void Database::TranslateForward() {
	// If Translator not present, build it!
	if(mItemTranslator == NULL)
		mItemTranslator = BuildItemTranslator();

	// Translate Database Rows
	for(uint32 row=0; row<mNumRows; row++)
		mRows[row]->TranslateForward(mItemTranslator);

	// Translate Alphabet -- this is not enough, because mAlphabetNumRows is incorrect in FIMI format and needs to be fully reconstructed
	/*alphabet *a = new alphabet();
	for(alphabet::iterator iter=mAlphabet->begin(); iter!=mAlphabet->end(); iter++)
		a->insert(alphabetEntry(mItemTranslator->ValueToBit(iter->first), iter->second));
	delete mAlphabet;
	mAlphabet = a;*/

	delete mAlphabet;
	mAlphabet = NULL;
	DetermineAlphabet();

	// Translate Column Definition
	for(uint32 i=0; i<mNumColumns; i++)
		mColumnDefinition[i]->TranslateForward(mItemTranslator);
}

/* Creates an ItemTranslator for translating raw item numbers [0-...] to bitval (occurrence sorted) [0-n] */
ItemTranslator* Database::BuildItemTranslator() {
	std::multimap<uint32,uint16> trans;
	for(alphabet::iterator iter = mAlphabet->begin(); iter!=mAlphabet->end(); iter++)
		trans.insert(std::pair<uint32,uint16>(iter->second, iter->first));

	uint16 *bitToVal = new uint16[mAlphabet->size()];	// size of alphabet, as just as many bits as alphabet elements are necessary
	uint16 idx = (uint16)mAlphabet->size()-1;
	for(std::multimap<uint32,uint16>::iterator iter = trans.begin(); iter!=trans.end(); iter++) {
		bitToVal[idx--] = iter->second;
	}

	ItemTranslator* it = new ItemTranslator(bitToVal, (uint16)mAlphabet->size());
	return it;
}

void Database::SortWithinRows() {
	for(uint32 row=0; row<mNumRows; row++)
		mRows[row]->Sort();
}
void Database::SwapRows(uint32 row1, uint32 row2) {
	ItemSet *is = mRows[row1];
	mRows[row1] = mRows[row2];
	mRows[row2] = is;
}

void Database::SwapRandomise(uint64 numSwaps) {
	uint64 num = numSwaps == 0 ? mNumItems : numSwaps;	
	size_t alphLen = mAlphabet->size();
	uint32 len, tries;
	uint32 row1, row2, col1, col2;
	ItemSet *is1, *is2;
	uint32 *vals1 = new uint32[alphLen], *vals2 = new uint32[alphLen];
	uint32 val1, val2;
	bool found;
	RandomUtils::Init();

	for(uint64 i=0; i<num; i++) {
		ECHO(3,	printf(" * Swapping databases:\t\t%d/%d\r", i, num));

		found = false;

		// find swappable items
		while(!found) {
			row2 = row1 = RandomUtils::UniformUint32(mNumRows); // uint32 from [0,mNumRows)
			is1 = mRows[row1];
			is1->GetValuesIn(vals1);
			col1 = RandomUtils::UniformUint32(is1->GetLength());
			val1 = vals1[col1];

			for(tries=0; (row1 == row2 || is2->IsItemInSet(val1)) && tries<mNumRows; tries++) {
				row2 = RandomUtils::UniformUint32(mNumRows); // uint32 from [0,mNumRows)
				is2 = mRows[row2];
				is2->GetValuesIn(vals2);
			}
			if(tries == mNumRows)
				continue;
			len = is2->GetLength();
			for(tries=0; tries<len; tries++) {
				col2 = RandomUtils::UniformUint32(len);
				val2 = vals2[col2];

				if(val1 != val2 && !is1->IsItemInSet(val2)) {
					found = true;
					break;
				}
			}
		}

		// perform swap
		is1->RemoveItemFromSet(val1);
		is1->AddItemToSet(val2);
		is2->RemoveItemFromSet(val2);
		is2->AddItemToSet(val1);
	}
	delete[] vals1;
	delete[] vals2;
}


void Database::RemoveFromDatabase(ItemSet *is) {
	mMaxSetLength = 0;		// want dan, via ComputeEssentials		DetermineMaxSetLength();

	for(uint32 row=0; row<mNumRows; row++) {
		if(	mRows[row]->IsSubset(is) )
			mRows[row]->Remove(is);
	}
	CountAlphabet();
	ComputeStdLengths();
}

// Row-based, cloning
Database** Database::Split(const uint32 numParts) {
	//if(numParts <= 1)
	//	throw string("Database::Split - Daar beginnen we niet aan.");

	// orig Split

	Database **splitDBs = new Database*[numParts];
	uint32 maxPartSize = (uint32)(mNumRows / (float)numParts);

	uint32 curRowIdx = 0;
	uint32 curPartTrans = 0;

	for(uint32 i=0; i<numParts; i++) {
		uint32 curPartNumRows = maxPartSize;
		if(i+1 == numParts)
			curPartNumRows = mNumRows - curRowIdx;
		

		ItemSet **curPartRows = new ItemSet*[curPartNumRows];
		for(uint32 j=0; j<curPartNumRows; j++) {
			curPartRows[j] = mRows[curRowIdx++]->Clone();
		}

		splitDBs[i] = new Database(curPartRows,curPartNumRows,mHasBinSizes);
		splitDBs[i]->SetAlphabet(new alphabet(*mAlphabet));
		if(mItemTranslator != NULL)
			splitDBs[i]->SetItemTranslator(new ItemTranslator(mItemTranslator));
		splitDBs[i]->CountAlphabet();
		splitDBs[i]->ComputeEssentials();
	}

	return splitDBs;


	/*
	// testvariantje:
	Database **splitDBs = new Database*[numParts];
	uint32 maxPartSize = (uint32)(mNumRows / (float)numParts);

	uint32 curRowIdx = 0;
	uint32 curPartTrans = 0;

	for(uint32 i=0; i<numParts; i++) {
		uint32 curPartNumRows = maxPartSize;

		ItemSet **curPartRows = new ItemSet*[curPartNumRows];
		for(uint32 j=0; j<curPartNumRows; j++) {
			curPartRows[j] = mRows[j]->Clone();
		}

		splitDBs[i] = new Database(curPartRows,curPartNumRows,mHasBinSizes);
		splitDBs[i]->SetAlphabet(new alphabet(*mAlphabet));
		splitDBs[i]->SetItemTranslator(new ItemTranslator(mItemTranslator));
		splitDBs[i]->CountAlphabet();
		splitDBs[i]->ComputeEssentials();
	}

	return splitDBs;
*/
}

Database** Database::SplitForCrossValidationIntoDBs(const uint32 numFolds, bool randomizeRowOrder /* = false */) {
	Database **foldsDBs = new Database*[numFolds];

	uint32 *partitionSizes = new uint32[numFolds];
	ItemSet *** partitions = Database::SplitForCrossValidation(numFolds, partitionSizes, randomizeRowOrder);

	for(uint32 i=0; i<numFolds; i++) {
		foldsDBs[i] = new Database(partitions[i], partitionSizes[i], mHasBinSizes);
		foldsDBs[i]->SetAlphabet(new alphabet(*mAlphabet));
		if(mItemTranslator != NULL)
			foldsDBs[i]->SetItemTranslator(new ItemTranslator(mItemTranslator));
		foldsDBs[i]->CountAlphabet();
		foldsDBs[i]->ComputeEssentials();
		ItemSet **colDef = new ItemSet*[mNumColumns];
		for(uint32 c=0; c<mNumColumns; c++)
			colDef[c] = mColumnDefinition[c]->Clone();
		foldsDBs[i]->SetColumnDefinition(colDef,mNumColumns);
		uint16 *classDef = new uint16[mNumClasses];
		memcpy(classDef,mClassDefinition,mNumClasses * sizeof(uint16));
		foldsDBs[i]->SetClassDefinition(mNumClasses,classDef);
	}
	delete[] partitions;
	delete[] partitionSizes;

	return foldsDBs;
}

ItemSet*** Database::SplitForCrossValidation(const uint32 numFolds, uint32 *partitionSizes, bool randomizeRowOrder /* = false */) {
	ItemSet ***partitions = new ItemSet **[numFolds];
	ItemSet **partition;
	uint32 size;

	if(randomizeRowOrder == true)
		RandomizeRowOrder();

	if(numFolds == 1) {
		partitionSizes[0] = mNumRows;
		partition = partitions[0] = new ItemSet *[mNumRows];
		for(uint32 i=0; i<mNumRows; i++)
			partition[i] = mRows[i]->Clone();

	} else {
		if(mHasBinSizes) {		// Use bin sizes
#if 0
			// Faster version -- gives less nice distribution though, too neat distribution
			uint32 *previous = new uint32[numFolds];
			uint32 *numRows = new uint32[numFolds];
			ItemSet ***iss = new ItemSet **[numFolds];
			for(uint32 i=0; i<numFolds; i++) {
				previous[i] = UINT_MAX;
				numRows[i] = 0;
				iss[i] = new ItemSet *[mNumRows];
			}

			uint32 fold = 0;
			uint32 cnt;
			for(uint32 row=0; row<mNumRows; row++) {
				cnt = mItemSets[row]->GetCount();
				for(uint32 c=0; c<cnt; c++) {
					if(previous[fold] == row)
						iss[fold][numRows[fold]-1]->AddOneToCount();
					else {
						iss[fold][numRows[fold]] = mItemSets[row]->Clone();
						iss[fold][numRows[fold]++]->SetCount(1);
						previous[fold] = row;
					}
					if(++fold == numFolds)
						fold = 0;
				}
				//printf("\rCurrent row: %d\t\t\t\t", row);
			}
			for(uint32 f=0; f<numFolds; f++) {
				partitions[f] = new ItemSet *[numRows[f]];
				memcpy(partitions[f], iss[f], numRows[f]*sizeof(ItemSet*));
				partitionSizes[f] = numRows[f];
				delete[] iss[f];
			}
			delete[] numRows;
			delete[] previous;
			delete[] iss;

#elif 1
			// Statistically correct version
			uint32 *counts = new uint32[mNumRows];
			uint32 *partition = new uint32[mNumRows];
			for(uint32 r=0; r<mNumRows; r++)
				counts[r] = mRows[r]->GetUsageCount();
			uint32 numLeft = mNumTransactions;
			size = (uint32)ceil(mNumTransactions / (float)numFolds);
			uint32 curNumRows;
			uint32 trans;
			uint32 row = 0;

			// foreach fold
			for(uint32 f=0; f<numFolds; f++) {
				// start with empty partition
				for(uint32 r=0; r<mNumRows; r++)
					partition[r] = 0;
				curNumRows = 0;

				for(uint32 i=0; i<size && numLeft>0; i++) {
					trans = RandomUtils::UniformUint32(numLeft);

					// find physhical row this transaction points at
					row = 0;
					while(trans>0) {
						if(trans < counts[row])
							break;
						trans -= counts[row++];
					}
					while(counts[row] == 0)
						row++;

					// add to current partition
					if(partition[row]==0)
						++curNumRows;
					partition[row]++;
					counts[row]--;
					numLeft--;

					if(numLeft % 100000 == 0)
						printf("\rPartition %d - %u left - size %d\t\t\t", f, numLeft, i);
				}
				printf("\r Creating partition %d\t\t\t\t\t\t", f);
				ItemSet **iss = new ItemSet *[curNumRows];
				curNumRows = 0;
				for(uint32 r=0; r<mNumRows; r++) {
					if(partition[r] > 0) {
						iss[curNumRows] = mRows[r]->Clone();
						iss[curNumRows++]->SetUsageCount(partition[r]);
					}
				}
				partitions[f] = iss;
				partitionSizes[f] = curNumRows;
			}
			delete[] partition;
			delete[] counts;
#endif

		} else {				// Faster version without bin sizes
			uint32 row = 0;
			uint32 i;
			size = (uint32)ceil(mNumRows / (float)numFolds);
			for(uint32 f=0; f<numFolds; f++) {
				partition = partitions[f] = new ItemSet *[size];
				for(i=0; i<size && row<mNumRows; i++)
					partition[i] = mRows[row++]->Clone();
				partitionSizes[f] = i;
			}
		}
	}
	return partitions;
}


void Database::SplitForClassification(const uint32 numClasses, const uint32 *classes, Database **classDbs) {
	ItemSet **iss;
	uint32 numRows;
	uint16 target;
	Database *db;
	uint8 outbak = Bass::GetOutputLevel(); Bass::SetOutputLevel(0);

	for(uint32 c=0; c<numClasses; c++) {
		target = classes[c];
		numRows = mHasBinSizes ? mNumRows : mAlphabet->find(target)->second; // less than optimal memusage with binsizes (but fast)
		iss = new ItemSet *[numRows];

		uint32 idx = 0;
		for(uint32 i=0; i<mNumRows && idx<numRows; i++)
			if(mRows[i]->IsItemInSet(target)) {
				iss[idx] = mRows[i]->Clone();
				iss[idx++]->RemoveItemFromSet(target);
			}

		db = classDbs[c] = new Database(iss, idx, mHasBinSizes);
		db->SetAlphabet(new alphabet(*mAlphabet));
		if(mItemTranslator != NULL)
			db->SetItemTranslator(new ItemTranslator(mItemTranslator));
		db->CountAlphabet();
		db->ComputeEssentials();
	}
	Bass::SetOutputLevel(outbak);
}

Database* Database::Merge(Database *db1, Database *db2) {
	uint32 numRows1 = db1->GetNumRows();
	uint32 numRows2 = db2->GetNumRows();
	uint32 numRowsN = numRows1 + numRows2;

	ItemSet** rows1 = db1->GetRows();
	ItemSet** rows2 = db2->GetRows();

	ItemSet **rows = new ItemSet*[numRowsN];
	for(uint32 i=0; i<numRows1; i++)
		rows[i] = rows1[i]->Clone();

	for(uint32 i=0; i<numRows2; i++)
		rows[numRows1 + i] = rows2[i]->Clone();

	Database *db = new Database(rows, numRowsN, db1->HasBinSizes() | db2->HasBinSizes());
	db->SetHasTransactionIds(db1->HasTransactionIds() | db2->HasTransactionIds());
	db->SetNumTransactions(db1->GetNumTransactions() + db2->GetNumTransactions());
	db->SetAlphabet(new alphabet(*db1->GetAlphabet()));
	if(db1->GetItemTranslator() != NULL)
		db->SetItemTranslator(new ItemTranslator(db1->GetItemTranslator()));
	db->CountAlphabet();
	db->ComputeEssentials();
	return db;
}
Database* Database::Merge(Database **dbs, uint32 numDbs) {
	uint32 numRows = 0;
	uint32 numTrans = 0;
	bool hasBinSizes = false;
	for(uint32 i=0; i<numDbs; i++) {
		numRows += dbs[i]->GetNumRows();
		numTrans += dbs[i]->GetNumTransactions();
		hasBinSizes |= dbs[i]->HasBinSizes();
	}

	ItemSet **rows = new ItemSet*[numRows];
	uint32 idx = 0;
	for(uint32 i=0; i<numDbs; i++) {
		uint32 nRows = dbs[i]->GetNumRows();
		ItemSet **iRows = dbs[i]->GetRows();
		for(uint32 j=0; j<nRows; j++)
			rows[idx++] = iRows[j]->Clone();
	}

	Database *db = new Database(rows, numRows, hasBinSizes);
	db->SetNumTransactions(numTrans);
	db->SetAlphabet(new alphabet(*dbs[0]->GetAlphabet()));
	db->SetItemTranslator(dbs[0]->GetItemTranslator()==NULL ? NULL : new ItemTranslator(dbs[0]->GetItemTranslator()));
	db->CountAlphabet();
	db->ComputeEssentials();
	return db;
}

Database** Database::SplitOnItemSet(ItemSet *is) {
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

	uint32 idxPos = 0, idxNeg = 0;
	for(uint32 i=0; i<mNumRows; i++) {
		if(mask[i])
			issPos[idxPos++] = mRows[i]->Clone();
		else
			issNeg[idxNeg++] = mRows[i]->Clone();
	}
	delete[] mask;

	Database **dbs = new Database *[2];

	dbs[0] = new Database(issNeg, numNeg, mHasBinSizes);
	dbs[0]->SetAlphabet(new alphabet(*mAlphabet));
	dbs[0]->SetItemTranslator(new ItemTranslator(mItemTranslator));
	dbs[0]->CountAlphabet();
	dbs[0]->ComputeEssentials();

	dbs[1] = new Database(issPos, numPos, mHasBinSizes);
	dbs[1]->SetAlphabet(new alphabet(*mAlphabet));
	dbs[1]->SetItemTranslator(new ItemTranslator(mItemTranslator));
	dbs[1]->CountAlphabet();
	dbs[1]->ComputeEssentials();

	Bass::SetOutputLevel(outbak);
	return dbs;
}

Database* Database::Subset(uint32 fromRow, uint32 toRow) {
	if(fromRow > toRow)
		return NULL;
	if(toRow > mNumRows)
		return NULL;
	
	uint32 numRows = toRow - fromRow + 1;
	uint32 idx = 0;
	ItemSet **iss = new ItemSet *[numRows];
	for(uint32 r=fromRow; r<=toRow; r++)
		iss[idx++] = mRows[r]->Clone();

	Database *db = new Database(iss, numRows, mHasBinSizes);
	db->SetAlphabet(new alphabet(*mAlphabet));
	if(mItemTranslator)
		db->SetItemTranslator(new ItemTranslator(mItemTranslator));
	db->CountAlphabet();
	db->ComputeEssentials();
	return db;
}

void Database::DetermineMaxSetLength() {
	mMaxSetLength = 0;
	for(uint32 i=0; i<mNumRows; i++)
		if(mRows[i]->GetLength() > mMaxSetLength)
			mMaxSetLength = mRows[i]->GetLength();
}


//////////////////////////////////////////////////////////////////////////
// Database Column Definition
//////////////////////////////////////////////////////////////////////////

void Database::SetColumnDefinition(ItemSet** colDef, uint32 numColumns) {
	for(uint32 i=0; i<mNumColumns; i++)
		delete mColumnDefinition[i];
	delete[] mColumnDefinition;
	mColumnDefinition = colDef;
	mNumColumns = numColumns;
	mHasColumnDefinition = numColumns > 0;
}

/*void Database::DetermineColumnDefinition(...) {
	// Laad .dat (if not yet loaded), extract coldef, stop in .db
}*/


//////////////////////////////////////////////////////////////////////////
// Classes
//////////////////////////////////////////////////////////////////////////
void Database::SetClassDefinition(uint32 numClasses, uint16 *classes) {
	if(mClassDefinition != NULL) {
		delete[] mClassDefinition;
		//	throw string("Database::SetClasses() -- Cannot reset class info.");
	}

	mNumClasses = numClasses;
	mClassDefinition = classes;
}



//////////////////////////////////////////////////////////////////////////
// Database Read/Write Operations
//////////////////////////////////////////////////////////////////////////

void Database::Write(const string &dbName, const string &path, const DbFileType dbType) {
	Database::WriteDatabase(this, dbName, path, dbType);
}

/* Retrieves database from repository directories (gDxecDir, gDataDir, gDBReposDir) */
Database* Database::RetrieveDatabase(const string &dbName, const ItemSetType dataType /* optional, BM128ItemSet default */, const DbFileType dbType /* optional, extension-dependent, defaults to FicDbFileType*/) {
	if(dbName.find_last_of("\\/") != string::npos)
		throw string("Please provide a database name to retrieve, not a path");

	string inDbBaseName = dbName.substr(0,dbName.find_first_of("[."));

	// - Determine the database filename
	string inDbFileName = dbName;
	string inDbFileExtension = "";
	if((dbName.find('.') == string::npos)) {
		// - Add right extension, .db as default, 
		inDbFileName += "." + ((dbType != DbFileTypeNone) ? DbFile::TypeToExt(dbType) : "db");
		inDbFileExtension = (dbType != DbFileTypeNone) ? DbFile::TypeToExt(dbType) : "db";
	} else
		inDbFileExtension = dbName.substr(dbName.find_last_of('.')+1);

	// - Determine type of file to load
	DbFileType inDbType = dbType == DbFileTypeNone ? DbFile::ExtToType(inDbFileExtension) : dbType;
	if(inDbType == DbFileTypeNone)
		throw string("Unknown database encoding.");

	// - Determine the path to the requested database
	string inDbPath = "";
	if(FileUtils::FileExists(Bass::GetExecDir() + inDbFileName))
		inDbPath = string(Bass::GetExecDir());
	else if(FileUtils::FileExists(Bass::GetWorkingDir() + inDbFileName))
		inDbPath = Bass::GetWorkingDir();
	else if(FileUtils::FileExists(Bass::GetExpDir() + inDbFileName))
		inDbPath = Bass::GetExpDir();
	else if(FileUtils::FileExists(Bass::GetDataDir() + inDbFileName))
		inDbPath = Bass::GetDataDir();
	else if(FileUtils::FileExists(Bass::GetDbReposDir() + inDbFileName))
		inDbPath = Bass::GetDbReposDir();
	else if(FileUtils::FileExists(Bass::GetDbReposDir() + inDbBaseName + "/" + inDbFileName))
		inDbPath = Bass::GetDbReposDir() + inDbBaseName + "/";
	else if (FileUtils::FileExists(Bass::GetDataDir() + inDbBaseName + "/" + inDbFileName))
		inDbPath = Bass::GetDataDir() + inDbBaseName + "/";
	else
		throw string("Unable to retrieve " + dbName + " from repository directories.");

	return Database::ReadDatabase(inDbFileName, inDbPath, dataType, inDbType);
}

/* Stores Database* `db` into the DatabaseRepository as a FicDbFile */
void Database::StoreDatabase(Database* const db, const string &dbName) {
	if(dbName.find_last_of("\\/") != string::npos)
		throw string("Please only providy a (file)name to store the database in the repository, not a path");
	if(dbName.length() == 0)
		throw string("Please provide a (file)name to store the database in the repository");
	//if(dbName.find('.') != string::npos && dbName.substr(dbName.find_last_of('.')).compare(DbFile::TypeToExt(FicDbFileType)) != 0)
	//	throw string("Can only store FicDb files into the repository");
	string dbBaseName = dbName.substr(0,dbName.find_first_of("[."));
	string reposPath = Bass::GetDbReposDir();
	if(FileUtils::DirectoryExists(reposPath + dbBaseName))
		reposPath = Bass::GetDbReposDir() + dbBaseName;

	Database::WriteDatabase(db, dbName, reposPath, DbFileTypeFic);
}


/* Reads database file of `dbName` from `path` into a Database object consisting of rows of ItemSetType `dataType`
	- Preferred use is to split database name (with or without extension) and the path. Alternatively, the path will be extracted from the provided filename.
	- Database (file)name extension is optional. Defaults to ".db"
	- Path defaults to current directory
	- ItemSetType defaults to BM128ItemSet
	- DbFileType defaults to (.db ->) FicDbFileType
*/
Database* Database::ReadDatabase(const string &dbName, const string &path /* optional, defaults to local directory */, const ItemSetType dataType /* optional, BM128ItemSet default */, const DbFileType dbType /* optional, extension-dependent, defaults to FicDbFileType*/) {
	string inDbPath = path;
	string inDbFileName = dbName;
	string inDbFileExtension = "";

	string inDbFullPath = DeterminePathFileExt(inDbPath, inDbFileName, inDbFileExtension, dbType);

	// - Check whether designated file exists
	if(!FileUtils::FileExists(inDbFullPath))
		throw string("Database file does not seem to exist.");

	// - Determine type of file to load
	DbFileType inDbType = dbType == DbFileTypeNone ? DbFile::ExtToType(inDbFileExtension) : dbType;
	if(inDbType == DbFileTypeNone)
		throw string("Unknown database encoding.");

	// - Create appropriate DbFile to load that type of database
	DbFile *dbFile = DbFile::Create(inDbType);

	// - Determine internal datatype to read into
	ItemSetType inDbItemSetType = dataType == NoneItemSetType ? BM128ItemSetType : dataType;
	dbFile->SetPreferredDataType(inDbItemSetType);

	Database *db = NULL;
	try {	// Memory-leak-protection-try/catch
		db = dbFile->Read(inDbFullPath);
		db->SetName(inDbFileName.substr(0,inDbFileName.find_last_of('.')));
	} catch(...) {	
		delete dbFile;	
		throw;
	}
	delete dbFile;

	if(!db)
		throw string("Could not read input database: " + inDbFullPath);
	return db;
}

// Shallt not fux0r the Database up.
string Database::WriteDatabase(Database * const db, const string &dbName, const string &path, const DbFileType dbType) {
	// - Determine whether `dbName` does not abusively contains a path
	size_t dbNameLastDirSep = dbName.find_last_of("\\/");

	// - Determine `outDbPath`
	string outDbPath = (path.length() == 0 && dbNameLastDirSep != string::npos) ? dbName.substr(0,dbNameLastDirSep) : path;

	//  - Check whether is a valid (slash or backslash-ended) path
	if(outDbPath.length() != 0 && outDbPath[outDbPath.length()-1] != '/' && outDbPath[outDbPath.length()-1] != '\\')
		outDbPath += '/';

	// - Determine `outDbFileName` = dbName + . + extension
	string outDbFileName = (dbNameLastDirSep != string::npos) ? dbName.substr(dbNameLastDirSep+1) : dbName;
	string outDbFileExtension = "";

	// - Determine `outDbType` (preliminary defaulting, in case extension is provided via `dbName`)
	DbFileType outDbType = (dbType == DbFileTypeNone) ? DbFileTypeFic : dbType;

	if(dbName.find('.') == string::npos) {
		//  - No extension provided, so extract from DbType
		outDbFileExtension = DbFile::TypeToExt(outDbType);
		outDbFileName += "." + outDbFileExtension;
	} else {
		// - Wel een punt-stuk in de db(file)name, niet per definitie een extensie
		string posExt = dbName.substr(dbName.find_last_of('.')+1);
		DbFileType posDbType = DbFile::ExtToType(posExt);

		// - Geen aparte extensie meegegeven, wel puntStuk in filename
		if(posDbType == DbFileTypeNone) {
			// - Geen bekende extensie, voeg zelf toe al naar gelang outDbType
			outDbFileExtension = (outDbType != DbFileTypeNone) ? DbFile::TypeToExt(outDbType) : "db";
			outDbFileName += "." + outDbFileExtension;
		} else {
			// - Bekende extensie
			if(dbType == DbFileTypeNone) {
				// - gebruik posExt
				outDbType = posDbType;
				outDbFileExtension = posExt;
				// outDbFileName = zit ie al in!
			} else {
				// - gebruik dbType
				outDbFileExtension = (outDbType != DbFileTypeNone) ? DbFile::TypeToExt(outDbType) : "db";
				outDbFileName += "." + outDbFileExtension;
			}
		}
	}

	// - Determine `outDbFullPath` = outDbPath + outDbFileName 
	string outDbFullPath = outDbPath + outDbFileName;

	// - Throws error in case unknown dbType.
	DbFile *dbFile = DbFile::Create(outDbType);
	if(!dbFile->Write(db, outDbFullPath)) {
		delete dbFile;
		throw string("Error: Could not write database.\n");
	}
	delete dbFile;

	return outDbFullPath;
}

// filename may or may not contain extension
string Database::DeterminePathFileExt(string &path, string &filename, string &extension, const DbFileType dbType) {
	// - Split path from filename (if required, does nothing otherwise)
	FileUtils::SplitPathFromFilename(filename, path);

	// DB files -moeten- een extensie hebben.
	if(filename.find('.') == string::npos) {
		// iig geen extensie in de (file)name
		if(extension.length() == 0) { 
			// - Geen aparte extensie meegegeven, voeg zelf toe al naar gelang dbType
			filename += "." + ((dbType != DbFileTypeNone) ? DbFile::TypeToExt(dbType) : "db");
			extension = (dbType != DbFileTypeNone) ? DbFile::TypeToExt(dbType) : "db";
		} else {	
			// - Aparte extensie meegegeven, zit per definitie nog niet in filename (nl geen '.' in filename) 
			if(extension[0] == '.')
				extension = extension.substr(1);
			filename += "." + extension;
		}
	} else {
		// - Wel een punt-stuk in de (file)name, niet per definitie een extensie
		string posExt = filename.substr(filename.find_last_of('.')+1);
		if(extension.length() == 0) {	
			// - Geen aparte extensie meegegeven, wel puntStuk in filename
			if(DbFile::ExtToType(posExt) == DbFileTypeNone) {
				// - Geen bekende extensie, voeg zelf toe al naar gelang dbType
				filename += "." + ((dbType != DbFileTypeNone) ? DbFile::TypeToExt(dbType) : "db");
				extension = (dbType != DbFileTypeNone) ? DbFile::TypeToExt(dbType) : "db";
			} else {
				// - Bekende extensie, zal 'm wel zijn dan
				extension = posExt;
				if(extension[0] == '.')
					extension = extension.substr(1);
			}
		} else {	
			// - Wel aparte extensie meegegeven
			if(extension[0] == '.')
				extension = extension.substr(1);
			if(posExt.compare(extension) != 0) {
				// - Alleen als anders dan opgegeven, voeg 'm maar toe
				filename += "." + extension;
			}	
		}
	}


	// - Determine `inDbFullPath`
	string fullPath = path + filename;

	// - Check whether designated file exists
	/*if(!FileUtils::FileExists(fullPath))
		throw string("Database file does not seem to exist.");*/

	return fullPath;
}

void Database::WriteStdLengths(const string &filename) {
	FILE *stdLenFile;
	if(fopen_s(&stdLenFile, filename.c_str(), "w") != 0)
		throw string("Database::WriteStdLengths() -- Cannot open file.");
	fprintf(stdLenFile, "ficstdlen-1.0\n");
	size_t num = GetAlphabetSize();
	fprintf(stdLenFile, "%d %d\n", num, mStdLaplace ? 1 : 0);
	for(size_t i=0; i<num; i++)
		fprintf(stdLenFile, "%f\n", mStdLengths[i]);
	fclose(stdLenFile);
}
void Database::ReadStdLengths(const string &filename) {
	char buffer[200];
	uint32 num, laplace;
	FILE *stdLenFile;
	if(!mStdLengths) mStdLengths = new double[GetAlphabetSize()];
	float len = 0.0;

	if(fopen_s(&stdLenFile, filename.c_str(), "r") != 0)
		throw string("Database::ReadStdLengths() -- Cannot open file.");
	if(!fgets(buffer, 200, stdLenFile))
		throw string("Database::ReadStdLengths() -- Cannot read first line from file.");
	if(strcmp(buffer, "ficstdlen-1.0\n") == 0) {
		if(fscanf_s(stdLenFile, "%d %d\n", &num, &laplace) < 2)
			throw string("Database::ReadStdLengths() -- Incorrect header.");
		if(num != GetAlphabetSize()) {
			sprintf_s(buffer, 200, "Database::ReadStdLengths() -- Incorrect alphabet size: %d != %d", num, GetAlphabetSize());
			throw string(buffer);
		}
		mStdLaplace = laplace == 1 ? true : false;
		for(uint32 i=0; i<num; i++)
			if(fscanf_s(stdLenFile, "%f\n", &len) < 1)
				throw string("Database::ReadStdLengths() -- Could not read std length.");
			else
				mStdLengths[i] = len;
	} else
		throw string("Database::ReadStdLengths() -- Unknown format.");
	fclose(stdLenFile);
}
