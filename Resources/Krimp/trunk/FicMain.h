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
