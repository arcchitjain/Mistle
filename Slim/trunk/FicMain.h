#ifndef __FICMAIN_H
#define __FICMAIN_H

class CodeTable;
class Config;
class Database;
class CodeTable;
class ItemSetCollection;

#include <isc/IscFile.h>
#include <itemstructs/ItemSet.h>

enum FisGenerateType {
	FisAll,
	FisClosed
};
struct CTFileInfo {
	uint32 minsup;
	string filename;
};

class FicMain {
public:
	FicMain(Config *config);
	~FicMain();

	/*-- Main execution --*/
	void	Execute();

	//////////////////////////////////////////////////////////////////////////
	// The New World
	//////////////////////////////////////////////////////////////////////////

	static void				Compress(Config *conf, const string tag = "");
	static void				DoCompress(Config *conf, Database *db, ItemSetCollection *isc, const string &tag, const string &timetag, const uint32 resumeSup, const uint64 resumeCand);
	static CodeTable*		CreateCodeTable(Database *db, Config *config, bool zapIsc = false, ItemSetCollection *isc = NULL);
	static CodeTable*		ProvideCodeTable(Database *db, Config *config, bool zapIsc = false);
	static void				MineAndWriteCodeTables(const string &path, Database *db, Config *config, bool zapIsc = false);

	// Tries to Retrieve the ItemSetCollection, else mines it and returns it. 
	// `beenMined` is a return value that indicates whether it has been mined or not
	// `storeMined` (optional, default false) sets whether if mined the set should be stored in the ICSRepos
	static ItemSetCollection* ProvideItemSetCollection(const string &iscTag, Database * const db, bool &beenMined, const bool storeMined = false, const bool loadAll = false, const IscFileType storeIscFileType = FicIscFileType, const IscFileType chunkIscFileType = FicIscFileType);

	static ItemSetCollection* MineItemSetCollection(const string &iscTag, Database * const db, const bool store = false, const bool loadAll = false, const IscFileType storeIscFileType = FicIscFileType, const IscFileType chunkIscFileType = FicIscFileType);

	static string			FindCodeTableFullDir(string ctDir);
	static CodeTable*		LoadCodeTableForMinSup(const Config *config, Database *db, const uint32 minsup, string ctDir = "");
	static CodeTable*		LoadFirstCodeTable(Database *db, string ctDir);
	static CodeTable*		LoadFirstCodeTable(const Config *config, Database *db, string ctDir = "");

	static CodeTable*		LoadLastCodeTable(Database *db, string ctDir);
	static CodeTable*		LoadLastCodeTable(const Config *config, Database *db, string ctDir = "");

	static CTFileInfo**		LoadCodeTableFileList(const Config *config, Database *db, uint32 &numCTs, string ctDir = "");
	static CTFileInfo**		LoadCodeTableFileList(const string &minsups, Database *db, uint32 &numCTs, string ctDir);

	static CodeTable*		RetrieveCodeTable(const string &tag, Database *db);
	static CodeTable*		RetrieveCodeTable(const Config* config, Database *db);

	static void				ParseCompressTag(const string &tag, string &iscName, string &pruneStrategy, string &timestamp);

protected:
	/*-- Member variables --*/
	Config	*mConfig;
	string	mTaskClass;

public:
	/*-- Static variables --*/
	static const string ficVersion;
};

#endif // __FICMAIN_H
