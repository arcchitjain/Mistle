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
#ifndef __ARRAYUTILS
#define __ARRAYUTILS

#include "QtilsApi.h"

/** A util class to do funky array stuffs */
class QTILS_API ArrayUtils {
public:

	// sorts array `arr` of length `arrLen` ascendingly
	template <class T> static void BSortArrayAsc(T* arr, uint32 arrLen) {
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

	// sorts array `arr` of length `arrLen` ascendingly, keeping track of the original indici in (filled by you) `origIdxArr`
	template <class T> static void BSortArrayAsc(T* arr, uint32 arrLen, uint32* origIdxArr) {
		// bubble-sort
		for(uint32 i=1; i<arrLen; i++) {
			for(uint32 j=0; j<arrLen-i; j++) {
				if(arr[j] > arr[j+1]) {
					T tmp = arr[j];
					arr[j] = arr[j+1];
					arr[j+1] = tmp;
					uint32 swap = origIdxArr[j];
					origIdxArr[j]=origIdxArr[j+1];
					origIdxArr[j+1]=swap;
				}
			}
		}
	}

	// sorts array `arr` of length `arrLen` descendingly
	template <class T> static void BSortArrayDesc(T* arr, uint32 arrLen) {
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

	// sorts array `arr` of length `arrLen` descendingly, keeping track of the original indici in (filled by you) `origIdxArr`
	template <class T> static void BSortArrayDesc(T* arr, uint32 arrLen, uint32* origIdxArr) {
		// bubble-sort
		for(uint32 i=1; i<arrLen; i++) {
			for(uint32 j=0; j<arrLen-i; j++) {
				if(arr[j] < arr[j+1]) {
					T tmp = arr[j];
					arr[j] = arr[j+1];
					arr[j+1] = tmp;
					uint32 swap = origIdxArr[j];
					origIdxArr[j]=origIdxArr[j+1];
					origIdxArr[j+1]=swap;
				}
			}
		}	
	}
	// sorts array `arr` of length `arrLen` descendingly, while synchronising any changes in the order to `syncArr'
	template <class T, class U> static void BSortArrayDesc(T* arr, uint32 arrLen, U* syncArr) {
		// bubble-sort
		for(uint32 i=1; i<arrLen; i++) {
			for(uint32 j=0; j<arrLen-i; j++) {
				if(arr[j] < arr[j+1]) {
					T tmp = arr[j];
					arr[j] = arr[j+1];
					arr[j+1] = tmp;
					U swap = syncArr[j];
					syncArr[j]=syncArr[j+1];
					syncArr[j+1]=swap;
				}
			}
		}	
	}

	template <class T> static void CreateStaticMess(uint32 maxArrLen, bool idxArr) {
		sMergeSortAuxArr = new T[(maxArrLen+1)/2];
		if(idxArr)
			sMergeSortIdxAuxArr = new uint32[(maxArrLen+1)/2];
	}
	static void CleanUpStaticMess();

	template <class T> static void MSortArrayAsc(T *arr, uint32 arrLen) {
		if(arrLen == 0)
			return;
		bool freshArray = false;
		if(sMergeSortAuxArr == NULL) {
			sMergeSortAuxArr = new T[(arrLen+1)/2];
			freshArray = true;
		}
		MSortArrayAscSort(arr, 0, arrLen-1, (T*)sMergeSortAuxArr);
		if(freshArray) {
			delete[] sMergeSortAuxArr;
			sMergeSortAuxArr = NULL;
		}
	}

	template <class T> static void MSortArrayAsc(T *arr, uint32 arrLen, uint32* origIdxArr) {
		if(arrLen == 0)
			return;
		bool freshAuxArray = false, freshIdxAuxArray = false;
		if(sMergeSortAuxArr == NULL) {
			sMergeSortAuxArr = new T[(arrLen+1)/2];
			freshAuxArray = true;
		}
		if(sMergeSortIdxAuxArr == NULL) {
			sMergeSortIdxAuxArr = new uint32[(arrLen+1)/2];
			freshIdxAuxArray = true;
		}

		MSortArrayAscSort(arr, 0, arrLen-1, (T*) sMergeSortAuxArr, origIdxArr, sMergeSortIdxAuxArr);
		if(freshAuxArray) {
			delete[] sMergeSortAuxArr;
			sMergeSortAuxArr = NULL;
		}
		if(freshIdxAuxArray) {
			delete[] sMergeSortIdxAuxArr;
			sMergeSortIdxAuxArr = NULL;
		}
	}
	template <class T> static void MSortArrayDesc(T *arr, uint32 arrLen) {
		if(arrLen == 0)
			return;
		bool freshArray = false;
		if(sMergeSortAuxArr == NULL) {
			sMergeSortAuxArr = new T[(arrLen+1)/2];
			freshArray = true;
		}
		MSortArrayDescSort(arr, 0, arrLen-1, (T*) sMergeSortAuxArr);
		if(freshArray) {
			delete[] sMergeSortAuxArr;
			sMergeSortAuxArr = NULL;
		}	
	}
	template <class T> static void MSortArrayDesc(T *arr, uint32 arrLen, uint32* origIdxArr) {
		if(arrLen == 0)
			return;
		bool freshAuxArray = false, freshIdxAuxArray = false;
		if(sMergeSortAuxArr == NULL) {
			sMergeSortAuxArr = new T[(arrLen+1)/2];
			freshAuxArray = true;
		}
		if(sMergeSortIdxAuxArr == NULL) {
			sMergeSortIdxAuxArr = new uint32[(arrLen+1)/2];
			freshIdxAuxArray = true;
		}
		MSortArrayDescSort(arr, 0, arrLen-1, (T*) sMergeSortAuxArr, origIdxArr, sMergeSortIdxAuxArr);
		if(freshAuxArray) {
			delete[] sMergeSortAuxArr;
			sMergeSortAuxArr = NULL;
		}
		if(freshIdxAuxArray) {
			delete[] sMergeSortIdxAuxArr;
			sMergeSortIdxAuxArr = NULL;
		}
	}

	template <class T> static uint32 BinarySearchAsc(T val, T* arr, uint32 arrLen) {
		uint32 minIdx = 0;
		uint32 maxIdx = arrLen-1;
		uint32 tstIdx = 0;
		bool found = false;
		while(minIdx <= maxIdx) {
			tstIdx = minIdx + (maxIdx - minIdx)/2;
			if(arr[tstIdx] > val) {
				maxIdx = tstIdx - 1;
			} else if(arr[tstIdx] < val) {
				minIdx = tstIdx + 1;
			} else {
				return tstIdx;
			}
		}
		return UINT32_MAX_VALUE;
	}

	template <class T> static uint32 BinarySearchMayNotExistAsc(T val, T* arr, uint32 arrLen) {
		uint32 minIdx = 0;
		uint32 maxIdx = arrLen-1;
		uint32 tstIdx = UINT32_MAX_VALUE;
		bool found = false;
		while(minIdx+1 < maxIdx) { // while there are at least 3 possibilities
			tstIdx = minIdx + (maxIdx - minIdx)/2;
			if(arr[tstIdx] > val) {
				maxIdx = tstIdx - 1;
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

	template <class T> static uint32 BinarySearchInsertionLocationAsc(T val, T* arr, uint32 arrLen) {
		uint32 minIdx = 0;
		uint32 maxIdx = arrLen-1;
		uint32 tstIdx = 0;
		bool found = false;
		while(minIdx <= maxIdx) {
			tstIdx = minIdx + (maxIdx - minIdx)/2;
			if(arr[tstIdx] > val) {
				maxIdx = tstIdx - 1;
			} else if(arr[tstIdx] < val) {
				minIdx = tstIdx + 1;
			} else {
				return tstIdx;
			}
		}
		return min(minIdx,maxIdx);
	}

protected:
	//static void MSortArrayAscAux(uint32 *arr, uint32 arrLen, uint32 *auxArray /* auxiliary array of length arrLen+1/2 */ );
	template <class T> static void MSortArrayAscMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr) {
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

	template <class T> static void MSortArrayAscSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr) {
		if(lo<hi) {
			uint32 m = (lo+hi)/2;
			MSortArrayAscSort(arr, lo, m, mergeSortAuxArr);
			MSortArrayAscSort(arr, m+1, hi, mergeSortAuxArr);
			MSortArrayAscMerge(arr, lo, m, hi, mergeSortAuxArr);
		}
	}

	template <class T> static void MSortArrayAscMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr, uint32* idxArr, uint32* idxAuxArr) {
		uint32 i, j, k;
		// copy first half of array a to auxiliary array b
		// i=0; j=lo; while (j<=m) mergeSortAuxArr[i++]=arr[j++];
		memcpy(mergeSortAuxArr,arr+lo,sizeof(T)*(m-lo+1));
		memcpy(idxAuxArr,idxArr+lo,sizeof(uint32)*(m-lo+1));
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
		memcpy(idxArr+k,idxAuxArr+i,sizeof(uint32)*(j-k));
	}
	template <class T> static void MSortArrayAscSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr, uint32* idxArr, uint32* idxAuxArr) {
		if(lo<hi) {
			uint32 m = (lo+hi)/2;
			MSortArrayAscSort(arr, lo, m, mergeSortAuxArr, idxArr, idxAuxArr);
			MSortArrayAscSort(arr, m+1, hi, mergeSortAuxArr, idxArr, idxAuxArr);
			MSortArrayAscMerge(arr, lo, m, hi, mergeSortAuxArr, idxArr, idxAuxArr);
		}
	}

	//static void MSortArrayDescAux(uint32 *arr, uint32 arrLen, uint32 *auxArray /* auxiliary array of length arrLen+1/2 */ );
	template <class T> static void MSortArrayDescMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr) {
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
	template <class T> static void MSortArrayDescSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr) {
		if(lo<hi) {
			uint32 m = (lo+hi)/2;
			MSortArrayDescSort(arr, lo, m, mergeSortAuxArr);
			MSortArrayDescSort(arr, m+1, hi, mergeSortAuxArr);
			MSortArrayDescMerge(arr, lo, m, hi, mergeSortAuxArr);
		}
	}

	template <class T> static void MSortArrayDescMerge(T *arr, uint32 lo, uint32 m, uint32 hi, T* mergeSortAuxArr, uint32* idxArr, uint32* idxAuxArr) {
		uint32 i, j, k;
		// copy first half of array a to auxiliary array b
		// i=0; j=lo; while (j<=m) mergeSortAuxArr[i++]=arr[j++];
		memcpy(mergeSortAuxArr,arr+lo,sizeof(T)*(m-lo+1));
		memcpy(idxAuxArr,idxArr+lo,sizeof(uint32)*(m-lo+1));
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
		memcpy(idxArr+k,idxAuxArr+i,sizeof(uint32)*(j-k));
	}
	template <class T> static void MSortArrayDescSort(T *arr, uint32 lo, uint32 hi, T* mergeSortAuxArr, uint32* idxArr, uint32* idxAuxArr) {
		if(lo<hi) {
			uint32 m = (lo+hi)/2;
			MSortArrayDescSort(arr, lo, m, mergeSortAuxArr, idxArr, idxAuxArr);
			MSortArrayDescSort(arr, m+1, hi, mergeSortAuxArr, idxArr, idxAuxArr);
			MSortArrayDescMerge(arr, lo, m, hi, mergeSortAuxArr, idxArr, idxAuxArr);
		}
	}

	static void* sMergeSortAuxArr;
	static uint32* sMergeSortIdxAuxArr;
};
#endif // __ARRAYUTILS

