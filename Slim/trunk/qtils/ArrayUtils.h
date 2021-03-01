#ifndef __ARRAYUTILS
#define __ARRAYUTILS

#include "QtilsApi.h"

#if defined (__GNUC__) && defined (__unix__)
#include <cstring>
#endif // defined (__GNUC__) && defined (__unix__)


/** A util class to do funky array stuffs */
class QTILS_API ArrayUtils {
public:

	template <class T> __inline static void Swap(T* arr, uint32 a, uint32 b);
	// inserts element `val` into `arr` at `pos`. numVals keeps track of number of used cells in `arr`, `arrSize` of the maximum number of elements. `minimalGrowth`, if enabled, if necessary, returns a new array of arrSize+1 instead of arrSize*2 
	template <class T> __inline static void Insert(T val, uint32 pos, T* &arr, uint32 &numVals, uint32 &arrSize, bool minimalGrowth = true);
	// removes the element of `arr` at index `pos`. numVals keeps track of number of used cells in `arr`, `arrSize` of the maximum number of elements. `resizeArray`, if enabled, returns a new array of arrSize-1 instead of leaving `arrSize` as is. 
	template <class T> __inline static void Remove(uint32 pos, T* &arr, uint32 &numVals, uint32& arrSize, bool resizeArray /* = true */);

	template <class T> static void Permute(T* arr, uint32* idxs, uint32 len);

	//////////////////////////////////////////////////////////////////////////
	// Intersection
	//////////////////////////////////////////////////////////////////////////
	
	// calculate intersection
	//template <class T> static T* Intersection(T* arr1, size_t arr1Len, T* arr2, size_t arr2Len, uint32 &intLen);

	// calculate intersection length
	template <class T> static size_t IntersectionLength(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len);

	// with minimal intersection length (eg, for minSup)
	template <class T> static size_t IntersectionLength(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minInt);


	//////////////////////////////////////////////////////////////////////////
	// Binary Search
	//////////////////////////////////////////////////////////////////////////
	
	template <class T> static uint32 BinarySearchAsc(const T val, const T* arr, const uint32 arrLen);
	template <class T> static uint32 BinarySearchMayNotExistAsc(const T val, const T* arr, const uint32 arrLen);

	// looks for the insertion position for `val` in `arr`. 
	// returns first index where `val` could be inserted, but arr[idx] could be `val' already
	template <class T> static uint32 BinarySearchInsertionLocationAsc(const T val, const T* arr, const uint32 arrLen);


	//////////////////////////////////////////////////////////////////////////
	// Bubble-Sort
	//////////////////////////////////////////////////////////////////////////

	// sorts array `arr` of length `arrLen` ascendingly
	template <class T> static void BSortAsc(T* arr, uint32 arrLen);

	// sorts array `arr` of length `arrLen` descendingly
	template <class T> static void BSortDesc(T* arr, uint32 arrLen);

	// sorts array `arr` of pointers, of length `arrLen` ascendingly, comparing *arr[i] > *arr[j] (and not the pointers!)
	template <class T> static void BSortAsc(T** arr, uint32 arrLen);

	// sorts array `arr` of pointers, of length `arrLen` descendingly, comparing *arr[i] > *arr[j] (and not the pointers!)
	template <class T> static void BSortDesc(T** arr, uint32 arrLen);

	// sorts array `arr` of length `arrLen` ascendingly, keeping track of the original indici in (filled by you) `origIdxArr`
	template <class T, class R> static void BSortAsc(T* arr, uint32 arrLen, R* origIdxArr);

	// sorts array `arr` of length `arrLen` descendingly, keeping track of the original indici in (filled by you) `origIdxArr`
	template <class T, class R> static void BSortDesc(T* arr, uint32 arrLen, R* origIdxArr);

	// sorts array `arr` of length `arrLen` ascendingly, keeping track of the original indici in (filled by you) `origIdxArr`
	template <class T, class R> static void BSortAsc(T** arr, uint32 arrLen, R* origIdxArr);


	//////////////////////////////////////////////////////////////////////////
	// Quick-sort
	//////////////////////////////////////////////////////////////////////////

	// sorts (in place, -not- stable) `arr` of length `arrLen` ascendingly. 
	template <class T> static void QSortAsc(T* arr, int32 arrLen);
	// sorts (in place, -not- stable) `arr` ascendingly between indici `left` and `right`. 
	template <class T> static void QSortAsc(T* arr, int32 left, int32 right);
	// sorts (in place, -not- stable) `arr` of length `arrLen` ascendingly, keeping track of the original indici in (filled by you) `origIdxArr`
	template <class T, class R> static void QSortAsc(T* arr, int32 arrLen, R* origIdxArr);
	// sorts (in place, -not- stable) `arr` ascendingly between indici `left` and `right`, keeping track of the original indici in (filled by you) `origIdxArr`
	template <class T, class R> static void QSortAsc(T* arr, int32 from, int32 to, R* origIdxArr);

	//////////////////////////////////////////////////////////////////////////
	// Merge-sort
	//////////////////////////////////////////////////////////////////////////

	template <class T, class R> static void InitMergeSort(uint32 maxArrLen, bool idxArr); 
	template <class T, class R> static void CleanUpMergeSort();

	template <class T> static void MSortAsc(T *arr, uint32 arrLen);
	template <class T> static void MSortDesc(T *arr, uint32 arrLen);
	template <class T, class R> static void MSortAsc(T *arr, uint32 arrLen, R* origIdxArr);
	template <class T, class R> static void MSortDesc(T *arr, uint32 arrLen, R* origIdxArr);


protected:
	//////////////////////////////////////////////////////////////////////////
	// Intersection Helper Functions
	//////////////////////////////////////////////////////////////////////////
	template <class T> static size_t _IntersectionLength_merge(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len);
	template <class T> static size_t _IntersectionLength_merge(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minLen);

	template <class T> static size_t _IntersectionLength_bsearch(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len);
	template <class T> static size_t _IntersectionLength_bsearch(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minLen);

	//template <class T> static size_t _IntersectionLength_bsearch2(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len);
	template <class T> static size_t _IntersectionLength_bsearch2(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minLen);

	//////////////////////////////////////////////////////////////////////////
	// Merge Sort Helper Functions
	//////////////////////////////////////////////////////////////////////////
	template <class T> static void MSortAscMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr);
	template <class T> static void MSortAscSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr);

	template <class T> static void MSortDescMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr);	
	template <class T> static void MSortDescSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr);

	template <class T, class R> static void MSortAscMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr, R* idxArr, R* idxAuxArr);
	template <class T, class R> static void MSortAscSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr, R* idxArr, R* idxAuxArr);

	template <class T, class R> static void MSortDescMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr, R* idxArr, R* idxAuxArr);
	template <class T, class R> static void MSortDescSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr, R* idxArr, R* idxAuxArr);

	static void* sMergeSortAuxArr;
	static void* sMergeSortIdxAuxArr;

	static size_t sMergeSortAuxArrNumBytes;
	static size_t sMergeSortIdxAuxArrNumBytes;

	//////////////////////////////////////////////////////////////////////////
	// Quick Sort Helper Functions
	//////////////////////////////////////////////////////////////////////////
	
};


#if defined (__GNUC__) && (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
#define __inline /*__inline*/
#define static /*static*/
#endif


template <class T> __inline static void ArrayUtils::Swap(T* arr, uint32 a, uint32 b) {
	T tmp = arr[a];
	arr[a] = arr[b];
	arr[b] = tmp;
}

//////////////////////////////////////////////////////////////////////////
// Intersection
//////////////////////////////////////////////////////////////////////////

template <class T> __inline static size_t ArrayUtils::IntersectionLength(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len) {
	/*const double minRatio = 0.1;
	if((double) arr1Len / arr2Len < minRatio) {
		return ArrayUtils::_IntersectionLength_bsearch2(arr1,arr1Len,arr2,arr2Len, 0);
	} else if((double) arr2Len / arr1Len < minRatio) {
		return ArrayUtils::_IntersectionLength_bsearch2(arr2,arr2Len,arr1,arr1Len, 0);
	} else*/ {
		return ArrayUtils::_IntersectionLength_merge(arr1, arr1Len, arr2, arr2Len);
	}
}
template <class T> __inline static size_t ArrayUtils::IntersectionLength(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minLen) {
	//const double minRatio = 0.1;
	/*const size_t maxShortLen = 100;
	const size_t minLongLen = 3000;
	if(arr1Len <= maxShortLen && arr2Len >= minLongLen) {
		return ArrayUtils::_IntersectionLength_bsearch2(arr1,arr1Len,arr2,arr2Len, minLen);
	} else if(arr2Len <= maxShortLen && arr1Len >= minLongLen) {
		return ArrayUtils::_IntersectionLength_bsearch2(arr2,arr2Len,arr1,arr1Len, minLen);
	} else*/ 
	/*const double minRatio = 0.1;
	if((double) arr1Len / arr2Len <= minRatio) {
		return ArrayUtils::_IntersectionLength_bsearch2(arr1,arr1Len,arr2,arr2Len, minLen);
	} else if((double) arr2Len / arr1Len < minRatio) {
		return ArrayUtils::_IntersectionLength_bsearch2(arr2,arr2Len,arr1,arr1Len, minLen);
	} else*/ {
		return ArrayUtils::_IntersectionLength_merge(arr1, arr1Len, arr2, arr2Len, minLen);
	}
}

/*template <class T> static uint32 ArrayUtils::IntersectionLength_baezayates(T* arr1, size_t arr1Len, T* arr2, size_t arr2Len) {

}*/

template <class T> __inline static size_t ArrayUtils::_IntersectionLength_bsearch(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len) {
	size_t counter = 0; 

	const T* first1 = arr1;
	const T* first2 = arr2;
	const T* last1 = arr1 + arr1Len;
	const T* last2 = arr2 + arr2Len;

	size_t num2 = arr2Len;

	for(; first1 < last1; ++first1) {
		uint32 idx2 = ArrayUtils::BinarySearchMayNotExistAsc((*first1), first2, (uint32) num2);
		if(idx2 != UINT32_MAX_VALUE) {
			first2 += idx2 + 1;
			num2 -= idx2 + 1;
			++counter;
		}
	}
	return counter;
}

template <class T> __inline static size_t ArrayUtils::_IntersectionLength_bsearch(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minLen) {
	size_t counter = 0; 
	size_t cub1 = arr1Len;

	const T* first1 = arr1;
	const T* first2 = arr2;
	const T* last1 = arr1 + arr1Len;
	//const T* last2 = arr2 + arr2Len;

	size_t num2 = arr2Len;

	for(; first1 < last1; ++first1) {
		uint32 idx2 = ArrayUtils::BinarySearchMayNotExistAsc((*first1), first2, (uint32) num2);
		if(idx2 != UINT32_MAX_VALUE) {
			first2 += idx2 + 1;
			num2 -= idx2 + 1;
			++counter;
		} else if(--cub1 < minLen) {
			return 0;
		}
	}
	return counter;
}

template <class T> __inline static size_t ArrayUtils::_IntersectionLength_bsearch2(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minLen) {
	size_t counter = 0; 
	//size_t cub1 = arr1Len;

	const T* first1 = arr1;
	const T* first2 = arr2;
	const T* last1 = arr1 + arr1Len;

	size_t num2 = arr2Len;

	//uint32 idx1 = ArrayUtils::BinarySearchInsertionLocationAsc(*first2, first1, (uint32) arr1Len);
	//first1 += idx1;

	/*uint32 idx1 = ArrayUtils::BinarySearchInsertionLocationAsc(*(first2 + arr2Len - 1), first1, (uint32) arr1Len);
	if(idx1 < arr1Len)
		last1 = first1 + idx1;*/

	uint32 idx2 = 0;
	for(; first1 < last1; ++first1) {
		idx2 = ArrayUtils::BinarySearchInsertionLocationAsc(*first1, first2, (uint32) num2);
		if(idx2 < num2 && *(first2 + idx2) == *first1) {
			++first2;
			--num2;
			++counter;
		} else if(idx2 >= num2)
			break;
		first2 += idx2;
		num2 -= idx2;
	}
	return counter;
}



template <class T> __inline static size_t ArrayUtils::_IntersectionLength_merge(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len) {
	size_t counter = 0; 
	const T* firstX = arr1;
	const T* firstY = arr2;
	const T* lastX = arr1 + arr1Len;
	const T* lastY = arr2 + arr2Len;
	while (firstX != lastX && firstY != lastY) {
		if (*firstX < *firstY) {
			++firstX; 
		} else if (*firstY < *firstX) {
			++firstY; 
		} else { 
			++counter;
			++firstX;
			++firstY; 
		}
	}
	return counter;
}
template <class T> __inline static size_t ArrayUtils::_IntersectionLength_merge(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minLen) {
	size_t counter = 0; 
	size_t ub2 = arr2Len, ub1 = arr1Len;
	const T* firstX = arr1;
	const T* firstY = arr2;
	const T* lastX = arr1 + ub1;
	const T* lastY = arr2 + ub2;
	while (firstX != lastX && firstY != lastY) {
		if (*firstX < *firstY) {
			++firstX; 
			if(--ub1 <= minLen)
				return 0;
		} else if (*firstY < *firstX) {
			++firstY; 
			if(--ub2 <= minLen)
				return 0;
		} else { 
			++counter;
			++firstX;
			++firstY; 
		}
	}
	return counter;
}
/*
template <class T> __inline static size_t ArrayUtils::_IntersectionLength_baeza(const T* arr1, const size_t arr1Len, const T* arr2, const size_t arr2Len, const size_t minLen) {
	size_t counter = 0

	size_t probe1, probe2;

	if ( (arr1Len) < (arr2Len) ) {
		if ( arrLen == 0 )
			return;
		probe1 = ( ( arr1Len ) >> 1 );
		probe2 = lower_bound< Probe >( begin2, end2, *probe1 );
		counter = _IntersectionLength_baeza(arr1, probe1, arr2, probe2, minLen); // intersect left
		if (! (probe2 == end2 || *probe1 < *probe2 ))
			*out++ = *probe2++;
		counter += _IntersectionLength_baeza(arr1 + ++probe1, arr1Len - probe1, probe2, end2, out); // intersect right
	}
	else {
		if ( begin2 == end2 )
			return;
		probe2 = begin2 + ( ( end2 - begin2 ) >> 1 );
		probe1 = lower_bound< Probe >( begin1, end1, *probe2 );
		baeza_intersect< Probe >(begin1, probe1, begin2, probe2, out); // intersect left
		if (! (probe1 == end1 || *probe2 < *probe1 ))
			*out++ = *probe1++;
		baeza_intersect< Probe >(probe1, end1, ++probe2, end2, out); // intersect right
	}

}
*/


/*
template< template <class, class> class Probe,
class RandomAccessIterator, class OutputIterator>
	void baeza_intersect(RandomAccessIterator begin1, RandomAccessIterator end1,
	RandomAccessIterator begin2, RandomAccessIterator end2,
	OutputIterator out)
{
	RandomAccessIterator probe1, probe2;


	if ( (end1 - begin1) < ( end2 - begin2 ) )
	{
		if ( begin1 == end1 )
			return;
		probe1 = begin1 + ( ( end1 - begin1 ) >> 1 );
		probe2 = lower_bound< Probe >( begin2, end2, *probe1 );
		baeza_intersect< Probe >(begin1, probe1, begin2, probe2, out); // intersect left
		if (! (probe2 == end2 || *probe1 < *probe2 ))
			*out++ = *probe2++;
		baeza_intersect< Probe >(++probe1, end1, probe2, end2, out); // intersect right
	}
	else
	{
		if ( begin2 == end2 )
			return;
		probe2 = begin2 + ( ( end2 - begin2 ) >> 1 );
		probe1 = lower_bound< Probe >( begin1, end1, *probe2 );
		baeza_intersect< Probe >(begin1, probe1, begin2, probe2, out); // intersect left
		if (! (probe1 == end1 || *probe2 < *probe1 ))
			*out++ = *probe1++;
		baeza_intersect< Probe >(probe1, end1, ++probe2, end2, out); // intersect right
	}
}
*/


// sorts array `arr` of length `arrLen` ascendingly
template <class T> static void ArrayUtils::BSortAsc(T* arr, uint32 arrLen) {
	// bubble-sort
	for(uint32 i=1; i<arrLen; i++) {
		for(uint32 j=0; j<arrLen-i; j++) {
			if(arr[j] > arr[j+1]) {
				T tmp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = tmp;
			}
		}
	}
}
// sorts array `arr` of length `arrLen` ascendingly
template <class T> static void ArrayUtils::BSortAsc(T** arr, uint32 arrLen) {
	// bubble-sort
	for(uint32 i=1; i<arrLen; i++) {
		for(uint32 j=0; j<arrLen-i; j++) {
			if(*arr[j] > *arr[j+1]) {
				T* tmp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = tmp;
			}
		}
	}
}

// sorts array `arr` of length `arrLen` ascendingly, keeping track of the original indici in (filled by you) `origIdxArr`
template <class T, class R> static void ArrayUtils::BSortAsc(T* arr, uint32 arrLen, R* origIdxArr) {
	// bubble-sort
	for(uint32 i=1; i<arrLen; i++) {
		for(uint32 j=0; j<arrLen-i; j++) {
			if(arr[j] > arr[j+1]) {
				T tmp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = tmp;
				R swap = origIdxArr[j];
				origIdxArr[j]=origIdxArr[j+1];
				origIdxArr[j+1]=swap;
			}
		}
	}
}
// sorts array `arr` of length `arrLen` ascendingly, keeping track of the original indici in (filled by you) `origIdxArr`
template <class T, class R> static void ArrayUtils::BSortAsc(T** arr, uint32 arrLen, R* origIdxArr) {
	// bubble-sort
	for(uint32 i=1; i<arrLen; i++) {
		for(uint32 j=0; j<arrLen-i; j++) {
			if(*arr[j] > *arr[j+1]) {
				T* tmp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = tmp;
				R swap = origIdxArr[j];
				origIdxArr[j]=origIdxArr[j+1];
				origIdxArr[j+1]=swap;
			}
		}
	}
}

// sorts array `arr` of length `arrLen` descendingly
template <class T> static void ArrayUtils::BSortDesc(T* arr, uint32 arrLen) {
	// bubble-sort
	for(uint32 i=1; i<arrLen; i++) {
		for(uint32 j=0; j<arrLen-i; j++) {
			if(arr[j] < arr[j+1]) {
				T tmp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = tmp;
			}
		}
	}
}
// sorts array `arr` of length `arrLen` descendingly
template <class T> static void ArrayUtils::BSortDesc(T** arr, uint32 arrLen) {
	// bubble-sort
	for(uint32 i=1; i<arrLen; i++) {
		for(uint32 j=0; j<arrLen-i; j++) {
			if(*arr[j] < *arr[j+1]) {
				T* tmp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = tmp;
			}
		}
	}
}

// sorts array `arr` of length `arrLen` descendingly, keeping track of the original indici in (filled by you) `origIdxArr`
template <class T, class R> static void ArrayUtils::BSortDesc(T* arr, uint32 arrLen, R* origIdxArr) {
	// bubble-sort
	for(uint32 i=1; i<arrLen; i++) {
		for(uint32 j=0; j<arrLen-i; j++) {
			if(arr[j] < arr[j+1]) {
				T tmp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = tmp;
				R swap = origIdxArr[j];
				origIdxArr[j]=origIdxArr[j+1];
				origIdxArr[j+1]=swap;
			}
		}
	}	
}

template <class T, class R> 
static void ArrayUtils::InitMergeSort(uint32 maxArrLen, bool idxArr) {
	if(sMergeSortAuxArr != NULL)
		THROW("Clean me up before you go go.")
	size_t mergeSortAuxArrLen = (maxArrLen+1)/2;
	sMergeSortAuxArr = new T[mergeSortAuxArrLen];
	sMergeSortAuxArrNumBytes = mergeSortAuxArrLen * sizeof(T);
	if(idxArr == true) {
		if(sMergeSortIdxAuxArr != NULL)
			THROW("Clean me up before you go go.")
		size_t mergeSortIdxAuxArrLen = (maxArrLen+1)/2;
		sMergeSortIdxAuxArr = new R[mergeSortIdxAuxArrLen];
		sMergeSortIdxAuxArrNumBytes = mergeSortIdxAuxArrLen * sizeof(R);
	}
}

template <class T, class R> 
static void ArrayUtils::CleanUpMergeSort() {
	delete[] sMergeSortAuxArr;
	sMergeSortAuxArr = NULL;
	sMergeSortAuxArrNumBytes = 0;
	delete[] sMergeSortIdxAuxArr;
	sMergeSortIdxAuxArr = NULL;
	sMergeSortIdxAuxArrNumBytes = 0;
}


template <class T> static void ArrayUtils::MSortAsc(T *arr, uint32 arrLen) {
	if(arrLen == 0)
		return;
	bool freshArray = false;
	size_t mergeSortAuxArrReqLen = (arrLen+1)/2;
	if(sMergeSortAuxArr == NULL) {
		sMergeSortAuxArr = new T[mergeSortAuxArrReqLen];
		sMergeSortAuxArrNumBytes = mergeSortAuxArrReqLen * sizeof(T);
		freshArray = true;
	} else if(sMergeSortAuxArrNumBytes < sizeof(T) * mergeSortAuxArrReqLen) {
		THROW("We need a bigger boat!");
	}

	MSortAscSort(arr, 0, arrLen-1, (T*)sMergeSortAuxArr);
	if(freshArray) {
		delete[] sMergeSortAuxArr;
		sMergeSortAuxArr = NULL;
		sMergeSortAuxArrNumBytes = 0;
	}
}

template <class T, class R> static void ArrayUtils::MSortAsc(T *arr, uint32 arrLen, R* origIdxArr) {
	if(arrLen == 0)
		return;
	bool freshAuxArray = false, freshIdxAuxArray = false;
	size_t mergeSortAuxArrReqLen = (arrLen+1)/2;
	if(sMergeSortAuxArr == NULL) {
		sMergeSortAuxArr = new T[mergeSortAuxArrReqLen];
		sMergeSortAuxArrNumBytes = mergeSortAuxArrReqLen * sizeof(T);
		freshAuxArray = true;
	} else if(sMergeSortAuxArrNumBytes < sizeof(T) * mergeSortAuxArrReqLen)
		THROW("We need a bigger boat!")
	if(sMergeSortIdxAuxArr == NULL) {
		sMergeSortIdxAuxArr = new R[mergeSortAuxArrReqLen];
		sMergeSortIdxAuxArrNumBytes = mergeSortAuxArrReqLen * sizeof(R);
		freshIdxAuxArray = true;
	} else if(sMergeSortIdxAuxArrNumBytes < sizeof(R) * mergeSortAuxArrReqLen)
		THROW("We need a bigger boat!")

	MSortAscSort(arr, 0, arrLen-1, (T*) sMergeSortAuxArr, origIdxArr, (R*) sMergeSortIdxAuxArr);
	if(freshAuxArray) {
		delete[] sMergeSortAuxArr;
		sMergeSortAuxArr = NULL;
		sMergeSortAuxArrNumBytes = 0;
	}
	if(freshIdxAuxArray) {
		delete[] sMergeSortIdxAuxArr;
		sMergeSortIdxAuxArr = NULL;
		sMergeSortIdxAuxArrNumBytes = 0;
	}
}
template <class T> static void ArrayUtils::MSortDesc(T *arr, uint32 arrLen) {
	if(arrLen == 0)
		return;
	bool freshArray = false;
	size_t mergeSortAuxArrReqLen = (arrLen+1)/2;
	if(sMergeSortAuxArr == NULL) {
		size_t mergeSortAuxArrLen = (arrLen+1)/2;
		sMergeSortAuxArr = new T[mergeSortAuxArrLen];
		sMergeSortAuxArrNumBytes = mergeSortAuxArrLen * sizeof(T);
		freshArray = true;
	} else if(sMergeSortAuxArrNumBytes < sizeof(T) * mergeSortAuxArrReqLen)
		THROW("We need a bigger boat!")
		MSortDescSort(arr, 0, arrLen-1, (T*) sMergeSortAuxArr);
	if(freshArray) {
		delete[] sMergeSortAuxArr;
		sMergeSortAuxArr = NULL;
		sMergeSortAuxArrNumBytes = 0;
	}
}
template <class T, class R> static void ArrayUtils::MSortDesc(T *arr, uint32 arrLen, R* origIdxArr) {
	if(arrLen == 0)
		return;
	bool freshAuxArray = false, freshIdxAuxArray = false;
	size_t mergeSortAuxArrReqLen = (arrLen+1)/2;
	if(sMergeSortAuxArr == NULL) {
		sMergeSortAuxArr = new T[mergeSortAuxArrReqLen];
		sMergeSortAuxArrNumBytes = mergeSortAuxArrReqLen * sizeof(T);
		freshAuxArray = true;
	} else if(sMergeSortAuxArrNumBytes < sizeof(T) * mergeSortAuxArrReqLen)
		THROW("We need a bigger boat!")
		if(sMergeSortIdxAuxArr == NULL) {
			sMergeSortIdxAuxArr = new R[mergeSortAuxArrReqLen];
			sMergeSortIdxAuxArrNumBytes = mergeSortAuxArrReqLen * sizeof(R);
			freshIdxAuxArray = true;
		} else if(sMergeSortIdxAuxArrNumBytes < sizeof(R) * mergeSortAuxArrReqLen)
			THROW("We need a bigger boat!")
			MSortDescSort(arr, 0, arrLen-1, (T*) sMergeSortAuxArr, origIdxArr, (R*) sMergeSortIdxAuxArr);
		if(freshAuxArray) {
			delete[] sMergeSortAuxArr;
			sMergeSortAuxArr = NULL;
			sMergeSortAuxArrNumBytes = 0;
		}
		if(freshIdxAuxArray) {
			delete[] sMergeSortIdxAuxArr;
			sMergeSortIdxAuxArr = NULL;
			sMergeSortIdxAuxArrNumBytes = 0;
		}
}

template <class T> static uint32 ArrayUtils::BinarySearchAsc(const T val, const T* arr, const uint32 arrLen) {
	uint32 minIdx = 0;
	uint32 maxIdx = arrLen-1;
	uint32 tstIdx = 0;
	bool found = false;
	while(minIdx <= maxIdx) {
		tstIdx = minIdx + (maxIdx - minIdx)/2;
		if(arr[tstIdx] > val) {
			maxIdx = tstIdx - 1;
			if(maxIdx > arrLen)
				return UINT32_MAX_VALUE;
		} else if(arr[tstIdx] < val) {
			minIdx = tstIdx + 1;
			if(minIdx > arrLen)
				return minIdx;
		} else {
			return tstIdx;
		}
	}
	return UINT32_MAX_VALUE;
}

template <class T> static uint32 ArrayUtils::BinarySearchMayNotExistAsc(const T val, const T* arr, const uint32 arrLen) {
	if(arrLen == 0)
		return UINT32_MAX_VALUE;

	uint32 minIdx = 0;
	uint32 maxIdx = arrLen-1;
	uint32 tstIdx = UINT32_MAX_VALUE;
	bool found = false;
	while(minIdx+1 < maxIdx) { // while there are at least 3 possibilities
		tstIdx = minIdx + (maxIdx - minIdx)/2;
		if(arr[tstIdx] > val) {
			maxIdx = tstIdx - 1;
			if(maxIdx > arrLen)
				return UINT32_MAX_VALUE;
		} else if(arr[tstIdx] < val) {
			minIdx = tstIdx + 1;
		} else {
			return tstIdx;
		}
	}
	if(arr[minIdx] == val)
		return minIdx;
	if(arr[maxIdx] == val)
		return maxIdx;
	return UINT32_MAX_VALUE; // not found
}

// looks for the insertion position for `val` in `arr`. 
// returns first index where `val` could be inserted, but arr[idx] could be `val' already
template <class T> static uint32 ArrayUtils::BinarySearchInsertionLocationAsc(const T val, const T* arr, const uint32 arrLen) {
	if(arrLen == 0)
		return 0;

	uint32 minIdx = 0;
	uint32 maxIdx = arrLen-1;
	uint32 tstIdx = 0;
	bool found = false;
	while(minIdx <= maxIdx) {
		tstIdx = minIdx + (maxIdx - minIdx)/2;
		if(arr[tstIdx] > val) {
			maxIdx = tstIdx - 1;
			if(maxIdx > arrLen)
				return 0;
		} else if(arr[tstIdx] < val) {
			minIdx = tstIdx + 1;
		} else {
			return tstIdx;
		}
	}
	return (uint32) max(minIdx,maxIdx);
}

template <class T> static void ArrayUtils::MSortAscMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr) {
	uint32 i, j, k;
	// copy first half of array a to auxiliary array b
	// i=0; j=lo; while (j<=m) mergeSortAuxArr[i++]=arr[j++];
	memcpy(mergeSortAuxArr,arr+lo,sizeof(T)*(m-lo+1));
	j=m+1;

	i=0; k=lo;
	// copy back next-greatest element at each time
	while (k<j && j<=hi)
		if (mergeSortAuxArr[i] < arr[j])
			arr[k++]=mergeSortAuxArr[i++];
		else
			arr[k++]=arr[j++];

	// copy back remaining elements of first half (if any)
	//	while (k<j)	arr[k++]=mergeSortAuxArr[i++];
	memcpy(arr+k,mergeSortAuxArr+i,sizeof(T)*(j-k));
}

template <class T> static void ArrayUtils::MSortAscSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr) {
	if(lo<hi) {
		uint32 m = (lo+hi)/2;
		MSortAscSort(arr, lo, m, mergeSortAuxArr);
		MSortAscSort(arr, m+1, hi, mergeSortAuxArr);
		MSortAscMerge(arr, lo, m, hi, mergeSortAuxArr);
	}
}

template <class T, class R> static void ArrayUtils::MSortAscMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr, R* idxArr, R* idxAuxArr) {
	uint32 i, j, k;
	// copy first half of array a to auxiliary array b
	// i=0; j=lo; while (j<=m) mergeSortAuxArr[i++]=arr[j++];
	memcpy(mergeSortAuxArr,arr+lo,sizeof(T)*(m-lo+1));
	memcpy(idxAuxArr,idxArr+lo,sizeof(R)*(m-lo+1));
	j=m+1;

	i=0; k=lo;
	// copy back next-greatest element at each time
	while (k<j && j<=hi) {
		if (mergeSortAuxArr[i] < arr[j]) {
			idxArr[k]=idxAuxArr[i];
			arr[k++]=mergeSortAuxArr[i++];
		} else {
			idxArr[k]=idxArr[j];
			arr[k++]=arr[j++];
		}
	}

	// copy back remaining elements of first half (if any)
	//	while (k<j)	arr[k++]=mergeSortAuxArr[i++];
	memcpy(arr+k,mergeSortAuxArr+i,sizeof(T)*(j-k));
	memcpy(idxArr+k,idxAuxArr+i,sizeof(R)*(j-k));
}
template <class T, class R> static void ArrayUtils::MSortAscSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr, R* idxArr, R* idxAuxArr) {
	if(lo<hi) {
		uint32 m = (lo+hi)/2;
		MSortAscSort(arr, lo, m, mergeSortAuxArr, idxArr, idxAuxArr);
		MSortAscSort(arr, m+1, hi, mergeSortAuxArr, idxArr, idxAuxArr);
		MSortAscMerge(arr, lo, m, hi, mergeSortAuxArr, idxArr, idxAuxArr);
	}
}

//static void MSortDescAux(uint32 *arr, uint32 arrLen, uint32 *auxArray /* auxiliary array of length arrLen+1/2 */ );
template <class T> static void ArrayUtils::MSortDescMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr) {
	uint32 i, j, k;
	// copy first half of array a to auxiliary array b
	// i=0; j=lo; while (j<=m) mergeSortAuxArr[i++]=arr[j++];
	memcpy(mergeSortAuxArr,arr+lo,sizeof(T)*(m-lo+1));
	j=m+1;

	i=0; k=lo;
	// copy back next-greatest element at each time
	while (k<j && j<=hi)
		if (mergeSortAuxArr[i] > arr[j])
			arr[k++]=mergeSortAuxArr[i++];
		else
			arr[k++]=arr[j++];

	// copy back remaining elements of first half (if any)
	//	while (k<j)	arr[k++]=mergeSortAuxArr[i++];
	memcpy(arr+k,mergeSortAuxArr+i,sizeof(T)*(j-k));

}
template <class T> static void ArrayUtils::MSortDescSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr) {
	if(lo<hi) {
		uint32 m = (lo+hi)/2;
		MSortDescSort(arr, lo, m, mergeSortAuxArr);
		MSortDescSort(arr, m+1, hi, mergeSortAuxArr);
		MSortDescMerge(arr, lo, m, hi, mergeSortAuxArr);
	}
}

template <class T, class R> static void ArrayUtils::MSortDescMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr, R* idxArr, R* idxAuxArr) {
	uint32 i, j, k;
	// copy first half of array a to auxiliary array b
	// i=0; j=lo; while (j<=m) mergeSortAuxArr[i++]=arr[j++];
	memcpy(mergeSortAuxArr,arr+lo,sizeof(T)*(m-lo+1));
	memcpy(idxAuxArr,idxArr+lo,sizeof(R)*(m-lo+1));
	j=m+1;

	i=0; k=lo;
	// copy back next-greatest element at each time
	while (k<j && j<=hi) {
		if (mergeSortAuxArr[i] > arr[j]) {
			idxArr[k]=idxAuxArr[i];
			arr[k++]=mergeSortAuxArr[i++];
		} else {
			idxArr[k]=idxArr[j];
			arr[k++]=arr[j++];
		}
	}

	// copy back remaining elements of first half (if any)
	//	while (k<j)	arr[k++]=mergeSortAuxArr[i++];
	memcpy(arr+k,mergeSortAuxArr+i,sizeof(T)*(j-k));
	memcpy(idxArr+k,idxAuxArr+i,sizeof(R)*(j-k));
}

template <class T, class R> static void ArrayUtils::MSortDescSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr, R* idxArr, R* idxAuxArr) {
	if(lo<hi) {
		uint32 m = (lo+hi)/2;
		MSortDescSort(arr, lo, m, mergeSortAuxArr, idxArr, idxAuxArr);
		MSortDescSort(arr, m+1, hi, mergeSortAuxArr, idxArr, idxAuxArr);
		MSortDescMerge(arr, lo, m, hi, mergeSortAuxArr, idxArr, idxAuxArr);
	}
}

template <class T> static void ArrayUtils::QSortAsc(T* arr, int32 arrLen) {
	if(arrLen > 1)
		QSortAsc(arr, 0, arrLen-1);
}
template <class T> static void ArrayUtils::QSortAsc(T* arr, int32 left, int32 right) {
	int pivot, leftIdx = left, rightIdx = right;
	if(right - left > 0) {
		pivot = (left + right) / 2;
		while(leftIdx <= pivot && rightIdx >= pivot) {
			while(arr[leftIdx] < arr[pivot] && leftIdx <= pivot)
				leftIdx = leftIdx + 1;
			while(arr[rightIdx] > arr[pivot] && rightIdx >= pivot)
				rightIdx = rightIdx - 1;
			ArrayUtils::Swap(arr, leftIdx, rightIdx);
			leftIdx = leftIdx+1;
			rightIdx = rightIdx-1;
			if(leftIdx-1 == pivot) {
				rightIdx = rightIdx + 1;
				pivot = rightIdx;
			} else if(rightIdx + 1 == pivot) {
				leftIdx = leftIdx - 1;
				pivot = leftIdx;
			}
		}
		ArrayUtils::QSortAsc(arr, left, pivot-1);
		ArrayUtils::QSortAsc(arr, pivot+1, right);
	}
}

template <class T, class R> static void ArrayUtils::QSortAsc(T* arr, int32 arrLen, R* idxArr) {
	if(arrLen > 1)
		QSortAsc(arr, 0, arrLen-1, idxArr);
}
template <class T, class R> static void ArrayUtils::QSortAsc(T* arr, int32 left, int32 right, R* idxArr) {
	int pivot, leftIdx = left, rightIdx = right;
	if(right - left > 0) {
		pivot = (left + right) / 2;
		while(leftIdx <= pivot && rightIdx >= pivot) {
			while(arr[leftIdx] < arr[pivot] && leftIdx <= pivot)
				leftIdx = leftIdx + 1;
			while(arr[rightIdx] > arr[pivot] && rightIdx >= pivot)
				rightIdx = rightIdx - 1;
			if(leftIdx != rightIdx) {
				ArrayUtils::Swap(arr, leftIdx, rightIdx);
				ArrayUtils::Swap(idxArr, leftIdx, rightIdx);
			}
			leftIdx = leftIdx+1;
			rightIdx = rightIdx-1;
			if(leftIdx-1 == pivot) {
				rightIdx = rightIdx + 1;
				pivot = rightIdx;
			} else if(rightIdx + 1 == pivot) {
				leftIdx = leftIdx - 1;
				pivot = leftIdx;
			}
		}
		ArrayUtils::QSortAsc(arr, left, pivot-1, idxArr);
		ArrayUtils::QSortAsc(arr, pivot+1, right, idxArr);
	}
}

template <class T> __inline static void ArrayUtils::Insert(T val, uint32 pos, T* &arr, uint32 &numVals, uint32 &arrSize, bool minimalGrowth /* = true */) {
	if(numVals + 1 == arrSize) {
		uint32 newArrSize = minimalGrowth == true ? arrSize +1 : arrSize * 2;
		T *tmpArr = new T[newArrSize];
		memcpy(tmpArr, arr, sizeof(T) * pos);
		memcpy(tmpArr + pos + 1, arr + pos, sizeof(T) * (arrSize - pos));
		tmpArr[pos] = val;
		delete[] arr;
		arr = tmpArr;
		arrSize = newArrSize;
	} else {
		memcpy(arr + pos + 1, arr + pos, sizeof(T) * (numVals - pos));
		arr[pos] = val;
	}
	numVals++;
}

template <class T> __inline static void ArrayUtils::Remove(uint32 pos, T* &arr, uint32 &numVals, uint32& arrSize, bool resizeArray /* = true */) {
	if(resizeArray == true) {
		uint32 newArrSize = arrSize - 1;
		T *tmpArr = new T[arrSize-1];
		memcpy_s(tmpArr, newArrSize * sizeof(T), arr, pos * sizeof(T));	// up till idx of val
		memcpy_s(tmpArr+pos, (newArrSize - pos) * sizeof(T), arr+pos+1, sizeof(T) * (numVals - pos - 1)); // from idx of val onward
		delete[] arr;
		arr = tmpArr;
		arrSize--;
	} else {
		memcpy(arr + pos, arr + pos + 1, sizeof(T) * (numVals - pos - 1));
	}
	numVals--;
}

template <class T> static void ArrayUtils::Permute(T* arr, uint32* idxs, uint32 len) {
	T* tmp = new T[len];
	for(uint32 i=0; i<len; i++) {
		tmp[i] = arr[idxs[i]];
	}
	memcpy_s(arr, sizeof(T)*len, tmp, sizeof(T)*len);
	delete[] tmp;
}

//template <class T>
//T* ArrayUtils<T>::sMergeSortAuxArr = NULL;

#if defined (__GNUC__) && (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
#undef __inline
#undef static
#endif


#endif // __ARRAYUTILS


