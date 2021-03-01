#ifndef _APRIORIMINER_H
#define _APRIORIMINER_H

#if defined (_WINDOWS)
	#include <Windows.h>
#endif

class Database;
class AprioriCroupier;
class AprioriTree;

class AprioriMiner
{
public:
	AprioriMiner(Database *db, const uint32 minSup, const uint32 maxLen);
	virtual ~AprioriMiner();

	void			BuildTree();
	void			ChopDownTree();

	void			MineOnline(AprioriCroupier *croupier, ItemSet **buffer, const uint32 bufferSize);

	// -------------- Get -------------
	uint32			GetMinSup()				{ return mMinSup; }
	uint32			GetMaxLen()				{ return mMaxLen; }
	uint32			GetActualMaxLen()		{ return mActualMaxLen; }

protected:

	// Member variables
	Database		*mDatabase;
	AprioriTree		*mTree;

	// Settings
	uint32			mMinSup;
	uint32			mMaxLen;

	// Stats maintained during mining
	uint32			mActualMaxLen;
};

#endif // _APRIORIMINER_H
