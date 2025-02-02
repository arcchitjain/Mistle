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
#ifndef __BINARYFICISCFILE_H
#define __BINARYFICISCFILE_H


enum IscOrderType;
enum ItemSetType;

enum BinFicIscFileVersion {
	BinFicIscFileVersion1_0 = 0,
	BinFicIscFileVersion1_1,
	BinFicIscFileVersion1_2
};

#include "IscFile.h"

class BM128ItemSet;
class Uint16ItemSet;
class BAI32ItemSet;

class BASS_API BinaryFicIscFile : public IscFile {
public:
	// Creates new BinaryFicIscFile, open it using Open(&filename)
	BinaryFicIscFile();
	// Creates new BinaryFicIscFile and tries to open filename
	//BinaryFicIscFile(const string &filename, Database *db, ItemSetType ist = NoneItemSetType);
	// Zaps the BinaryFicIscFile, cleans up it's mess.
	virtual ~BinaryFicIscFile();

	// Opens the file, reads the header, and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Open(const string &filename);

	// Opens the file, reads all data and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Read(const string &filename);

	// Writes the provided ItemSetCollection to file (static-wannabee, vanwege overerving..)
	virtual bool WriteItemSetCollection(ItemSetCollection *isc, const string &fullpath);
	// Writes the loaded ItemSets in the provided ItemSetCollection to file (static-wannabee, vanwege overerving..)
	virtual bool WriteLoadedItemSetCollection(ItemSetCollection * const isc, const string &fullpath);
	// Writes the loaded ItemSets in the provided ItemSetCollection to the currently opened file (static-wannabee, vanwege overerving..)
	virtual void WriteLoadedItemSets(ItemSetCollection * const isc);


	virtual ItemSet*	ReadNextItemSet();

	virtual IscFileType	GetType() { return BinaryFicIscFileType; }

protected:
	virtual void		OpenFile(const string &filename, const FileOpenType openType = FileReadable);

	virtual void		ReadFileHeader();

	virtual void		ReadBuf(char* buffer, uint32 count);

	virtual BM128ItemSet*	ReadNextBM128ItemSet();

	virtual BAI32ItemSet*	ReadNextBAI32ItemSet();

	virtual Uint16ItemSet*	ReadNextUint16ItemSet();


	// Side effect: sets mHasSepNumRows, such that WriteItemSet knows what to write
	virtual void		WriteHeader(const ItemSetCollection *isc, const uint64 numSets);

	virtual void		WriteItemSet(ItemSet *is);

	virtual void		WriteBM128ItemSet(BM128ItemSet *is);

	virtual void		WriteBAI32ItemSet(BAI32ItemSet *is);

	virtual void		WriteUint16ItemSet(Uint16ItemSet *is);


	// Via ReadFileHeader ingelezen stuff die naar ISC gepass't moet worden:
	string				mDatabaseName;
	string				mPatternType;
	uint64				mNumItemSets;
	uint32				mMaxSetLength;
	uint32				mMinSup;
	IscOrderType		mIscOrder;
	string				mIscTag;
	bool				mHasSepNumRows;	// read ItemSet has separate supports for db rows and transactions

	// Array of size of largest set in the file
	uint16*				mSetArray;
	uint32				mBinaryLen;

	BinFicIscFileVersion	mFileFormat;
	ItemSetType			mFileDataType;
	uint32				mNumBMBytes;
	uint32				mNumBMDWords;
};
#endif // __FICISCFILE_H
