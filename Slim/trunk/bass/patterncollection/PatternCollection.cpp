#ifdef __PCDEV

#include "../Bass.h"
//#include "../global.h"

#include <RandomUtils.h>
#include <SystemUtils.h>
#include <FileUtils.h>

#include "../db/Database.h"
//#include "PatternCollectionFile.h"

#include "PatternCollection.h"

PatternCollection::PatternCollection() {
	mNumPatterns = 0;
	mNumLoadedPatterns = 0;
	mCurLoadedPattern = 0;

	mPatternType = "?";
	mPatternFilter = "?";
	mPatternOrder = "?";

	mPatternCollectionFile = NULL;

	mCollection = NULL;
}
PatternCollection::~PatternCollection() {
	for(uint32 i=0; i<mNumLoadedPatterns; i++) {
		delete mCollection[i];
	} delete[] mCollection;

	//delete mPatternCollectionFile;
}

#endif