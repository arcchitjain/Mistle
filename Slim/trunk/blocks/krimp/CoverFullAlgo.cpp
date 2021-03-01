#include "../../global.h"

#include "CoverFullAlgo.h"
#include "codetable/coverfull/CFCodeTable.h"

CoverFullAlgo::CoverFullAlgo(CodeTable *ct) {
	mCT = ct;
}

CoverFullAlgo::~CoverFullAlgo() {
	//delete mCT; // afblijven!
}
