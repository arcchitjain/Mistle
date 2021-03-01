#ifndef _APRIORICROUPIER_H
#define _APRIORICROUPIER_H

#if defined (_WINDOWS)
	#include <Windows.h>
#endif
#include <isc/IscFile.h>

#if defined (__GNUC__)
	#include <isc/ItemSetCollection.h>
#endif

#include "../Croupier.h"

class AprioriMiner;

enum IscOrderType;
class CROUPIER_API AprioriCroupier : public Croupier
{
public:
	AprioriCroupier(Config *config);
	AprioriCroupier(const string &iscName);
	AprioriCroupier();
	virtual ~AprioriCroupier();

	/* --- Online Dealing (pass patterns in memory when possible) --- */
	virtual bool	CanDealOnline()					{ return true; }

	virtual ItemSetCollection* MinePatternsToMemory(Database *db);

	/* --- Offline Dealing (patterns to file) --- */
	virtual string	BuildOutputFilename();
	virtual bool	MinePatternsToFile(const string &dbFilename, const string &outputFullPath, Database *db = NULL);

	virtual string	GetPatternTypeStr();

protected:
	void			Configure(const string &iscName);
	virtual void	SetIscProperties(ItemSetCollection *isc);

	// Online
	virtual void	InitOnline(Database *db);
	virtual void	MinerIsErVolVanCBFunc(ItemSet **iss, uint32 numSets);
	virtual void	MinerIsErMooiKlaarMeeCBFunc(ItemSet **buf, uint32 numSets);

	// To file
	bool			MyMinePatternsToFile(Database *db, const string &outputFile);

	string		mType;
	IscOrderType mOrder;
	float		mMinSup;
	string		mMinSupMode;
	uint32		mMaxLen;

	AprioriMiner *mMiner;

	friend class AprioriNode;
	friend class AprioriTree;
};

#endif // _APRIORICROUPIER_H
