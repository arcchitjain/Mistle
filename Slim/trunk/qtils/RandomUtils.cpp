
#include "qlobal.h"

#include <time.h>

#include "RandomUtils.h"

bool	RandomUtils::mSecondNormalDouble = false;
double	RandomUtils::mNormalDouble2 = 0;

uint32 RandomUtils::Init(uint32 seed) {
	srand(seed);
	return seed;
}

uint32 RandomUtils::Init() {
	return Init((uint32) time(NULL));
}

/*
void RandomUtils::Test() {
	RandomUtils::Init();
	string rndFileStr = Bass::GetWorkingDir() + "rndTest.txt";
	FILE *rndFile = fopen(rndFileStr.c_str(),"w");
	uint32 numBins = 100;
	uint32 *testArr = new uint32[numBins*2];
	for(uint32 i=0; i<numBins*2; i++)
	testArr[i] = 0;
	for(uint32 i=0; i<numBins * 10000; i++) {
	uint32 rndVal = RandomUtils::UniformUint32(numBins);
	testArr[rndVal]++;
	}
	uint32 sum = 0;
	for(uint32 i=0; i<numBins*2; i++) {
	sum += testArr[i];
	fprintf(rndFile, "%d: %d, ", i, testArr[i]);
	}
	fprintf(rndFile, "\n%d ", sum);
	fclose(rndFile);
}
*/

bool RandomUtils::UniformBool(float p) {
	if(rand() < (int)((RAND_MAX) * p))
		return true;
	else
		return false;
}

// Generate a random uint8 in the half-closed interval
// [0, max). In other words, 0 <= random number < max <= 255
uint8 RandomUtils::UniformUint8(uint8 max) {
	return (uint8) ((double)rand() / (RAND_MAX + 1) * (max));
}

// Generate a random uint16 in the half-closed interval
// [0, max). In other words, 0 <= random number < max <= 65535
uint16 RandomUtils::UniformUint16(uint16 max) {
	uint32 val1, val2, val;
	val1 = rand();	// levert 0x00ff
	val2 = rand();  // levert 0xff00
	val = (val1 & 0x000000FF) | ((val2 & 0x000000FF) << 8);
	return val % (max);
}

// Generate a random uint32 in the half-closed interval
// [0, max). In other words, 0 <= random number < max <= 4294967295
uint32 RandomUtils::UniformUint32(uint32 max) {
	uint32 val1, val2, val3, val;
	val1 = rand();	// levert 0x00000fff
	val2 = rand();  // levert 0x00fff000
	val3 = rand();  // levert 0xff000000
	val = (val1 & 0x00000FFF) | ((val2 & 0x00000FFF) << 12) | (val3 & 0x000000FF) << 24;
	return val % (max);
}

// Generate a random uint64 in the half-closed interval
// [0, max). In other words, 0 <= random number < max <= UINT64_MAX
uint64 RandomUtils::UniformUint64(uint64 max) {
	uint32 val1, val2, val3;
	uint64 valA, valB, val;
	val1 = rand();	// levert 0x00000fff
	val2 = rand();  // levert 0x00fff000
	val3 = rand();  // levert 0xff000000
	valA = (val1 & 0x00000FFF) | ((val2 & 0x00000FFF) << 12) | (val3 & 0x000000FF) << 24;
	val1 = rand();	// levert 0x00000fff
	val2 = rand();  // levert 0x00fff000
	val3 = rand();  // levert 0xff000000
	valB = (val1 & 0x00000FFF) | ((val2 & 0x00000FFF) << 12) | (val3 & 0x000000FF) << 24;
	val = (valA << 32) | valB;
	return val % (max);
}

float RandomUtils::UniformFloat(float max) {
	return UniformFloatRange(0, max);
}

float RandomUtils::UniformFloatRange(float min, float max) {
	return min + (fabs(max - min)) * (rand() / (float)RAND_MAX);
}

double RandomUtils::UniformDouble(double max) {
	return UniformDoubleRange(0,max);
}

double RandomUtils::UniformDoubleRange(double min, double max) {
	return min + (fabs(max - min)) * (rand() / (double)RAND_MAX);
}

double RandomUtils::NormalDouble() {
	if(mSecondNormalDouble) {
		mSecondNormalDouble = false;
		return mNormalDouble2;
	}

	double x1, x2, w;
	do {
		x1 = 2.0f * UniformDouble() - 1.0f;
		x2 = 2.0f * UniformDouble() - 1.0f;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0f );
	w = sqrt( (-2.0f * log( w ) ) / w );

	double mNormalDouble1 = x1 * w;
	mNormalDouble2 = x2 * w;

	mSecondNormalDouble = true;

	return mNormalDouble1;
}

/*
uint32 RandomUtils::UniformChooseUint32(uint32 *array, uint32 len) {
	return (uint32) array[UniformUint32(len)];	
}

uint16 RandomUtils::UniformChooseUint16(uint16 *array, uint32 len) {
	return (uint16) array[UniformUint32(len)];	
}

uint8 RandomUtils::UniformChooseUint8(uint8 *array, uint32 len) {
	return (uint8) array[UniformUint32(len)];	
}
*/

uint32* RandomUtils::GenerateOrderedList(uint32 from, uint32 to, uint32 step) {
	uint32 len = (to - from) / step;
	uint32 val = from;

	uint32 *list = new uint32[len];
	for(uint32 i=0; i<len; i++) {
		list[i] = val;
		val += step;
	}
	return list;
}

uint32* RandomUtils::GeneratePermutedList(uint32 from, uint32 to, uint32 step) {
	uint32 *list = GenerateOrderedList(from, to, step);
	PermuteArray(list, (to - from) / step);
	return list;
}

/*
void RandomUtils::PermuteList(uint32 *list, uint32 length) {
	uint32 idx1, idx2, val;
	for(uint32 i=0; i<length; i++) {
		idx1 = UniformUint32(length);
		idx2 = UniformUint32(length);
		val = list[idx1];
		list[idx1] = list[idx2];
		list[idx2] = val;
	}
}

void RandomUtils::PermuteList(uint8 *list, uint32 length) {
	uint8 idx1, idx2, val;
	for(uint32 i=0; i<length; i++) {
		idx1 = UniformUint32(length);
		idx2 = UniformUint32(length);
		val = list[idx1];
		list[idx1] = list[idx2];
		list[idx2] = val;
	}
}
*/
