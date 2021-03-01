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
