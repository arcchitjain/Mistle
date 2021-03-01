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
#ifndef __FIMIISCFILE_H
#define __FIMIISCFILE_H

enum FimiIscFileFormat {
	FimiIscFileNoHeader,
	FimiIscFileVersion1_0,
	FimiIscFileVersion1_1,
};

#include "IscFile.h"

class BASS_API FimiIscFile : public IscFile {
public:
	FimiIscFile();
	//IscFimiFile(const string &filename, Database *db, ItemSetType ist = NoneItemSetType);
	virtual ~FimiIscFile();

	// Opens the file, reads the header, and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Open(const string &filename);

	// Opens the file, reads all data and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Read(const string &filename);

	// Writes the provided ItemSetCollection to file
	virtual bool WriteLoadedItemSetCollection(ItemSetCollection * const isc, const string &fullpath);
	// Writes the loaded ItemSets in the provided ItemSetCollection to the currently opened file
	virtual void WriteLoadedItemSets(ItemSetCollection * const isc);

	virtual ItemSet*	ReadNextItemSet();

	virtual IscFileType	GetType() { return FimiIscFileType; }

protected:
	virtual void		ReadFileHeader();
	void				SetIscProperties(ItemSetCollection *isc);
	// Sets isc properties: IscFile, numItemSets, minSup, maxSetLength, dataType
	void				ScanForIscProperties();

	virtual void		WriteHeader(const ItemSetCollection *isc, const uint64 numSets);

	virtual void		WriteItemSet(ItemSet *is);

	uint16				*mSetArray;		// tmp array of size db->GetMaxSetLength()
	bool				mHasSepNumRows;	// read ItemSet has separate supports for db rows and transactions
	uint32				mMinSup;
	uint32				mMaxSetLength;
	uint64				mNumItemSets;
	string				mPatternType;
	string				mDatabaseName;
	FimiIscFileFormat	mFileFormat;

};
#endif // __FIMIISCFILE_H
