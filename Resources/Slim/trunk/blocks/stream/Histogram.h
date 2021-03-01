#ifndef __HISTOGRAM_H
#define __HISTOGRAM_H

class Histogram {
public:
	Histogram(const uint32 numBins, const double *samples, const uint32 numSamples);
	virtual ~Histogram();

	void	SetCutOff(float percentage);
	bool	IsCutOff(double value);

	void	WriteToDisk(const string &filename);

protected:
	uint32	mNumBins, mNumSamples;
	uint32	*mBins;
	double	mBinWidth;
	double	mMinValue, mMaxValue;
	double	mMean;

	double mLowerCutOff, mUpperCutOff;
};

#endif // __HISTOGRAM_H
