#ifdef BLOCK_STREAM

#include "../../global.h"

#include <Primitives.h>

#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <glibc_s.h>
#endif

#include "Histogram.h"

Histogram::Histogram(const uint32 numBins, const double *samples, const uint32 numSamples) {
	mNumBins = numBins;
	mNumSamples = numSamples;
	mMinValue = samples[0];
	mMaxValue = samples[0];
	for(uint32 i=1; i<numSamples; i++)
		if(samples[i] < mMinValue)
			mMinValue = samples[i];
		else if(samples[i] > mMaxValue)
			mMaxValue = samples[i];
	mBinWidth = (mMaxValue - mMinValue) / mNumBins;

	mBins = new uint32[mNumBins];
	for(uint32 i=0; i<mNumBins; i++)
		mBins[i] = 0;
	uint32 bin;
	double boundary, start = mMinValue + mBinWidth, sample, total = 0.0;
	for(uint32 i=0; i<mNumSamples; i++) {
		bin = 0;
		boundary = start;
		sample = samples[i];
		total += sample;
		while(sample > boundary) {
			boundary += mBinWidth;
			++bin;
		}
		if(bin >= mNumBins) bin = mNumBins - 1;
		++mBins[bin];
	}
	mMean = total / mNumSamples;
}

Histogram::~Histogram() {
	delete[] mBins;
}

void Histogram::SetCutOff(float percentage) {
	if(percentage == 0.0f) {
		mLowerCutOff = mMinValue;
		mUpperCutOff = mMaxValue;
		return;
	} else if(percentage < 0.0f) {
		percentage *= -1.0f;
		mLowerCutOff = mMean * (1.0f - percentage);
		mUpperCutOff = mMean * (1.0f + percentage);
		return;
	}
	uint32 num = (uint32)(mNumSamples * percentage);
	uint32 bin = 0;
	uint32 count = mBins[bin];
	while(count < num)
		count += mBins[++bin];
	mLowerCutOff = mMinValue + (bin+1)*mBinWidth;
	bin = mNumBins-1;
	count = mBins[bin];
	while(count < num)
		count += mBins[--bin];
	mUpperCutOff = mMinValue + bin*mBinWidth;
}

bool Histogram::IsCutOff(double value) {
	if(value < mLowerCutOff)
		return true;
	if(value > mUpperCutOff)
		return true;
	return false;
}
void Histogram::WriteToDisk(const string &filename) {
	FILE *histFile;
	if(fopen_s(&histFile, filename.c_str(), "w") != 0)
		throw string("Histogram::WriteToDisk() -- Cannot open file.");
	fprintf(histFile, "numBins;%d;numSamples;%d\n", mNumBins, mNumSamples);
	fprintf(histFile, "minVal;%f;maxVal;%f;binWidth;%f\n", mMinValue, mMaxValue, mBinWidth);
	fprintf(histFile, "lowerBoundary;%f;upperBoundary;%f\n", mLowerCutOff, mUpperCutOff);
	for(size_t i=0; i<mNumBins; i++)
		fprintf(histFile, "%d\n", mBins[i]);
	fclose(histFile);
}

#endif // BLOCK_STREAM
