#ifndef __ALIGNED_ALLOCATER_H
#define __ALIGNED_ALLOCATER_H

#include <limits>

#if defined (__unix__)
	#include <malloc.h>
#elif defined (__APPLE__) && defined (__MACH__)
	#include <sys/malloc.h>
	#include <stddef.h>
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
#elif defined (__APPLE__) && defined (__MACH__)
		pointer p;
		posix_memalign(&p, N, num * sizeof(T));
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
