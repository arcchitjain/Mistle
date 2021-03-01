#ifndef __TEMPLATES_H
#define __TEMPLATES_H

// Template function for equality comparison through pointer through pointer
template<class T>
bool eq(const T* the, const T* that) {
  return *the == *that;
}

// Template function for non-equality comparision through pointer through pointer
template<class T>
bool neq(const T* the, const T* that) {
  return *the != *that;
}

// Template function for greater-than inequality comparison through pointer through pointer
template<class T>
bool gt(const T* the, const T* that) {
  return *the > *that;
}

// Template function for less-than inequality comparison through pointer through pointer
template<class T>
bool lt(const T* the, const T* that) {
  return *the < *that;
}

// Template function for greater-than-or-equal-to comparison through pointer
template<class T>
bool ge(const T* the, const T* that) {
  return *the >= *that;
}

// Template function for lesser-than-or-equal-to comparison through pointer
template<class T>
bool le(const T* the, const T* that) {
  return *the <= *that;
}


// Support functions (Boost extension).
// Called repeatedly to incrementally create a hash value from several variables.
template <class T>
inline void hash_combine(std::size_t& seed, T const& v) {
	std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

#endif // __TEMPLATES_H
