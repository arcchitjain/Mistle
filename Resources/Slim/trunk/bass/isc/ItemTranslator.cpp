#include "../Bass.h"

#include "ItemTranslator.h"

ItemTranslator::ItemTranslator(ItemTranslator *it) {
	mAlphabetLen = it->mAlphabetLen;
	mBitToVal = new uint16[mAlphabetLen];
	memcpy(mBitToVal, it->mBitToVal, sizeof(uint16) * mAlphabetLen);
	mValToBit = new valBitMap(*it->mValToBit);
}

ItemTranslator::ItemTranslator(alphabet *a) {
	mAlphabetLen = (uint16)a->size();
	mBitToVal = new uint16[mAlphabetLen];
	mValToBit = new valBitMap();

	uint16 idx = 0;
	for(alphabet::iterator iter = a->begin(); iter != a->end(); iter++, idx++) {
		mBitToVal[idx] = iter->first;
		mValToBit->insert(valBitMapEntry(iter->first, idx));
	}

	/*
	std::multimap<uint32,uint16> reverse;

	for(alphabet::iterator iter = mAlphabet->begin(); iter!=mAlphabet->end(); iter++)
		reverse.insert(std::pair<uint32,uint16>(iter->second, iter->first));
	uint16 *bitToVal = new uint16[mAlphabet->size()];
	uint16 idx = (uint16)mAlphabet->size()-1;
	for(std::multimap<uint32,uint16>::iterator iter = reverse.begin(); iter!=reverse.end(); iter++)
		bitToVal[idx--] = iter->second;
	mItemTranslator = new ItemTranslator(bitToVal, (uint16)mAlphabet->size());

	this->ApplyReverseTranslation();
	this->SortWithinRows();

	*/
}

ItemTranslator::ItemTranslator(uint16 *bitToVal, uint16 alphabetLen) {
	mAlphabetLen = alphabetLen;
	mBitToVal = bitToVal;
	mValToBit = new valBitMap;
	for(uint16 i=0; i<mAlphabetLen; i++)
		mValToBit->insert(valBitMapEntry(mBitToVal[i], i));
}

ItemTranslator::~ItemTranslator() {
	delete[] mBitToVal;
	delete mValToBit;
}

void ItemTranslator::WriteToFile(FILE *fp) const {
	for(uint16 i=0; i<mAlphabetLen; i++)
		fprintf(fp, "%d ", mBitToVal[i]);
	fprintf(fp, "\n");
}

string ItemTranslator::ToString() const {
	string it = "";
	char buffer[16];
	for(uint16 i=0; i<mAlphabetLen; i++) {
		sprintf_s(buffer, 16, "%d%s", mBitToVal[i], (i<mAlphabetLen-1 ? " " : ""));
		it.append(buffer);
	}

	return it;
}