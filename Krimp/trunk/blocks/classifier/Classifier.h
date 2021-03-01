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
#ifndef __CLASSIFIER_H
#define __CLASSIFIER_H

#include <Bass.h>

class Config;
class Database;
class CodeTable;
class DbFile;

class Classifier {
public:
	Classifier(const Config *config);
	~Classifier();

	void Classify();

protected:
	void DoClassification(ClassedDatabase *cdb, CodeTable **CTs, uint32 reportVal, supScoreMap **good, supScoreMap **bad, uint32 **confusion);
	void DoMajorityVoteClassification(ClassedDatabase *cdb, CodeTable **CTs, uint32 reportVal, supScoreMap **good, supScoreMap **bad, uint32 **confusion);

	void WriteTopscoreFile(const string &filename, supScoreMap ***goodT, supScoreMap ***badT, supScoreMap ***good, supScoreMap ***bad, bool percentage);

	void WriteScoreFile(const string &expDir, supScoreMap ***good, supScoreMap ***bad, bool percentage);
	void WriteConfusionFile(const string &filename, uint32 **confusion);
	uint32 LoadCodeTableForSup(Database *db, CodeTable **codeTable, const string &ctType, const string &dir, const uint32 fold, const uint16 classlabel, const uint32 support, string &filename);
		
	const Config *mConfig;

	uint32		mMinSup, mOverallMaxSup, mReportSup;
	uint16		*mClasses;
	uint32		mNumClasses;
	uint32		*mClassSizes;
	uint32		mNumFolds;
	bool		mSanitize;
};

#endif // __CLASSIFIER_H
