#ifndef __BAI32ITEMSET_H
#define __BAI32ITEMSET_H

#include "ItemSet.h"

class BASS_API BAI32ItemSet : public ItemSet {
public:
	// Copy*-Constructor
	BAI32ItemSet(const BAI32ItemSet* is);
	// Empty Set Constructor
	BAI32ItemSet(uint32 abetlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	// Singleton Set Constructor
	BAI32ItemSet(uint16 elem, uint32 abetlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);
	// Defined Set Constructor
	BAI32ItemSet(const uint16 *set, uint32 setlen, uint32 abetlen, uint32 cnt=1, uint32 freq=1, uint32 numOcc=0, double stdLen=0, uint32 *occurrences=NULL);

	virtual ~BAI32ItemSet();
	virtual void		CleanUpStaticMess();

	virtual BAI32ItemSet*	Clone() const;
	void				CopyFrom(BAI32ItemSet *is);

	virtual uint32		GetLength();

	// Memory-Usage estimator for this BAI32ItemSet
	virtual uint32		GetMemUsage() const;
	// Generic Memory-Usage estimator for a BAI32ItemSet for an alphabet of length `abLen`
	static uint32		MemUsage(const uint32 abLen);

	// Bitwise dingen
	uint32				GetMaskLenBits() const { return mMaskLenBits; }
	uint32				GetMaskLenDWord() const { return mMaskLenDWord; }

	void __inline		Set(uint32 i) {
		uint32 idx = i >> 5;
		uint32 bvx = 0x01 << (i % 32);
		mMask[idx] = mMask[idx] | bvx;
		mBitCountFresh = false;
	}

	void				UnSet(uint32 i);

	bool				Get(uint32 i) const;
	void				Not();

	void				Or(BAI32ItemSet *ba);
	void __inline		Or(uint32 *ar);

	void				And(BAI32ItemSet *ba);
	void __inline		And(uint32 *ar);

	// returns ownership of an uint16 array with therein the values of the item set
	virtual uint16*		GetValues() const;
	// returns values of the item set in the provided uint16 values array
	virtual void		GetValuesIn(uint16 *vals) const;
	// returns values of the item set in the provided uint32 values array
	virtual void		GetValuesIn(uint32* vals) const;
	// increment the values of the item set in the provided uint32 values array
	virtual void		IncrementValues(uint32* vals, int32 cnt) const { IncrementValues(vals, cnt, 0); };
	virtual void		IncrementValues(uint32* vals, int32 cnt, uint32* items) const;

	virtual bool		IsItemInSet(uint16 val) const;
	virtual uint16 const GetLastItem() const;

	// Fills the set with 1's from [o,max>
	virtual void		FillHerUp(uint16 max);
	// Empties the set
	virtual void		CleanTheSlate();

	// Keeps the bitcount as fresh as it was
	virtual void		AddItemToSet(uint16 val);
	// Keeps the bitcount as fresh as it was
	virtual void		RemoveItemFromSet(uint16 val);

	virtual bool		Equals(ItemSet *is) const;
	bool				Equals(uint32 *ar) const;

	virtual bool		Intersects(ItemSet *is) const;
	bool				Intersects(uint32 *ar) const;
	uint32				IntersectionLength(uint32 *msk) const;

	virtual bool		IsSubset(ItemSet *is) const; // misschien een !virtual variant maken voor eigen coverset?
	bool				IsSubset(uint32 *ar) const;

	virtual void		Remove(ItemSet *is);
	void				Remove(uint32 *ar);
	void				RemoveSubset(ItemSet *is);
	void				RemoveSubset(uint32 *ar, uint32 subsetlen);

	virtual ItemSet*	Union(ItemSet *is) const;
	virtual ItemSet*	Intersection(ItemSet *is) const;

	virtual void		Unite(ItemSet *is);

	virtual void		Sort() { } // not applicable

	// Translate from Fimi to Fic
	virtual void		TranslateForward(ItemTranslator *it);
	// Translate from Fic to Fimi
	virtual void		TranslateBackward(ItemTranslator *it);

	virtual ItemSetType GetType() const { return BAI32ItemSetType; }

	virtual string		ToString(bool printCount=true, bool printFreq=false);
	virtual string		ToCodeTableString();

	uint32*				GetMask() const { return mMask; }
	void				CopyMaskFrom(uint32* mask, uint32 len, uint16 lastItem);

	// Compare item sets on alphabetical order. Returns 0 iff equal, -1 iff this<is, 1 iff this>is.
	virtual int8		CmpLexicographically(ItemSet *b);

protected:
	void				CalcLength();
	bool				GetBitCountFresh() const { return mBitCountFresh; }


	void 				UniteNonOverlappingSet(uint32 *msk, uint32 noverlaplen);

	uint32			*mMask;
	uint32			mMaskLenBits;			// array length in bits
	uint32			mMaskLenDWord;		// array length in dwords
	mutable uint32			mBitCount;			// number of bits 1
	mutable bool			mBitCountFresh;		// is mBitCount up-to-date?
	uint16			mLastItem;

	static const unsigned char _count[256];
	static const uint32 _idxon[256][8];

	static uint32	*sValueDumper;

	friend class BAI32CoverSet;
};

#endif // __BAI32ITEMSET_H
