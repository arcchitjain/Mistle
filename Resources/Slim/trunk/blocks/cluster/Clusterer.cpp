#ifdef BLOCK_CLUSTER

#if defined (_WINDOWS)
	#include <direct.h>
#endif

#include "../../global.h"

// -- qtils
#include <Config.h>
#include <FileUtils.h>

// -- bass
#include <db/ClassedDatabase.h>
#include <db/DbFile.h>
#include <isc/ItemSetCollection.h>
#include <itemstructs/ItemSet.h>

#include "../../FicConf.h"
#include "../../FicMain.h"
#include "../../algo/Algo.h"

#include "Clusterer1G.h"
#include "DBOntwarreraar.h"
#include "Ontwar2Clusterer.h"
#include "OntwarKClusterer.h"

#include "Clusterer.h"

Clusterer::Clusterer(Config *config) {
	mConfig = config;
	mBaseDir = "";
	mDbName = "";

	mLogFile = NULL;
	mSplitFile = NULL;
}
Clusterer::~Clusterer() {

}

Clusterer* Clusterer::CreateClusterer(Config *config) {
	string clType = config->Read<string>("cltype","");
	if(clType.compare("ontwardb") == 0)
		return new DBOntwarreraar(config);
	else if(clType.compare("ontwar2ct") == 0)
		return new Ontwar2Clusterer(config);
	else if(clType.compare("ontwarkct") == 0)
		return new OntwarKClusterer(config);
	else if(clType.compare("cluster1g") == 0)
		return new Clusterer1G(config);
	else
		throw string("Clusterer::CreateClusterer - Unknown clustertype: `") + clType + "`";
}

#endif // BLOCK_CLUSTER
