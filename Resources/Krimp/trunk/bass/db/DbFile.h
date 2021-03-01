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
#ifndef __DBFILE_H
#define __DBFILE_H

#include "../BassApi.h"

enum DbFileType {
	DbFileTypeNone = 0,
	DbFileTypeFic,
	DbFileTypeFimi,
};
#if defined (_MSC_VER)
	enum ItemSetType;
#elif defined (__GNUC__)
	#include "../itemstructs/ItemSet.h"
#endif
class Database;

class BASS_API DbFile {
public:
	DbFile();
	virtual ~DbFile();

	virtual Database *Read(string filename) =0;
	virtual bool Write(Database *db, const string &fullpath) =0;

	virtual DbFileType	GetType() =0;
	string				GetFileName();

	ItemSetType			GetPreferredDataType() { return mPreferredDataType; }
	void				SetPreferredDataType(ItemSetType ist) { mPreferredDataType = ist; }

	static DbFileType	StringToType(const string &desc);
	static DbFileType	ExtToType(const string &extension);
	static string		TypeToExt(const DbFileType dbType);

	static DbFile*		Create(DbFileType type);

protected:
	string		mFilename;
	ItemSetType	mPreferredDataType;
};

#endif // __DBFILE_H
