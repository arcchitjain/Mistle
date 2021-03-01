#include "../../../../global.h"

#include "CoverPartial.h"
#include "../CodeTable.h"

CoverPartial::CoverPartial(CodeTable *ct) : KrimpAlgo() {
	mCT = ct;
}

CoverPartial::~CoverPartial() {
	//delete mCT; // afblijven, oetlul.
}

