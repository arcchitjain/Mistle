#ifndef __COVERMASKARRAY_H
#define __COVERMASKARRAY_H

#include "../Bass.h"

#include "CoverMask.h"

#include <iterator>

template <ItemSetType T>
class CoverMaskArray;


template <>
class CoverMaskArray<BM128ItemSetType> {
public:
	typedef ::CoverMask<BM128ItemSetType> Mask;

	CoverMaskArray() {
	}

	CoverMaskArray& operator=(const CoverMaskArray& m) {
		masks = m.masks;
		return *this;
	}

	Mask* operator[](uint32 index) {
		return (Mask*) &masks[index];
	}

	void Reserve(uint32 rows, uint32 size=0) {
		masks.reserve(rows);
	}

	inline void Insert(uint32 index, const Mask* m) {
		masks.insert(masks.begin() + index, *m);
	}

	void Erase(uint32 index) {
		masks.erase(masks.begin() + index);
	}

private:
	vector<Mask> masks;
};


template <>
class CoverMaskArray<BAI32ItemSetType> {
public:
	typedef ::CoverMask<BAI32ItemSetType> Mask;

	CoverMaskArray() {
	}

	CoverMaskArray& operator=(const CoverMaskArray& m) {
		masks = m.masks;
		return *this;
	}

	Mask* operator[](uint32 index) {
		return (Mask*) &masks[index * Mask::sSize];
	}

	void Reserve(uint32 rows, uint32 size=0) {
		masks.reserve(rows * Mask::sSize);
	}

	void Insert(uint32 index, const Mask* m) {
		masks.insert(masks.begin() + index * Mask::sSize, m->mask, m->mask + Mask::sSize);
	}

	void Erase(uint32 index) {
		masks.erase(masks.begin() + index * Mask::sSize, masks.begin() + (index + 1) * Mask::sSize);
	}

private:
	vector<uint32> masks;
};


template <>
class CoverMaskArray<Uint16ItemSetType> {
public:
	typedef ::CoverMask<Uint16ItemSetType> Mask;

	CoverMaskArray() {
		offsets.push_back(0);
	}

	CoverMaskArray& operator=(const CoverMaskArray& m) {
		masks = m.masks;
		offsets = m.offsets;
		return *this;
	}

	Mask* operator[](uint32 index) {
		return (Mask*) &masks[offsets[index]];
	}

	void Reserve(uint32 rows, uint32 size=0) {
		masks.reserve(size);
		offsets.reserve(rows + 1);
	}

	void Insert(uint32 index, const Mask* m) {
		uint32 size = m->mStuff[0] + 1;
		uint32 insertoffset = offsets[index];
		masks.insert(masks.begin() + offsets[index], (uint16*) m, (uint16*) m + size);

		offsets.insert(offsets.begin() + index, insertoffset);

		for (uint32 i=index+1; i<offsets.size(); i++) {
			offsets[i] += size;
		}
	}

	void Erase(uint32 index) {
		masks.erase(masks.begin() + offsets[index], masks.begin() + offsets[index+1]);
		uint32 size = offsets[index+1] - offsets[index];
		for (uint32 i=index+1; i<offsets.size(); i++) {
			offsets[i] -= size;
		}
		offsets.erase(offsets.begin() + index);
	}

private:
	vector<uint16> masks;
	vector<uint32> offsets;
};


#endif // __COVERMASKARRAY_H
