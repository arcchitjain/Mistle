#ifndef _LOWENTROPYSETMINER_H
#define _LOWENTROPYSETMINER_H

#include "../Croupier.h"

#define LEMINER_BUFFER_LENGTH 1024
#define LEMINER_FLOATBUFFER_LENGTH 10
#define LEMINER_DEFAULT_VERSION 31
#define LEMINER_DEFAULT_MAXENTROPY 1.0
#define LEMINER_DEFAULT_MINSETLEN 1
#define LEMINER_DEFAULT_MAXSETLEN 16
#define LEMINER_DEFAULT_MINFREQ 0.0
#define LEMINER_DEFAULT_MAXFREQ 1.0
#define LEMINER_DEFAULT_MAXNUMSETS UINT32_MAX_VALUE

class CROUPIER_API LESetMiner : public Croupier {
public:
	LESetMiner(Config *conf);
	LESetMiner();
	virtual ~LESetMiner();

	virtual string	GetPatternTypeStr() { return "lesets"; }

	virtual void	SetMaxEntropy(double maxEntropy) {mMaxEntropy = maxEntropy; };
	virtual void	SetMaxSetLength(uint32 maxSetLen) { mMaxSetLen = maxSetLen; };
	virtual void	SetMinSetLength(uint32 minSetLen) { mMinSetLen = minSetLen; };
	virtual void	SetMaxFrequency(float maxFreq) { mMaxFrequency = maxFreq; }
	virtual void	SetMinFrequency(float minFreq) { mMinFrequency = minFreq; }
	virtual void	SetMaxNumSets(uint32 maxNumSets) { mMaxNumSets = maxNumSets; }

	virtual double	GetMaxEntropy() { return mMaxEntropy; }
	virtual uint32	GetMaxSetLength() { return mMaxSetLen; }
	virtual uint32	GetMinSetLength() { return mMinSetLen; }
	virtual double	GetMinFrequency() { return mMinFrequency; }
	virtual double	GetMaxFrequency() { return mMaxFrequency; }
	virtual uint64	GetMaxNumSets() { return mMaxNumSets; }

	/* --- Offline Dealing --- */
	virtual string	GenerateOutputFileName(const Database *db);
	virtual bool	MinePatternsToFile(const string &dbFilename, const string &outputFile, Database *db = NULL);
	virtual bool	MinePatternsToFile(const Database *db, const string &outputFile);

	virtual uint32	GetNumSetsWritten() { return mNumSetsWritten; }

protected:
	void			MineLESets();
	void inline		OutputEntropySet(uint16 *cand, uint32 candLen, double entropy);

	void			MineLESetsDepthFirstV41(double maxEntropy, uint16* abItems, uint32 numAbItems, uint16* prefix, uint32 prefixLen, double prefixEntropy);
	void			MineLESetsDepthFirstV4(double maxEntropy, uint16* abItems, uint32 numAbItems, uint16* prefix, uint32 prefixLen, double prefixEntropy);
	void			MineLESetsDepthFirstV31(double maxEntropy, uint16* abItems, uint32 numAbItems, uint32 *numAbRowIdxs, uint32 **abRowIdxs, uint16* prefix, uint32 prefixLen);
	void			MineLESetsDepthFirstV3(double maxEntropy, uint16* abItems, uint32 numAbItems, uint16* prefix, uint32 prefixLen);
	void			MineLESetsDepthFirstV1(double maxEntropy, uint16* abItems, uint32 numAbItems, uint16* prefix, uint32 prefixLen);

	string	mOutFileName;
	FILE*	mOutFile;

	uint32	mUseMinerVersion;
	uint32	mMinSetLen;
	uint32	mMaxSetLen;
	double	mMaxEntropy;
	float	mMinFrequency;
	float	mMaxFrequency;
	const Database *mDB;
	char	*mOutBuffer;
	char	*mFloatBuffer;

	uint32 ***mInstRowIdxs;
	uint32 **mInstIdxs;
	uint32 **mInstCnts;
	uint32	mNumSetsWritten;
	uint32	mMaxNumSets;

	// v41 specific
	uint32	*mNumInstUsed;
//	uint32	*mInstMask;
	uint32	**mInstIdxStart;
};

#endif // _LOWENTROPYSETMINER_H
