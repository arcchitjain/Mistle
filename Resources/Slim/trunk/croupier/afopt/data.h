#ifndef _DATA_H
#define _DATA_H

/*----------------------------------------------------------------------
  File     : data.h
  Contents : data set management
----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include <Primitives.h>

class Database;
class ItemSet;

namespace Afopt {

	class Output;

	class Transaction
	{
	public:

		Transaction(int l,int c) : length(l),count(c) {t = new int[l];}
		Transaction(const Transaction &tr);
		~Transaction(){delete [] t;}

		int length;
		int *t;
		int count;
	};

	class Data
	{
	public:

		Data(Database *db, Output *out);
		~Data();

		Transaction *getNextTransaction();

	protected:
		uint32	row;
		uint32	numRows;
		uint32	*values;
		ItemSet **rows;
	};

	extern Data* gpdata;

}
#endif
