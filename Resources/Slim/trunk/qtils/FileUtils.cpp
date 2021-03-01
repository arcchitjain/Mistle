#include "qlobal.h"

#if defined (_MSC_VER)
# include <windows.h>
# include <shlobj.h>
#elif defined (__GNUC__)
# if defined (__APPLE__) && defined (__MACH__)
#  include <sys/uio.h>
# else
#  include <sys/io.h>   // For access().
#  include <sys/sendfile.h>
# endif
# include <sys/types.h>  // For stat().
# include <sys/stat.h>   // For stat().
# include <fcntl.h>
# include <unistd.h> 
#endif

#include "FileUtils.h"

void FileUtils::SplitPathFromFilename(string &filename, string &path) {
	// - Determine whether `filename` contains a path
	size_t lastDirSep = filename.find_last_of("\\/");

	// - Determine `path`
	path = (path.length() == 0 && lastDirSep != string::npos) ? filename.substr(0,lastDirSep) : path;
	//  - Check whether path is valid (slash or backslash-ended)
	if(path.length() != 0 && path[path.length()-1] != '/' && path[path.length()-1] != '\\')
		path += '/';

	// - Determine `filename`
	filename = (lastDirSep != string::npos) ? filename.substr(lastDirSep+1) : filename;
}
string FileUtils::ExtractExtension(const string &filename) {
	return filename.substr(filename.find_last_of('.'));
}
string FileUtils::ExtractFileName(const string &filename) {
	return filename.substr(0,filename.find_last_of('.'));
}
string FileUtils::FileOpenTypeToString(FileOpenType p) {
	if(p == FileReadable)
		return "r";
	else if(p == FileBinaryReadable)
		return "rb";
	else if(p == FileBinaryWritable)
		return "wb";
	else if(p == FileWritable)
		return "w";
	else
		throw string(__FUNCTION__) + " - Unknown FileOpenType";
}
bool FileUtils::FileExists(const string &path) {
	FILE *fp;
	if(fopen_s(&fp, path.c_str(), "r") != 0)
		return false;
	fclose(fp);
	return true;
}

bool FileUtils::DirectoryExists(const string &path) {
#ifdef WIN32	
	uint32 attributes = ::GetFileAttributesA( path.c_str() );
	if ( attributes == 0xFFFFFFFF )
		return false;
	//	throw string("Error determining whether directory exists.");	
	return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	if(access(path.c_str(), 0) == 0) {
		struct stat status;
		stat(path.c_str(), &status);
		if (status.st_mode & S_IFDIR) {
			return true; // directory
		} else {
			return false; // file
		}
	} else {
		return false;
	}
#endif
}
bool FileUtils::IsDirectory(const string &path) {
	return DirectoryExists(path);
}

bool FileUtils::Exists(const string &path) {
#ifdef WIN32	
	if(::GetFileAttributesA( path.c_str() ) == 0xFFFFFFFF) {
		uint32 err = ::GetLastError();
		if((err == ERROR_FILE_NOT_FOUND)
			|| (err == ERROR_INVALID_PARAMETER)
			|| (err == ERROR_NOT_READY)
			|| (err == ERROR_PATH_NOT_FOUND)
			|| (err == ERROR_INVALID_NAME)
			|| (err == ERROR_BAD_NETPATH ))
			return false; 
		return true;
	}
	return true;
#else
	if(access(path.c_str(), 0) == 0) {
		struct stat status;
		stat(path.c_str(), &status);
		if (status.st_mode & S_IFDIR) {
			return true; // directory
		} else {
			return true; // file
		}
	} else {
		return false;
	}
#endif
}

bool FileUtils::CreateDir(const string &path) {
#ifdef WIN32
	return SHCreateDirectoryExA(NULL, path.c_str(), NULL) == ERROR_SUCCESS ? true : false;
#else
	return mkdir(path.c_str(),0777) == 0;
#endif
}
bool FileUtils::CreatePath(const string &path) {
#ifdef WIN32
	return SHCreateDirectoryExA(NULL, path.c_str(), NULL) == ERROR_SUCCESS ? true : false;
#else
	return mkpath(path.c_str(),0777) == 0;
#endif
}
bool FileUtils::CreateDirIfNotExists(const string &path) {
	if(!DirectoryExists(path))
		return CreateDir(path);
	return true;
}
bool FileUtils::CreatePathIfNotExists(const string &path) {
	if(path.length() > 1 && !DirectoryExists(path)) {
		FileUtils::CreatePathIfNotExists(path.substr(0,path.find_last_of('/',path.length()-2)));
		return CreateDir(path);
	}
	return true;
}
bool FileUtils::RemoveDir(const string &path) {
#ifdef WIN32
	return RemoveDirectoryA(path.c_str()) ? true : false;
#else
	return rmdir(path.c_str())==0;
#endif
}
bool FileUtils::RemoveFile(const string &path) {
#ifdef WIN32
	return DeleteFileA(path.c_str()) ? true : false;
#else
	return remove(path.c_str())==0;
#endif
}
bool FileUtils::FileMove(const string &fromPath, const string &toPath) {
#ifdef WIN32
	return MoveFileA(fromPath.c_str(), toPath.c_str()) ? true : false;
#elif defined (__GNUC__) && defined (__unix__)
	if (rename(fromPath.c_str(), toPath.c_str()))
		if (errno == EXDEV) { // EXDEV	oldpath and newpath are not on the same mounted filesystem. (Linux permits a filesystem to be mounted at multiple points, but rename(2) does not work across different mount points, even if the same filesystem is mounted on both.)
			// See: Exploring The sendfile System Call LG #91
			int src;               /* file descriptor for source file */
			int dest;              /* file descriptor for destination file */
			struct stat stat_buf;  /* hold information about input file */
			off_t offset = 0;      /* byte offset used by sendfile */
			int rc;                /* return code from sendfile */

			/* check that source file exists and can be opened */
			src = open(fromPath.c_str(), O_RDONLY);
			if (src == -1)
				throw string("Unable to open '" + fromPath + "': " + string(strerror(errno)) + "\n");

			/* get size and permissions of the source file */
			fstat(src, &stat_buf);

			/* open destination file */
			dest = open(toPath.c_str(), O_WRONLY|O_CREAT, stat_buf.st_mode);
			if (dest == -1)
				throw string("Unable to open '" + toPath + "': " + string(strerror(errno)) + "\n");

			/* copy file using sendfile */
			rc = sendfile (dest, src, &offset, stat_buf.st_size);
			if (rc == -1)
				throw string("Error from sendfile: " + string(strerror(errno)) + "\n");
			if (rc != stat_buf.st_size) {
				char buffer[20];
				throw string("Incomplete transfer from sendfile: " + string(itoa(rc, buffer, 10)) + " of " + string(itoa((int)stat_buf.st_size, buffer, 10)) + " bytes\n");
			}

			/* clean up and exit */
			close(dest);
			close(src);

			/* remove source file */
			rc = remove(fromPath.c_str());
			if (rc == -1)
				throw string("Unable to remove '" + fromPath + "': " + string(strerror(errno)) + "\n");

			return true;
		}
		else
			throw string("Error renaming file: " + string(strerror(errno)) + "\n");
	else
	    return true;
#endif
}


string FileUtils::GetCurDirectory() {
	char buf[512];
#ifdef WIN32
	if(GetCurrentDirectoryA(512,buf) > 0)
#else
	if(getcwd(buf,512) == buf)
#endif
		return string(buf) + "/";
	else
		throw string("Unable to figure out the current directory!");
}

uint32 FileUtils::CountFiles(const string &path, const string &query) {
#ifdef WIN32
	HANDLE				handle;
	WIN32_FIND_DATAA	fileData;

	string p = path + (path.find_last_of("\\/") != path.length()-1 ? "/" : "");
	if(!FileUtils::DirectoryExists(p))
		throw string("FileUtils::CountFiles() -- path does not exist.");
	handle = FindFirstFileA((p + query).c_str(), &fileData);
	if(handle == INVALID_HANDLE_VALUE)
		return 0;
	uint32 cnt = 1;
	while(FindNextFileA(handle, &fileData) != 0)
		++cnt;
	return cnt;
#else
	directory_iterator dir_iter = directory_iterator(path, query);
	uint32 cnt = 0;
	while (dir_iter.next())
		++cnt;
	return cnt;
#endif
}

#ifdef WIN32
// directory iterator
directory_iterator::directory_iterator(const string &path, const string &query) {
	entry_path = path + (path.find_last_of("\\/") != path.length()-1 ? "/" : "");

	firstFound = false;
	firstNexted = false;

	if(!FileUtils::DirectoryExists(entry_path))
		throw string("directory_iterator :: path does not exist");

	handle = FindFirstFileA((entry_path + query).c_str(), &FoundFileData);
	if (handle == INVALID_HANDLE_VALUE) {
		uint32 dwError = GetLastError();
		if(dwError != ERROR_FILE_NOT_FOUND)
			throw string("directory_iterator :: invalid handle value");
		else
			firstFound = false;
	} else	// alleen als er een file gevonden is
		firstFound = true;
}
directory_iterator::directory_iterator() {
	entry_path = "";
	firstFound = false;
	firstNexted = false;
	handle = INVALID_HANDLE_VALUE;
}
directory_iterator::~directory_iterator() {
	if ( handle != INVALID_HANDLE_VALUE ) FindClose( handle );
}
bool directory_iterator::equals(const directory_iterator &b) {
	return (handle == b.handle);
}
bool directory_iterator::next() {
	if(firstFound == false) {			// FindFirstFile returned ERROR_NO_MORE_FILES
		return false;
	} else if(firstNexted == false) {	// FindFirstFile returned a valid FileData
		firstNexted = true;
		return true;
	}
	return FindNextFileA(handle, &FoundFileData) != 0;
}
string directory_iterator::filename() {
	return FoundFileData.cFileName;
}
string directory_iterator::fullpath() {
	return entry_path + FoundFileData.cFileName;

}
WIN32_FIND_DATAA directory_iterator::filedata() {
	return FoundFileData;
}

/*
{
string path;
dirit it = dirit(path);
while(it.has_more()) {
string filename = it.getFileName();
}
}





//  directory_iterator implementation  ---------------------------------------//

void dir_itr_init( dir_itr_imp_ptr & m_imp, const path & dir_path ) {
	m_imp.reset( new dir_itr_imp );
	BOOST_SYSTEM_DIRECTORY_TYPE scratch;
	const char * name = 0;  // initialization quiets compiler warnings
	if ( dir_path.empty() )
		m_imp->handle = BOOST_INVALID_HANDLE_VALUE;
	else
	{
		name = find_first_file( dir_path.native_directory_string().c_str(),
			m_imp->handle, scratch );  // sets handle
		if ( m_imp->handle == BOOST_INVALID_HANDLE_VALUE
			&& name ) // eof
		{
			m_imp.reset(); // make end iterator
			return;
		}
	}
	if ( m_imp->handle != BOOST_INVALID_HANDLE_VALUE )
	{
		m_imp->entry_path = dir_path;
		// append name, except ignore "." or ".."
		if ( !dot_or_dot_dot( name ) )
		{ 
			m_imp->entry_path.m_path_append( name, no_check );
		}
		else
		{
			m_imp->entry_path.m_path_append( "dummy", no_check );
			dir_itr_increment( m_imp );
		}
	}
	else
	{
		boost::throw_exception( filesystem_error(  
			"boost::filesystem::directory_iterator constructor",
			dir_path, fs::detail::system_error_code() ) );
	}  
}

string& dir_itr_dereference(	const dir_itr_imp_ptr & m_imp ) {
	assert( m_imp.get() ); // fails if dereference end iterator
	return m_imp->entry_path;
}

void dir_itr_increment( dir_itr_imp_ptr & m_imp ) {
	assert( m_imp.get() ); // fails on increment end iterator
	assert( m_imp->handle != BOOST_INVALID_HANDLE_VALUE ); // reality check

	BOOST_SYSTEM_DIRECTORY_TYPE scratch;
	const char * name;

	while ( (name = find_next_file( m_imp->handle,
		m_imp->entry_path, scratch )) != 0 )
	{
		// append name, except ignore "." or ".."
		if ( !dot_or_dot_dot( name ) )
		{
			m_imp->entry_path.m_replace_leaf( name );
			return;
		}
	}
	m_imp.reset(); // make base() the end iterator
}
	}
}
*/
#endif // end of WIN32-only directory_iterator

#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))

directory_iterator::directory_iterator(const string &path, const string &query)
	: dir(NULL), direntry(NULL)
{
	entry_path = path + (path.find_last_of("\\/") != path.length()-1 ? "/" : "");

	// Turn query into a POSIX regex
	string regex(query);
	int pos = regex.find('*');
	while (pos != string::npos) {
		regex.replace(pos, 1, ".*" );
		pos = regex.find('*', pos + 2);
	} 
	regex = '^' + regex + '$'; // Match the whole filename

	if (regcomp(&preg, regex.c_str(), 0))
		throw string("directory_iterator :: failed compiling regular expresion");

	if(!FileUtils::DirectoryExists(entry_path))
		throw string("directory_iterator :: path does not exist");

	dir = opendir(entry_path.c_str());
	if (dir == NULL)
		throw string("directory_iterator :: unable to open the directory (" + entry_path + ")");

}

directory_iterator::~directory_iterator() {
	if (dir) closedir(dir);
	regfree(&preg);
}

bool directory_iterator::next() {
	while ((direntry = readdir(dir)) != NULL)
		if (regexec(&preg, direntry->d_name, 0, NULL, 0) == 0)
			return true;
	return false;
}

string directory_iterator::filename() {
	return string(direntry->d_name);
}

string directory_iterator::fullpath() {
	return entry_path + string(direntry->d_name);
}

#endif // (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
