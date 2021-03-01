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
#ifndef __ALIGNED_ALLOCATER_H
#define __ALIGNED_ALLOCATER_H

#include <limits>

#if defined (__unix__)
	#include <malloc.h>
#endif


template <class T, size_t N>
class AlignedAllocator {
public:
	typedef size_t		size_type;
	typedef ptrdiff_t	difference_type;
	typedef T*			pointer;
	typedef const T*	const_pointer;
	typedef T&			reference;
	typedef const T&	const_reference;
	typedef T			value_type;

	template <class U>
	struct rebind {
		typedef AlignedAllocator<U, N> other;
	};

	AlignedAllocator() { }
	AlignedAllocator(const AlignedAllocator&) { }
	template <class U>
	AlignedAllocator(const AlignedAllocator<U, N>&) { }

	size_type max_size () const {
		return std::numeric_limits<std::size_t>::max() / sizeof(T);
	}

	pointer addres(reference value) const {
		return &value;
	}

	const_pointer address(const_reference value) const {
		return &value;
	}

	pointer allocate(size_type num, const void* hint = 0) {
#if defined (_WINDOWS)
		pointer p = (pointer) _aligned_malloc(num * sizeof(T), N);
#elif defined (__unix__)
		pointer p = (pointer) memalign(N, num * sizeof(T));
#endif
		if (!p) {
			throw std::bad_alloc("AlignedAllocater");
		}
		return p;
	}

	void construct(pointer p, const_reference value) {
		new((void*) p) T(value);
	}

	void destroy(pointer p) {
		p->~T();
	}

	void deallocate(pointer p, size_type num) {
		_aligned_free(p);
	}
};

template <class T1, class T2, size_t N>
bool operator==(const AlignedAllocator<T1, N>& a, const AlignedAllocator<T2, N>& b) {
	return true;
}

template <class T1, class T2, size_t N>
bool operator!=(const AlignedAllocator<T1, N>& a, const AlignedAllocator<T2, N>& b) {
	return false;
}



#endif // __ALIGNED_ALLOCATER_H
