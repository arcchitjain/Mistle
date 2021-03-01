#ifndef __COVERMASK_H
#define __COVERMASK_H

#include "../Bass.h"

#include "ItemSet.h"
#include "../db/Database.h"
#include "BM128ItemSet.h"
#include "BAI32ItemSet.h"
#include "Uint16ItemSet.h"

template <ItemSetType T>
class CoverMaskArray;


template <ItemSetType T>
class CoverMask;


template <>
class CoverMask<BM128ItemSetType> {
public:
	static CoverMask* Create() {
		return new CoverMask;
	}

	void Load(const CoverMask* m) {
		memcpy(mask, m->mask, sizeof(CoverMask::mask));
	}

	void LoadFromItemSet(ItemSet* is) {
		memcpy(mask, ((BM128ItemSet*) is)->GetMask(), sizeof(CoverMask::mask));
	}

	void Store(CoverMask* m) const {
		memcpy(m->mask, mask, sizeof(CoverMask::mask));
	}

	static void UseThisStuff(Database* db, uint32 abLen) {
	}

	bool CanCover(const CoverMask* m) const {
		return !(~mask[0] & m->mask[0] || ~mask[1] & m->mask[1] || ~mask[2] & m->mask[2] || ~mask[3] & m->mask[3]);
	}


	bool Cover(const CoverMask* m) {
		if (CanCover(m)) {
			mask[0] ^= m->mask[0];
			mask[1] ^= m->mask[1];
			mask[2] ^= m->mask[2];
			mask[3] ^= m->mask[3];
			return true;
		} else {
			return false;
		}
	}

	//covers the alphabet and adds row to the vector
	uint32 CoverAlphabet(vector<int32>& alphabet, vector<uint32> **AlphabetUsageAdd, uint32 row) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<4; i++) {
			if (mask[i]) {
				uint32 x = 0x80000000;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx]++;
						AlphabetUsageAdd[idx]->push_back(row);
						numUncovered++;
					}
					x >>= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered;
	}

	uint32 CoverAlphabet(vector<int32>& alphabet) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<4; i++) {
			if (mask[i]) {
				uint32 x = 0x80000000;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx]++;
						numUncovered++;
					}
					x >>= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered;
	}

	uint32 CoverAlphabet(vector<int32>& alphabet, int32 count) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<4; i++) {
			if (mask[i]) {
				uint32 x = 0x80000000;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx] += count;
						numUncovered++;
					}
					x >>= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered * count;
	}

	uint32 UncoverAlphabet(vector<int32>& alphabet) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<4; i++) {
			if (mask[i]) {
				uint32 x = 0x80000000;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx]--;
						numUncovered++;
					}
					x >>= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered;
	}

	//Uncovers the alphabet and adds row to the vector
	uint32 UncoverAlphabet(vector<int32>& alphabet, vector<uint32> **AlphabetUsageZap, uint32 row) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<4; i++) {
			if (mask[i]) {
				uint32 x = 0x80000000;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx]--;
						AlphabetUsageZap[idx]->push_back(row);//mg
						numUncovered++;
					}
					x >>= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered;
	}

	uint32 UncoverAlphabet(vector<int32>& alphabet, int32 count) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<4; i++) {
			if (mask[i]) {
				uint32 x = 0x80000000;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx] -= count;
						numUncovered++;
					}
					x >>= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered * count;
	}

	double AlphabetCodes(vector<int32>& alphabet, uint32& count) const {
		double codeLength = 0.0;
		uint32 idx = 0;
		for(uint32 i=0; i<4; i++) {
			if (mask[i]) {
				uint32 x = 0x80000000;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						codeLength -= Log2(alphabet[idx]);
						++count;
					}
					x >>= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return codeLength;
	}

private:
	uint32 mask[4];

	friend class CoverMaskArray<BM128ItemSetType>;
};



template <>
class CoverMask<BAI32ItemSetType> {
public:
	static CoverMask* Create() {
		return (CoverMask*) new uint32[sSize];
	}

	void Load(const CoverMask* m) {
		memcpy(mask, m->mask, sSize * sizeof(uint32));
	}

	void LoadFromItemSet(ItemSet* is) {
		memcpy(mask, ((BAI32ItemSet*) is)->GetMask(), sSize * sizeof(uint32));
	}

	void Store(CoverMask* m) const {
		memcpy(m->mask, mask, sSize * sizeof(uint32));
	}

	static void UseThisStuff(Database* db, uint32 abLen) {
		sSize = (abLen + 31) / 32;
	}

	bool CanCover(const CoverMask* m) const {
		for (uint32 i=0; i<sSize; i++) {
			if (~mask[i] & m->mask[i]) {
				return false;
			}
		}
		return true;
	}


	bool Cover(const CoverMask* m) {
		if (CanCover(m)) {
			for (uint32 i=0; i<sSize; i++) {
				mask[i] ^= m->mask[i];
			}
			return true;
		} else {
			return false;
		}
	}

	//covers the alphabet and adds row to the vector
	uint32 CoverAlphabet(vector<int32>& alphabet, vector<uint32> **AlphabetUsageAdd, uint32 row) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<sSize; i++) {
			if (mask[i]) {
				uint32 x = 1;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx]++;
						AlphabetUsageAdd[idx]->push_back(row);//mg
						numUncovered++;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered;
	}

	uint32 CoverAlphabet(vector<int32>& alphabet) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<sSize; i++) {
			if (mask[i]) {
				uint32 x = 1;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx]++;
						numUncovered++;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered;
	}

	uint32 CoverAlphabet(vector<int32>& alphabet, int32 count) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<sSize; i++) {
			if (mask[i]) {
				uint32 x = 1;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx] += count;
						numUncovered++;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered * count;
	}


	uint32 UncoverAlphabet(vector<int32>& alphabet) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<sSize; i++) {
			if (mask[i]) {
				uint32 x = 1;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx]--;
						numUncovered++;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered;
	}

	//Uncovers the alphabet and adds row to the vector
	uint32 UncoverAlphabet(vector<int32>& alphabet, vector<uint32> **AlphabetUsageZap, uint32 row) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<sSize; i++) {
			if (mask[i]) {
				uint32 x = 1;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx]--;
						AlphabetUsageZap[idx]->push_back(row);//mg
						numUncovered++;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered;
	}

	uint32 UncoverAlphabet(vector<int32>& alphabet, int32 count) const {
		uint32 numUncovered = 0;
		uint32 idx = 0;
		for(uint32 i=0; i<sSize; i++) {
			if (mask[i]) {
				uint32 x = 1;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						alphabet[idx] -= count;
						numUncovered++;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return numUncovered * count;
	}

	double AlphabetCodes(vector<int32>& alphabet, uint32& count) const {
		double codeLength = 0.0;
		uint32 idx = 0;
		for(uint32 i=0; i<sSize; i++) {
			if (mask[i]) {
				uint32 x = 1;
				for(uint32 j=0; j<32; j++) {
					if (mask[i] & x) {
						codeLength -= Log2(alphabet[idx]);
						++count;
					}
					x <<= 1;
					idx++;
				}
			} else {
				idx += 32;
			}
		}
		return codeLength;
	}

private:
	uint32 mask[1];

	static uint32 BASS_API sSize;

	friend class CoverMaskArray<BAI32ItemSetType>;
};



template <>
class CoverMask<Uint16ItemSetType> {
public:
	static CoverMask* Create() {
		return (CoverMask*) new uint16[sMaxSetLength + 1];
	}

	void Load(const CoverMask* m) {
		memcpy(mStuff, m->mStuff, ((m->mStuff[0]) + 1)* sizeof(uint16));
	}

	void LoadFromItemSet(ItemSet* is) {
		mStuff[0] = is->GetLength();
		memcpy(mStuff+1, ((Uint16ItemSet*) is)->GetSet(), is->GetLength() * sizeof(uint16));
	}

	static void UseThisStuff(Database* db, uint32 abLen) {
		sMaxSetLength = db->GetMaxSetLength();
	}

	void Store(CoverMask* m) const {
		memcpy(m->mStuff, mStuff, (1+mStuff[0]) * sizeof(uint16));
	}

	bool CanCover(const CoverMask* m) const {
		if (m->mStuff[0] > mStuff[0]) {
			return false;
		}
		uint32 i = 0, j = 0;
		while(i < mStuff[0] && j < m->mStuff[0]) {
			if (mStuff[1+i] < m->mStuff[1+j]) {
				i++;
			} else if (mStuff[1+i] == m->mStuff[1+j]) {
				i++; j++;
			} else {
				return false;
			}
		}
		return j == m->mStuff[0];
	}


	bool Cover(const CoverMask* m) {
		if (CanCover(m)) {
			uint32 i = 0, j = 0, len = 0;
			while (i < mStuff[0] && j < m->mStuff[0]) {
				if (mStuff[1+i] < m->mStuff[1+j]) {
					mStuff[1+len] = mStuff[1+i]; len++; i++;
				} else {
					// mSet[1+i] == m->mSet[1+j]
					i++; j++;
				}
			}
			while (i < mStuff[0]) {
				mStuff[1+len] = mStuff[1+i]; len++; i++;
			}
			mStuff[0] = len;
			return true;
		} else {
			return false;
		}
	}

	//covers the alphabet and adds row to the vector
	uint32 CoverAlphabet(vector<int32>& alphabet, vector<uint32> **AlphabetUsageAdd, uint32 row) const {
		for (uint32 i=0; i<mStuff[0]; i++) {
			alphabet[mStuff[1+i]]++;
			AlphabetUsageAdd[mStuff[1+i]]->push_back(row);
		}
		return mStuff[0];
	}

	uint32 CoverAlphabet(vector<int32>& alphabet) const {
		for (uint32 i=0; i<mStuff[0]; i++) {
			alphabet[mStuff[1+i]]++;
		}
		return mStuff[0];
	}

	uint32 CoverAlphabet(vector<int32>& alphabet, int32 count) const {
		for (uint32 i=0; i<mStuff[0]; i++) {
			alphabet[mStuff[1+i]] += count;
		}
		return mStuff[0] * count;
	}

	//Uncovers the alphabet and adds row to the vector
	uint32 UncoverAlphabet(vector<int32>& alphabet, vector<uint32> **AlphabetUsageZap, uint32 row) const {
		for (uint32 i=0; i<mStuff[0]; i++) {
			alphabet[mStuff[1+i]]--;
			AlphabetUsageZap[mStuff[1+i]]->push_back(row);//mg
		}
		return mStuff[0];
	}

	uint32 UncoverAlphabet(vector<int32>& alphabet) const {
		for (uint32 i=0; i<mStuff[0]; i++) {
			alphabet[mStuff[1+i]]--;
		}
		return mStuff[0];
	}

	uint32 UncoverAlphabet(vector<int32>& alphabet, int32 count) const {
		for (uint32 i=0; i<mStuff[0]; i++) {
			alphabet[mStuff[1+i]] -= count;
		}
		return mStuff[0] * count;
	}

	double AlphabetCodes(vector<int32>& alphabet, uint32& count) const {
		double codeLength = 0.0;
		for (uint32 i=0; i<mStuff[0]; i++) {
			codeLength -= Log2(alphabet[mStuff[1+i]]);
		}
		count += mStuff[0];
		return codeLength;
	}

private:
	uint16 mStuff[1];

	static uint32 BASS_API sMaxSetLength;

	friend class CoverMaskArray<Uint16ItemSetType>;
};

#endif // __COVERMASK_H
