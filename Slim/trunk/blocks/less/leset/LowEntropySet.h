#ifndef __LOWENTROPYSET_H
#define __LOWENTROPYSET_H

#include <Primitives.h>
//#include <itemstructs/ItemSet.h>

class ItemSet;
class LowEntropySetInstantiation;

class LowEntropySet {
public:
	LowEntropySet();
	LowEntropySet(float entropy, uint32 cnt=1, double stdlen=0);
	LowEntropySet(ItemSet *attrDef, LowEntropySetInstantiation** insts, float entropy=0, uint32 cnt=1, double stdlen=0);

	virtual ~LowEntropySet();
	//virtual LowEntropySet*	Clone() const;

	// How many attributes does this LE-set define?
	virtual uint32		GetLength()	{ return mLength; }
	// Returns the attributes this LE-set defines as an ItemSet
	ItemSet*			GetAttributeDefinition() { return mAttributeDef; }
	void				SetAttributeDefinition(ItemSet *attr);

	// How many instantiations does this LE-set define?
	uint32				GetNumInstantiations() const { return mLength > 0 ? (1 << mLength) : 0; }
	// Returns an ItemSet for which the attributes '1' according to the provided instantiation Id
	LowEntropySetInstantiation*		GetInstantiation(uint32 instId) { return mInstantiations[instId]; }
	// Returns an array of length GetNumInstantiations(), of all instantiation item sets
	LowEntropySetInstantiation**	GetInstantiations() { return mInstantiations; }

	void				SetInstantiations(LowEntropySetInstantiation **insts);

	// What is the entropy between the attributes?
	float				GetEntropy() const	{ return mEntropy; }
	void				SetEntropy(float entropy) { mEntropy = entropy; }

	double __inline		GetStandardLength() const { return mStdLength; }

	uint32				GetCount() const { return mCount; }
	uint32				GetPrevCount() const{ return mPrevCount; }
	void				SetPrevCount(uint32 cnt) { mPrevCount = cnt; }
	bool				CountChanged() const		{ return mCount != mPrevCount; }
	int32				GetCountChange() const	{ return mCount - mPrevCount; }
	bool __inline		Unused() const		{ return mCount == 0 && mPrevCount == 0; }
	bool __inline		IsCountZero() const	{ return mCount == 0; }
	void __inline		DoNothing() const { 	}
	void __inline		SetCount(uint32 c)	{ mCount = c; }
	void __inline		AddCount(uint32 c)	{ mCount += c; }
	void __inline		SupCount(uint32 c)	{ mCount -= c; }
	void __inline		AddOneToCount()		{ ++mCount; }
	void __inline		SupOneFromCount()	{ --mCount; }
	void ResetCount();
	void BackupCount();
	void RollbackCount();

	virtual	int8		Compare(LowEntropySet *b);

	//static LowEntropySet*	Create(ItemSet* attrdef, ItemSet** insts, double ent=0, uint32 cnt=1);
	//static LowEntropySet*	CreateSingleton(ItemSetType type, uint16 elem, uint32 abetlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);

	// Calculates the approximate memory usage of an ItemSet of `type` with an alphabet of `abLen` and `setLen` items
	static uint32		MemUsage(const ItemSetType type, const uint32 abLen, const uint32 setLen);
	// Calculates the approximate memory usage of a pattern (ItemSet) based on the given database
	static uint32		MemUsage(Database *db);

	void				Print();
	string				ToCodeTableString(bool hasBins = false);

protected:
	uint32	mLength;
	uint32	mNumInsts;
	float	mEntropy;
	uint32	mCount;
	uint32	mFreq;
	uint32	mPrevCount;
	double	mStdLength;

	ItemSet*	mAttributeDef;
	LowEntropySetInstantiation**	mInstantiations;
};

typedef std::list<LowEntropySet*> leslist;

#endif // __LOWENTROPYSET_H
