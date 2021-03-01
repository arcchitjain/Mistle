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
	else throw "Wrong CoverSetType.";
}
CoverSet* CoverSet::Create(Database * const db, ItemSet *initwith, uint32 numipoints) { 
	ItemSetType type = db->GetDataType();
	if(type == Uint16ItemSetType)
		return new Uint16CoverSet(initwith, numipoints);
	else if(type == BAI32ItemSetType)
		return new BAI32CoverSet((uint32)db->GetAlphabetSize(), initwith, numipoints);
	else if(type == BM128ItemSetType)
		return new BM128CoverSet((uint32)db->GetAlphabetSize(), initwith, numipoints);
	else throw "Wrong CoverSetType.";
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