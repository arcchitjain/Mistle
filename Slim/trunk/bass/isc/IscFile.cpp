#include "../Bass.h"
#include "../db/Database.h"

#include <sys/stat.h>

/*
#if defined (__APPLE__) && defined (__MACH__)
#include <libio/libio.h>	
#endif
*/

#include <FileUtils.h>

#include "../db/Database.h"

#include "ItemSetCollection.h"

#include "FicIscFile.h"
#include "FicIscFileAsync.h"
#include "BinaryFicIscFile.h"
#include "BinaryFicIscFileAsync.h"
#include "FimiIscFile.h"

#include "IscFile.h"

IscFile::IscFile() {
	mFile = NULL;
	mFilename = "";

	mBuffer = NULL;									// Init'd in subclass with appropriate size

	mPreferredDataType = NoneItemSetType;

	mDatabase = NULL;
	mStdLengths = NULL;
}
IscFile::~IscFile() {
	CloseFile();
	delete[] mBuffer;
}
ItemSetCollection* IscFile::Open(const string &filename, Database * const db) {
	if(mFile != NULL)
		THROW("File already open.");

	mDatabase = db;
	mStdLengths = db->GetStdLengths();

	return Open(filename);
}
ItemSetCollection* IscFile::Read(const string &filename, Database * const db) {
	if(mFile != NULL)
		THROW("File already open.");

	mDatabase = db;
	mStdLengths = db->GetStdLengths();

	return Read(filename);
}

void IscFile::RewindFile() {
	if(mFile != NULL) {
		rewind(mFile);
		ReadFileHeader();
	}
}

void IscFile::OpenFile(const string &filename, const FileOpenType openType /* = FileReadable */) {
	if(mFile != NULL)
		THROW("Filehandle already open");

	if(fopen_s(&mFile, filename.c_str(), FileUtils::FileOpenTypeToString(openType).c_str()))
		THROW("Unable to open file");

	mFilename = filename;
}


void IscFile::CloseFile() {
	if(mFile != NULL)
		fclose(mFile);
	mFile = NULL;
	mFilename = "";
}

void IscFile::SetDatabase(Database *db) {
	 mDatabase = db;
	 mStdLengths = db->GetStdLengths();
}

bool IscFile::WriteItemSetCollection(ItemSetCollection * isc, const string &fullpath) {
	OpenFile(fullpath, FileWritable);

	uint64 numItemSets = isc->GetNumItemSets();
	if(!isc->HasMinSupport()) isc->DetermineMinSupport();
	if(!isc->HasMaxSetLength())	isc->DetermineMaxSetLength();
	WriteHeader(isc, numItemSets);

	// - Write ItemSets
	for(uint64 i=0; i<numItemSets; i++) {
		ItemSet *is = isc->GetNextItemSet();
		WriteItemSet(is);
		delete is;
	}

	return true;	
}


bool IscFile::IsOpen() {
	return mFile != NULL;
}

size_t IscFile::ReadLine(char* buffer, int size) {
	char* fgot = NULL;
	try{
		fgot = fgets(buffer, size, mFile);
	} catch (...) {
		THROW("Error while reading file");
	}
	if (fgot == NULL)
		THROW("Error while reading file");

	return strlen(buffer);
}

int64 IscFile::GetFileSize() { 
	if(mFile == NULL || ferror(mFile))
		THROW("Unable to retrieve file size - file error or not opened.");
#if defined(_WINDOWS)
	struct _stati64 file;
	_fstati64(mFile->_file, &file);
#elif (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
	struct stat file;
#if defined (__APPLE__) && defined (__MACH__)
	fstat(fileno(mFile), &file);
#else
	fstat(mFile->_fileno, &file);
#endif
#endif
	return file.st_size;
}

IscFile* IscFile::Create(const IscFileType type) {
	if(type == FimiIscFileType) {
		return new FimiIscFile();
	} else if(type == FicIscFileType) {
		return new FicIscFile();
	} else if(type == BinaryFicIscFileType) {
		return new BinaryFicIscFile();
	}
	THROW("That's a pretty weird ISC file type you got there, dude...");
}

IscFileType IscFile::StringToType(const string &filename) {
	size_t dotPos = filename.find_last_of('.');
	string extension = dotPos == string::npos ? filename : filename.substr(dotPos + 1);
	if(extension.compare("fi") == 0 || extension.compare("fimi") == 0 || extension.compare("dat") == 0)
		return FimiIscFileType;
	else if(extension.compare("isc") == 0 || extension.compare("isc-tmp") == 0)
	 	return FicIscFileType;
	else if(extension.compare("bisc") == 0 || extension.compare("bisc-tmp") == 0)
		return BinaryFicIscFileType;
	else 
		return NoneIscFileType;
	//THROW("That's a pretty weird IscFile extension you got there, dude...");
}
IscFileType IscFile::ExtToType(const string &extension) { 
	if(extension.compare("fi") == 0 || extension.compare("fimi") == 0 || extension.compare("dat") == 0)
		return FimiIscFileType;
	else if(extension.compare("isc") == 0)
		return FicIscFileType;
	else if(extension.compare("bisc") == 0)
		return BinaryFicIscFileType;
	else 
		return NoneIscFileType;
	//THROW("That's a pretty weird IscFile extension you got there, dude...");
}
string IscFile::TypeToExt(const IscFileType iscType) {
	if(iscType == FimiIscFileType)
		return "fi";
	else if(iscType == FicIscFileType)
		return "isc";
	else if(iscType == BinaryFicIscFileType)
		return "bisc";
	THROW("That's a pretty weird IscFileType you got there, dude...");
}

void IscFile::Delete() {
	if(mFile != NULL)
		fclose(mFile);
	mFile = NULL;
	remove( mFilename.c_str() );
	mFilename = "";
}
