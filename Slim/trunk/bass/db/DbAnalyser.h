#ifndef __DBANALYSER_H
#define __DBANALYSER_H

#include "../BassApi.h"

class Database;

class BASS_API DbAnalyser {
public:
	DbAnalyser();
	~DbAnalyser();

	void Analyse(Database *db, string &outputFile);

protected:

};

#endif //__DBANALYSER_H
