#ifdef BLOCK_LESS

#include "../../global.h"

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <itemstructs/BM128ItemSet.h>
#include <itemstructs/Uint16ItemSet.h>

#include "LowEntropySetInstantiation.h"
#include "LowEntropySet.h"

LowEntropySet::LowEntropySet() {
	mLength = 0;
	mAttributeDef = NULL;
	mInstantiations = NULL;

	mEntropy = 0.0;
	mStdLength = 0.0;
	mCount = 0;
	mPrevCount = 0;
	mNumInsts = 0;
}

LowEntropySet::LowEntropySet(float entropy/* =0 */, uint32 cnt/* =1 */, double stdlen /* =0 */) {
	mAttributeDef = NULL;
	mLength = 0;
	mInstantiations = NULL;
	mEntropy = entropy;
	mCount = cnt;
	mStdLength = stdlen;
	mNumInsts = 0;
}


LowEntropySet::LowEntropySet(ItemSet *attrDef, LowEntropySetInstantiation** insts, float entropy/* =0 */, uint32 cnt/* =1 */, double stdlen /* =0 */) {
	mAttributeDef = attrDef;
	mLength = mAttributeDef->GetLength();
	mInstantiations = insts;
	mEntropy = entropy;
	mCount = cnt;
	mStdLength = stdlen;
	mNumInsts = GetNumInstantiations();
}

LowEntropySet::~LowEntropySet() {
	if(mInstantiations != NULL) {
		for(uint32 i=0; i<mNumInsts; i++)
			delete mInstantiations[i];
	}
	delete[] mInstantiations;

	delete mAttributeDef;
}

void LowEntropySet::Print() {
	string str = mAttributeDef->ToString(false,false);
	printf("%s (%d)\n", str.c_str(), mCount);
}
string LowEntropySet::ToCodeTableString(bool hasBins) {
	string str = mAttributeDef->ToString(false,false);
	char buffer[100];
	str += " (";
	_itoa_s(GetCount(), buffer, 20, 10);
	str.append(buffer);
	str += " : ";
	for(uint32 i=0; i<mNumInsts; i++) {
		_itoa_s(mInstantiations[i]->GetItemSet()->GetUsageCount(), buffer, 20, 10);
		str.append(buffer);
		if(i+1 < mNumInsts)
			str += " ";
	}
	str += ")";
	return str;
}
void LowEntropySet::SetAttributeDefinition(ItemSet *attr) {
	 mAttributeDef = attr; 
	 mLength = mAttributeDef->GetLength();
}
void LowEntropySet::SetInstantiations(LowEntropySetInstantiation **insts) {
	mInstantiations = insts;
	mNumInsts = GetNumInstantiations();
}

void LowEntropySet::ResetCount() { 
	mPrevCount = mCount; 
	mCount = 0; 
	for(uint32 i=0; i<mNumInsts; i++)
		mInstantiations[i]->GetItemSet()->ResetUsage();
}
void LowEntropySet::BackupCount() { 
	mPrevCount = mCount; 
	for(uint32 i=0; i<mNumInsts; i++)
		mInstantiations[i]->GetItemSet()->BackupUsage();
}
void LowEntropySet::RollbackCount() { 
	mCount = mPrevCount; 
	for(uint32 i=0; i<mNumInsts; i++)
		mInstantiations[i]->GetItemSet()->RollbackUsage();
}
int8 LowEntropySet::Compare(LowEntropySet *b) {
	return mAttributeDef->Compare(b->GetAttributeDefinition());
}

uint32 LowEntropySet::MemUsage(const ItemSetType type, const uint32 abLen, const uint32 setLen) {
	return sizeof(LowEntropySet) + ((1 << (setLen-1))) * sizeof(Uint16ItemSet);
}
uint32 LowEntropySet::MemUsage(Database *db) {
	return sizeof(LowEntropySet) + ((1 << (db->GetMaxSetLength()-1))) * sizeof(Uint16ItemSet);
}

/*uint32 LowEntropySet::MemUsage() {
	return sizeof(LowEntropySet) + (2 + mNumInsts) * sizeof(BM128ItemSet);
}*/
#endif // BLOCK_LESS
