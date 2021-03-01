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
#ifndef __MASKARRAY_H
#define __MASKARRAY_H

#include "../Bass.h"

#include "Mask.h"

#include <iterator>

template <ItemSetType T>
class MaskArray;


template <>
class MaskArray<BM128ItemSetType> {
public:
#if defined (_MSC_VER)
	typedef Mask<BM128ItemSetType> Mask;
#elif defined (__GNUC__)
	typedef ::Mask<BM128ItemSetType> Mask;
#endif

	MaskArray() {
	}

	MaskArray& operator=(const MaskArray& m) {
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
class MaskArray<BAI32ItemSetType> {
public:
#if defined (_WINDOWS)
	typedef Mask<BAI32ItemSetType> Mask;
#elif defined (__GNUC__) && defined (__unix__)
	typedef ::Mask<BAI32ItemSetType> Mask;
#endif

	MaskArray() {
	}

	MaskArray& operator=(const MaskArray& m) {
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
class MaskArray<Uint16ItemSetType> {
public:
#if defined (_MSC_VER)
	typedef Mask<Uint16ItemSetType> Mask;
#elif defined (__GNUC__)
	typedef ::Mask<Uint16ItemSetType> Mask;
#endif

	MaskArray() {
		offsets.push_back(0);
	}

	MaskArray& operator=(const MaskArray& m) {
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


#endif // __MASKARRAY_H
