#ifndef __BM128ITEMSET_H
#define __BM128ITEMSET_H

#include "ItemSet.h"

class BASS_API BM128ItemSet : public ItemSet {
public:
	// Copy*-Constructor
	BM128ItemSet(const BM128ItemSet * is);
	// Copy*-Constructor
	BM128ItemSet(const BM128ItemSet * is, bool copyUsages);
	// Empty Set Constructor
	BM128ItemSet(uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	// Singleton Set Constructor
	BM128ItemSet(uint16 elem, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	// Defined Set Constructor
	BM128ItemSet(const uint16 *set, uint32 setlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);

	virtual ~BM128ItemSet();
	virtual void		CleanUpStaticMess();

	virtual BM128ItemSet*	Clone() const;
	virtual BM128ItemSet*	CloneLight() const;

	virtual uint32		GetLength();

	// Memory-Usage estimator for this BM128ItemSet
	virtual uint32		GetMemUsage() const ;
	// Generic Memory-Usage estimator for a BM128ItemSet
	static uint32		MemUsage();

	// returns ownership of an uint16 array with therein the values of the item set
	virtual uint16*		GetValues() const ;
	// returns values of the item set in the provided uint16 values array
	virtual void		GetValuesIn(uint16 *vals) const ;
	// returns values of the item set in the provided uint32 values array
	virtual void		GetValuesIn(uint32* vals) const ;
	// increment the values of the item set in the provided uint32 values array
	virtual void		IncrementValues(uint32* vals, int32 cnt) const { IncrementValues(vals, cnt, 0); };
	virtual void		IncrementValues(uint32* vals, int32 cnt, uint32* items) const ;

	// Keeps the bitcount as fresh as it was
	virtual void		AddItemToSet(uint16 val);
	// Keeps the bitcount as fresh as it was
	virtual void		RemoveItemFromSet(uint16 val);
	virtual bool		IsItemInSet(uint16 val) const;
	virtual const uint16 GetLastItem() const;

	// Fills the set with 1's from [o,max>
	virtual void		FillHerUp(uint16 max);
	// Fills the set with 0's from [o,max>
	virtual void		CleanTheSlate();

	virtual bool		Equals(ItemSet *is) const;
	bool				Equals(uint32 *msk) const;

	void				XOR(ItemSet *is);
	void				Difference(ItemSet *is);

	virtual bool		Intersects(ItemSet *is) const;
	bool				Intersects(uint32 *msk) const;

	uint32				IntersectionLength(uint32 *msk) const;

	virtual bool		IsSubset(ItemSet *is) const; // misschien een !virtual variant maken voor eigen coverset?
	bool				IsSubset(uint32 *msk) const;

	virtual void		Remove(ItemSet *is);

	virtual ItemSet*	Union(ItemSet *is) const;
	virtual ItemSet*	Intersection(ItemSet *is) const;

	virtual void		Unite(ItemSet *is);

	virtual void		Sort() {} // not applicable

	// Translate from Fimi to Fic
	virtual void		TranslateForward(ItemTranslator *it);
	// Translate from Fic to Fimi
	virtual void		TranslateBackward(ItemTranslator *it);

	virtual ItemSetType GetType() const { return BM128ItemSetType; }

	virtual string		ToString(bool printCount=true, bool printFreq=false);
	virtual string		ToCodeTableString();

	void __inline		Set(uint32 i) {
		uint32 idx = i >> 5;
		//uint32 bvx = 0x01 << (i % 32);	// orig, lsb-like
		uint32 bvx = 0x80000000 >> (i % 32);		// new, msb-like
		mMask[idx] = mMask[idx] | bvx;
		mBitCountFresh = false;
	}
	void				UnSet(uint32 i);


	uint32*				GetMask() const { return mMask; }
	void				CopyMaskFrom(uint32* mask, uint32 len, uint16 lastItem);

	bool				IsSubSubSet(uint32 *attr, uint32 *msk) const;

	// Compare item sets on alphabetical order. Returns 0 iff equal, -1 iff this<is, 1 iff this>is.
	virtual int8		CmpLexicographically(ItemSet *b);

protected:
	void				CopyFrom(BM128ItemSet *is);

	void				CalcLength();
	bool				GetBitCountFresh() const { return mBitCountFresh; }

	void 				Remove(uint32 *msk);
	void				RemoveSubset(ItemSet *is);
	void 				RemoveSubset(uint32 *msk, uint32 subsetlen);

	void 				UniteNonOverlappingSet(uint32 *msk, uint32 noverlaplen);

	uint32			*mMask;
	uint32			mBitCount;			// number of bits 1
	bool			mBitCountFresh;		// is mBitCount up-to-date?
	uint16			mLastItem;

	static uint32	*sValueDumper;

	static const unsigned char _count[256];
	static const uint32 _idxon[256][8];

	friend class BM128CoverSet;
};

#endif // __BM128ITEMSET_H
