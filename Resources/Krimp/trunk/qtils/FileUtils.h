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
#ifndef _FILEUTILS_H
#define _FILEUTILS_H

#include "QtilsApi.h"
#if defined (_WINDOWS)
	#include <windows.h>
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	#include <dirent.h>
	#include <regex.h>
#endif

enum FileOpenType {
	FileReadable,
	FileWritable,
	FileBinaryReadable,
	FileBinaryWritable
};

class QTILS_API FileUtils
{
public:
	static void		SplitPathFromFilename(string &filename, string &path);
	static string	ExtractExtension(const string &filename);
	static string	ExtractFileName(const string &filename);

	static bool		Exists(const string &path);
	static bool		FileExists(const string &path);
	static bool		DirectoryExists(const string &path);
	static bool		IsDirectory(const string &path);

	static uint32	CountFiles(const string &path, const string &query="");

	static string	GetCurDirectory();
	static bool		CreateDir(const string &path);
	static bool		CreatePath(const string &path);
	static bool		CreateDirIfNotExists(const string &path);
	static bool		CreatePathIfNotExists(const string &path);
	static bool		RemoveDir(const string &path);

	static bool		RemoveFile(const string &path);

	static bool		FileMove(const string &fromPath, const string &toPath);

	static string	FileOpenTypeToString(FileOpenType p);

};

class QTILS_API directory_iterator
{
public:
	string		entry_path;

#if defined (_WINDOWS)
	HANDLE		handle;
	WIN32_FIND_DATAA FoundFileData;
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	DIR* dir;
	struct dirent* direntry;
	regex_t preg;
#endif

	directory_iterator();

	/* Will return an iterator for files matching `query` in `path`
		Before data extraction via filename or fullpath, first `next()`

		Example:
		directory_iterator dirit("d:\dir","*.file");
		(.
	*/	
	directory_iterator(const string &path, const string &query="");

	~directory_iterator();

	bool equals(const directory_iterator &b);
	bool next();
	string filename();
	string fullpath();
#if defined (_WINDOWS)
	WIN32_FIND_DATAA filedata();
#endif

protected:
	bool	firstFound;
	bool	firstNexted;

};
#endif // _FILEUTILS_H
