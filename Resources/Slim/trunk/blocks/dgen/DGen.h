#ifndef __DGEN_H
#define __DGEN_H

class CodeTable;
class Database;
class CoverSet;
class Config;

class DGSet;

class DGen {
public:
	DGen();
	virtual ~DGen();

	virtual string GetShortName()=0;

	virtual void SetInputDatabase(Database *db);
	virtual void SetColumnDefinition(string coldefstr);
	virtual void SetColumnDefinition(ItemSet **coldef, uint32 numColumns);

	virtual void BuildModel(Database *db, Config *config, const string &ctfname)=0;
	// deze is als API veel mooier, maar nu geen zin om overal te implementeren:
	virtual void BuildModel(Database *db, Config *config, CodeTable *ct) { 
								throw string("niet standaard. succes met implementeren!"); }

	virtual Database* GenerateDatabase(uint32 numRows)=0;

	static DGen*	Create(string type);

	virtual DGSet**	GetCodingSets();
	virtual uint32	GetNumCodingSets();

protected:
	virtual void BuildCodingSets(const string &ctfname);
	virtual void BuildCodingSets(CodeTable *ct);

	Database*		mDB;
	ItemSetType		mDataType;
	uint32			mAlphabetLen;

	DGSet**			mCodingSets;
	uint32			mNumCodingSets;
	uint32			mCSABStart;

	ItemSet**		mColDef;
	uint32			mNumCols;
	uint32			*mItemToCol;
};

#endif // __DGEN_H
