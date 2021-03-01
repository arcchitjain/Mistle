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
#ifndef __CTFILE_H
#define __CTFILE_H

enum CTFileFormat {
	CTFileNoHeader,
	CTFileVersion1_0
};

// >= max line length
#define		CTFILE_BUFFER_LENGTH		200000


// Currently only used for READING CodeTables. Not used for writing, because of speed.
class CTFile {
public:
	// Creates new CTFile, open it using Open(&filename)
	CTFile(Database *db);
	// Creates new CTFile and tries to open filename
	CTFile(const string &filename, Database *db);

	~CTFile();

	void				Open(const string &filename);
	void				Close();

	uint32				GetNumSets()					{ return mNumSets; }
	uint32				GetAlphLen()					{ return mAlphLen; }
	uint32				GetMaxSetLen()					{ return mMaxSetLen; }

	void				SetNeedSupports(bool f)			{ mNeedSupports = f; }
	bool				GetNeedSupports()				{ return mNeedSupports; }

	void				ReadHeader();
	ItemSet*			ReadNextItemSet();

protected:
	uint32				CountCols(char *buffer);

	Database			*mDB;

	string				mFilename;
	FILE				*mFile;
	char				*mBuffer;

	uint16				*mSet;
	double				*mStdLengths;

	uint32				mNumSets;
	uint32				mAlphLen;
	uint32				mMaxSetLen;

	bool				mNeedSupports;
	CTFileFormat		mFileFormat;
	bool				mHasNumOccs;
};
#endif // __CTFILE_H
