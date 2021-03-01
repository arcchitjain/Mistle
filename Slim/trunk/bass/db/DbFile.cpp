#include "../Bass.h"

#include "FicDbFile.h"
#include "FimiDbFile.h"
#include "../itemstructs/ItemSet.h"

#include "DbFile.h"

DbFile::DbFile() {
	mPreferredDataType = Uint16ItemSetType;
}
DbFile::~DbFile() {

}

/*
string DbFile::GetFileName() {
	return mFilename;
}
*/

DbFileType DbFile::StringToType(const string &desc) {
	if(desc.compare("fimi") == 0)
		return DbFileTypeFimi;
	if(desc.compare("fic") == 0)
		return DbFileTypeFic;
	return DbFileTypeNone;
}

DbFileType DbFile::ExtToType(const string &extension) {
	if(extension.compare("dat") == 0)
		return DbFileTypeFimi;
	if(extension.compare("db") == 0)
		return DbFileTypeFic;
	return DbFileTypeNone;
}

string DbFile::TypeToExt(const DbFileType dbType) {
	if(dbType == DbFileTypeFimi)
		return "dat";
	if(dbType == DbFileTypeFic)
		return "db";
	return "unknown";
}

DbFile *DbFile::Create(DbFileType type) {
	if(type == DbFileTypeFimi)
		return new FimiDbFile();
	if(type == DbFileTypeFic)
		return new FicDbFile();
	return NULL;
}
