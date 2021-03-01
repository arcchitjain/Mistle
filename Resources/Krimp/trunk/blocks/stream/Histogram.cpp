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
#ifdef BLOCK_STREAM

#include "../../global.h"

#include <Primitives.h>

#if defined (__unix__)
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
