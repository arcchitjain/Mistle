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
#ifndef __ITEMSET_H
#define __ITEMSET_H

// For InterlockedIncrement and InterlockedDecrement
#if defined (_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

//////////////////////////////////////////////////////////////////////////
// ItemSet is used for three different purposes:
//
//	[Function]				[Support]				[Usage Count]				[numOcc]
//	Database row			-						binSize (#transactions)		-
//	Candidate itemset		support in db			-							number of physical db rows in which itemset occurs (numOcc)
//	Codetable element		support in db			usage count in encoding		number of physical db rows in which itemset occurs (numOcc)
//
//////////////////////////////////////////////////////////////////////////


#include "../BassApi.h"

using namespace std;

enum ItemSetType {
	NoneItemSetType = 0,
	Uint16ItemSetType,
	BAI32ItemSetType,
	BM128ItemSetType
};

enum ItemSetOrder {
	ItemSetOrderNone = 0,
};

enum IscOrderType {
	NoneIscOrder=0,			// 0	u
	UnknownIscOrder,		// 1	u
	LengthDescIscOrder,		// 2	d
	LengthAscIscOrder,		// 3	a
	AreaLengthIscOrder,		// 4	al
	AreaSupportIscOrder,	// 5	as
	AreaSquareIscOrder,		// 6	aq
	RandomIscOrder,			// 7	r
	LengthAFreqDIscOrder,	// 8	z
	LengthDFreqDIscOrder,	// 9	x
	LengthAFreqAIscOrder,	// 10	q
	JTestIscOrder,			// 11	j
	JTestRevIscOrder,		// 12	jr
	LexAscIscOrder,			// 13	la
	LexDescIscOrder,		// 14	ld
	IdAscLexAscIscOrder,	// 15
	UsageDescIscOrder,		// 16
};

class Database;
class ItemTranslator;
class ItemSet;

typedef std::list<ItemSet*> islist;
typedef int8 (ItemSet::*ItemSetCompFunc)(const ItemSet *b) const;

class BASS_API ItemSet {
public:
	ItemSet();
	ItemSet(const ItemSet& is);
	virtual ~ItemSet();
	virtual void		CleanUpStaticMess() {};
	virtual ItemSet*	Clone() const =0;

	__inline bool operator > (const ItemSet *b) const {
		return (this->*mCompFunc)(b) == 1;
	}
	__inline bool operator < (const ItemSet *b) const {
		return (this->*mCompFunc)(b) == -1;
	}
	__inline bool operator == (const ItemSet *b) const {
		return (this->*mCompFunc)(b) == 0;
	}

	// Calculates (approximately) how much memory the ItemSet uses
	virtual uint32		GetMemUsage() const =0;

	// GetLength is not const as the bitmap-implementations may require a recount
	virtual uint32		GetLength() =0;
	uint32				GetUsageCount() const		{ return mUsageCount; }
	uint32				GetSupport() const			{ return mSupport; }
	void				SetSupport(const uint32 f)	{ mSupport = f; }
	uint32				GetPrevUsageCount() const	{ return mPrevUsageCount; }
	void				SetPrevUsageCount(uint32 cnt) { mPrevUsageCount = cnt; }
	bool				Changed() const				{ return mUsageCount != mPrevUsageCount; }
	int32				GetChange() const			{ return mUsageCount - mPrevUsageCount; }
	bool __inline		Unused() const				{ return mUsageCount == 0 && mPrevUsageCount == 0; }
	bool __inline		IsUsageCountZero() const	{ return mUsageCount == 0; }
	void __inline		DoNothing() const			{ 	}
	void __inline		SetUsageCount(uint32 c)		{ mUsageCount = c; }
	void __inline		AddUsageCount(uint32 c)		{ mUsageCount += c; }
	void __inline		SupUsageCount(uint32 c)		{ mUsageCount -= c; }
	void __inline		AddOneToUsageCount()		{ ++mUsageCount; }
	void __inline		SupOneFromUsageCount()		{ --mUsageCount; }
	void __inline		ResetUsage()				{ mPrevUsageCount = mUsageCount; mUsageCount = 0; 
													  mPrevUsageArea = mUsageArea; mUsageArea = 0; }
	void __inline		BackupUsage()				{ mPrevUsageCount = mUsageCount; 
													  mPrevUsageArea = mUsageArea; }
	void __inline		RollbackUsage()				{ mUsageCount = mPrevUsageCount; 
													  mUsageArea = mPrevUsageArea; }

	// Counter for how much or how often (part) of this set is used during 
	//  covering for instance for area-based covering. (optional!)
	uint32				GetUsageArea() const { return mUsageArea; }
	void __inline		SetUsageArea(uint32 c) {mUsageArea = c;}
	void __inline		AddUsagerea(uint32 c) {mUsageArea += c;}
	void __inline		SupUsageArea(uint32 c) {mUsageArea -= c;}

	virtual void		AddItemToSet(uint16 val) { throw "Not implemented for this type of itemset."; }
	virtual void		RemoveItemFromSet(uint16 val) { throw "Not implemented for this type of itemset."; }
	virtual bool		IsItemInSet(uint16 val) const =0;
	// Returns the last item of the set this ItemSet was initialised with. 
	// Only use if length>0, as that goes unchecked here.
	virtual const uint16	GetLastItem() const =0;

	// Fills the set with 0's from [o,max>
	virtual void		FillHerUp(uint16 max)=0;
	virtual void		CleanTheSlate()=0;

	// returns the anwser whether 'a' and 'b' contain exactly the same items.
	virtual bool		Equals(ItemSet *is) const =0;

	/* returns the answer to the question whether 'b' intersects with 'a'.
		a->Intersects(b), a = {1,2}, b = {1} -> true
		a->Intersects(b), a = {2}, b = {1} -> false
	*/
	virtual bool		Intersects(ItemSet *is) const =0;

	/* returns the answer to the question whether 'b' is a subset of 'a'.
		a->IsSubset(b), a = {1,2}, b = {1} -> true
		a->IsSubset(b), a = {1}, b = {1,2} -> false
	*/
	virtual bool		IsSubset(ItemSet *is) const =0;

	// zaps the items of is from this
	virtual void		Remove(ItemSet *is) =0;

	virtual ItemSet*	Remainder(ItemSet *is) const;
	virtual ItemSet*	Union(ItemSet *is) const =0;
	virtual ItemSet*	Intersection(ItemSet *is) const =0;

	// unites items of is to this
	virtual void		Unite(ItemSet *is) =0;

	virtual void		Sort() =0;	// may have empty implementation when not applicable for datatype

	// Translate from Fimi to Fic
	virtual void		TranslateForward(ItemTranslator *it) =0;
	// Translate from Fic to Fimi
	virtual void		TranslateBackward(ItemTranslator *it) =0;

	// returns ownership of an uint16 array with therein the values of the item set
	virtual uint16*		GetValues() const =0;
	// returns values of the item set in the provided uint16 values array
	virtual void		GetValuesIn(uint16 *vals) const =0;
	// returns values of the item set in the provided uint32 values array
	virtual void		GetValuesIn(uint32* vals) const =0;
	// increment the values of the item set in the provided uint32 values array
	virtual void		IncrementValues(uint32* vals, int32 cnt) const =0;

	double __inline		GetStandardLength() const { return mStdLength; }
	void				SetStandardLength(double stdLen) { mStdLength = stdLen; }

	void				SetDBOccurrence(uint32 *dbRows);
	uint32*				GetDBOccurrence();
	bool				HasDBOccurrence() { return mDBOccurrence != NULL; }
	uint32				GetNumDBOccurs();
	void				SetNumDBOccurs(uint32 num) { mNumDBOccurs = num; }
	// pass-thru function for db->CalcDBOcc(...)
	void				DetermineDBOccurence(Database *db, bool calcNumOcc = true);

	virtual string		ToString(bool printCount=true, bool printFreq=false) =0;
	virtual string		ToCodeTableString() =0;

	virtual ItemSetType GetType() const =0;

	uint64				GetID() { return mID; }
	void				SetID(uint64 id) { mID = id; }

	inline void			Ref();
	inline void			UnRef();

	static ItemSet*		Create(ItemSetType type, const uint16 *set, uint32 setlen, uint32 abetlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	static ItemSet*		CreateSingleton(ItemSetType type, uint16 elem, uint32 abetlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	//static ItemSet*		CreateEmpty(ItemSetType type, uint32 abetlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	static ItemSet*		CreateEmpty(ItemSetType type, size_t abetlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);

	// Calculates the approximate memory usage of an ItemSet of `type` with an alphabet of `abLen` and `setLen` items
	static uint32		MemUsage(const ItemSetType type, const uint32 abLen, const uint32 setLen);
	// Calculates the approximate memory usage of a pattern (ItemSet) based on the given database
	static uint32		MemUsage(Database *db);

	//////////////////////////////////////////////////////////////////////////
	// ItemSetType Functions
	//////////////////////////////////////////////////////////////////////////
	static ItemSetType	StringToType(const string desc);
	static string		TypeToString(const ItemSetType type);
	static string		TypeToFullName(const ItemSetType type);

	// ItemSetOrderStuff
	//static void			SortItemSetArray(ItemSet **isarr, uint32 len, ItemSetOrder io);

	//////////////////////////////////////////////////////////////////////////
	// ItemSet Container Conversion
	//////////////////////////////////////////////////////////////////////////
	static ItemSet**	ConvertItemSetListToArray(islist *isl);
	
	static void			SortItemSetArray(ItemSet **itemSetArray, uint32 numItemSets, IscOrderType desiredOrder);


	// Compare item sets on alphabetical order. Returns 0 iff equal, -1 iff this<is, 1 iff this>is.
	// not const as for the bitmap-implementations a recount may be required
	virtual int8		CmpLexicographically(ItemSet *b)=0;

protected:
	virtual int8		CmpSupport(const ItemSet *b) const;
	virtual int8		CmpArea(ItemSet *b);					// not const ivm GetLength :-(

	ItemSetCompFunc		mCompFunc;

	uint64  mID;
	uint32	mUsageCount;	// set usage count, e.g. for row-based covering
	uint32	mUsageArea;		// set usage area, for instance for area-based covering
	uint32	mSupport;
	uint32	mPrevUsageCount;// set usage count, e.g. for row-based covering
	uint32	mPrevUsageArea;	// set usage area, for instance for area-based covering
	double	mStdLength;
	uint32	*mDBOccurrence;
	uint32	mNumDBOccurs;
	LONG	mRefCount;
};


void ItemSet::Ref() {
#if defined (_WINDOWS)
	InterlockedIncrement(&mRefCount);
#elif defined (__unix__)
	__sync_add_and_fetch (&mRefCount, 1);
#endif
}

void ItemSet::UnRef() {
#if defined (_WINDOWS)
	if(InterlockedDecrement(&mRefCount) == 0) {
		delete this;
	}
#elif defined (__unix__)
	if(__sync_add_and_fetch (&mRefCount, -1) == 0) {
		delete this;
	}
#endif
}


#endif // __ITEMSET_H
