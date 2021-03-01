#ifndef _RANDOMUTILS_H
#define _RANDOMUTILS_H

#include "QtilsApi.h"

/** A util class to make randomification easier */
class QTILS_API RandomUtils {
public:
	/** Static method to initialize the random number generator */
	static uint32 Init(uint32 seed);
	static uint32 Init();

	/**
	* Static method that returns a random boolean value
	* @param p is the probability of return 1
	* @return the random boolean value
	*/
	static bool UniformBool(float p=0.5);

	/** Static method that returns a random uint8 in range [0,max).
		In other words, 0 <= random number < max <= UINT8_MAX_VALUE */
	static uint8 UniformUint8(uint8 max);
	/** Static method that returns a random uint16 in range [0,max).
		In other words, 0 <= random number < max <= UINT16_MAX_VALUE */
	static uint16 UniformUint16(uint16 max);
	/** Static method that returns a random uint32 in range [0,max).
		In other words, 0 <= random number < max <= UINT32_MAX_VALUE */
	static uint32 UniformUint32(uint32 max);
	/** Static method that returns a random uint64 in range [0,max).
		In other words, 0 <= random number < max <= UINT64_MAX_VALUE */
	static uint64 UniformUint64(uint64 max);

	/** Static method that returns a random float in range [0,max] */
	static float UniformFloat(float max=1);
	/** Static method that returns a random float in range [min,max] */
	static float UniformFloatRange(float min, float max);

	/** Static method that returns a random double in range [0,max] */
	static double UniformDouble(double max=1);
	/** Static method that returns a random double in range [min,max] */
	static double UniformDoubleRange(double min, double max);

	/** Static method that returns a random double from normal distribution with mean 0 and variance 1. */
	static double NormalDouble();
	/** Static method that returns a random double from normal distribution with given mean and variance. */
	static double NormalDouble(double mean, double variance) { return mean + variance * NormalDouble(); }

	/**
	* Static method that returns a random element from the supplied array in the range 0 and len
	* @param array pointer to array
	* @param len range of which to choose from
	* @return the random element 
	*/
	template <class T> static T UniformChoose(T *array, uint32 len) {
		return array[UniformUint32(len)];	
	}
	//static uint8  UniformChooseUint8(uint8 *array, uint32 len);
	//static uint16 UniformChooseUint16(uint16 *array, uint32 len);
	//static uint32 UniformChooseUint32(uint32 *array, uint32 len);


	/** Generates array containing all number from from to to, with step, ordered. 
		Please note: up to to, but not including to. */
	static uint32	*GenerateOrderedList(uint32 from, uint32 to, uint32 step = 1);
	/** Generates array like GenerateOrderedList(), but also permutes these. */
	static uint32 *GeneratePermutedList(uint32 from, uint32 to, uint32 step = 1);

	/** Permutes the given array `arr' of `length' elements. */
	template <class T> static void PermuteArray(T *arr, uint32 length) {
		uint32 idx1, idx2;
		T val;
		for(uint32 i=0; i<length; i++) {
			idx1 = UniformUint32(length);
			idx2 = UniformUint32(length);
			val = arr[idx1];
			arr[idx1] = arr[idx2];
			arr[idx2] = val;
		}
	}

protected:
	static bool		mSecondNormalDouble;
	static double	mNormalDouble2;
};

#endif /*_RANDOMUTILS_H */
