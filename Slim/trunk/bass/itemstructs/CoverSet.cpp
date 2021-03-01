#include "../Bass.h"

#include "../db/Database.h"

#include "CoverSet.h"
#include "Uint16CoverSet.h"
#include "BAI32CoverSet.h"
#include "BM128CoverSet.h"

CoverSet::CoverSet() {
	mNumUncovered = 0;
}

CoverSet::~CoverSet() {

}

CoverSet* CoverSet::Create(ItemSetType type, size_t abetlen, ItemSet *initwith, uint32 numipoints) { 
	if(type == Uint16ItemSetType)
		return new Uint16CoverSet(initwith, numipoints);
	else if(type == BAI32ItemSetType)
		return new BAI32CoverSet((uint32) abetlen, initwith, numipoints);
	else if(type == BM128ItemSetType)
		return new BM128CoverSet((uint32) abetlen, initwith, numipoints);
	else throw string("Wrong CoverSetType.");
}
CoverSet* CoverSet::Create(Database * const db, ItemSet *initwith, uint32 numipoints) { 
	ItemSetType type = db->GetDataType();
	if(type == Uint16ItemSetType)
		return new Uint16CoverSet(initwith, numipoints);
	else if(type == BAI32ItemSetType)
		return new BAI32CoverSet((uint32)db->GetAlphabetSize(), initwith, numipoints);
	else if(type == BM128ItemSetType)
		return new BM128CoverSet((uint32)db->GetAlphabetSize(), initwith, numipoints);
	else throw string("Wrong CoverSetType.");
}



ItemSetType CoverSet::StringToType(string &desc) {
	if(desc.compare("uint16") == 0)
		return Uint16ItemSetType;
	else if(desc.compare("bai32") == 0)
		return BAI32ItemSetType;
	else if(desc.compare("bm128") == 0)
		return BM128ItemSetType;
	else return NoneItemSetType;
}
uint32 CoverSet::Cover(ItemSet *codeElement) {
	return Cover(codeElement->GetLength(), codeElement);
}
bool CoverSet::CanCover(ItemSet *iset) {
	return CanCover(iset->GetLength(), iset);
}
uint32 CoverSet::NumCover(ItemSet *iset) {
	return NumCover(iset->GetLength(), iset);
}