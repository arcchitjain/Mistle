#ifndef __ITEMTRANSLATOR_H
#define __ITEMTRANSLATOR_H

#include "../BassApi.h"

class BASS_API ItemTranslator {
public:
	ItemTranslator(ItemTranslator *it);
	ItemTranslator(alphabet *a);
	ItemTranslator(uint16 *bitToVal, uint16 alphabetLen);
	~ItemTranslator();

	__inline uint16 ValueToBit(uint16 val)		{ return mValToBit->find(val)->second; }
	__inline uint16 BitToValue(uint16 bitpos)	{ return mBitToVal[bitpos]; }

	void WriteToFile(FILE *fp) const;
	string ToString() const; 

protected:
	uint16 mAlphabetLen;
	uint16 *mBitToVal;
	valBitMap *mValToBit;
};

#endif // __ITEMTRANSLATOR_H
