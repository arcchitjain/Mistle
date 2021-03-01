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
#ifndef __ISCFILE_H
#define __ISCFILE_H

#include <FileUtils.h>

#include "../BassApi.h"
#include "../itemstructs/ItemSet.h"

enum IscFileType {
	FimiIscFileType,
	FicIscFileType,
	BinaryFicIscFileType,
	NoneIscFileType
};

#define	ISCFILE_BUFFER_LENGTH		200000

class Database;
class ItemSet;
class ItemSetCollection;

class BASS_API IscFile {
public:
	/* Initializes mBuffer and mDatabase and mSetArray with NULL */
	IscFile();

	/* Closes file link and deletes the read buffer */
	virtual ~IscFile();

	// Opens the file, reads the header, and preps the returned ItemSetCollection with this info. Requires `db` for StdSizes.
	virtual ItemSetCollection*	Open(const string &filename, Database * const db);
	// Opens the file, reads the header, and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Open(const string &filename) =0;

	// Opens the file, reads all data and preps the returned ItemSetCollection with this info. Requires `db` for StdSizes.
	virtual ItemSetCollection*	Read(const string &filename, Database * const db);
	// Opens the file, reads all data and preps the returned ItemSetCollection with this info. No stdSizes!
	virtual ItemSetCollection*	Read(const string &filename) =0;

	// Writes the provided ItemSetCollection to file (static-wannabee, vanwege overerving..)
	virtual bool WriteItemSetCollection(ItemSetCollection *isc, const string &fullpath);
	// Writes the loaded ItemSets in the provided ItemSetCollection to file (static-wannabee, vanwege overerving..)
	virtual bool WriteLoadedItemSetCollection(ItemSetCollection * const isc, const string &fullpath) =0;
	// Writes the loaded ItemSets in the provided ItemSetCollection to the currently opened file (static-wannabee, vanwege overerving..)
	virtual void WriteLoadedItemSets(ItemSetCollection * const isc) =0;

	// Iterative reading from IscFile
	virtual ItemSet*	ReadNextItemSet()=0;

	virtual void		SetPreferredDataType(ItemSetType ist) { mPreferredDataType = ist; }
	virtual ItemSetType	GetPreferredDataType() { return mPreferredDataType; }
	
	virtual Database*	GetDatabase() { return mDatabase; }
	virtual void		SetDatabase(Database *db);

	virtual string		GetFileName() { return mFilename; }

	static IscFileType	StringToType(const string &filename);
	static IscFileType	ExtToType(const string &extension);
	static string		TypeToExt(const IscFileType iscType);

	static IscFile*		Create(const IscFileType type);

	/* Returns the actual IscFileType of the IscFile object */
	virtual IscFileType	GetType()=0;

	void				Delete();

protected:
	/* Opens a filehandle, remembers the filename */
	virtual void		OpenFile(const string &filename, const FileOpenType openType = FileReadable);
	/* Universal implementation of IscFile::Close() fpcloses the file */
	virtual void		CloseFile();
	/* Returns whether a file handle is open */
	virtual bool		IsOpen();

	/* Reads a line into the buffer */
	virtual size_t		ReadLine(char* buffer, int size);

	/* Reads the first couple 'o lines, if required for the file type. Call after using OpenFile. */
	virtual void		ReadFileHeader()=0;

	// Writes the file header, using the provided number of sets (aka you decide loaded or all Item Sets)
	virtual void		WriteHeader(const ItemSetCollection *isc, const uint64 numSets)=0;

	virtual void		WriteItemSet(ItemSet *is)=0;

	int64				GetFileSize();

	FILE		*mFile;
	string		mFilename;

	char		*mBuffer;

	ItemSetType	mPreferredDataType;

	Database	*mDatabase;
	double		*mStdLengths;

	friend class ItemSetCollection;	// ConvertISC heeft OpenFile, WriteHeader & WriteItemSet nodig
};
#endif // __ISCFILE_H
