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

