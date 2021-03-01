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
#ifndef __STREAMALGO_H
#define __STREAMALGO_H

class Database;
class ItemSetCollection;
class CodeTable;
class Histogram;

class StreamAlgo {
public:
	StreamAlgo(const uint32 blockSize, const uint32 maxIterations, const bool dbSizeOnly);
	virtual ~StreamAlgo();

	CodeTable*	FindGoodEnoughCT(Config *config, Database *db, const uint32 offset, const float acldThresh);

	Histogram*	BuildBlockSizeHistogram(CodeTable *ct, uint32 blockSize, uint32 histNumBins, uint32 blockHistNumSamples);
	bool		DistributionIsTheSame(Database *db, CodeTable *ct, Histogram *hist, uint32 offset, uint32 blockSize);
	bool		AcceptNewCT(CodeTable *ct, CodeTable *newCT, const float acldThreshold, float &acceptedAcldSum);

	// Apply Krimp to a series of databases of increasing size.
	void	SerialKrimp(Config *config, Database *db, const uint32 offset);

	void	SetStorageDir(const string &s) { mStorageDir = s; FileUtils::CreateDirIfNotExists(s); }

protected:
	// No additional computational and memory requirements, only save numCTs CTs to disk.
	void	SerialKrimpNoStatistics(Config *config, Database *db, const uint32 offset);
	// Apply Krimp to a series of databases, keep them in memory and compute additional numbers on-the-fly.
	void	SerialKrimpAndStatistics(Config *config, Database *db, const uint32 offset);

	uint32	mBlockSize;
	uint32	mMaxIterations;
	bool	mDbSizeOnly;
	string	mStorageDir;
};

#endif // __STREAMALGO_H
