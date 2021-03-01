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